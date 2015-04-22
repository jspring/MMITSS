#include "sys_os.h"
#include <sys/select.h>
#include <search.h>
#include "server_lib.h"
#include "mmitss_ports_and_message_numbers.h"
#include "timestamp.h"
#include "msgs.h"
#include "ab3418commudp.h"
#include "udp_utils.h"

#include "asn_application.h"
#include "asn_internal.h"       /* for _ASN_DEFAULT_STACK_MAX */
#include "SPAT.h"
#include "TimeMark.h"

#define BUFSIZE 1500

// "OUT" and "IN" ports are from the perspective of the named 
// module.  So, for instance, the TrafficControlInterface's output port
// (to the Data Manager) would be TRAF_CTL_OUTPORT and the output port
// from the Data Manager to the TrafficControlInterface would be
// TRAF_CTL_INPORT.
typedef struct {
	char portdesc[100];
	unsigned char  port_type;
#define UDP 0
#define TCP 1
	unsigned short outport;
	unsigned short inport;
	int inportfd;
	unsigned short fwdfd[3];// file descriptor of port to forward data from a read buffer to a write buffer
#define NUMOUTPORTS	3
} port_t;

port_t port[] = { 
		{"PriorityRequestModule", UDP, PRIO_REQ_OUTPORT, PRIO_REQ_INPORT, 0, {0, 0, 0} },
		{"TrajectoryAwareModule", UDP,  TRAJ_AWARE_OUTPORT, TRAJ_AWARE_INPORT, 0, {0, 0, 0} },
		{"NomadicTrajectoryAwareModule", UDP,  NOMADIC_TRAJ_AWARE_OUTPORT, NOMADIC_TRAJ_AWARE_INPORT, 0, {0, 0, 0} },
		{"TrafficControlModule", TCP,  TRAF_CTL_OUTPORT, TRAF_CTL_INPORT, 0, {TRAF_CTL_IFACE_INPORT, 0, 0} },
		{"TrafficControlInterfaceModule", TCP,  TRAF_CTL_IFACE_OUTPORT, TRAF_CTL_IFACE_INPORT, 0, {SPAT_MAP_BCAST_INPORT, 0, 0} },
		{"SpatMapBroadcastModule", UDP,  SPAT_MAP_BCAST_OUTPORT, SPAT_MAP_BCAST_INPORT, 0, {0, 0, 0} },
		{"PerformanceObserverModule", UDP,  PERF_OBSRV_OUTPORT, PERF_OBSRV_INPORT, 0, {0, 0, 0} },
		{"NomadicDeviceModule", UDP,  NOMADIC_DEV_OUTPORT, NOMADIC_DEV_INPORT, 0, {0, 0, 0} }
};

int NUMPORTS = sizeof(port) / sizeof(port_t);

// The MMITSS message header convention is: 
//   Header	InternalMsgHeader -- two bytes (0xFFFF)
//   msgID	MessageID -- one byte(0x01)
//   timeStamp	CurEpochTime --four bytes (0.01s)

// Array for allocating storage for messages, indexed
// by message ID. The format should be:
//   mmitss_msg_tag[msgID] = {sizeof(msg_struct_t), struct *msg_struct};
//
typedef struct { 
	unsigned int size; 
	long *struct_ptr; 
} IS_PACKED mmitss_msg_t;

static int write_out(const void *buffer, size_t size, void *key);

int main( int argc, char *argv[]) {
	int retval;
	int spatmsgsize;
	asn_dec_rval_t rval;
	int sockfd[2 * NUMPORTS];
	int newsockfd[2 * NUMPORTS];
	char *local_ip = "127.0.0.1";
	char *remote_ip = "127.0.0.1";
	socklen_t localaddrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in remote_addr[2 * NUMPORTS];
        int msg_len;
        fd_set readfds, readfds_sav;
        fd_set writefds, writefds_sav;
	int maxfd = 0;
	int numready = 0;

        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";


	unsigned short rcv_port = 0;
	int option;
	int verbose = 0;
	unsigned short socket_timeout = 10000;       //socket timeout in msec
	unsigned char buf[NUMPORTS][BUFSIZE];
	unsigned char spatbuf[BUFSIZE];
	mmitss_msg_hdr_t *mmitss_msg_hdr[NUMPORTS];
	int i;
	int j;
	int k;
	int l;
	int m = 0;
	char tempstr[] = "Got message!";
	char tempstr2[100];
	unsigned int ms_since_midnight;
	timestamp_t ts;
	asn_enc_rval_t erv;
	static asn_TYPE_descriptor_t PDU_Type;
	static asn_TYPE_descriptor_t *pduType = &PDU_Type;
        SPAT_t *spatType_decode=0;
        spatType_decode = (SPAT_t *) calloc(1, sizeof(SPAT_t));


	mmitss_msg_t mmitss_msg[256];

	mmitss_msg[MSG_ID_SIG_PLAN_POLL].size = 3;
	sig_plan_msg_t *sig_plan_msg = NULL;
	mmitss_msg[MSG_ID_SIG_PLAN].size = sizeof(sig_plan_msg_t);
	mmitss_msg[MSG_ID_SIG_PLAN].struct_ptr = (long *)sig_plan_msg;
	sig_plan_msg = calloc(sizeof(sig_plan_msg_t), 1);
	char *psig_plan_msg = (char *)&sig_plan_msg;

	mmitss_msg[MSG_ID_SPAT_POLL].size = 3;
	SPAT_t *j2735_spat = NULL;
	mmitss_msg[MSG_ID_SPAT].size = sizeof(SPAT_t);
	mmitss_msg[MSG_ID_SPAT].struct_ptr = (long *)j2735_spat;
	j2735_spat = calloc(sizeof(SPAT_t), 1);
	char *pj2735_spat = (char *)&j2735_spat;

   //Need input and output ports
   for(i=0; i< 2 * NUMPORTS; i++) {
	memset(&sockfd[i], -1, sizeof(sockfd[i]));
	memset(&newsockfd[i], -1, sizeof(newsockfd[i]));
	memset(&remote_addr[i], 0, sizeof(remote_addr[i]));
   }
   
   //Need only input buffers
   for(i=0; i<NUMPORTS; i++) {
	mmitss_msg_hdr[i] = (mmitss_msg_hdr_t *)&buf[i][0];
   }


   while ((option = getopt(argc, argv, "vl:r:p:k:h")) != EOF) {
      switch (option) {
      case 'v':
         verbose = 1;
         break;
      case 'l':
         local_ip = strdup(optarg);
         break;
      case 'r':
         remote_ip = strdup(optarg);
         break;
      case 'p':
         rcv_port = (unsigned short) atoi(optarg);
         break;
      case 'k':
         socket_timeout = (unsigned short) atoi(optarg);
         printf("socket_timeout %d msec\n", socket_timeout);
         fflush(stdout);
         break;
      case 'h':
      default:
         printf("Usage: %s -v verbose -l <local IP> -r <remote IP -p <port>\n", argv[0]);
         exit(EXIT_FAILURE);
         break;
      }
   }

   //Zero out saved fds
   FD_ZERO(&readfds_sav);
   FD_ZERO(&writefds_sav);

   for(i = 0; i < NUMPORTS; i++) {
	j = 2 * i;
	if(port[i].port_type == UDP) {
		newsockfd[j] = udp_allow_all(port[i].outport);
		if(newsockfd[j] < 0) {
			sprintf(tempstr2, "udp_allow_all failed for %s output port (%d)", port[i].portdesc, port[i].outport);
			perror(tempstr2);
			exit(EXIT_FAILURE);
		}
		else {
			printf("udp_allow_all succeeded for %s output port (%d)\n", port[i].portdesc, port[i].outport);
			FD_SET(newsockfd[j], &readfds_sav); //Remember: "out" is FROM the module TO the data manager
			if(newsockfd[j] > maxfd) maxfd = newsockfd[j];
				printf("UDP newsockfd[%d] %d maxfd %d\n", j, newsockfd[j], maxfd);
		}
		newsockfd[j+1] = udp_allow_all(port[i].inport);
		if(newsockfd[j+1] < 0) {
			sprintf(tempstr2, "udp_allow_all failed for %s input port (%d)", port[i].portdesc, port[i].inport);
			perror(tempstr2);
			exit(EXIT_FAILURE);
		}
		else {
			printf("udp_allow_all succeeded for %s input port (%d)\n", port[i].portdesc, port[i].inport);
			FD_SET(newsockfd[j+1], &writefds_sav); //Remember: "in" is FROM the data manager TO the module
			port[i].inportfd = newsockfd[j+1];
			if(newsockfd[j+1] > maxfd) maxfd = newsockfd[j+1];
				printf("UDP newsockfd[%d] %d maxfd %d\n", j+1, newsockfd[j+1], maxfd);
		}
	}
	else {
		sockfd[j] = OpenServerListener(local_ip, remote_ip, port[i].outport);
		if(sockfd[j] < 0) {
			sprintf(tempstr2, "OpenServerListener failed for %s output port (%d)", port[i].portdesc, port[i].outport);
			perror(tempstr2);
			CloseServerListener(sockfd[j]);
			exit(EXIT_FAILURE);
		}
		else {
			/** set up remote socket addressing and port */
			sprintf(tempstr2, "OpenServerListener succeeded for %s output port (%d)", port[i].portdesc, port[i].outport);
			perror(tempstr2);
			memset(&remote_addr[i], 0, sizeof(struct sockaddr_in));
			remote_addr[j].sin_family = AF_INET;
			remote_addr[j].sin_addr.s_addr = inet_addr(remote_ip);  //htonl(INADDR_ANY);
			remote_addr[j].sin_port = htons(port[i].outport);
			FD_SET(sockfd[j], &readfds_sav);
			if(sockfd[j] > maxfd) maxfd = sockfd[j];
				printf("TCP sockfd[%d] %d maxfd %d\n", j, sockfd[j], maxfd);
		}
		sockfd[j+1] = OpenServerListener(local_ip, remote_ip, port[i].inport);
		if(sockfd[j+1] < 0) {
			sprintf(tempstr2, "OpenServerListener failed for %s input port (%d)", port[i].portdesc, port[i].inport);
			perror(tempstr2);
			CloseServerListener(sockfd[j+1]);
			exit(EXIT_FAILURE);
		}
		else {
			/** set up remote socket addressing and port */
			sprintf(tempstr2, "OpenServerListener succeeded for %s input port (%d)", port[i].portdesc, port[i].inport);
			perror(tempstr2);
			memset(&remote_addr[j+1], 0, sizeof(struct sockaddr_in));
			remote_addr[j+1].sin_family = AF_INET;
			remote_addr[j+1].sin_addr.s_addr = inet_addr(remote_ip);  //htonl(INADDR_ANY);
			remote_addr[j+1].sin_port = htons(port[i].inport);
			port[i].inportfd = sockfd[j+1];
			FD_SET(sockfd[j+1], &writefds_sav);
			if(sockfd[j+1] > maxfd) maxfd = sockfd[j+1];
				printf("TCP sockfd[%d] %d maxfd %d\n", j+1, sockfd[j+1], maxfd);
		}
	}
    }
    //Map forwarding file descriptors to nonzero fwdfd's (which were originally set to inport numbers)
    //File descriptors are only determined after a socket call, so the place of them in the port
    //struct is initially taken by the (known) number of the destination inport
    //Here, we replace the destination inport by the destination file descriptor
    for(i = 0; i < NUMPORTS; i++) {
	for(j = 0; (j < 3) && (port[i].fwdfd[j] != 0); j++) {
		for(k = 0; k < NUMPORTS; k++) {
			if(port[i].fwdfd[j] == port[k].inport) {
				printf("Setting %s forward port to input port %d ", port[i].portdesc, port[k].inport);
				port[i].fwdfd[j] = port[k].inportfd;
				printf("file descriptor %d\n", port[i].fwdfd[j]);
    			}
    		}
    	}
    }

   while(1) {

	readfds = readfds_sav;
	writefds = writefds_sav;

        numready = select(maxfd+1, &readfds, NULL, NULL, NULL); // Tells me one of the old or new sockets is ready to read
   	for(i = 0; i < NUMPORTS; i++) {
	    j = 2 * i;
	    printf("i %d\n", i);

	    //Accept TCP connections if they haven't already been accepted 
	    if( (port[i].port_type == TCP) && (newsockfd[j] <= 0) && (FD_ISSET(sockfd[j], &readfds)) ){
		printf("Trying accept for %s output port (%d)\n", port[i].portdesc, port[i].outport);
		if ((newsockfd[j] = accept(sockfd[j], 
			(struct sockaddr *) &remote_addr[j], &localaddrlen)) <= 0) {
		   	sprintf(tempstr2, "accept failed for %s output port (%d)", port[i].portdesc, port[i].outport);
			perror(tempstr2);
			continue;
		}
		   	printf("accept succeeded for %s output port (%d)\n", port[i].portdesc, port[i].outport);
			FD_CLR(sockfd[j], &readfds_sav);
			close(sockfd[j]);
			FD_SET(newsockfd[j], &readfds_sav);
			if(newsockfd[j] > maxfd) maxfd = newsockfd[j];
	   }	
	    if( (port[i].port_type == TCP) && (newsockfd[j+1] <= 0) && (FD_ISSET(sockfd[j+1], &writefds)) ){
		printf("Trying accept for %s input port (%d)\n", port[i].portdesc, port[i].inport);
		if ((newsockfd[j+1] = accept(sockfd[j+1], 
			(struct sockaddr *) &remote_addr[j+1], &localaddrlen)) <= 0) {
		   	sprintf(tempstr2, "accept failed for %s input port (%d)", port[i].portdesc, port[i].inport);
			perror(tempstr2);
			continue;
		}
		   	printf("accept succeeded for %s input port (%d)\n", port[i].portdesc, port[i].inport);
			FD_CLR(sockfd[j+1], &readfds_sav);
			close(sockfd[j+1]);
			FD_SET(newsockfd[j+1], &writefds_sav);
			if(newsockfd[j+1] > maxfd) maxfd = newsockfd[j+1];
	   }	

	   //Now try to read client output sockets
	   if( (newsockfd[j] > 0) && FD_ISSET(newsockfd[j], &readfds) ) {
		memset(buf[i], 0, BUFSIZE);
		printf("Trying read for %s(%d)\n", port[i].portdesc, port[i].outport);

		//If the read fails, close it and reopen it
		if ((retval = read(newsockfd[j], &buf[i][0], BUFSIZE)) <= 0) {
		   	sprintf(tempstr2, "read failed for %s(%d)", port[i].portdesc, port[i].outport);
			perror(tempstr2);
			FD_CLR(newsockfd[j], &readfds_sav);
			FD_CLR(newsockfd[j+1], &writefds_sav);
			close(newsockfd[j]);
//			close(newsockfd[j+1]);
			newsockfd[j] = -1;
//			newsockfd[j+1] = -1;

		    //Error on TCP socket
		    if(port[i].port_type == TCP) {
			sockfd[j] = OpenServerListener(local_ip, remote_ip, port[i].outport);
			if (sockfd[j] < 0) {
		   	    sprintf(tempstr2, "OpenServerListener failed for %s output port (%d)", port[i].portdesc, port[i].outport);
		   	    perror(tempstr2);
			    continue;
			}
			else {
   				/** set up remote socket addressing and port */
   				memset(&remote_addr[j], 0, sizeof(struct sockaddr_in));
   				remote_addr[j].sin_family = AF_INET;
   				remote_addr[j].sin_addr.s_addr = inet_addr(remote_ip);  //htonl(INADDR_ANY);
   				remote_addr[j].sin_port = htons(port[i].outport);
				FD_SET(sockfd[j], &readfds_sav);
	    		}
//			sockfd[j+1] = OpenServerListener(local_ip, remote_ip, port[i].inport);
//			if (sockfd[j+1] < 0) {
//		   	    sprintf(tempstr2, "OpenServerListener failed for %s output port (%d)", port[i].portdesc, port[i].inport);
//		   	    perror(tempstr2);
//			    continue;
//			}
//			else {
//   				/** set up remote socket addressing and port */
//   				memset(&remote_addr[j+1], 0, sizeof(struct sockaddr_in));
//   				remote_addr[j+1].sin_family = AF_INET;
//   				remote_addr[j+1].sin_addr.s_addr = inet_addr(remote_ip);  //htonl(INADDR_ANY);
//   				remote_addr[j+1].sin_port = htons(port[i].inport);
//				FD_SET(sockfd[j+1], &writefds_sav);
//	    		}
	    	    }
		    else {
			printf("UDP socket failed\n");
			newsockfd[j] = udp_allow_all(port[i].outport);
			if(newsockfd[j] < 0) {
				sprintf(tempstr2, "udp_allow_all failed for %s(%d)", port[i].portdesc, port[i].outport);
				perror(tempstr2);
				exit(EXIT_FAILURE);
			}
		    }
	    	}//end of failed read block
        	else {
			get_current_timestamp(&ts);
			ms_since_midnight = (3600000 * ts.hour) + (60000 * ts.min) + (1000 * ts.sec) + ts.millisec;
			printf("Read succeeded for %s output port (%d) ms-since-midnight %d retval %d size %d buf[%d][0] %d m %d mmitss_msg_hdr[i]->msgid %d\n", port[i].portdesc, port[i].outport, ms_since_midnight, retval, mmitss_msg[mmitss_msg_hdr[i]->msgid].size, i, buf[i][0], m++, mmitss_msg_hdr[i]->msgid);
	
			//All MMITSS messages start with the InternalMsgHeader=0xFFFF
			if(mmitss_msg_hdr[i]->InternalMsgHeader == 0xFFFF) {
				//The following looks like magic; it's not. At the beginning of the program, I assigned mmitss_msg_header[i] = &buf[i][0].
				if(mmitss_msg[mmitss_msg_hdr[i]->msgid].struct_ptr == NULL)
					mmitss_msg[mmitss_msg_hdr[i]->msgid].struct_ptr = calloc(mmitss_msg[mmitss_msg_hdr[i]->msgid].size, 1);
//				printf("mmitss_msg_hdr[i]->msgid %d mmitss_msg[mmitss_msg_hdr[i]->msgid].size %d\n", mmitss_msg_hdr[i]->msgid, mmitss_msg[mmitss_msg_hdr[i]->msgid].size);
				memcpy(&mmitss_msg[mmitss_msg_hdr[i]->msgid].struct_ptr, &buf[i][0], mmitss_msg[mmitss_msg_hdr[i]->msgid].size);

				switch(mmitss_msg_hdr[i]->msgid) {
					case MSG_ID_SPAT:
						switch(buf[i][1+7]) {
							case 0x82:
								spatmsgsize = (buf[i][2+7] << 8) + buf[i][3+7] + 4;
								break;
							case 0x81:
								spatmsgsize = buf[i][2+7] + 3;
								break;
							case 0x80:
								spatmsgsize = buf[i][1+7] + 2;
								break;
							default:
								printf("Not a J2735 message (has 0xFFFF header, but not 0x8[012] size\n");
						}
						//Copy input buffer to spatbuf
						memcpy(buf[i], spatbuf, retval);

						//Print out SPaT if desired
						if(verbose) {
							spatType_decode=0;
							rval = ber_decode(0, &asn_DEF_SPAT,(void **)&spatType_decode, &buf[i][0+7], spatmsgsize);
							xer_fprint(stdout, &asn_DEF_SPAT, spatType_decode);
							printf("spatType_decode->msgID %d\n", (int)spatType_decode->msgID);
						}

   						if( FD_ISSET(newsockfd[j+1], &writefds)){
							printf("Trying j2735_spat write for %s(%d)\n", port[i].portdesc, port[i].inport);
							if( (write(newsockfd[j+1], j2735_spat, sizeof(SPAT_t))) != sizeof(SPAT_t)) {
						   		sprintf(tempstr2, "j2735_spat write failed for %s(%d)", 
								port[i].portdesc, port[i].inport);
							perror(tempstr2);
							}
						if(verbose) {
							printf("j2735_spat received:\n");
							for(l=0; l<sizeof(SPAT_t); l+=10){
							    printf("%d: ", l);
							    for(m=0; m<10; m++)
		    						printf("%hhx ", pj2735_spat[l+m]);
								printf("\n");
							}
						}
////////////////////////	`		write(STDOUT_FILENO, &j2735_spat, sizeof(SPAT_t));
//					`	erv = der_encode(pduType, pj2735_spat, write_out, stdout);
//	`					if(erv.encoded < 0) 
//		`					fprintf(stderr, ": Cannot convert %s into DER\n",
//			`				pduType->name);
//				`			exit(1);
//					
//					`	DEBUG("Encoded in %ld bytes of DER", (long)erv.encoded);
						}
						break;
				for(j=0; j<retval; j++)
			    		printf("%hhx ", buf[i][j]);
				printf("\n");
			    }//end of reading in MMITSS message
			
			//Now try to forward MMITSS message to all appropriate clients
			if(port[i].fwdfd[0] != 0) {
				for(k = 0; (port[i].fwdfd[k] != 0) && (k < NUMOUTPORTS); k++) {
					printf("Trying forwarding write from %s output port (%d) to input fd %d\n", port[i].portdesc, port[i].outport, port[i].fwdfd[k]);
					if( (write(port[i].fwdfd[k], &buf[i][0], retval)) != retval ) {
		   				sprintf(tempstr2, "forwarding write failed for %s output port (%d) to input fd %d", port[i].portdesc, port[i].outport, port[i].fwdfd[k]);
						perror(tempstr2);
					}
				}
			}//end of forwarding MMITTSS message
	  	}//end of MMITSS message test
		else {
			switch(buf[i][2]) {
				case MSG_ID_SIG_PLAN_POLL:
	    				if( FD_ISSET(newsockfd[j+1], &writefds)){
						printf("Trying sig_plan_msg write for %s(%d)\n", port[i].portdesc, port[i].inport);
						if( (write(newsockfd[j+1], sig_plan_msg, sizeof(sig_plan_msg_t))) != sizeof(sig_plan_msg_t)) {
					   		sprintf(tempstr2, "sig_plan_msg write failed for %s(%d)", 
								port[i].portdesc, port[i].inport);
							perror(tempstr2);
						}
						if(verbose) {
							printf("sig_plan_msg send: ");
							for(k=0; k<sizeof(sig_plan_msg_t); k++)
		    						printf("%hhx ", psig_plan_msg[k]);
							printf("\n");
						}
					}
					break;
				case MSG_ID_SPAT_POLL:
	    				if( FD_ISSET(newsockfd[j+1], &writefds)){
						printf("Trying j2735_spat write for %s(%d)\n", port[i].portdesc, port[i].inport);
						if( (write(newsockfd[j+1], j2735_spat, sizeof(SPAT_t))) != sizeof(SPAT_t)) {
					   		sprintf(tempstr2, "j2735_spat write failed for %s(%d)", 
								port[i].portdesc, port[i].inport);
							perror(tempstr2);
						}
						if(verbose) {
							printf("j2735_spat send:\n");
							for(l=0; l<sizeof(SPAT_t); l+=10){
							    printf("%d: ", l);
							    for(m=0; m<10; m++)
		    						printf("%hhx ", pj2735_spat[l+m]);
								printf("\n");
							}
						}
					}
					break;
				default:
			    		if(verbose) {
						printf("Not a known message. Here's the hex string:\n");
						for(j=0; j<retval; j++)
							printf("%hhx ", buf[i][j]);
						printf("Done with unknown message\n\n");
					}
			}//end of switch(buf[i][2])
	  	}//end of non-MMITSS else
	}//end of successful message read else
    }//end of attempt to read client output socket
	}//end of for loop for all defined ports
    }//end of while loop
}//end of main

/* Dump the buffer out to the specified FILE */
//static int write_out(const void *buffer, size_t size, void *key) {
//        FILE *fp = (FILE *)key;
//        return (fwrite(buffer, 1, size, fp) == size) ? 0 : -1;
//}

