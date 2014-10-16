/* ab3418commudp - AB3418<-->database communicator
*/

#include <sys_os.h>
#include "msgs.h"
#include "sys_rt_linux.h"
#include "ab3418_libudp.h"
#include "ab3418commudp.h"
#include <udp_utils.h>
#include "local.h"

#include "mmitss_ports_and_message_numbers.h"

#include "asn_application.h"
#include "asn_internal.h"       /* for _ASN_DEFAULT_STACK_MAX */

#include "SPAT.h"
#include "TimeMark.h"

static jmp_buf exit_env;

static void sig_hand( int code )
{
        if( code == SIGALRM )
                return;
        else
                longjmp( exit_env, code );
}

static int sig_list[] =
{
        SIGINT,
        SIGQUIT,
        SIGTERM,
        SIGALRM,
	ERROR
};

int OpenTCPIPConnection(char *controllerIP, unsigned short port);
int process_phase_status( get_long_status8_resp_mess_typ *pstatus, int verbose, unsigned char greens, phase_status_t *pphase_status);
int init_spat(SPAT_t *spatType, unsigned char *spatbuf, int *spatmsgsize, sig_plan_msg_t *sig_plan_msg, raw_signal_status_msg_t *raw_signal_status_msg, phase_timing_t *pphase_timing[], get_long_status8_resp_mess_typ *long_status8, plan_params_t *pplan_params[], int verbose);

#define NUMMOVEMENTS	8

int main(int argc, char *argv[]) {

        struct sockaddr_in snd_addr;    /// used in sendto call
	int i;
	int j;
	int ab3418_fdin = -1;
	int ab3418_fdout = -1;
	int ca_spat_fdin = -1;
	int ca_spat_fdout = -1;
	int do_ab3418 = 1;
	char datamgr_readbuff[60];
	int len;
	int wait_for_data = 1;
	gen_mess_typ readBuff;
	get_long_status8_resp_mess_typ long_status8; 
	sig_plan_msg_t sig_plan_msg;
	phase_timing_t phase_timing[MAX_PHASES];
	phase_timing_t *pphase_timing[MAX_PHASES];
	raw_signal_status_msg_t raw_signal_status_msg;
	plan_params_t plan_params[MAX_PLANS + 1]; //Plan[0]=Free, Plan[10]=Saved plan
	plan_params_t *pplan_params[MAX_PLANS + 1];

	SPAT_t *spatType;
        IntersectionState_t *this_intersection;
        SPAT_t * spatType_decode=0;
	MovementState_t movestate[NUMMOVEMENTS];
	unsigned char spatbuf[1000];
	int spatmsgsize;

	mschedule_t mschedule;
	db_timing_set_2070_t db_timing_set_2070;
	db_timing_get_2070_t db_timing_get_2070;
	int retval;
	int check_retval;
	char ca_spat_port[20] = "/dev/ttyS0";
	char ab3418_port[20] = "/dev/ttyS1";
	char strbuf[300];
	struct sockaddr_storage datamgr_addr;


	struct timespec start_time;
	struct timespec end_time;
	struct timespec tp;
	struct tm *ltime;
	int dow;

//        struct timeb timeptr_raw;
//        struct tm time_converted;
	int opt;
	int verbose = 0;
	int low_spat = -1;
	int high_spat = -1;
	unsigned char no_control = 0;
//	unsigned char no_control_sav = 0;
//	char detector = 0;
	char *ip_str = NULL;
	short temp_port = 51013;
	struct sockaddr_in dest_addr;
	socklen_t addrlen = sizeof(struct sockaddr);
//	int rem;
//	unsigned char new_phase_assignment;	
	unsigned char output_spat_binary = 0;

	int datamgr_out = -5;             /// socket descriptor for UDP/TCP send
        int bytes_sent;         /// returned from sendto
 
        int datamgr_in = -5;             /// socket descriptor for UDP receive 

	struct timeval timeout;
	fd_set readfds, readfds_sav;
	fd_set writefds, writefds_sav;
	int maxfd = 0;
	int numready;
	unsigned char curr_plan_num = 255;
	unsigned char curr_plan_num_sav = 255;

        while ((opt = getopt(argc, argv, "A:S:v:na:beo:s:z")) != -1)
        {
                switch (opt)
                {
                  case 'A':
			memset(ab3418_port, 0, sizeof(ab3418_port));
                        strncpy(&ab3418_port[0], optarg, 16);
                        break;
                  case 'S':
			memset(ca_spat_port, 0, sizeof(ca_spat_port));
                        strncpy(&ca_spat_port[0], optarg, 16);
                        break;
                  case 'v':
                        verbose = 1;
                        break;
                  case 'n':
                        no_control = 1;
                        break;
                  case 'a':
//                      new_phase_assignment = (unsigned char)atoi(optarg);
                        break;
                  case 'b':
                        output_spat_binary = 1;
                        break;
                  case 'e':
                        do_ab3418 = 0;
                        break;
                  case 'l':
                        low_spat = atoi(optarg);
                        break;
                  case 'h':
                        high_spat = atoi(optarg);
                        break;
                  case 's':
                        ip_str = strdup(optarg);
                        break;
                  case 'o':
                        temp_port = atoi(optarg);
                        break;
		  case 'z':
		  default:
			fprintf(stderr, "Usage: %s -A <AB3418 port, (def. /dev/ttyS0)> -S <CA SPaT port, (def. /dev/ttyS1)> -v (verbose) -b (output binary SPaT message) -s <UDP unicast destination (def. 127.0.0.1)> -o <UDP unicast port> -l <lowest Battelle byte to display> -h <highest Battelle byte to display>\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if(low_spat >= 0) {
		if( (low_spat > sizeof(spatbuf)) || (high_spat > sizeof(spatbuf)) ) {
			fprintf(stderr, "Battelle bytes must be in the range 0-%d. Exiting....\n", sizeof(spatbuf));
			exit(EXIT_FAILURE);
		}
		if(high_spat < low_spat)
			high_spat = ((low_spat + 16) > sizeof(spatbuf)) ? sizeof(spatbuf): low_spat + 16;
	}

	//Allocate memory for J2735 SPaT 
        spatType = (SPAT_t *) calloc(1, sizeof(SPAT_t));
        spatType_decode = (SPAT_t *) calloc(1, sizeof(SPAT_t));
        this_intersection = (IntersectionState_t *) calloc(1, sizeof(IntersectionState_t));
        asn_sequence_add(&spatType->intersections, this_intersection);
	for(i=0; i<NUMMOVEMENTS ; i++) {
        	asn_sequence_add(&spatType->intersections.list.array[0]->states, &movestate[i]);
	}
 
	// Clear message structs
	memset(&db_timing_set_2070, 0, sizeof(db_timing_set_2070));
	memset(&raw_signal_status_msg, 0, sizeof(raw_signal_status_msg));
	memset(&snd_addr, 0, sizeof(snd_addr));
	memset(spatbuf, 0, sizeof(spatbuf));
	for(i=0; i<MAX_PHASES; i++) 
		pphase_timing[i] = &phase_timing[i];
	for(i=0; i<MAX_PLANS+1; i++) 
		pplan_params[i] = &plan_params[i];

	if(ip_str != NULL) {
		datamgr_out = udp_unicast_init(&dest_addr, ip_str, temp_port);
	}
	else {
		datamgr_out = OpenTCPIPConnection("localhost", (unsigned short)TRAF_CTL_IFACE_OUTPORT);
		if(datamgr_out >= 0)
			datamgr_in = datamgr_out;
	}
        /* Initialize serial port. */
	if(do_ab3418)
		check_retval = check_and_reconnect_serial(0, &ab3418_fdin, &ab3418_fdout, ab3418_port);
	check_retval = check_and_reconnect_serial(0, &ca_spat_fdin, &ca_spat_fdout, ca_spat_port);


		if (setjmp(exit_env) != 0) {
                	if(ab3418_fdin)
                       		close(ab3418_fdin);
                	if(ab3418_fdout)
                       		close(ab3418_fdout);
                	if(ca_spat_fdin)
                       		close(ca_spat_fdin);
                	if(ca_spat_fdout)
                       		close(ca_spat_fdout);
			exit(EXIT_SUCCESS);
		} else {
			sig_ign(sig_list, sig_hand);
		}


	if(do_ab3418){
	    printf("main 1: getting timing settings before infinite loop\n");
	    for(i=0; i<MAX_PHASES; i++) {
		db_timing_get_2070.phase = i+1;	
		db_timing_get_2070.page = 0x100; //phase timing page	
		retval = get_timing(&db_timing_get_2070, wait_for_data, &phase_timing[i], &ab3418_fdin, &ab3418_fdout, verbose);
		if(retval < 0) {
			usleep(500000);
			retval = get_timing(&db_timing_get_2070, wait_for_data, &phase_timing[i], 
				&ab3418_fdin, &ab3418_fdout, verbose);
		}
	    }
	}

	if(do_ab3418){
	    printf("main 2: getting coordination plan settings before infinite loop\n");
	    for(i=1; i<=MAX_PLANS; i++) {
		retval = get_coord_params(&plan_params[i], i, wait_for_data, &ab3418_fdout, &ab3418_fdin, verbose);
		if(retval < 0) {
			printf("get_coord_params returned %d for plan %d\n",
				retval,
				i
			);
			exit(EXIT_FAILURE);
		}
	    }
	}

	//Zero out saved fds
	FD_ZERO(&readfds_sav);
	FD_ZERO(&writefds_sav);
	if(datamgr_in >= 0) {
		FD_SET(datamgr_in, &readfds_sav);
		maxfd = datamgr_in;
	}
	if(datamgr_out >= 0){
		FD_SET(datamgr_out, &writefds_sav);
		maxfd = (datamgr_out > maxfd) ? datamgr_out : maxfd;
	}

	if(do_ab3418){
	    if(ab3418_fdin >= 0){
		FD_SET(ab3418_fdin, &readfds_sav);
		maxfd = (ab3418_fdin > maxfd) ? ab3418_fdin : maxfd;
	    }
	    if(ab3418_fdout >= 0){
		FD_SET(ab3418_fdout, &writefds_sav);
		maxfd = (ab3418_fdout > maxfd) ? ab3418_fdout : maxfd;
	    }
	}
	if(ca_spat_fdin >= 0){
		FD_SET(ca_spat_fdin, &readfds_sav);
		maxfd = (ca_spat_fdin > maxfd) ? ca_spat_fdin : maxfd;
	}
	if(ca_spat_fdout >= 0){
		FD_SET(ca_spat_fdout, &writefds_sav);
		maxfd = (ca_spat_fdout > maxfd) ? ca_spat_fdout : maxfd;
	}

while(1) {

        readfds = readfds_sav;
        writefds = writefds_sav;
	timeout.tv_sec = 0;
	timeout.tv_usec = 400000;
        numready = select(maxfd+1, &readfds, NULL, NULL, &timeout); // Tells me one of the old or new sockets is ready to read
	if(numready <=0) {
		if(errno != EINTR)
			perror("select");
		if(numready == 0) {
			fprintf(stderr, "select timeout: not getting CA SPaT within last 400 ms. Will try to reconnect to %s\n", ca_spat_port);
//			check_retval = check_and_reconnect_serial(0, &ca_spat_fdin, &ca_spat_fdout, ca_spat_port);
//			continue;
		}
	}

	if(FD_ISSET(ca_spat_fdin, &readfds)) {
		if(do_ab3418){
			retval = get_status(wait_for_data, &readBuff, ab3418_fdin, ab3418_fdout, verbose);
		}
		retval = get_spat(wait_for_data, &raw_signal_status_msg, ca_spat_fdin, verbose, output_spat_binary);
		memcpy(&long_status8, &readBuff, sizeof(get_long_status8_resp_mess_typ));
//printf("ab3418commudp: Got to 1\n");
		memset(spatbuf, 0, sizeof(spatbuf));
		spatmsgsize = 0;
		init_spat(spatType, spatbuf, &spatmsgsize, &sig_plan_msg, &raw_signal_status_msg, pphase_timing, &long_status8, pplan_params, verbose);
//        printf("ab3418commudp: Call Decoder...:\n");
//        ber_decode(0, &asn_DEF_SPAT,(void **)&spatType_decode, spatbuf, spatmsgsize);
//        xer_fprint(stdout, &asn_DEF_SPAT, spatType_decode);
printf("<currState> interval A %d B %d spatmsgsize %d\n", raw_signal_status_msg.interval_A, raw_signal_status_msg.interval_B, spatmsgsize);
printf("ab3418commudp: Got to 2 spatmsgsize %d sizeof(spatbuf) %d\n", spatmsgsize, sizeof(spatbuf));
               	 printf("spatbuf:\n");
        	for(i=0;i < spatmsgsize; i++)
               	 printf("#%d %x\t", i, (uint8_t) spatbuf[i]);
               	 printf("\n");

if(verbose) {                                           
        printf("ab3418commudp print spatbuf:\n");
        for(j=0; j<spatmsgsize; j+=10){             
                printf("%d: ", j);                                  
                for(i=0; i<10; i++)                                 
                        printf("%hhx ", spatbuf[j+i]);           
                printf("\n");
        }                                                       
}                                                       

		if(low_spat >= 0) {
			printf("ab3418udp:Battelle Spat: ");
			for(i=low_spat; i <= high_spat; i++)
				printf("#%d %#hhx ", i, spatbuf[i]);
			printf("\n");
		}
		if(do_ab3418){
		    if(long_status8.pattern < 252) 
			curr_plan_num = (long_status8.pattern / 3) + 1;
		    else
			curr_plan_num = 0;
		    if(curr_plan_num != curr_plan_num_sav) {
			printf("Changing plan number from %d to %d long_status8.pattern %d\n", curr_plan_num_sav, curr_plan_num, long_status8.pattern);
			curr_plan_num_sav = curr_plan_num;
//			if(long_status8.pattern < 252) 
			retval = get_coord_params(&plan_params[curr_plan_num], 
				curr_plan_num, 
//				wait_for_data, &ab3418_fdout, &ab3418_fdin, verbose);
				wait_for_data, &ab3418_fdout, &ab3418_fdin, 1);
//			if(retval < 0) {
				printf("get_coord_params returned %d for plan %d %d\n",
					retval, curr_plan_num, raw_signal_status_msg.plan_num);
//			}  
			for(i=0; i<MAX_PHASES; i++) {
				db_timing_get_2070.phase = i+1;	
				db_timing_get_2070.page = 0x100; //phase timing page	
				retval = get_timing(&db_timing_get_2070, wait_for_data, &phase_timing[i], 
					&ab3418_fdin, &ab3418_fdout, verbose);
				if(retval < 0) {
					usleep(500000);
					retval = get_timing(&db_timing_get_2070, wait_for_data, &phase_timing[i], 
						&ab3418_fdin, &ab3418_fdout, verbose);
				}
			}
			retval = build_sigplanmsg(&sig_plan_msg, pphase_timing, 
//				&plan_params[curr_plan_num], &long_status8, verbose);
				&plan_params[curr_plan_num], &long_status8, 1);
			if(datamgr_out >= 0)
//			    bytes_sent = sendto(datamgr_out, &sig_plan_msg, sizeof(sig_plan_msg_t), 0,
//				(struct sockaddr *) &snd_addr, sizeof(snd_addr));
			    bytes_sent = write(datamgr_out, &sig_plan_msg, sizeof(sig_plan_msg_t));
			else {
				datamgr_out = OpenTCPIPConnection("localhost", (unsigned short)TRAF_CTL_IFACE_OUTPORT);
				datamgr_in = datamgr_out;

				//Zero out saved fds
				FD_ZERO(&readfds_sav);
				FD_ZERO(&writefds_sav);
				if(datamgr_in >= 0) {
					FD_SET(datamgr_in, &readfds_sav);
					maxfd = datamgr_in;
				}
				if(datamgr_out >= 0){
					FD_SET(datamgr_out, &writefds_sav);
					maxfd = (datamgr_out > maxfd) ? datamgr_out : maxfd;
				}

				if(ab3418_fdin >= 0){
					FD_SET(ab3418_fdin, &readfds_sav);
					maxfd = (ab3418_fdin > maxfd) ? ab3418_fdin : maxfd;
				}
				if(ab3418_fdout >= 0){
					FD_SET(ab3418_fdout, &writefds_sav);
					maxfd = (ab3418_fdout > maxfd) ? ab3418_fdout : maxfd;
				}
				if(ca_spat_fdin >= 0){
					FD_SET(ca_spat_fdin, &readfds_sav);
					maxfd = (ca_spat_fdin > maxfd) ? ca_spat_fdin : maxfd;
				}
				if(ca_spat_fdout >= 0){
					FD_SET(ca_spat_fdout, &writefds_sav);
					maxfd = (ca_spat_fdout > maxfd) ? ca_spat_fdout : maxfd;
				}
			}

		    }
		}
//	    bytes_sent = write(datamgr_out, &spatbuf, spatmsgsize);
	    bytes_sent = sendto(datamgr_out, &spatbuf, spatmsgsize, 0, (struct sockaddr *)&dest_addr, addrlen);

//exit(EXIT_SUCCESS);
	}
	if(do_ab3418){
	    if( (datamgr_in >=0) && (FD_ISSET(datamgr_in, &readfds)) ) {
		memset(datamgr_readbuff, 0, sizeof(datamgr_readbuff));
		if( (retval = recvfrom(datamgr_in, datamgr_readbuff, sizeof(datamgr_readbuff), 0,
			(struct sockaddr *)&datamgr_addr, (socklen_t *)&len)) > 0) {
			if(datamgr_readbuff[2] == SIGNAL_SCHED_MSG) {
				printf("datamgr_readbuff: \n");
				for(i=0; i<sizeof(datamgr_readbuff); i++) 
					printf("#%d %hhx ", i, datamgr_readbuff[i]);
				printf("\n");
				memcpy(&mschedule, datamgr_readbuff, sizeof(mschedule_t));
				printf("Timing schedule request:\nPhase sequence: ");
				for(i=0; i<8; i++)
					printf("%03hhu ", mschedule.phase_sequence.phase_sequence[i]);
				printf("\nPhase duration: ");
				for(i=0; i<8; i++)
					printf("%03hhu ", mschedule.phase_duration.phase_duration[i]);
				printf("\n");

no_control = 0;
				if(no_control == 0)
					memcpy(&plan_params[10], &plan_params[9], sizeof(plan_params_t));
					printf("Hall cycle length %d\n", plan_params[10].cycle_length);
					retval = set_coord_params(&plan_params[9], 9, &mschedule, 1, 
						ab3418_fdout, ab3418_fdin, verbose);
			}
		}
	    }
	}

	if(output_spat_binary) {
		bytes_sent = sendto(datamgr_out, &spatbuf, spatmsgsize, 0,
			(struct sockaddr *) &snd_addr, sizeof(snd_addr));
	}
//	else {
//		memset(strbuf, 0, sizeof(strbuf));
//		for(i=0; i<8; i++)
//                sprintf(strbuf+(12*i), "#%u %hhu %hhu ",
//			i+1,
//                        battelle_spat.movement[i].min_time_remaining.mintimeremaining,
//                        battelle_spat.movement[i].max_time_remaining.maxtimeremaining
//                );
//                sprintf(strbuf+(12*(i+1))+1, "\n");
//		bytes_sent = sendto(datamgr_out, strbuf, sizeof(strbuf), 0,
//		     (struct sockaddr *) &snd_addr, sizeof(snd_addr));
//	}

	fflush(NULL);

	if (verbose) {
		printf("%d bytes sent\n", bytes_sent);
		printf("port %d addr 0x%08x\n", ntohs(snd_addr.sin_port),
			ntohl(snd_addr.sin_addr.s_addr));
		fflush(stdout);
	}

//	if (bytes_sent < 0) {
//		perror("sendto error");
//		printf("port %d addr 0x%08x\n", ntohs(snd_addr.sin_port),
//			ntohl(snd_addr.sin_addr.s_addr));
//		fflush(stdout);
//	}

		if(verbose) {
			clock_gettime(CLOCK_REALTIME, &end_time);
			printf("%s: Time for function call %f sec\n", argv[0], 
				(end_time.tv_sec + (end_time.tv_nsec/1.0e9)) - 
				(start_time.tv_sec + (start_time.tv_nsec/1.0e9))
				);
			clock_gettime(CLOCK_REALTIME, &start_time);
		}

		clock_gettime(CLOCK_REALTIME, &tp);
		ltime = localtime(&tp.tv_sec);
		dow = ltime->tm_wday;
//		printf("dow=%d dow%%6=%d hour %d\n", dow, dow % 6, ltime->tm_hour);

		if( ((dow % 6) == 0) || (ltime->tm_hour < 15) || (ltime->tm_hour >= 19) ) {
			no_control = 1;
		}
		else {
			no_control = 0;
		}

	}
	return retval;
}

int OpenTCPIPConnection(char *controllerIP, unsigned short port) {
        struct addrinfo hints;
        struct addrinfo *result, *rp;
        int sfd, s;
	char port_str[6];

	sprintf(port_str, "%hu", port);

        /* Obtain address(es) matching host/port */
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_STREAM; /* TCP socket */
        hints.ai_flags = 0;
        hints.ai_protocol = IPPROTO_TCP;     /* Any protocol */
        s = getaddrinfo(controllerIP, port_str, &hints, &result);
        if (s != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
        }

        /* Obtain address(es) matching host/port */
        /* getaddrinfo() returns a list of address structures.
        Try each address until we successfully connect(2).
        If socket(2) (or connect(2)) fails, we (close the socket
        and) try the next address. */

        for (rp = result; rp != NULL; rp = rp->ai_next) {
                sfd = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
                if (sfd == -1) {
                        perror("socket");
                        continue;
                }
                if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
                        break;              /* Success */
                }
                perror("connect");
                close(sfd);
        }
        if (rp == NULL) {                /* No address succeeded */
                fprintf(stderr, "Could not connect\n");
                return -1;
        }
        freeaddrinfo(result);       /* No longer needed */
        return sfd;
}

#define RED	0
#define YELOW	1
#define GREEN	2

char interval_color[] = {
	GREEN,
	RED,
	GREEN,
	RED,
	GREEN,
	GREEN,
	GREEN,
	GREEN,
	RED,
	RED,
	RED,
	RED,
	YELOW,
	YELOW,
	RED,
	RED
};

int process_phase_status( get_long_status8_resp_mess_typ *pstatus, int verbose, unsigned char greens, phase_status_t *pphase_status) {


        struct timeb timeptr_raw;
        struct tm time_converted;
        static struct timespec start_time;
        struct timespec end_time;
        int i;
        unsigned char interval_temp = 0;
	static unsigned char greens_sav = 0;

        memset(pphase_status, 0, sizeof(phase_status_t));
        pphase_status->greens = greens;
        for(i=0; i<8; i++) {
                if(pstatus->active_phase & (1 << i)) {
                        if( (pstatus->active_phase & (1 << i)) <= 8) {
                                interval_temp = pstatus->interval & 0xf;
                                pphase_status->phase_status_colors[i] = interval_color[interval_temp];
                                if( (interval_temp == 0xc) || (interval_temp == 0xd) )
                                        pphase_status->yellows |= pstatus->active_phase & 0xf;
                        }
                        else
                                interval_temp = (pstatus->interval >> 4) & 0xf;
                                pphase_status->phase_status_colors[i] = interval_color[interval_temp];
                                if( (interval_temp == 0xc) || (interval_temp == 0xd) )
                                        pphase_status->yellows |= pstatus->active_phase & 0xf0;
                }
        }
        pphase_status->reds = ~(pphase_status->greens | pphase_status->yellows);

	if( (greens_sav == 0x0) && ((greens  == 0x44 )) )
		clock_gettime(CLOCK_REALTIME, &start_time);
	if( (greens_sav == 0x44) && ((greens  == 0x40 )) ) {
		clock_gettime(CLOCK_REALTIME, &end_time);
		printf("process_phase_status: Green time for phase 3 %f sec\n\n", 
			(end_time.tv_sec + (end_time.tv_nsec/1.0e9)) -
			(start_time.tv_sec +(start_time.tv_nsec/1.0e9))
		);
	}
	if( (greens_sav == 0x22) && ((greens & greens_sav) == 0 )) {
		printf("\n\nPhases 2 and 6 should be yellow now\n");
		pphase_status->barrier_flag = 1;
	}
	else
		pphase_status->barrier_flag = 0;
	greens_sav = greens;
		

//                Get time of day and save in the database.
        ftime ( &timeptr_raw );
        localtime_r ( &timeptr_raw.time, &time_converted );
        pphase_status->ts.hour = time_converted.tm_hour;
        pphase_status->ts.min = time_converted.tm_min;
        pphase_status->ts.sec = time_converted.tm_sec;
        pphase_status->ts.millisec = timeptr_raw.millitm;

        return 0;
}	

/*
int get_detector(int wait_for_data, gen_mess_typ *readBuff, int ab3418_fdin, int ab3418_fdout, char detector, char verbose) {
        int msg_len;
        fd_set readfds;
        fd_set writefds;
        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";
        int ser_driver_retval;
        tsmss_get_msg_request_t detector_get_request;

        if(verbose != 0)
                printf("get_detector 1: Starting get_detector request\n");
        detector_get_request.get_hdr.start_flag = 0x7e;
        detector_get_request.get_hdr.address = 0x05;
        detector_get_request.get_hdr.control = 0x13;
        detector_get_request.get_hdr.ipi = 0xc0;
        detector_get_request.get_hdr.mess_type = 0x87;
        detector_get_request.get_hdr.page_id = 0x07;
        detector_get_request.get_hdr.block_id = (detector/NUM_DET_PER_BLOCK) + 1;//Block ID 1=det 1-4
        detector_get_request.get_tail.FCSmsb = 0x00;
        detector_get_request.get_tail.FCSlsb = 0x00;

        Now append the FCS. 
        msg_len = sizeof(tsmss_get_msg_request_t) - 4;
        fcs_hdlc(msg_len, &detector_get_request, verbose);

        FD_ZERO(&writefds);
        FD_SET(ab3418_fdout, &writefds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        if( (selectval = select(ab3418_fdout+1, NULL, &writefds, NULL, &timeout)) <=0) {
                perror("select 56");
                outportisset = (FD_ISSET(ab3418_fdout, &writefds)) == 0 ? "no" : "yes";
                printf("get_detector 2: ab3418_fdout %d selectval %d outportisset %s\n", ab3418_fdout, selectval, outportisset);
                return -3;
        }
        write ( ab3418_fdout, &detector_get_request, msg_len+4 );
        fflush(NULL);
        sleep(2);


        ser_driver_retval = 100;

        if(wait_for_data && readBuff) {
                FD_ZERO(&readfds);
                FD_SET(ab3418_fdin, &readfds);
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                if( (selectval = select(ab3418_fdin+1, &readfds, NULL, NULL, &timeout)) <=0) {
                        perror("select 57");
                        inportisset = (FD_ISSET(ab3418_fdin, &readfds)) == 0 ? "no" : "yes";
                        printf("get_detector 3: ab3418_fdin %d selectval %d inportisset %s\n", ab3418_fdin, selectval, inportisset);
                        return -2;
                }
                ser_driver_retval = ser_driver_read(readBuff, ab3418_fdin, verbose);
                if(ser_driver_retval == 0) {
                        printf("get_detector 4: Lost USB connection\n");
                        return -1;
                }
        }
        if(verbose != 0)
                printf("get_detector 5-end: ab3418_fdin %d selectval %d inportisset %s ab3418_fdout %d selectval %d outportisset %s ser_driver_retval %d\n", ab3418_fdin, selectval, inportisset, ab3418_fdout, selectval, outportisset, ser_driver_retval);
        return 0;
}

int set_detector(detector_msg_t *pdetector_set_request, int ab3418_fdin, int ab3418_fdout, char detector, char verbose) {
        int msg_len;
        fd_set readfds;
        fd_set writefds;
        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";
        int ser_driver_retval;
        int wait_for_data = 1;
        gen_mess_typ readBuff;
        char *tempbuf = (char *)pdetector_set_request;
        int i;
printf("set_detector input message: ");
for(i=0; i<sizeof(detector_msg_t); i++)
        printf("%#hhx ", tempbuf[i]);
printf("\n");

        if(verbose != 0)
                printf("set_detector 1: Starting set_detector request\n");
        pdetector_set_request->detector_hdr.start_flag = 0x7e;
        pdetector_set_request->detector_hdr.address = 0x05;
        pdetector_set_request->detector_hdr.control = 0x13;
        pdetector_set_request->detector_hdr.ipi = 0xc0;
        pdetector_set_request->detector_hdr.mess_type = 0x96;
        pdetector_set_request->detector_hdr.page_id = 0x07;
        pdetector_set_request->detector_hdr.block_id = (detector/NUM_DET_PER_BLOCK) + 1;//Block ID 1=det 1-4
        pdetector_set_request->detector_tail.FCSmsb = 0x00;
        pdetector_set_request->detector_tail.FCSlsb = 0x00;

        // Now append the FCS. 
        msg_len = sizeof(detector_msg_t) - 4;
        fcs_hdlc(msg_len, pdetector_set_request, verbose);

printf("set_detector output message: ");
for(i=0; i<sizeof(detector_msg_t); i++)
        printf("%#hhx ", tempbuf[i]);
printf("\n");

        FD_ZERO(&writefds);
        FD_SET(ab3418_fdout, &writefds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        if( (selectval = select(ab3418_fdout+1, NULL, &writefds, NULL, &timeout)) <=0) {
                perror("select 14");
                outportisset = (FD_ISSET(ab3418_fdout, &writefds)) == 0 ? "no" : "yes";
                printf("set_detector 2: ab3418_fdout %d selectval %d outportisset %s\n", ab3418_fdout, selectval, outportisset);
                return -3;
        }
        write ( ab3418_fdout, pdetector_set_request, sizeof(detector_msg_t));
        fflush(NULL);
        sleep(2);

        ser_driver_retval = 100;
        if(wait_for_data) {
                FD_ZERO(&readfds);
                FD_SET(ab3418_fdin, &readfds);
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                if( (selectval = select(ab3418_fdin+1, &readfds, NULL, NULL, &timeout)) <=0) {
                        perror("select 15");
                        inportisset = (FD_ISSET(ab3418_fdin, &readfds)) == 0 ? "no" : "yes";
                        printf("set_detector 3: ab3418_fdin %d selectval %d inportisset %s\n", ab3418_fdin, selectval, inportisset);
                        return -2;
                }
                ser_driver_retval = ser_driver_read(&readBuff, ab3418_fdin, verbose);
                if(ser_driver_retval == 0) {
                        printf("set_detector 4: Lost USB connection\n");
                        return -1;
                }
        }
        if(verbose != 0)
                printf("set_detector 5-end: ab3418_fdin %d selectval %d inportisset %s ab3418_fdout %d selectval %d outportisset %s ser_driver_retval %d\n", ab3418_fdin, selectval, inportisset, ab3418_fdout, selectval, outportisset, ser_driver_retval);
        return 0;
}
*/

int init_spat(SPAT_t *spatType, unsigned char *spatbuf, int *spatmsgsize, sig_plan_msg_t *sig_plan_msg, raw_signal_status_msg_t *ca_spat, phase_timing_t *pphase_timing[], get_long_status8_resp_mess_typ *long_status8, plan_params_t *pplan_params[], int verbose) {
        

	int ret;
	asn_enc_rval_t ec; /* Encoder return value */
	asn_dec_rval_t rval;
        
        
	char *spat_msg_name = "MMITSS_SPaT_msg";
	char *intersection_name = "El_Camino_and_Matadero";
	char movement_name[8] = "Phase 1";
	char intersection_id[2]={0x12, 0x34};
	char intersection_status;
	char laneset[4] = {1, 2, 3, 4};
	int tstemp;
	static long movementcnt = NUMMOVEMENTS;
	static long lanecnt[NUMMOVEMENTS] = {0};
	static long SignalLightState[NUMMOVEMENTS] = {0};
	static long pedstate[NUMMOVEMENTS] = {0};
	static long yellstate[NUMMOVEMENTS] = {0};
	static long yellpedstate[NUMMOVEMENTS] = {0};
	static long specialstate[NUMMOVEMENTS] = {0};
	static long stateconfidence[NUMMOVEMENTS] = {0};
	static long objectcount[NUMMOVEMENTS] = {0};
	static long peddetect[NUMMOVEMENTS] = {0};
	static long timeToChange[NUMMOVEMENTS] = {0};
	static long yellTimeToChange[NUMMOVEMENTS] = {0};
        
        int i;
        int j;
	timestamp_t ts;

        SPAT_t * spatType_decode=0;
        spatType_decode = (SPAT_t *) calloc(1, sizeof(SPAT_t));

	//Define SPAT_t members
        spatType->msgID = 13;  	//SPAT message ID
        spatType->name = 	//SPAT intersection name (-1 uses strlen to get size)
		OCTET_STRING_new_fromBuf(&asn_DEF_OCTET_STRING, spat_msg_name, -1);


        spatType->intersections.list.array[0]->name =
		OCTET_STRING_new_fromBuf(&asn_DEF_OCTET_STRING, intersection_name, -1);

        spatType->intersections.list.array[0]->id =
		*OCTET_STRING_new_fromBuf(&asn_DEF_OCTET_STRING, intersection_id, 2);  //IntersectionID


	//IntersectionStatusObject
        if( (long_status8->pattern == 254) || (long_status8->status == 2) )
                intersection_status = INTERSECTION_STATUS_CONFLICT_FLASH;
        else if(long_status8->preemption != 0)
                intersection_status = INTERSECTION_STATUS_PREEMPT;
        else if( ((long_status8->interval & 0x0f) == 0x0a) || ((long_status8->interval & 0xf0) == 0xa0) )
                intersection_status = INTERSECTION_STATUS_STOP_TIME;
        else
                intersection_status = INTERSECTION_STATUS_MANUAL;
        spatType->intersections.list.array[0]->status =
		*OCTET_STRING_new_fromBuf(&asn_DEF_OCTET_STRING, &intersection_status, 1);  //IntersectionStatusObject

        //TimeMark
        get_current_timestamp(&ts);
        tstemp = (1000 * ((3600 * ts.hour) + (60 * ts.min) + ts.sec)) + ts.millisec;
        spatType->intersections.list.array[0]->timeStamp = (TimeMark_t *)&tstemp;

        //lanesCnt: Number of Movement States to follow
        spatType->intersections.list.array[0]->lanesCnt = &movementcnt;

	//Define SPAT_t->IntersectionState_t members
        //Sequence of IntersectionState (there's only one intersection per MRP)

#define GREENBALL	0x0001
#define YELLOWBALL	0x0002
#define REDBALL		0x0004
#define DONT_WALK	0x0001
#define FLASH_DONOT_WALK	0x0002
#define WALK		0x0004
#define IGNORE_STATE	-1

for(i = 0; i<NUMMOVEMENTS; i++) {
	j = 1 << i;

        SignalLightState[i] = 0;  //reset the signal state
	pedstate[i] = 0; 	  //reset the pedestrian state

	if(ca_spat->active_phase & j) {
		if(j < 4) {
			switch(ca_spat->interval_A) {
				case 0x00: //Walk
        				//Ped state
					pedstate[i] = WALK;
					//Next ped state
					yellpedstate[i] = FLASH_DONOT_WALK;

					//Signal state
        				spatType->intersections.list.array[0]->states.list.array[i]->currState = NULL;
					//Next signal state
					spatType->intersections.list.array[0]->states.list.array[i]->yellState = NULL;
					spatType->intersections.list.array[0]->states.list.array[i]->pedState = &pedstate[i];
					spatType->intersections.list.array[0]->states.list.array[i]->yellPedState = &yellpedstate[i];

					break;
				case 0x01: //Don't Walk
					pedstate[i] = FLASH_DONOT_WALK;
					yellpedstate[i] = DONT_WALK;
					//Signal state
        				spatType->intersections.list.array[0]->states.list.array[i]->currState = NULL;
        				//yellow state - the next state of a motorised lane
					spatType->intersections.list.array[0]->states.list.array[i]->yellState = NULL;

					spatType->intersections.list.array[0]->states.list.array[i]->pedState = &pedstate[i];
					spatType->intersections.list.array[0]->states.list.array[i]->yellPedState = &yellpedstate[i];
					break;
				case 0x02: //Min Green
				case 0x04: //Added initial
				case 0x05: //Passage-resting
				case 0x06: //Max Gap
				case 0x07: //Min Gap
        				//SignalLightState - Motorised lane
					SignalLightState[i] = GREENBALL;  //Greenball
					yellstate[i] = YELLOWBALL;
        				spatType->intersections.list.array[0]->states.list.array[i]->currState = &SignalLightState[i];
					spatType->intersections.list.array[0]->states.list.array[i]->yellState = &yellstate[i];
        				spatType->intersections.list.array[0]->states.list.array[i]->pedState = NULL;
        				spatType->intersections.list.array[0]->states.list.array[i]->yellPedState = NULL;
					break;
				case 0x0C: //Max termination
				case 0x0D: //Gap termination
        				//SignalLightState - Motorised lane
					SignalLightState[i] = YELLOWBALL;  //Greenball
					yellstate[i] = REDBALL;
        				spatType->intersections.list.array[0]->states.list.array[i]->currState = &SignalLightState[i];
					spatType->intersections.list.array[0]->states.list.array[i]->yellState = &yellstate[i];
        				spatType->intersections.list.array[0]->states.list.array[i]->pedState = NULL;
        				spatType->intersections.list.array[0]->states.list.array[i]->yellPedState = NULL;
					break;
				default:
					SignalLightState[i] = REDBALL;  //Greenball
					yellstate[i] = GREENBALL;
        				spatType->intersections.list.array[0]->states.list.array[i]->currState = &SignalLightState[i];
					spatType->intersections.list.array[0]->states.list.array[i]->yellState = &yellstate[i];
        				spatType->intersections.list.array[0]->states.list.array[i]->pedState = NULL;
        				spatType->intersections.list.array[0]->states.list.array[i]->yellPedState = NULL;
					break;
			}
		}
		else {
			switch(ca_spat->interval_B) {
				case 0x00: //Walk
        				//Ped state
					pedstate[i] = WALK;
					//Next ped state
					yellpedstate[i] = FLASH_DONOT_WALK;

					//Signal state
        				spatType->intersections.list.array[0]->states.list.array[i]->currState = NULL;
					//Next signal state
					spatType->intersections.list.array[0]->states.list.array[i]->yellState = NULL;
					spatType->intersections.list.array[0]->states.list.array[i]->pedState = &pedstate[i];
					spatType->intersections.list.array[0]->states.list.array[i]->yellPedState = &yellpedstate[i];

					break;
				case 0x01: //Don't Walk
					pedstate[i] = FLASH_DONOT_WALK;
					yellpedstate[i] = DONT_WALK;
					//Signal state
        				spatType->intersections.list.array[0]->states.list.array[i]->currState = NULL;
        				//yellow state - the next state of a motorised lane
					spatType->intersections.list.array[0]->states.list.array[i]->yellState = NULL;

					spatType->intersections.list.array[0]->states.list.array[i]->pedState = &pedstate[i];
					spatType->intersections.list.array[0]->states.list.array[i]->yellPedState = &yellpedstate[i];
					break;
				case 0x02: //Min Green
				case 0x04: //Added initial
				case 0x05: //Passage-resting
				case 0x06: //Max Gap
				case 0x07: //Min Gap
        				//SignalLightState - Motorised lane
					SignalLightState[i] = GREENBALL;  //Greenball
					yellstate[i] = YELLOWBALL;
        				spatType->intersections.list.array[0]->states.list.array[i]->currState = &SignalLightState[i];
					spatType->intersections.list.array[0]->states.list.array[i]->yellState = &yellstate[i];
        				spatType->intersections.list.array[0]->states.list.array[i]->pedState = NULL;
        				spatType->intersections.list.array[0]->states.list.array[i]->yellPedState = NULL;
					break;
				case 0x0C: //Max termination
				case 0x0D: //Gap termination
        				//SignalLightState - Motorised lane
					SignalLightState[i] = YELLOWBALL;  //Greenball
					yellstate[i] = REDBALL;
        				spatType->intersections.list.array[0]->states.list.array[i]->currState = &SignalLightState[i];
					spatType->intersections.list.array[0]->states.list.array[i]->yellState = &yellstate[i];
        				spatType->intersections.list.array[0]->states.list.array[i]->pedState = NULL;
        				spatType->intersections.list.array[0]->states.list.array[i]->yellPedState = NULL;
					break;
				default:
					SignalLightState[i] = REDBALL;  //Greenball
					yellstate[i] = GREENBALL;
        				spatType->intersections.list.array[0]->states.list.array[i]->currState = &SignalLightState[i];
					spatType->intersections.list.array[0]->states.list.array[i]->yellState = &yellstate[i];
        				spatType->intersections.list.array[0]->states.list.array[i]->pedState = NULL;
        				spatType->intersections.list.array[0]->states.list.array[i]->yellPedState = NULL;
					break;
			}
		}
	}
	else {
		SignalLightState[i] = REDBALL;
        	spatType->intersections.list.array[0]->states.list.array[i]->currState = &SignalLightState[i];
	}

        //MovementState No1.
	sprintf(movement_name, "Phase %d", i+1);
        spatType->intersections.list.array[0]->states.list.array[i]->movementName =
		OCTET_STRING_new_fromBuf(&asn_DEF_OCTET_STRING, movement_name, -1);  //movementName
	lanecnt[i] = pphase_timing[i]->min_green;
        spatType->intersections.list.array[0]->states.list.array[i]->laneCnt = &lanecnt[i];  //lane count
        ret=OCTET_STRING_fromBuf(&spatType->intersections.list.array[0]->states.list.array[i]->laneSet, laneset, 4);  //lane set
        //Special state
        specialstate[i] = 0;
        spatType->intersections.list.array[0]->states.list.array[i]->specialState = &specialstate[i];
        //time to change
        timeToChange[i] = 200;
        spatType->intersections.list.array[0]->states.list.array[i]->timeToChange = timeToChange[i];
        //state confidence
        stateconfidence[i] = 0;
        spatType->intersections.list.array[0]->states.list.array[i]->stateConfidence = &stateconfidence[i];
        //yellow time to change
        yellTimeToChange[i] = 12001;
        spatType->intersections.list.array[0]->states.list.array[i]->yellTimeToChange = (TimeMark_t *)&yellTimeToChange[i];
        //yellow state confidence
        stateconfidence[i] = 0;
        spatType->intersections.list.array[0]->states.list.array[i]->yellStateConfidence = &stateconfidence[i];
        //vehicle count
        spatType->intersections.list.array[0]->states.list.array[i]->vehicleCount = &objectcount[i];
        //ped detect
//        peddetect[i] |= 1<<1;
        peddetect[i] = 1;
        spatType->intersections.list.array[0]->states.list.array[i]->pedDetect = &peddetect[i];
        //ped count
        objectcount[i] += 1;
        spatType->intersections.list.array[0]->states.list.array[i]->pedCount=&objectcount[i];

}
        ec = der_encode_to_buffer(&asn_DEF_SPAT, spatType, spatbuf, 1000);

	switch(spatbuf[1]) {
		case 0x82:
			*spatmsgsize = (spatbuf[2] << 8) + spatbuf[3] + 4;
			break;
		case 0x81:
			*spatmsgsize = spatbuf[2] + 3;
			break;
		default:
			*spatmsgsize = spatbuf[1] + 2;
			break;
	}
	if(verbose) {
		printf("get_spat: Call Decoder...:\n");
		rval = ber_decode(0, &asn_DEF_SPAT,(void **)&spatType_decode, spatbuf, *spatmsgsize);
		xer_fprint(stdout, &asn_DEF_SPAT, spatType_decode);
	}
	return 0;
}
