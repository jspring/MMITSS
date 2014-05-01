/*
Concurrent TCP server using Select API in Linux C
Started by fkl, Jun 06 2011 01:11 PM

Posted 06 June 2011 - 01:11 PM
A Partially concurrent TCP server using Linux C Select API

I have already written a basic TCP client and server in two previous tutorials and would like to build upon those. Until now the server has been able to receive only one client message and terminate.

http://forum.codecal...inux-c-api.html
http://forum.codecal...ng-linux-c.html

I just picked up my server.c code from the previous tutorial and added a loop

while(1) // Just before the print “Waiting for connection”
{

} // close right after close(conn_desc) statement


The intention was, to be able to read from multiple clients.

Now I can connect to my server through multiple clients (one after the other) though not in parallel. The problem is that each new client has to keep waiting until the server has finished processing the earlier one and you can’t really tell if a client is going to hold on for long and send data after some time.

To accommodate such a situation C language has already provided an API to be used on any type of descriptors whether they are files or sockets.

The basic concept of this API is that you can listen on multiple descriptors / sockets at the same time and keep waiting until data is available on ANY one of them. As soon as somebody sends you a message, select will return that descriptor to the program.

Note the difference as well as constraints:

1. We are listening to receive data from multiple clients which are all going to send data to server, but we don’t know who would do that first.

2. This does not accommodate the situation when one client is reading and other is writing. You need multiple threads or processes to handle that for sure.

3. Without this, the problem could be that if client one was connected first, is not sending data while client two who wants to get connected and immediately send data, but is waiting for connection until one is serviced by the server. This situation would be resolved by using select since it allows both clients to be connected and server waiting for anyone who sends data first. If both of them send together, the one whose data is received first is serviced first.

The output is demonstrated by the image.
TCPSelectServer.png TCPSelectServer.png

I have one server running and I connected two clients to it in parallel. While both clients are connected, I can type text in any one of them and it is displayed immediately on the server. Basically server is monitoring both of the descriptors using select and responds when data is available on any of them.

Implementation wise, we create a descriptor set structure to which we can add any number of descriptors. Then we pass that descriptor set to select API. This will be a blocking call (unless we have provided a timeout to select) until data is available on any of those descriptors. Then upon return from select, we call accept using the monitored descriptor and receive data.

The code below works for any number of descriptors (and hence clients). The reason of calling this server partial is that it only works when you are expecting all clients to be sending data. If one client is sending data and the other is expecting data from you, it requires creating multiple threads or processes. We will be creating those in forth coming tutorials.
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>

#define MAX_SIZE 50

int main()
{
    int listen_desc, conn_desc; // main listening descriptor and connected descriptor
    int maxfd, maxi; // max value descriptor and index in client array
    int i,j,k;  // loop variables
    fd_set tempset, savedset;  // descriptor set to be monitored
    int client[FD_SETSIZE], numready; // array of client descriptors (FD_SETSIZE=1024)
    struct sockaddr_in serv_addr, client_addr;
    char buff[MAX_SIZE];
    struct timeval tv;
    int yes;

    listen_desc = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if(listen_desc < 0)
        printf("Failed creating socket\n");

    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(1234);

    yes = 1;

    if (setsockopt(listen_desc, SOL_SOCKET, SO_REUSEADDR,
        (char *) &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        close(listen_desc);
        return -1;
    }


    if (bind(listen_desc, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        printf("Failed to bind\n");

    listen(listen_desc, 5);

    maxfd = listen_desc; // Initialize the max descriptor with the first valid one we have
    maxi = -1; // index in the client connected descriptor array
    for (i=0; i<FD_SETSIZE; i++)
        client[i] = -1;  // this indicates the entry is available. It will be filled with a valid descriptor
    FD_ZERO(&savedset); // initialize the descriptor set to be monitored to empty
    FD_SET(listen_desc, &savedset); // add the current listening descriptor to the monitored set

    while(1) // main server loop
    {
        // assign all currently monitored descriptor set to a local variable. This is needed because select
        // will overwrite this set and we will lose track of what we originally wanted to monitor.
        memcpy(&tempset, &savedset, sizeof(fd_set));
	tv.tv_sec = 0;
	tv.tv_usec = 1000000;

        numready = select(maxfd+1, &tempset, NULL, NULL, &tv); // pass max descriptor and wait indefinitely until data arrives

        printf("Waiting maxfd %d\n", maxfd);
   if(numready > 0) {
        if(FD_ISSET(listen_desc, &tempset)) // new client connection
        {
            printf("new client connection\n");
            int size = sizeof(client_addr);
            conn_desc = accept(listen_desc, (struct sockaddr *)&client_addr, &size);
            for (j=0; j<FD_SETSIZE; j++)
                if(client[j] < 0)
                {
                    client[j] = conn_desc; // save the descriptor
//                    break;
                }

                FD_SET(conn_desc, &savedset); // add new descriptor to set of monitored ones
                if(conn_desc > maxfd)
                    maxfd = conn_desc; // max for select
                if(j > maxi)
                    maxi = j;   // max used index in client array
		printf("maxi %d\n", maxi);
        }

        for(k=0; k<=maxi+1; k++) // check all clients if any received data
        {
            if(client[k] > 0)
            {
                if(FD_ISSET(client[k], &savedset))
                {
                    int num_bytes;
                    if( (num_bytes = read(client[k], buff, MAX_SIZE)) > 0)
                    {
                        buff[num_bytes] = '\0';
                        printf("Received:- %s", buff);
                    }

                    if(num_bytes == 0)  // connection was closed by client
                    {
                        close(client[k]);
                        FD_CLR(client[k], &savedset);
                        client[k] = -1;
                    }

                    if(--numready <=0) // num of monitored descriptors returned by select call
                        break; 
                }
            }
        }
	}
    } // End main listening loop

    close(listen_desc);
    return 0;
}
