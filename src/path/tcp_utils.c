#include "tcp_utils.h"

// Fills in an AF_INET address structure with integer IP address and port 
// IPv4 address is in network order, port is not 
void set_inet_addr1(struct sockaddr_in *paddr, int ip_nworder, short port)
{
	paddr->sin_family = AF_INET;       // host byte order
	paddr->sin_port = htons(port);     // short, network byte order	  
	paddr->sin_addr.s_addr = ip_nworder; 
	memset(&(paddr->sin_zero), '\0', 8); // zero the rest of the struct
}

// Sets up a TCP socket for reception on a port from any address 
int tcp_allow_all(short port)
{
	int sockfd; 			 // listen on sock_fd
	struct sockaddr_in addr;       // IP info for socket calsl

	//Create socket	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		perror("socket");
		return (-1);
	}
	
	// Prepare the sockaddr_in structure
	set_inet_addr1(&addr, INADDR_ANY, port);
	
	// bind
	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1) {
		perror("bind");
		return (-2);
	}
	
	// listen
	if (listen(sockfd,3) == -1) {
		perror("listen");
		return (-3);
	}

	return sockfd;
}

// Sets up a TCP socket for sending unicast messages 
// and initializes the sockaddr_in structure to be used for the sends.
int tcp_unicast(struct sockaddr_in *paddr,char *ip_str,short port)
{
	int sockfd;  
	int ip_nworder;
	
	if ((sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) == -1) {
		perror("socket");
		return (-1);
	}

	if ( (ip_nworder = inet_addr(ip_str)) == -1) {
		perror("inet_addr");
		return (-2);
	}
	
	set_inet_addr1(paddr, ip_nworder, port);	
	
	return sockfd;
}
	
	
