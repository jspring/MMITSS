/*
TCP Based Client Using Linux C
Started by fkl, May 29 2011 03:50 PM

Posted 29 May 2011 - 03:50 PM
Writing a TCP based client using POSIX Linux C API (Part 2)

I explained some TCP/IP theory and wrote a sockets based TCP server in previous tutorial. I intend to continue that with a client.

A client is an application which connects itself to the server and is the initiator of the communication. Generally speaking client code is much simpler than the server. However, apart from code’s point of view, I would like to explain a few very important concepts that I have found severely lacking in even people experienced with sockets based programming.

In every communication using TCP as transport protocol, we have a connection. Now what exactly does that mean? I know it is 3 way handshake i.e. 3 packets exchanged called syn, syn-ack and ack-ack.
Mostly people would say it kind of checks if the other end is ready to talk or is used for achieving reliability.

Let me clarify. Reliability in TCP comes from sending acknowledgements i.e. every packet sent contains a sequence number and the other side has a way to acknowledge that sequence number so that we know what we sent was successfully received. So we could simply have a client which starts by sending data and expects acknowledgements. Does that make it reliable? Yes! Does that make it TCP? No!!

What do we do in the first three packets? One better known aspect is, we exchange an initial sequence number so the other end knows what number my data bytes will be ordered from. But other than that, we say we establish connection! Now obviously we are not dedicating a physical line or getting something similar. But there still is a very fundamental difference between how tcp packets travel vs how UDP does. TCP establishes a virtual circuit which means for the life of this connection ALL Packets follow the same path. So it is this path that is negotiated in the initial 3 way handshake. In contrast all UDP packets use datagram switching i.e. they make routing decisions at every node and might follow different paths for one communication set. Of course it is not all, 3 way handshake also exchanges the window size and may be a few other parameters. But this is a very important difference between the behavior of TCP vs UDP packets.

IP address helps to reach the destination machine where as port number identifies which application we are actually communicating with.
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#define MAX_SIZE 50

int main()
{

        // Client socket descriptor which is just integer number used to access a socket
        int sock_descriptor;
        struct sockaddr_in serv_addr;
	FILE *myfile;

        // Structure from netdb.h file used for determining host name from local host's ip address
        struct hostent *server;

        // Buffer to input data from console and write to server
        char buff[MAX_SIZE];

        // Create socket of domain - Internet (IP) address, type - Stream based (TCP) and protocol unspecified
        // since it is only useful when underlying stack allows more than one protocol and we are choosing one.
        // 0 means choose the default protocol.
        sock_descriptor = socket(AF_INET, SOCK_STREAM, 0);

        if(sock_descriptor < 0)
          printf("Failed creating socket\n");

        bzero((char *)&serv_addr, sizeof(serv_addr));

        server = gethostbyname("127.0.0.1");
        
        if(server == NULL)
        {       
            printf("Failed finding server name\n");
        	return -1;
        }

        serv_addr.sin_family = AF_INET;
        memcpy((char *) &(serv_addr.sin_addr.s_addr), (char *)(server->h_addr), server->h_length);

        // 16 bit port number on which server listens
        // The function htons (host to network short) ensures that an integer is  
        // interpreted correctly (whether little endian or big endian) even if client and 
        // server have different architectures
        serv_addr.sin_port = htons(1234);
 
        if (connect(sock_descriptor, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    	{
        	printf("Failed to connect to server\n");
            return -1;
	    }
       
       	else
        	printf("Connected successfully - Please enter string\n");
	myfile = fdopen(sock_descriptor, "w");

while(1) {
        fgets(buff, MAX_SIZE-1, stdin);

        int count = write(sock_descriptor, buff, strlen(buff));
       
        if(count < 0)
        	printf("Failed writing rquested bytes to server\n");
	fflush(myfile);
}
        

        close(sock_descriptor); 
	return 0;
}
