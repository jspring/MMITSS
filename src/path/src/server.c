#include <db_include.h>
#include "server_lib.h"

#define BUFSIZE 1500

int main( int argc, char *argv[]) {
	int retval;
	int newsockfd;
//	char *local_ip = "192.168.1.121";
//	char *remote_ip = "192.168.1.126";
	char *local_ip = "127.0.0.1";
	char *remote_ip = "127.0.0.1";
	unsigned short rcv_port = 1234;
	int use_db = 0;
	struct timespec tspec;
	int option;
	int verbose = 0;
	unsigned short socket_timeout = 10000;       //socket timeout in msec
	char buf[BUFSIZE];
	int i;
	char tempstr[] = "Got message!";


   while ((option = getopt(argc, argv, "vdl:r:k:")) != EOF) {
      switch (option) {
      case 'v':
         verbose = 1;
         break;
      case 'd':
         use_db = 1;
         break;
      case 'l':
         local_ip = strdup(optarg);
         break;
      case 'r':
         remote_ip = strdup(optarg);
         break;
      case 'k':
         socket_timeout = (unsigned short) atoi(optarg);
         printf("socket_timeout %d msec\n", socket_timeout);
         fflush(stdout);
         break;
      case 'h':
      default:
         printf("Usage: %s -v verbose -d use database -l <local IP> -r <remote IP -k <socket timeout (ms)>\n", argv[0]);
         exit(EXIT_FAILURE);
         break;
      }
   }


   fd_set myfdset;
   struct timeval tv;
//smserr_t smserr;

	newsockfd = OpenServerListener(local_ip, remote_ip, rcv_port);
	if(newsockfd <= 0) {
		perror("OpenServerListener");
		CloseServerListener(newsockfd );
		return newsockfd;
	}
   while(1) {
      FD_ZERO(&myfdset);
      FD_SET(newsockfd, &myfdset);
      tv.tv_usec = socket_timeout * 1000;
//      tv.tv_usec = smserr.socket_timeout * 1000;
      tv.tv_sec = 0;
      retval = select(newsockfd + 1, &myfdset, NULL, NULL, &tv);
      if (retval <= 0) {
//         smserr.socket_err_cnt++;
         clock_gettime(CLOCK_REALTIME, &tspec);
//         smserr.last_socket_err_time = (time_t) tspec.tv_sec;
//         if (use_db)
//            db_clt_write(pclt, DB_SMS_ERR_VAR, sizeof(smserr_t), &smserr);
      if (retval < 0) {
//         print_err("Select() error\n");
         perror("Select");
         return -1;
	}
      if (retval == 0) {
	perror("timeout");
	close(newsockfd);
	newsockfd = OpenServerListener(local_ip, remote_ip, rcv_port);
	if (newsockfd < 0) {
		perror("OpenServerListener");
//                 print_err("OpenServerListener failed");
	}
//            continue;
      }
      } else {
	memset(buf, 0, BUFSIZE);
	if ((retval = read(newsockfd, buf, BUFSIZE)) <= 1) {
		perror("read");
		close(newsockfd);
		newsockfd = OpenServerListener(local_ip, remote_ip, rcv_port);
		if (newsockfd < 0) {
			perror("OpenServerListener");
//                  print_err("OpenServerListener failed");
		}
//            continue;
         } else {
		for(i=0; i<retval; i++)
			printf("%hhx ", buf[i]);
		printf("\n");
	  }
	if (write(newsockfd, tempstr, sizeof(tempstr)) != sizeof(tempstr)) {
                     fprintf(stderr, "partial/failed write\n");
                     exit(EXIT_FAILURE);
                 }

	}
    sleep(1);
    }
}
