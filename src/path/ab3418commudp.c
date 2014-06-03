/* ab3418comm - AB3418<-->database communicator
*/

#include <sys_os.h>
#include "msgs.h"
#include "sys_rt_linux.h"
#include "ab3418_libudp.h"
#include "ab3418commudp.h"
//#include "urms.h"
#include <udp_utils.h>

#define MAX_PHASES	7
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
};

int OpenTSCPConnection(char *controllerIP, char *port);
int process_phase_status( get_long_status8_resp_mess_typ *pstatus, int verbose, unsigned char greens, phase_status_t *pphase_status);

int main(int argc, char *argv[]) {

	posix_timer_typ *ptmr;

        struct sockaddr_in snd_addr;    /// used in sendto call
	int i;
	int fpin = 0;
	int fpout = 0;
	int trafficctlfd = 0;
	char trafficctlreadbuf[1000];
	int len;
	FILE *fifofd = 0;
	phase_status_t phase_status;
	int wait_for_data = 1;
	gen_mess_typ readBuff;
	phase_timing_t phase_timing[MAX_PHASES];
	raw_signal_status_msg_t raw_signal_status_msg;
	db_timing_set_2070_t db_timing_set_2070;
	db_timing_get_2070_t db_timing_get_2070;
	int retval;
	int check_retval;
	char port[14] = "/dev/ttyS0";
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
	int use_db = 0;
	int interval = 100;
	int verbose = 0;
	unsigned char greens = 0;
	unsigned char no_control = 0;
//	unsigned char no_control_sav = 0;
//	char detector = 0;
	unsigned int temp_addr;
	short temp_port;
//	int blocknum;
//	int rem;
//	unsigned char new_phase_assignment;	
	unsigned char output_spat_binary = 0;

        int sd_out;             /// socket descriptor for UDP send
        short udp_port = 0;    /// set from command line option
        char *udp_name = NULL;  /// address of UDP destination
        int bytes_sent;         /// returned from sendto
 
        while ((opt = getopt(argc, argv, "p:uvi:cnd:a:bho:s:")) != -1)
        {
                switch (opt)
                {
                  case 'p':
			memset(port, 0, sizeof(port));
                        strncpy(&port[0], optarg, 13);
                        break;
                  case 'u':
                        use_db = 1;
                        break;
                  case 'v':
                        verbose = 1;
                        break;
                  case 'i':
                        interval = atoi(optarg);
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
                        udp_port = (short)atoi(optarg);
                        break;
                  case 's':
                        udp_name = strdup(optarg);
                        break;
		  case 'h':
		  default:
			fprintf(stderr, "Usage: %s -p <port, (def. /dev/ttyS0)> -u (use db) -v (verbose) -i <loop interval> -b (output binary SPaT message) -s <UDP unicast destination> -o <UDP unicast port>\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	// Clear message structs
	memset(&db_timing_set_2070, 0, sizeof(db_timing_set_2070));
	memset(&raw_signal_status_msg, 0, sizeof(raw_signal_status_msg));
	memset(&snd_addr, 0, sizeof(snd_addr));

	if ((ptmr = timer_init( interval, 0)) == NULL) {
		fprintf(stderr, "Unable to initialize delay timer\n");
		exit(EXIT_FAILURE);
	}

	if( (udp_port != 0) && (udp_name != NULL) ) {
		fprintf(stderr, "Opening UDP unicast to destination %s port %hu\n",
			udp_name, udp_port);
                sd_out = udp_unicast_init(&snd_addr, udp_name, udp_port);

		if (sd_out < 0) {
			printf("failure opening socket on %s %d\n",
				udp_name, udp_port);
			exit(EXIT_FAILURE);
		}
		else {
			printf("Success opening socket %hhu on %s %d\n",
				sd_out, udp_name, udp_port);
			printf("port %d addr 0x%08x\n", ntohs(snd_addr.sin_port),
			ntohl(snd_addr.sin_addr.s_addr));
			temp_addr = snd_addr.sin_addr.s_addr;
			temp_port = snd_addr.sin_port;
		}
	}

        /* Initialize serial port. */
	check_retval = check_and_reconnect_serial(0, &fpin, &fpout, port);
while(1) {
	retval = get_spat(wait_for_data, &raw_signal_status_msg, fpin, fpout, verbose, output_spat_binary);
	snd_addr.sin_port = temp_port;
	snd_addr.sin_addr.s_addr = temp_addr;
	if(output_spat_binary) {
	bytes_sent = sendto(sd_out, &raw_signal_status_msg.active_phase, sizeof(raw_signal_status_msg_t) - 9, 0,
		(struct sockaddr *) &snd_addr, sizeof(snd_addr));
	}
	else {
		memset(strbuf, 0, sizeof(strbuf));
                sprintf(strbuf, "%#hhx %#hhx  %#hhx %.1f %.1f %#hhx %#hhx %#hhx %hhu %hhu %hhu %#hhx %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu\n",
                        raw_signal_status_msg.active_phase,
                        raw_signal_status_msg.interval_A,
                        raw_signal_status_msg.interval_B,
                        raw_signal_status_msg.intvA_timer/10.0,
                        raw_signal_status_msg.intvB_timer/10.0,
                        raw_signal_status_msg.next_phase,
                        raw_signal_status_msg.ped_call,
                        raw_signal_status_msg.veh_call,
                        raw_signal_status_msg.plan_num,
                        raw_signal_status_msg.local_cycle_clock,
                        raw_signal_status_msg.master_cycle_clock,
                        raw_signal_status_msg.preempt,
                        raw_signal_status_msg.permissive[0],
                        raw_signal_status_msg.permissive[1],
                        raw_signal_status_msg.permissive[2],
                        raw_signal_status_msg.permissive[3],
                        raw_signal_status_msg.permissive[4],
                        raw_signal_status_msg.permissive[5],
                        raw_signal_status_msg.permissive[6],
                        raw_signal_status_msg.permissive[7],
                        raw_signal_status_msg.force_off_A,
                        raw_signal_status_msg.force_off_B,
                        raw_signal_status_msg.ped_permissive[0],
                        raw_signal_status_msg.ped_permissive[1],
                        raw_signal_status_msg.ped_permissive[2],
                        raw_signal_status_msg.ped_permissive[3],
                        raw_signal_status_msg.ped_permissive[4],
                        raw_signal_status_msg.ped_permissive[5],
                        raw_signal_status_msg.ped_permissive[6],
                        raw_signal_status_msg.ped_permissive[7]
                );
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

	if (bytes_sent < 0) {
		perror("sendto error");
		printf("port %d addr 0x%08x\n", ntohs(snd_addr.sin_port),
			ntohl(snd_addr.sin_addr.s_addr));
		fflush(stdout);
	}

//        ftime ( &timeptr_raw );
//        localtime_r ( &timeptr_raw.time, &time_converted );
//	printf("Time %02d:%02d:%02d.%03d\n",
//		time_converted.tm_hour,
//		time_converted.tm_min,
//		time_converted.tm_sec,
//		timeptr_raw.millitm);
	TIMER_WAIT(ptmr);
}
	exit(EXIT_SUCCESS);

		if (setjmp(exit_env) != 0) {

			if(retval < 0) 
				check_retval = check_and_reconnect_serial(retval, &fpin, &fpout, port);
                	if(fpin)
                       		close(fpin);
                	if(fpout)
                       		close(fpout);
			exit(EXIT_SUCCESS);
		} else
			sig_ign(sig_list, sig_hand);


	if ((ptmr = timer_init( interval, 0)) == NULL) {
		fprintf(stderr, "Unable to initialize delay timer\n");
		exit(EXIT_FAILURE);
	}

	printf("main 1: getting timing settings before infinite loop\n");
	for(i=0; i<MAX_PHASES; i++) {
		db_timing_get_2070.phase = i+1;	
		db_timing_get_2070.page = 0x100; //phase timing page	
		retval = get_timing(&db_timing_get_2070, wait_for_data, &phase_timing[i], &fpin, &fpout, verbose);
// Instead of db_clt_write here, the phase timing maybe should be sent to another process via udp?
//		db_clt_write(pclt, DB_PHASE_1_TIMING_VAR + i, sizeof(phase_timing_t), &phase_timing[i]);
		usleep(500000);
	}

	while(1) {
		if( (retval = recvfrom(trafficctlfd, trafficctlreadbuf, sizeof(trafficctlreadbuf), 0,
			(struct sockaddr *)&trafficctl_addr, (socklen_t *)&len)) > 0) {
			printf("Hallelujah, I got %d bytes!\n", retval);
			if(no_control == 0) {
//				retval = set_timing(&db_timing_set_2070, &msg_len, fpin, fpout, verbose);
			}
		}
		else {	
			retval = get_status(wait_for_data, &readBuff, fpin, fpout, verbose);
			if(retval < 0) 
				check_retval = check_and_reconnect_serial(retval, &fpin, &fpout, port);
			if(use_db && (retval == 0) ) {
// Should this be a udp send?	db_clt_write(pclt, DB_TSCP_STATUS_VAR, sizeof(get_long_status8_resp_mess_typ), (get_long_status8_resp_mess_typ *)&readBuff);
				retval = process_phase_status( (get_long_status8_resp_mess_typ *)&readBuff, verbose, greens, &phase_status);
// Should this be a udp send?	db_clt_write(pclt, DB_PHASE_STATUS_VAR, sizeof(phase_status_t), &phase_status);
				fifofd = fopen("/tmp/blah", "w");
				for(i=0; i<8; i++) {
					fprintf(fifofd, "%d", phase_status.phase_status_colors[i]);
				}
				fclose(fifofd);
			if(verbose) 
				print_status( (get_long_status8_resp_mess_typ *)&readBuff);
			}
			else 
				if(retval < 0) 
					printf("get_status returned negative value: %d\n", retval);
			usleep(80000);
			retval = get_short_status(wait_for_data, &readBuff, fpin, fpout, verbose);
			if(retval < 0) 
				check_retval = check_and_reconnect_serial(retval, &fpin, &fpout, port);
			if(use_db && (retval == 0) ) {
			    greens = readBuff.data[5];	
			    if(verbose) 
				printf("ab3418comm: get_short_status: greens %#hhx\n", readBuff.data[5]);
// Should this be a udp send?	db_clt_write(pclt, DB_SHORT_STATUS_VAR, sizeof(get_short_status_resp_t), (get_short_status_resp_t *)&readBuff);
			}
			else 
				if(retval < 0)
					printf("get_short_status returned negative value: %d\n", retval);
		}
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
		printf("dow=%d dow%%6=%d hour %d\n", dow, dow % 6, ltime->tm_hour);

		if( ((dow % 6) == 0) || (ltime->tm_hour < 15) || (ltime->tm_hour >= 19) ) {
			no_control = 1;
		}
		else {
			no_control = 0;
		}

		if(!use_db)
			TIMER_WAIT(ptmr);
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
int get_detector(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char detector, char verbose) {
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
        FD_SET(fpout, &writefds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        if( (selectval = select(fpout+1, NULL, &writefds, NULL, &timeout)) <=0) {
                perror("select 56");
                outportisset = (FD_ISSET(fpout, &writefds)) == 0 ? "no" : "yes";
                printf("get_detector 2: fpout %d selectval %d outportisset %s\n", fpout, selectval, outportisset);
                return -3;
        }
        write ( fpout, &detector_get_request, msg_len+4 );
        fflush(NULL);
        sleep(2);


        ser_driver_retval = 100;

        if(wait_for_data && readBuff) {
                FD_ZERO(&readfds);
                FD_SET(fpin, &readfds);
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
                        perror("select 57");
                        inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
                        printf("get_detector 3: fpin %d selectval %d inportisset %s\n", fpin, selectval, inportisset);
                        return -2;
                }
                ser_driver_retval = ser_driver_read(readBuff, fpin, verbose);
                if(ser_driver_retval == 0) {
                        printf("get_detector 4: Lost USB connection\n");
                        return -1;
                }
        }
        if(verbose != 0)
                printf("get_detector 5-end: fpin %d selectval %d inportisset %s fpout %d selectval %d outportisset %s ser_driver_retval %d\n", fpin, selectval, inportisset, fpout, selectval, outportisset, ser_driver_retval);
        return 0;
}

int set_detector(detector_msg_t *pdetector_set_request, int fpin, int fpout, char detector, char verbose) {
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
        FD_SET(fpout, &writefds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        if( (selectval = select(fpout+1, NULL, &writefds, NULL, &timeout)) <=0) {
                perror("select 14");
                outportisset = (FD_ISSET(fpout, &writefds)) == 0 ? "no" : "yes";
                printf("set_detector 2: fpout %d selectval %d outportisset %s\n", fpout, selectval, outportisset);
                return -3;
        }
        write ( fpout, pdetector_set_request, sizeof(detector_msg_t));
        fflush(NULL);
        sleep(2);

        ser_driver_retval = 100;
        if(wait_for_data) {
                FD_ZERO(&readfds);
                FD_SET(fpin, &readfds);
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
                        perror("select 15");
                        inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
                        printf("set_detector 3: fpin %d selectval %d inportisset %s\n", fpin, selectval, inportisset);
                        return -2;
                }
                ser_driver_retval = ser_driver_read(&readBuff, fpin, verbose);
                if(ser_driver_retval == 0) {
                        printf("set_detector 4: Lost USB connection\n");
                        return -1;
                }
        }
        if(verbose != 0)
                printf("set_detector 5-end: fpin %d selectval %d inportisset %s fpout %d selectval %d outportisset %s ser_driver_retval %d\n", fpin, selectval, inportisset, fpout, selectval, outportisset, ser_driver_retval);
        return 0;
}
*/
