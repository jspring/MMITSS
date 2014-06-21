/* ab3418commudp - AB3418<-->database communicator
*/

#include <sys_os.h>
#include "msgs.h"
#include "sys_rt_linux.h"
#include "ab3418_libudp.h"
#include "ab3418commudp.h"
#include <udp_utils.h>
#include "Battelle_SPaT_MIB.h"
#include "local.h"

#define TRAFFICCTLPORT	5300

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

int OpenTSCPConnection(char *controllerIP, char *port);
int process_phase_status( get_long_status8_resp_mess_typ *pstatus, int verbose, unsigned char greens, phase_status_t *pphase_status);
extern int spat2battelle(raw_signal_status_msg_t *ca_spat, battelle_spat_t *battelle_spat, phase_timing_t *phase_timing[8], int verbose);

int main(int argc, char *argv[]) {

        struct sockaddr_in snd_addr;    /// used in sendto call
	int i;
	int ab3418_fdin = -1;
	int ab3418_fdout = -1;
	int ca_spat_fdin = -1;
	int ca_spat_fdout = -1;
	char trafficctlreadbuf[60];
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
	battelle_spat_t battelle_spat;
	char *pbattelle_spat;
	mschedule_t mschedule;
	db_timing_set_2070_t db_timing_set_2070;
	db_timing_get_2070_t db_timing_get_2070;
	int retval;
	int check_retval;
	char ab3418_port[20] = "/dev/ttyS0";
	char ca_spat_port[20] = "/dev/ttyS1";
	char strbuf[300];
	struct sockaddr_storage trafficctl_addr;


	struct timespec start_time;
	struct timespec end_time;
	struct timespec tp;
	struct tm *ltime;
	int dow;

//        struct timeb timeptr_raw;
//        struct tm time_converted;
	int opt;
	int verbose = 0;
	int low_battelle = -1;
	int high_battelle = -1;
	unsigned char no_control = 0;
//	unsigned char no_control_sav = 0;
//	char detector = 0;
	unsigned int temp_addr;
	short temp_port;
//	int rem;
//	unsigned char new_phase_assignment;	
	unsigned char output_spat_binary = 0;

	int sd_out = -5;             /// socket descriptor for UDP send
        short udp_out_port = -1;    /// set from command line option
        char *udp_out_name = "127.0.0.1";  /// address of UDP destination
        int bytes_sent;         /// returned from sendto
 
        int sd_in = -5;             /// socket descriptor for UDP receive 
        short udp_in_port = 0;    /// set from command line option

	struct timeval timeout;
	fd_set readfds, readfds_sav;
	fd_set writefds, writefds_sav;
	int maxfd = 0;
	int numready;
	int use_tcp = 0;

 
	pbattelle_spat = (char *)&battelle_spat;

        while ((opt = getopt(argc, argv, "A:S:vi:tna:bo:s:l:h:z")) != -1)
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
                  case 'i':
                        udp_in_port = (short)atoi(optarg);
                        break;
                  case 't':
                        use_tcp = 1;
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
                  case 'o':
                        udp_out_port = (short)atoi(optarg);
                        break;
                  case 's':
                        udp_out_name = strdup(optarg);
                        break;
                  case 'l':
                        low_battelle = atoi(optarg);
                        break;
                  case 'h':
                        high_battelle = atoi(optarg);
                        break;
		  case 'z':
		  default:
			fprintf(stderr, "Usage: %s -A <AB3418 port, (def. /dev/ttyS0)> -S <CA SPaT port, (def. /dev/ttyS1)> -v (verbose) -i <UDP/TCP input port, def. UDP> -t (use TCP) -b (output binary SPaT message) -s <UDP unicast destination (def. 127.0.0.1)> -o <UDP unicast port> -l <lowest Battelle byte to display> -h <highest Battelle byte to display>\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if(low_battelle >= 0) {
		if( (low_battelle > 244) || (high_battelle > 244) ) {
			fprintf(stderr, "Battelle bytes must be in the range 0-244. Exiting....\n");
			exit(EXIT_FAILURE);
		}
		if(high_battelle < low_battelle)
			high_battelle = ((low_battelle + 16) > 244) ? 244 : low_battelle + 16;
	}

	// Clear message structs
	memset(&db_timing_set_2070, 0, sizeof(db_timing_set_2070));
	memset(&raw_signal_status_msg, 0, sizeof(raw_signal_status_msg));
	memset(&snd_addr, 0, sizeof(snd_addr));
	for(i=0; i<MAX_PHASES; i++) 
		pphase_timing[i] = &phase_timing[i];
	for(i=0; i<MAX_PLANS+1; i++) 
		pplan_params[i] = &plan_params[i];

	if( udp_out_port >= 0  ) {
		fprintf(stderr, "Opening UDP unicast to destination %s port %hu\n",
			udp_out_name, udp_out_port);
                sd_out = udp_unicast_init(&snd_addr, udp_out_name, udp_out_port);

		if (sd_out < 0) {
			printf("Failure opening unicast socket on %s %d\n",
				udp_out_name, udp_out_port);
			exit(EXIT_FAILURE);
		}
		else {
			printf("Success opening socket %hhu on %s %d\n",
				sd_out, udp_out_name, udp_out_port);
			printf("port %d addr 0x%08x\n", ntohs(snd_addr.sin_port),
			ntohl(snd_addr.sin_addr.s_addr));
			temp_addr = snd_addr.sin_addr.s_addr;
			temp_port = snd_addr.sin_port;
		}
	}
	if( udp_in_port >= 0  ) {
		fprintf(stderr, "Opening UDP listener on port %hu\n",
			udp_in_port);

        	sd_in = udp_allow_all(udp_in_port);

		if (sd_in < 0) {
			printf("Failure opening listener socket on port %d\n",
				udp_in_port);
			exit(EXIT_FAILURE);
		}
		else {
			printf("Success opening socket %hhu on port %d\n",
				sd_in, udp_in_port);
			printf("port %d addr 0x%08x\n", ntohs(snd_addr.sin_port),
			ntohl(snd_addr.sin_addr.s_addr));
			temp_addr = snd_addr.sin_addr.s_addr;
			temp_port = snd_addr.sin_port;
		}
	}

        /* Initialize serial port. */
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


	printf("main 1: getting timing settings before infinite loop\n");
	for(i=0; i<MAX_PHASES; i++) {
		db_timing_get_2070.phase = i+1;	
		db_timing_get_2070.page = 0x100; //phase timing page	
		retval = get_timing(&db_timing_get_2070, wait_for_data, &phase_timing[i], &ab3418_fdin, &ab3418_fdout, verbose);
// Instead of db_clt_write here, the phase timing maybe should be sent to another process via udp?
//		db_clt_write(pclt, DB_PHASE_1_TIMING_VAR + i, sizeof(phase_timing_t), &phase_timing[i]);
		usleep(500000);
	}

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

	//Zero out saved fds
	FD_ZERO(&readfds_sav);
	FD_ZERO(&writefds_sav);
	if(sd_in >= 0) {
		FD_SET(sd_in, &readfds_sav);
		maxfd = sd_in;
	}
	if(sd_out >= 0){
		FD_SET(sd_out, &writefds_sav);
		maxfd = (sd_out > maxfd) ? sd_out : maxfd;
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
		retval = get_status(wait_for_data, &readBuff, ab3418_fdin, ab3418_fdout, verbose);
		retval = get_spat(wait_for_data, &raw_signal_status_msg, ca_spat_fdin, verbose, output_spat_binary);
		memcpy(&long_status8, &readBuff, sizeof(get_long_status8_resp_mess_typ));
		retval = build_spat(&sig_plan_msg, &raw_signal_status_msg, pphase_timing, &long_status8, pplan_params, &battelle_spat, verbose);
		snd_addr.sin_port = temp_port;
		snd_addr.sin_addr.s_addr = temp_addr;
		if(low_battelle >= 0) {
			printf("ab3418udp:Battelle Spat: ");
			for(i=low_battelle; i <= high_battelle; i++)
				printf("#%d %#hhx ", i, pbattelle_spat[i]);
			printf("\n");
		}
	}
	if(FD_ISSET(sd_in, &readfds)) {
		memset(trafficctlreadbuf, 0, sizeof(trafficctlreadbuf));
		if( (retval = recvfrom(sd_in, trafficctlreadbuf, sizeof(trafficctlreadbuf), 0,
			(struct sockaddr *)&trafficctl_addr, (socklen_t *)&len)) > 0) {
			if(trafficctlreadbuf[2] == SIGNAL_SCHED_MSG) {
				printf("trafficctlreadbuf: \n");
				for(i=0; i<sizeof(trafficctlreadbuf); i++) 
					printf("#%d %hhx ", i, trafficctlreadbuf[i]);
				printf("\n");
				memcpy(&mschedule, trafficctlreadbuf, sizeof(mschedule_t));
				printf("Timing schedule request:\nPhase sequence: ");
				for(i=0; i<8; i++)
					printf("%03hhu ", mschedule.phase_sequence.phase_sequence[i]);
				printf("\nPhase duration: ");
				for(i=0; i<8; i++)
					printf("%03hhu ", mschedule.phase_duration.phase_duration[i]);
				printf("\n");

				retval = get_coord_params(&plan_params[9], 9, 
					wait_for_data, &ab3418_fdout, &ab3418_fdin, verbose);
				if(retval < 0) {
					printf("get_coord_params returned %d for plan 9\n",
						retval);
				} 
				else {
no_control = 0;
					if(no_control == 0)
						memcpy(&plan_params[10], &plan_params[9], sizeof(plan_params_t));
						printf("Hall cycle length %d\n", plan_params[10].cycle_length);
						retval = set_coord_params(&plan_params[9], 9, &mschedule, 1, ab3418_fdout, ab3418_fdin, verbose);
				}
			}
		}
	}

	if(output_spat_binary) {
		bytes_sent = sendto(sd_out, &battelle_spat, sizeof(battelle_spat_t), 0,
			(struct sockaddr *) &snd_addr, sizeof(snd_addr));
	}
	else {
		memset(strbuf, 0, sizeof(strbuf));
		for(i=0; i<8; i++)
                sprintf(strbuf+(12*i), "#%u %hhu %hhu ",
			i+1,
                        battelle_spat.movement[i].min_time_remaining.mintimeremaining,
                        battelle_spat.movement[i].max_time_remaining.maxtimeremaining
                );
                sprintf(strbuf+(12*(i+1))+1, "\n");
		bytes_sent = sendto(sd_out, strbuf, sizeof(strbuf), 0,
		     (struct sockaddr *) &snd_addr, sizeof(snd_addr));
	}

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

int OpenTSCPConnection(char *controllerIP, char *port) {
        struct addrinfo hints;
        struct addrinfo *result, *rp;
        int sfd, s;

        /* Obtain address(es) matching host/port */
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_STREAM; /* TCP socket */
        hints.ai_flags = 0;
        hints.ai_protocol = 0;     /* Any protocol */
        s = getaddrinfo(controllerIP, port, &hints, &result);
        if (s != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
        }

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
