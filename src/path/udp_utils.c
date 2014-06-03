/*\\file
 *	Utility functions to do common combinations of open, set options,
 *	and bind operations on UDP sockets, making the
 *	real calls and setting the address structures appropiately.
 *
 *	All functions return the socket descriptor if successful,
 *	a negative number if unsuccessful
 *
 *	Copyright (c) 2006   Regents of the University of California
 *
 */
#include <udp_utils.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Fills in an AF_INET address structure with integer IP address and port 
// IPv4 address is in network order, port is not 
void set_inet_addr(struct sockaddr_in *paddr, int ip_nworder, short port)
{
	memset(paddr, 0, sizeof(struct sockaddr_in));
	paddr->sin_family = AF_INET;       // host byte order
	paddr->sin_port = htons(port);     // short, network byte order	  
	paddr->sin_addr.s_addr = ip_nworder; 
	memset(&(paddr->sin_zero), '\0', 8); // zero the rest of the struct
}

// Sets up a UDP socket for reception on a port from any address 
int udp_allow_all(short port)
{
	int sockfd; 			 // listen on sock_fd
	struct sockaddr_in addr;       // IP info for socket calsl
	  
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("socket");
		return -1;
	}
	set_inet_addr(&addr, INADDR_ANY, port);
	  
	if (bind(sockfd, (struct sockaddr *)&addr,
					 sizeof(struct sockaddr)) == -1) {
		perror("bind");
		return -2;
	}
	return sockfd;
}

// Sets up a UDP socket for reception on a port from a particular address
int udp_allow_from(int ip_nworder, short port)
{
	int sockfd; 		 // listen on sock_fd
	struct sockaddr_in addr; // IP info for socket calsl
	  
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("socket");
		return -1;
	}
	set_inet_addr(&addr, ip_nworder, port);
	  
	if (bind(sockfd, (struct sockaddr *)&addr,
					 sizeof(struct sockaddr)) == -1) {
		perror("bind");
		return -2;
	}
	if (connect(sockfd, (struct sockaddr *)&addr,
					 sizeof(struct sockaddr)) == -1) {
		perror("connect");
		return -2;
	}
	return sockfd;
}

// Sets up a UDP socket for broadcast
int udp_broadcast()
{
	int hold_one = 1;
	int sockfd;  

	if ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("socket");
		return -1;
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &hold_one, sizeof(int))
			< 0) {
		perror("setsockopt");
		return -2;
	}
	return sockfd;
}

// Sets up a UDP socket for sending unicast messages 
int udp_unicast()
{
	int sockfd;  

	if ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("socket");
		return -1;
	}
	return sockfd;
}

// Sets up a UDP socket for sending unicast messages 
// and initializes the sockaddr_in structure to be used for the sends.

int udp_unicast_init(struct sockaddr_in *paddr,char *ip_str,short port)
{
	int sockfd;  
	struct in_addr ia;

	sockfd = udp_unicast();

	if (!inet_aton(ip_str, &ia))
		return (-2);

	set_inet_addr(paddr, ia.s_addr, port);	
	return sockfd;
}

// Sets up a UDP socket for sending broadcast messages 
// and initializes the sockaddr_in structure to be used for the sends.
// In some cases, may want to use ip_str e.g. 192.168.1.255 instead of
// 255.255.255.255, although the latter is fine if there is no
// routing out of the subnet, as in some vehicle applications.

int udp_broadcast_init(struct sockaddr_in *paddr,char *ip_str,short port)
{
	int sockfd;  
	struct in_addr ia;

	sockfd = udp_broadcast();

	if (!inet_aton(ip_str, &ia))
		return (-2);

	set_inet_addr(paddr,ia.s_addr,port);	
	return sockfd;
}
	
	
