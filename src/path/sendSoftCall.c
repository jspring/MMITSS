#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <setjmp.h>

#include "tcp_utils.h"

//#include "mmitss_ports_and_message_numbers.h"
//#include "msgs.h"

// if commenting out the above two header files, please comment out below ----------------//

#define TRAF_CTL_OUTPORT 56004

#define SOFT_CALL_MSG    0x44
typedef struct
{
	unsigned short internal_msg_header; //=0xFFFF
	unsigned char  soft_call_msg_id; //=0x44
	unsigned int   ms_since_midnight;

	unsigned char call_phase;	// Bit mapped phase, bits 0-7 phase 1-8
	unsigned char call_obj;		// 1 = pedestrian
					// 2 = vehicular
					// 3 = TSP
	unsigned char call_type;	// 1 = call (for ped, vehicular & TSP early green)
					// 2 = extension (for vehicular & TSP green extension)
					// 3 = cancel (for vehicular & TSP)
	unsigned char call_number;	// how many times the SET soft-call commands need to be sent to the controller
					// = 1 when call_type = 1 or 3
					// = x (x <= 3) when call_obj = 2 & call_type = 2
					// = 0 when call_obj = 3 & call_type = 2
}__attribute__((packed)) mmitss_control_msg_t;

// if commenting out the above two header files, please comment out aove ----------------//

const uint8_t phaseBitMap[8] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};

static jmp_buf exit_env;

void sigproc(int code)
{
	longjmp(exit_env, code);
}

int main(int argc, char *argv[])
{
	// variables for socket
	int sockfd;
	struct sockaddr_in send_addr;
	char *send2addr = "127.0.0.1";
	int sendbytes;	
	short send_port = TRAF_CTL_OUTPORT;
	
	mmitss_control_msg_t control_msg;
	
	// create socket
	sockfd = tcp_unicast(&send_addr,send2addr,send_port);
	if (sockfd < 0)
	{
		fprintf(stderr,"%s: failed to create socket\n",argv[0]);
		exit(-1);
	}
	
	// connect to socket	
	if (connect(sockfd,(struct sockaddr *)&send_addr, sizeof(send_addr)) < 0)
	{
		perror("connect");
		close(sockfd);
		exit(-1);
	}
	else
	{
		fprintf(stdout,"%s: socket connection established ...\n",argv[0]);
	}

	// intercepts signals 
	signal(SIGINT, sigproc);
	signal(SIGTERM, sigproc);
	if (setjmp(exit_env) != 0)
	{
		fprintf(stdout,"%s: received termination command, exit ...\n",argv[0]);
		close(sockfd);		
		return (0);
	}
	
	while(1)
	{
		memset(&control_msg,0,sizeof(control_msg));
		
		int call_obj = 0;
		int call_type = 0;
		int call_phase = 0;
		int call_number = 1;
		while (!(call_obj >= 1 && call_obj <=3))
		{
			fprintf(stdout,"\nInput call object: 1-Ped, 2-Veh,3-TSP: ");
			scanf("%d",&call_obj);
		}
		while (!(call_type >= 1 && call_type <=3))
		{
			fprintf(stdout,"\nInput call type: 1-Call, 2-Extension,3-Cancel: ");
			scanf("%d",&call_type);
		}
		while (!(call_phase >= 1 && call_phase <=8))
		{
			fprintf(stdout,"\nInput call phase: 1 ... 8: ");
			scanf("%d",&call_phase);
		}
		fprintf(stdout,"\n");
		
		if (call_type == 2)
		{
			if (call_obj == 2)
			{
				call_number = 3;
			}
			else if (call_obj == 3)
			{
				call_number = 0;
			}
		}
		control_msg.internal_msg_header = 0xFFFF;
		control_msg.soft_call_msg_id = 0x44;
		control_msg.ms_since_midnight = 0;		
		control_msg.call_obj = call_obj;
		control_msg.call_type = call_type;
		control_msg.call_number = call_number;
		control_msg.call_phase = phaseBitMap[call_phase-1];
		
		sendbytes = sendto(sockfd, (char *)&control_msg, sizeof(control_msg), 0,
			(struct sockaddr *)&send_addr, sizeof(struct sockaddr));
		if (sendbytes != sizeof(control_msg))
		{
			fprintf(stderr,"%s: sent %d bytes instead of %d\n",argv[0],sendbytes,sizeof(control_msg));
		}	
		else
		{
			fprintf(stdout,"%s: sent %d bytes\n",argv[0],sendbytes);
		}
	}
}

	
