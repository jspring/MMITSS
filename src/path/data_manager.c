//#include <db_include.h>
#include "sys_os.h"
#include <sys/select.h>
#include <search.h>
#include "server_lib.h"
#include "mmitss_ports_and_message_numbers.h"
#include "timestamp.h"
#include "msgs.h"
#include "ab3418commudp.h"
#include "udp_utils.h"
#include <asn_codecs.h>
#include <constr_TYPE.h>

#define BUFSIZE 1500

/* Convert "Type" defined by -DPDU into "asn_DEF_Type" */
//#define ASN_DEF_PDU(t)  asn_DEF_ ## t
//#define DEF_PDU_Type(t) ASN_DEF_PDU(t)
//#define PDU_Type        DEF_PDU_Type(PDU)
#define PDU_Type        asn_DEF_SPAT

typedef struct {
	char portdesc[100];
	unsigned char  port_type;
#define UDP 0
#define TCP 1
	unsigned short portnum;
	unsigned short outfd[3];
#define NUMOUTPORTS	3
} port_t;

port_t port[] = { 
		{"PriorityRequestPort", UDP, PRIO_REQ_PORT, {0, 0, 0} },
		{"TrajectoryAwarePort", UDP, TRAJ_AWARE_PORT, {0, 0, 0} },
		{"NomadicTrajectoryAwarePort", UDP, NOMADIC_TRAJ_AWARE_PORT, {0, 0, 0} },
		{"TrafficControlPort", TCP, TRAF_CTL_PORT, {0, 0, 0} },
		{"TrafficControlInterfacePort", TCP, TRAF_CTL_IFACE_PORT, {0, 0, 0} },
		{"SpatBroadcastPort", UDP, SPAT_BCAST_PORT, {0, 0, 0} },
		{"PerformanceObserverPort", UDP, PERF_OBSRV_PORT, {0, 0, 0} },
		{"NomadicDevicePort", UDP, NOMADIC_DEV_PORT, {0, 0, 0} }
};

int NUMPORTS = sizeof(port) / sizeof(port_t);

// Array for allocating storage for messages, indexed
// by message ID. The format should be:
//   mmitss_msg_tag[msgID] = {sizeof(msg_struct_t), struct *msg_struct};
//
// The MMITSS message header convention is: 
//   Header	InternalMsgHeader -- two bytes (0xFFFF)
//   msgID	MessageID -- one byte(0x01)
//   timeStamp	CurEpochTime --four bytes (0.01s)
//
typedef struct { 
	unsigned int size; 
	long *struct_ptr; 
} IS_PACKED mmitss_msg_t;

static int write_out(const void *buffer, size_t size, void *key);

int main( int argc, char *argv[]) {
	int retval;
	int sockfd[NUMPORTS];
	int newsockfd[NUMPORTS];
	char *local_ip = "127.0.0.1";
	char *remote_ip = "127.0.0.1";
	socklen_t localaddrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in remote_addr[NUMPORTS];
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
	char buf[NUMPORTS][BUFSIZE];
	mmitss_msg_hdr_t *mmitss_msg_hdr[NUMPORTS];
	int i;
	int j;
	char tempstr[] = "Got message!";
	char tempstr2[100];
	unsigned int ms_since_midnight;
	timestamp_t ts;
	asn_enc_rval_t erv;
	static asn_TYPE_descriptor_t PDU_Type;
	static asn_TYPE_descriptor_t *pduType = &PDU_Type;


	mmitss_msg_t mmitss_msg[256];

	mmitss_msg[MSG_ID_SIG_PLAN_POLL].size = 3;
	sig_plan_msg_t *sig_plan_msg = NULL;
	mmitss_msg[MSG_ID_SIG_PLAN].size = sizeof(sig_plan_msg_t);
	mmitss_msg[MSG_ID_SIG_PLAN].struct_ptr = (long *)sig_plan_msg;
	sig_plan_msg = calloc(sizeof(sig_plan_msg_t), 1);
	char *psig_plan_msg = (char *)&sig_plan_msg;

	mmitss_msg[MSG_ID_SPAT_POLL].size = 3;
	battelle_spat_t *battelle_spat = NULL;
	mmitss_msg[MSG_ID_SPAT].size = sizeof(battelle_spat_t);
	mmitss_msg[MSG_ID_SPAT].struct_ptr = (long *)battelle_spat;
	battelle_spat = calloc(sizeof(battelle_spat_t), 1);
	char *pbattelle_spat = (char *)&battelle_spat;
//printf("Got to 0 NUMPORTS %d\n", NUMPORTS);

   for(i=0; i<NUMPORTS; i++) {
	memset(&sockfd[i], -1, sizeof(sockfd[i]));
	memset(&newsockfd[i], -1, sizeof(newsockfd[i]));
	memset(&remote_addr[i], 0, sizeof(remote_addr[i]));
	mmitss_msg_hdr[i] = (mmitss_msg_hdr_t *)&buf[i][0];
   }

//printf("Got to 1 NUMPORTS %d\n", NUMPORTS);

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

//printf("Got to 2\n");
   //Zero out saved fds
   FD_ZERO(&readfds_sav);
   FD_ZERO(&writefds_sav);

   for(i = 0; i < NUMPORTS; i++) {
	if(port[i].port_type == UDP) {
		newsockfd[i] = udp_allow_all(port[i].portnum);
		if(newsockfd[i] < 0) {
			sprintf(tempstr2, "udp_allow_all failed for %s(%d)", port[i].portdesc, port[i].portnum);
			perror(tempstr2);
			exit(EXIT_FAILURE);
		}
		else {
//			printf("udp_allow_all succeeded for %s(%d)\n", port[i].portdesc, port[i].portnum);
			FD_SET(newsockfd[i], &readfds_sav);
			FD_SET(newsockfd[i], &writefds_sav);
			if(newsockfd[i] > maxfd) maxfd = newsockfd[i];
//			printf("UDP newsockfd[%d] %d maxfd %d\n", i, newsockfd[i], maxfd);
		}
	}
	else {
		sockfd[i] = OpenServerListener(local_ip, remote_ip, port[i].portnum);
		if(sockfd[i] < 0) {
			sprintf(tempstr2, "OpenServerListener failed for %s(%d)", port[i].portdesc, port[i].portnum);
			perror(tempstr2);
			CloseServerListener(sockfd[i]);
			exit(EXIT_FAILURE);
		}
		else {
			/** set up remote socket addressing and port */
			memset(&remote_addr[i], 0, sizeof(struct sockaddr_in));
			remote_addr[i].sin_family = AF_INET;
			remote_addr[i].sin_addr.s_addr = inet_addr(remote_ip);  //htonl(INADDR_ANY);
			remote_addr[i].sin_port = htons(port[i].portnum);
			FD_SET(sockfd[i], &readfds_sav);
			FD_SET(sockfd[i], &writefds_sav);
			if(sockfd[i] > maxfd) maxfd = sockfd[i];
//			printf("TCP sockfd[%d] %d maxfd %d\n", i, sockfd[i], maxfd);
		}
	}
    }

//printf("Got to 3\n");
   while(1) {

	readfds = readfds_sav;
	writefds = writefds_sav;
//printf("Got to 4\n");

        numready = select(maxfd+1, &readfds, NULL, NULL, NULL); // Tells me one of the old or new sockets is ready to read
//printf("Received data\n");	
   	for(i = 0; i < NUMPORTS; i++) {
//	    printf("i %d\n", i);
	    if( (newsockfd[i] <= 0) && (FD_ISSET(sockfd[i], &readfds)) ){
//		printf("Trying accept for %s(%d)\n", port[i].portdesc, port[i].portnum);
		if ((newsockfd[i] = accept(sockfd[i], 
			(struct sockaddr *) &remote_addr[i], &localaddrlen)) <= 0) {
		   	sprintf(tempstr2, "accept failed for %s(%d)", port[i].portdesc, port[i].portnum);
			perror(tempstr2);
			continue;
		}
//		   	printf("accept succeeded for %s(%d)\n", port[i].portdesc, port[i].portnum);
			FD_CLR(sockfd[i], &readfds_sav);
			close(sockfd[i]);
			FD_SET(newsockfd[i], &readfds_sav);
			FD_SET(newsockfd[i], &writefds_sav);
			if(newsockfd[i] > maxfd) maxfd = newsockfd[i];
	   }	
	   if( (newsockfd[i] > 0) && FD_ISSET(newsockfd[i], &readfds) ) {
		memset(buf[i], 0, BUFSIZE);
//		printf("Trying read for %s(%d)\n", port[i].portdesc, port[i].portnum);
		if ((retval = read(newsockfd[i], buf[i], BUFSIZE)) <= 0) {
		   	sprintf(tempstr2, "read failed for %s(%d)", port[i].portdesc, port[i].portnum);
			perror(tempstr2);
			FD_CLR(newsockfd[i], &readfds_sav);
			FD_CLR(newsockfd[i], &writefds_sav);
			close(newsockfd[i]);
			newsockfd[i] = -1;

		    //Error on TCP socket
		    if(port[i].port_type == TCP) {
			sockfd[i] = OpenServerListener(local_ip, remote_ip, port[i].portnum);
			if (sockfd[i] < 0) {
		   	    sprintf(tempstr2, "OpenServerListener failed for %s(%d)", port[i].portdesc, port[i].portnum);
		   	    perror(tempstr2);
			    continue;
			}
			else {
   				/** set up remote socket addressing and port */
   				memset(&remote_addr[i], 0, sizeof(struct sockaddr_in));
   				remote_addr[i].sin_family = AF_INET;
   				remote_addr[i].sin_addr.s_addr = inet_addr(remote_ip);  //htonl(INADDR_ANY);
   				remote_addr[i].sin_port = htons(port[i].portnum);
				FD_SET(sockfd[i], &readfds_sav);
				FD_SET(sockfd[i], &writefds_sav);
	    		}
	    	    }
		    else {
			printf("UDP socket failed\n");
			newsockfd[i] = udp_allow_all(port[i].portnum);
			if(newsockfd[i] < 0) {
				sprintf(tempstr2, "udp_allow_all failed for %s(%d)", port[i].portdesc, port[i].portnum);
				perror(tempstr2);
				exit(EXIT_FAILURE);
			}
		    }
	    	}
        	else {
			get_current_timestamp(&ts);
			ms_since_midnight = (3600000 * ts.hour) + (60000 * ts.min) + (1000 * ts.sec) + ts.millisec;
//			printf("Read succeeded for %s(%d) ms-since-midnight %d\n", port[i].portdesc, port[i].portnum, ms_since_midnight);
			if(mmitss_msg[mmitss_msg_hdr[i]->msgid].struct_ptr == NULL)
				mmitss_msg[mmitss_msg_hdr[i]->msgid].struct_ptr = calloc(mmitss_msg[mmitss_msg_hdr[i]->msgid].size, 1);
//printf("mmitss_msg_hdr[i]->msgid %d mmitss_msg[mmitss_msg_hdr[i]->msgid].size %d\n", mmitss_msg_hdr[i]->msgid, mmitss_msg[mmitss_msg_hdr[i]->msgid].size);
//			if(retval == mmitss_msg[mmitss_msg_hdr[i]->msgid].size) 
				{
//				memcpy(&mmitss_msg[mmitss_msg_hdr[i]->msgid].struct_ptr, &buf[i][j], mmitss_msg[mmitss_msg_hdr[i]->msgid].size);
//printf("Got to blah\n");
				if(mmitss_msg_hdr[i]->InternalMsgHeader == 0xFFFF) {
				switch(mmitss_msg_hdr[i]->msgid) {
					case MSG_ID_SIG_PLAN_POLL:
	    					if( FD_ISSET(newsockfd[i], &writefds)){
							printf("Trying sig_plan_msg write for %s(%d)\n", port[i].portdesc, port[i].portnum);
							if( (write(newsockfd[i], sig_plan_msg, sizeof(sig_plan_msg_t))) != sizeof(sig_plan_msg_t)) {
						   		sprintf(tempstr2, "sig_plan_msg write failed for %s(%d)", 
									port[i].portdesc, port[i].portnum);
								perror(tempstr2);
							}
							if(verbose) {
								printf("sig_plan_msg send: ");
								for(j=0; j<sizeof(sig_plan_msg_t); j++)
			    						printf("%hhx ", psig_plan_msg[j]);
								printf("\n");
							}
						}
						break;
					case MSG_ID_SPAT_POLL:
	    					if( FD_ISSET(newsockfd[i], &writefds)){
							printf("Trying battelle_spat write for %s(%d)\n", port[i].portdesc, port[i].portnum);
							if( (write(newsockfd[i], battelle_spat, sizeof(battelle_spat_t))) != sizeof(battelle_spat_t)) {
						   		sprintf(tempstr2, "battelle_spat write failed for %s(%d)", 
									port[i].portdesc, port[i].portnum);
								perror(tempstr2);
							}
							if(verbose) {
								printf("battelle_spat send:\n");
								for(j=0; j<sizeof(battelle_spat_t); j+=10){
								    printf("%d: ", j);
								    for(i=0; i<10; i++)
			    						printf("%hhx ", pbattelle_spat[j+i]);
									printf("\n");
								}
							}
						}
						break;
					}
				}
				else {
				    switch(buf[i][0]) {
					case MSG_ID_SPAT:
//	    					if( FD_ISSET(newsockfd[i], &writefds)){
//							printf("Trying battelle_spat write for %s(%d)\n", port[i].portdesc, port[i].portnum);
//							if( (write(newsockfd[i], battelle_spat, sizeof(battelle_spat_t))) != sizeof(battelle_spat_t)) {
//						   		sprintf(tempstr2, "battelle_spat write failed for %s(%d)", 
//									port[i].portdesc, port[i].portnum);
//								perror(tempstr2);
//							}
							if(verbose) {
								printf("battelle_spat received:\n");
								for(j=0; j<sizeof(battelle_spat_t); j+=10){
								    printf("%d: ", j);
								    for(i=0; i<10; i++)
			    						printf("%hhx ", pbattelle_spat[j+i]);
									printf("\n");
								}
							}
////////////////////////							write(STDOUT_FILENO, &battelle_spat, sizeof(battelle_spat_t));
//							erv = der_encode(pduType, pbattelle_spat, write_out, stdout);
//							if(erv.encoded < 0) {
//								fprintf(stderr, ": Cannot convert %s into DER\n",
//								pduType->name);
//								exit(1);
//							}
//							DEBUG("Encoded in %ld bytes of DER", (long)erv.encoded);
//						}
						break;
				    }
	    			}
			}
//			else {
//				printf("Message size %d not equal to defined size %d of message number %hhx\n",
//					retval,
//					mmitss_msg[mmitss_msg_hdr[i]->msgid].size,
//					mmitss_msg_hdr[i]->msgid
//				);
//			}
			if(verbose) {
				for(j=0; j<retval; j++)
			    		printf("%hhx ", buf[i][j]);
				printf("\n");
			}
			
	  	}
	  }
	}
   	for(i = 0; i < NUMPORTS; i++) {
	    if( (newsockfd[i] >0) && (FD_ISSET(newsockfd[i], &writefds)) ){
//		printf("Trying general write for %s(%d)\n", port[i].portdesc, port[i].portnum);
		if(port[i].outfd[0] != 0) {
			for(j = 0; j < NUMOUTPORTS; j++) {
				if( (write(port[i].outfd[j], &buf[i][0], retval)) != retval ) {
		   			sprintf(tempstr2, "general write failed for %s(%d)", port[i].portdesc, port[i].portnum);
					perror(tempstr2);
				}
			}
		}
	    }
	}
    sleep(1);
    }
}

/* Dump the buffer out to the specified FILE */
static int write_out(const void *buffer, size_t size, void *key) {
        FILE *fp = (FILE *)key;
        return (fwrite(buffer, 1, size, fp) == size) ? 0 : -1;
}

