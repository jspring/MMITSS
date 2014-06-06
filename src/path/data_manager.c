//#include <db_include.h>
#include "sys_os.h"
#include <sys/select.h>
#include <search.h>
#include "server_lib.h"
#include "data_manager.h"
#include "msgs.h"

#define BUFSIZE 1500

typedef struct {
	char portdesc[100];
	unsigned short portnum;
	unsigned short outfd[3];
#define NUMOUTPORTS	3
} port_t;

port_t port[] = { 
		{"PriorityRequestPort", PRIO_REQ_PORT, {0, 0, 0} },
		{"TrajectoryAwarePort", TRAJ_AWARE_PORT, {0, 0, 0} },
		{"NomadicTrajectoryAwarePort", NOMADIC_TRAJ_AWARE_PORT, {0, 0, 0} },
		{"TrafficControlPort", TRAF_CTL_PORT, {0, 0, 0} },
		{"TrafficControlInterfacePort", TRAF_CTL_IFACE_PORT, {0, 0, 0} },
		{"SpatBroadcastPort", SPAT_BCAST_PORT, {0, 0, 0} },
		{"PerformanceObserverPort",PERF_OBSRV_PORT, {0, 0, 0} },
		{"NomadicDevicePort", NOMADIC_DEV_PORT, {0, 0, 0} }
};

int NUMPORTS = sizeof(port) / sizeof(port_t);

// Array for allocating storage for messages. Hopefully, we can
// agree on a limit of 256 messages and up to 256 bytes per message.
// Also it would be nice if the first byte were message ID, the 
// second byte the size of the message, and bytes 2-6 the millisecond
// since midnight timestamp.
struct mmitss_msg_tag { unsigned char size; long *struct_ptr };
struct mmitss_msg_tag mmitss_msg[256];

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
	int i;
	int j;
	char tempstr[] = "Got message!";
	char tempstr2[100];


printf("Got to 0 NUMPORTS %d\n", NUMPORTS);

   for(i=0; i<NUMPORTS; i++) {
	memset(&sockfd[i], -1, sizeof(sockfd[i]));
	memset(&newsockfd[i], -1, sizeof(newsockfd[i]));
	memset(&remote_addr[i], 0, sizeof(remote_addr[i]));
   }

printf("Got to 1 NUMPORTS %d\n", NUMPORTS);

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

printf("Got to 2\n");
   //Zero out saved fds
   FD_ZERO(&readfds_sav);
   FD_ZERO(&writefds_sav);

   for(i = 0; i < NUMPORTS; i++) {
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
			printf("sockfd[%d] %d maxfd %d\n", i, sockfd[i], maxfd);
		}
   }

printf("Got to 3\n");
   while(1) {

	readfds = readfds_sav;
	writefds = writefds_sav;
printf("Got to 4\n");

        numready = select(maxfd+1, &readfds, NULL, NULL, NULL); // Tells me one of the old or new sockets is ready to read
	
   	for(i = 0; i < NUMPORTS; i++) {
	    printf("i %d\n", i);
	    if( (newsockfd[i] <= 0) && (FD_ISSET(sockfd[i], &readfds)) ){
		printf("Trying accept for %s(%d)\n", port[i].portdesc, port[i].portnum);
		if ((newsockfd[i] = accept(sockfd[i], 
			(struct sockaddr *) &remote_addr[i], &localaddrlen)) <= 0) {
		   	sprintf(tempstr2, "accept failed for %s(%d)", port[i].portdesc, port[i].portnum);
			perror(tempstr2);
			continue;
		}
		   	printf("accept succeeded for %s(%d)\n", port[i].portdesc, port[i].portnum);
			FD_CLR(sockfd[i], &readfds_sav);
			close(sockfd[i]);
			FD_SET(newsockfd[i], &readfds_sav);
			FD_SET(newsockfd[i], &writefds_sav);
			if(newsockfd[i] > maxfd) maxfd = newsockfd[i];
	   }	
	   if( (newsockfd[i] > 0) && FD_ISSET(newsockfd[i], &readfds) ) {
		memset(buf[i], 0, BUFSIZE);
		printf("Trying read for %s(%d)\n", port[i].portdesc, port[i].portnum);
		if ((retval = read(newsockfd[i], buf[i], BUFSIZE)) <= 0) {
		   	sprintf(tempstr2, "read failed for %s(%d)", port[i].portdesc, port[i].portnum);
			perror(tempstr2);
			FD_CLR(newsockfd[i], &readfds_sav);
			FD_CLR(newsockfd[i], &writefds_sav);
			close(newsockfd[i]);

			newsockfd[i] = -1;
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
			printf("Read succeeded for %s(%d)\n", port[i].portdesc, port[i].portnum);
			if(mmitss_msg[buf[i][0]].struct_ptr == NULL)
				mmitss_msg[buf[i][0]].struct_ptr = calloc(mmitss_msg[buf[i][0]].size, 1);
			memcpy(&mmitss_msg[buf[i][0]].struct_ptr, &buf[i][j], mmitss_msg[buf[i][0]].size);
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
		printf("Trying write for %s(%d)\n", port[i].portdesc, port[i].portnum);
		if(port[i].outfd[0] != 0) {
			for(j = 0; j < NUMOUTPORTS; j++) {
				if( (write(port[i].outfd[j], &buf[i][0], retval)) != retval ) {
		   			sprintf(tempstr2, "write failed for %s(%d)", port[i].portdesc, port[i].portnum);
					perror(tempstr2);
				}
			}
		}
	    }
	}
    sleep(1);
    }
}
