/* server_lib.c - function definitions for creating an ethernet server
** that will listen for a client. The client will send messages for 
** reading and writing database variables.
*/

#include <db_include.h>

int OpenServerListener(char *local_ip, char *remote_ip, unsigned short rcv_port)
{

   int sockfd;
   int newsockfd;
   int backlog = 1;
   struct sockaddr_in local_addr;
//   socklen_t localaddrlen = sizeof(local_addr);
//   struct sockaddr_in remote_addr;
   int reuse = 1;

   // Open connection to SMS subnetwork controller
   sockfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
//   sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (sockfd < 0) {
      perror("socket");
      return -1;
   }
   if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
	perror("setsockopt");
	return -2;
   }

    /** set up local socket addressing and port */
   memset(&local_addr, 0, sizeof(struct sockaddr_in));
   local_addr.sin_family = AF_INET;
   local_addr.sin_addr.s_addr = inet_addr(local_ip);    //htonl(INADDR_ANY);//
   local_addr.sin_port = htons(rcv_port);
   if ((bind(sockfd, (struct sockaddr *) &local_addr, sizeof(local_addr))) < 0) {
      if (errno != EADDRINUSE) {
         perror("bind");
         close(sockfd);
         return -3;
      }
   }

   if ((listen(sockfd, backlog)) < 0) {
      perror("listen");
      close(sockfd);
      return -4;
   }

//    /** set up remote socket addressing and port */
//   memset(&remote_addr, 0, sizeof(struct sockaddr_in));
//   remote_addr.sin_family = AF_INET;
//   remote_addr.sin_addr.s_addr = inet_addr(remote_ip);  //htonl(INADDR_ANY);
//   remote_addr.sin_port = htons(rcv_port);
//   localaddrlen = sizeof(remote_addr);

//   if ((newsockfd = accept(sockfd, (struct sockaddr *) &remote_addr, &localaddrlen)) < 0) {
//      perror("accept");
//      close(sockfd);
//      return -5;
//   };
//   close(sockfd);
//   return newsockfd;
   return sockfd;
}

int CloseServerListener(int sockfd)
{
   close(sockfd);
   return 0;
}

int print_err(char *errstr)
{

   struct timespec tmspec;
   struct tm *errtime;

   clock_gettime(CLOCK_REALTIME, &tmspec);
   errtime = localtime(&tmspec.tv_sec);
   fprintf(stderr, "%2.2d:%2.2d:%2.2d ", errtime->tm_hour, errtime->tm_min, errtime->tm_sec);
   perror(errstr);
   fflush(stderr);
   return 0;
}
