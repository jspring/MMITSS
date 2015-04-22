#ifndef TCP_UTILS_H
#define TCP_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

// Fills in an AF_INET address structure with integer IP address and port 
// IPv4 address is in network order, port is not 
void set_inet_addr1(struct sockaddr_in *paddr, int ip_nworder, short port);

// Sets up a TCP socket on a port for reception from any address 
extern int tcp_allow_all(short port);

// Sets up a UDP socket for sending unicast messages to "port" at "ip_str"
// and initializes the sockaddr_in structure to be used for the sends.
extern int tcp_unicast_init(struct sockaddr_in *paddr,char *ip_str,short port);

#endif

