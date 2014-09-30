/*FILE: ab3418_libudp.c   Function library for sending and receiving AB3418 messages from 2070 Controller and receiving UDP commands from outside utility
 *
 *
 * Copyright (c) 2013   Regents of the University of California
 *
 *  Process to send and read AB3418 messages from 2070.  We'll expect
 *  to see message types:
 *  (0xcc) - Get long status8 message.
 *  (0xc9) - Get timing settings message.
 *
 *  In order for this message to be sent, we must first request it by
 *  sending message type 0x8c (Get long status8 message) or 0x89 (Get controller timing message).
 *
 *  The signals SIGINT, SIGQUIT, and SIGTERM are trapped, and cause the
 *  process to terminate.  Upon termination log out of the database.
 *
 */

#include <termios.h>
#include "sys_os.h"
//#include "atsc_clt_vars.h"
#include "atsc.h"
#include "msgs.h"
#include "fcs.h"
#include "ab3418_libudp.h"
#include <sys/select.h>

#undef DEBUG_TRIG

#define GET_REQUEST	0X80
#define SET_REQUEST	0X90
#define GET_RESPONSE	0XC0
#define SET_RESPONSE	0XD0
#define GET_ERROR_RESPONSE	0XE0
#define SET_ERROR_RESPONSE	0XF0


#define FLAGS		0X06
#define TIMING_DATA	0X07
#define LONG_STATUS8	0X0D

int print_status(get_long_status8_resp_mess_typ *status);
int set_timing(db_timing_set_2070_t *db_timing_set_2070, int *msg_len, int fpin, int fpout, char verbose);
int get_timing(db_timing_get_2070_t *db_timing_get_2070, int wait_for_data, phase_timing_t *phase_timing, int *fpin, int *fpout, char verbose);
int get_status(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose);
int get_overlap(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose);
int set_overlap(overlap_msg_t *overlap_set_request, int fpin, int fpout, char verbose);
int get_special_flags(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose);
int get_spat(int wait_for_data, raw_signal_status_msg_t *praw_signal_status_msg, int fpin, char verbose, char print_packed_binary);
int set_special_flags(get_set_special_flags_t *special_flags, int fpin, int fpout, char verbose);
void fcs_hdlc(int msg_len, void *msgbuf, char verbose);
int get_short_status(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose);
int get_request(unsigned char msg_type, unsigned char page, unsigned char block, int fpout, char verbose);
int spat2battelle(raw_signal_status_msg_t *ca_spat, battelle_spat_t *battelle_spat, phase_timing_t *phase_timing[8], int verbose);
int get_coord_params(plan_params_t *plan_params, int plan_num, int wait_for_data, int *fpout, int *fpin, char verbose);
int set_coord_params(plan_params_t *plan_params, int plan_num, mschedule_t *mschedule, int wait_for_data, int fdout, int fdin, char verbose);
int build_sigplanmsg(sig_plan_msg_t *sig_plan_msg, phase_timing_t *phase_timing[], plan_params_t *current_plan_params,get_long_status8_resp_mess_typ *long_status8, int verbose);


char *timing_strings[] = {
        "Walk_1",
        "Flash_dont_walk",
        "Minimum_green",
        "Detector_limit",
        "Add_per_vehicle",
        "Extension",
        "Maximum_gap",
        "Minimum_gap",
        "Max_green_1",
        "Max_green_2",
        "Max_green_3",
        "Maximum_initial?",
        "Reduce_gap_by",
        "Reduce_every",
        "Yellow",
        "All-red"
};

char *plan_strings[] = {
        "Cycle_length",
        "Green_factor_1",
        "Green_factor_2",
        "Green_factor_3",
        "Green_factor_4",
        "Green_factor_5",
        "Green_factor_6",
        "Green_factor_7",
        "Green_factor_8",
        "Multiplier",
        "Plan(A)",
        "Offset(B)",
        "(C)"
};

bool_typ ser_driver_read( gen_mess_typ *pMessagebuff, int fpin, char verbose) 
{
	unsigned char msgbuf [100];
	int i;
	int ii;
	unsigned short oldfcs;
	unsigned short newfcs;
	atsc_typ atsc;
	struct timeb timeptr_raw;
	struct tm time_converted;
        fd_set readfds;
        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";

	/* Read from serial port. */
	/* Blocking read is used, so control doesn't return unless data is
	 * available.  Keep reading until the beginning of a message is 
	 * determined by reading the start flag 0x7e.  */
	memset( msgbuf, 0x0, 100 );
	while ( msgbuf[0] != 0x7e ) {
		if(verbose != 0) 
			printf("ser_driver_read 1: Ready to read:\n");
		FD_ZERO(&readfds);
		FD_SET(fpin, &readfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
		    if(errno != EINTR) {
			perror("select 1");
			inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
			printf("\n\nser_driver_read 1: fpin %d selectval %d inportisset %s\n\n", fpin, selectval, inportisset);
			return FALSE;
		    }
		}

		FD_ZERO(&readfds);
		FD_SET(fpin, &readfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
		    if(errno != EINTR) {
			perror("select 2");
			inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
			printf("\n\nser_driver_read 2: fpin %d selectval %d inportisset %s\n\n", fpin, selectval, inportisset);
			return FALSE;
		    }
		}

	    read ( fpin, &msgbuf[0], 1);
		if(verbose != 0) {
			printf("%x \n",msgbuf[0]);
			fflush(stdout);
		}
	
		if(verbose != 0) 
			printf("\n");
	
		/* Read next character.  If this is 0x7e, then this is really
		 * the start of new message, previous 0x7e was end of previous message.
		*/

		FD_ZERO(&readfds);
		FD_SET(fpin, &readfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
		    if(errno != EINTR) {
			perror("select 3");
			inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
			printf("\n\nser_driver_read 3: fpin %d selectval %d inportisset %s\n\n", fpin, selectval, inportisset);
			return FALSE;
		    }
		}

		read ( fpin, &msgbuf[1], 1 );
		if(verbose != 0) 
			printf("%x ", msgbuf[1] );
		if ( msgbuf[1] != 0x7e ) {
			ii=2;
			if(verbose != 0) 
				printf("%x ",msgbuf[1]);
		}
		else {
			ii=1;
			if(verbose != 0) 
				printf("\n");
		}
	
		/* Header found, now read remainder of message. Continue reading
		 * until end flag is found (0x7e).  If more than 95 characters are
		 * read, this message is junk so just take an error return. */
		for ( i=ii; i<100; i++ ) {

		FD_ZERO(&readfds);
		FD_SET(fpin, &readfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
		    if(errno != EINTR) {
			perror("select 4");
			inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
			printf("\n\nser_driver_read 4: fpin %d selectval %d inportisset %s\n\n", fpin, selectval, inportisset);
			return FALSE;
		    }
		}

			read ( fpin, &msgbuf[i], 1);
			if(verbose != 0) {
				printf("%x ", msgbuf[i]);
				fflush(stdout);
			}
			if ( i>95 ) {
				printf("ser_driver_read: message > 95 bytes\n");
				return( FALSE );
			}
			if ( msgbuf[i] == 0x7e )
			break;
			/* If the byte read was 0x7d read the next byte.  If the next
			* byte is 0x5e, convert the first byte to 0x7e.  If the next
			* byte is 0x5d, the first byte really should be 0x7d.  If
			* the next byte is neither 0x5e nor 0x5d, take an error exit. */
			if ( msgbuf[i] == 0x7d ) {

		FD_ZERO(&readfds);
		FD_SET(fpin, &readfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
		    if(errno != EINTR) {
			perror("select 5");
			inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
			printf("\n\nser_driver_read 5: fpin %d selectval %d inportisset %s\n\n", fpin, selectval, inportisset);
			return FALSE;
		    }
		}

				read ( fpin, &msgbuf[i+1], 1 );
				if(verbose != 0) 
				printf("%x ", msgbuf[i+1] );
				if ( msgbuf[i+1] == 0x5e )
				msgbuf[i] = 0x7e;
				else if ( msgbuf[i+1] != 0x5d ) {
					printf("Illegal 0x7d\n");
					return (FALSE);
				}
			}
		}

	
		memcpy( pMessagebuff->data, &msgbuf[0], 100);
	
		oldfcs = ~(msgbuf[i-2] << 8) | ~msgbuf[i-1];
		newfcs = pppfcs( oldfcs, &msgbuf[1], i-1 );
		if(verbose != 0){ 
			printf("newfcs=%x\n",newfcs);
			fflush(stdout);
		}
		if ( newfcs != 0xf0b8 ) {
			printf( "FCS error, msg type %x\n", msgbuf[4] );
			return (FALSE);
		}
		else {
	if(verbose != 0) 
		printf("ser_driver_read: message type %#hhx ",pMessagebuff->data[4]);
        switch( pMessagebuff->data[4] ) {
            case 0xc4:    // GetShortStatus message
		// Get time of day and save in the database. 
                ftime ( &timeptr_raw );
                localtime_r ( &timeptr_raw.time, &time_converted );
                atsc.ts.hour = time_converted.tm_hour;
                atsc.ts.min = time_converted.tm_min;
                atsc.ts.sec = time_converted.tm_sec;
                atsc.ts.millisec = timeptr_raw.millitm;

		if(verbose != 0) 
			printf("ser_driver_read: %02d:%02d:%02d:%03d: Got get_short_status response. Green %#hhx status %#hhx pattern %#hhx\n",
				atsc.ts.hour,atsc.ts.min,
				atsc.ts.sec,atsc.ts.millisec,
				pMessagebuff->data[5],
				pMessagebuff->data[6],
				pMessagebuff->data[7] );
		break;
            case 0xcc:    // GetLongStatus8 message
		// Get time of day and save in the database. 
                ftime ( &timeptr_raw );
                localtime_r ( &timeptr_raw.time, &time_converted );
                atsc.ts.hour = time_converted.tm_hour;
                atsc.ts.min = time_converted.tm_min;
                atsc.ts.sec = time_converted.tm_sec;
                atsc.ts.millisec = timeptr_raw.millitm;

		if(verbose != 0) 
			printf("ser_driver_read: %02d:%02d:%02d:%03d\n",atsc.ts.hour,atsc.ts.min,
				atsc.ts.sec,atsc.ts.millisec );
		if(verbose == 2)
			print_status( (get_long_status8_resp_mess_typ *) pMessagebuff );
		break;
	    case 0xc7:
		printf("ser_driver_read: get_overlap returned OK\n");
		break;
	    case 0xc9:
		if(verbose)
			printf("ser_driver_read: GetControllerTimingData returned OK\n");
		break;
	    case 0xce:
		break;
	    case 0xd6:
		if( (pMessagebuff->data[5] == 2) && (pMessagebuff->data[6] == 4) )
			printf("ser_driver_read: SetOverlap returned OK\n");
		else
			printf("ser_driver_read: set TSMSS message returned; page ID %d block ID %d\n",
				pMessagebuff->data[5], pMessagebuff->data[6]); 
		break;
	    case 0xd9:
		printf("ser_driver_read: SetControllerTiming returned OK\n");
		break;
	    case 0xe4:
		printf("ser_driver_read: GetShortStatus error: %hhd index %hhd\n",
			pMessagebuff->data[5], pMessagebuff->data[6]);
		break;
	    case 0xe9:
		printf("ser_driver_read: GetControllerTiming error: %hhd index %hhd\n",
			pMessagebuff->data[5], pMessagebuff->data[6]);
		break;
	    case 0xf9:
		printf("ser_driver_read: SetControllerTiming error: %hhd index %hhd\n",
			pMessagebuff->data[5], pMessagebuff->data[6]);
		break;
	    default:
	    	printf("ser_driver_read: Unknown message type : 0x%x\n", pMessagebuff->data[4] );
	    	break;
	}  
		    return (TRUE);
		}
	}
	return (TRUE);
}

int print_status(get_long_status8_resp_mess_typ *status) {

	char *interval_str[] = {"Walk", "Don't walk", "Min green","" ,"Added initial", "Passage - resting",
				"Max gap", "Min gap", "Red rest","Preemption","Stop time","Red revert",
				"Max termination","Gap termination","Force off","Red clearance"};
	char *color_str[] = {"Green", "Red", "Green","" ,"Green", "Green",
				"Green", "Green", "Red","Preemption","Stop time","Red",
				"Max termination","Yellow","Force off","Red"};
	char *ring_0_str[] = {"","1", "2", "","3" ,"","","","4"};
	char *ring_1_str[] = {"","5", "6", "","7" ,"","","","8"};

	printf("phas %#hhx r0 %s color %s int %s r1 %s color %s int %s GYOV %#hhx det1 %hhx det2 %#hhx det3 %#hhx det4 %#hhx master clock %d seq number %d\n",
		status->active_phase,
		ring_0_str[status->active_phase & 0x0f],
		color_str[status->interval & 0x0f],      // Bits 0-3: ring 0 interval, bits 4-7: ring 1 interval.
		interval_str[status->interval & 0x0f],      // Bits 0-3: ring 0 interval, bits 4-7: ring 1 interval.
		ring_1_str[(status->active_phase >> 4) & 0x0f],
		color_str[(status->interval >> 4) & 0x0f],      // Bits 0-3: ring 0 interval, bits 4-7: ring 1 interval.
		interval_str[(status->interval >> 4)& 0x0f],      // Bits 0-3: ring 0 interval, bits 4-7: ring 1 interval.
					  // Interval encoding is as follows:
					     // 0X00 = walk, 0x01 = don't walk, 0x02 = min green,
					  // 0x03 = unused, 0x04 = added initial, 0x05 = passage -resting,
					  // 0x06 = max gap, 0x07 = min gap, 0x08 = red rest,
					// 0x09 = preemption, 0x0a = stop time, 0x0b = red revert,
					  // 0x0c = max termination, 0x0d = gap termination,
					  // 0x0e = force off, 0x0f = red clearance 
		status->green_yellow_overlap, // Bits 0-3 green overlaps A-D,
					  // bits 4-7 yellow overlaps A-D 
		status->presence1,     // Bits 0-7: detector 1-8. Presence bits set true for
					  // positive presence. 
		status->presence2,     // Bits 0-7: detector 9-16 
		status->presence3,     // Bits 0-7: detector 17-24 
		status->presence4,     // Bits 0-3: detector 25-28, bits 4-7 unused 
		status->master_clock,  // Master background cycle clock.  Counts up to cycle length 
		status->seq_number    // Sample sequence number 
	);
/*	printf("active phase %#hhx \tphase call %#hhx \tring 0 interval %s \tring 1 interval %s \nstatus %d \tpattern %d \tgreen-yellow overlap %hhx \tpreemption %hhx \tped call %hhx \tdetector presence %hhx (1-8) %hhx (9-16) %hhx (17-24) %hhx (25-28) \tmaster clock %d \tlocal clock %d \tsample sequence number %d \tsystem detector volume/occupancy: 1 %hhd %hhd    2 %hhd %hhd    3 %hhd %hhd    4 %hhd %hhd    5 %hhd %hhd    6 %hhd %hhd    7 %hhd %hhd    8 %hhd %hhd \n",
		status->active_phase,
		status->phase_call,
		interval_str[status->interval & 0x0f],      // Bits 0-3: ring 0 interval, bits 4-7: ring 1 interval.
		interval_str[(status->interval >> 4)& 0x0f],      // Bits 0-3: ring 0 interval, bits 4-7: ring 1 interval.
					  // Interval encoding is as follows:
					     // 0X00 = walk, 0x01 = don't walk, 0x02 = min green,
					  // 0x03 = unused, 0x04 = added initial, 0x05 = passage -resting,
					  // 0x06 = max gap, 0x07 = min gap, 0x08 = red rest,
					// 0x09 = preemption, 0x0a = stop time, 0x0b = red revert,
					  // 0x0c = max termination, 0x0d = gap termination,
					  // 0x0e = force off, 0x0f = red clearance 
		status->status,		// Bit 7 = critical alarm, bit 6 = non-critical alarm,
				 	// bit 5 = detector fault, bit 4 = coordination alarm,
					 // bit 3 = local override, bit 2 = passed local zero,
					 // bit 1 = cabinet flash, bit 0 = preempt. 
		status->pattern,      // Pattern (0-250, 251-253 reserved, 254 flash, 255 free) 
		status->green_yellow_overlap, // Bits 0-3 green overlaps A-D,
					  // bits 4-7 yellow overlaps A-D 
		status->preemption,    // Bits 0-1 EV A-D, bits 4-6 RR 1-2,
					  // bit 6 = pattern transition, bit 7 unused 
		status->ped_call,      // Ped call 1-8, (bit 7 = ped 8, bit 0 = ped 1) 
		status->presence1,     // Bits 0-7: detector 1-8. Presence bits set true for
					  // positive presence. 
		status->presence2,     // Bits 0-7: detector 9-16 
		status->presence3,     // Bits 0-7: detector 17-24 
		status->presence4,     // Bits 0-3: detector 25-28, bits 4-7 unused 
		status->master_clock,  // Master background cycle clock.  Counts up to cycle length 
		status->local_clock,   // Local cycle clock.  Counts up to cycle length. 
		status->seq_number,    // Sample sequence number 
		status->volume1,	// System detector 1 
		status->occupancy1,    // System detector 1.  Value 0-200 = detector occupancy in
					  // 0.5% increments, 201-209 = reserved, 210 = stuck ON fault,
					  // 211 = stuck OFF fault, 212 = open loop fault,
					  // 213 = shorted loop fault, 214 = excessive inductance fault,
					  // 215 = overcount fault. 
		status->volume2,	// System detector 2 
		status->occupancy2,    // system detector 2 
		status->volume3,	// System detector 3 
		status->occupancy3,    // system detector 3 
		status->volume4,	// System detector 4 
		status->occupancy4,    // system detector 4 
		status->volume5,	// System detector 5 
		status->occupancy5,    // system detector 5 
		status->volume6,	// System detector 6 
		status->occupancy6,    // system detector 6 
		status->volume7,	// System detector 7 
		status->occupancy7,    // system detector 7 
		status->volume8,	// System detector 8 
		status->occupancy8    // system detector 8 
	);
*/
return 0;
}

void fcs_hdlc(int msg_len, void *msgbuf, char verbose) {

	unsigned char *pchar;
	mess_union_typ *writeBuff = (mess_union_typ *) msgbuf;
	int i;
	int j;
	int jjj;

        pchar = (unsigned char *) writeBuff;
        get_modframe_string( pchar+1, &msg_len );

        /* Make into HDLC frame by checking for any 0x7e in message and replacing 
        ** with the combo 0x7d, 0x5e.  Replace any 0x7d with 0x7d, 0x5d. */
        for( i=1; i <= msg_len; i++) {
                /* Within a message, replace any 0x7e with 0x7d, 0x5e. */
                if ( writeBuff->gen_mess.data[i] == 0x7e ) {
                        for ( j=msg_len; j > i; j-- ) {
                                writeBuff->gen_mess.data[j+1] = writeBuff->gen_mess.data[j];
                        }
                        writeBuff->gen_mess.data[i] = 0x7d;
                        writeBuff->gen_mess.data[i+1] = 0x5e;
                        msg_len++;
                        i++;
                }
                /* Within a message, replace any 0x7d with 0x7d, 0x5d. */
                if ( writeBuff->gen_mess.data[i] == 0x7d ) {
                        for ( j=msg_len; j > i; j-- ) {
                                writeBuff->gen_mess.data[j+1] = writeBuff->gen_mess.data[j];
                                writeBuff->gen_mess.data[i] = 0x7d;
                                writeBuff->gen_mess.data[i+1] = 0x5d;
                                msg_len++;
                                i++;
                        }
                }
        }

        /* Now add the end flag and send message. */
        writeBuff->gen_mess.data[msg_len+1] = 0x7e;
        if(verbose != 0) {
                printf("fcs_hdlc 1: New message request\n");
                for (jjj=0; jjj<msg_len+2; jjj++)
                        printf("%x ",writeBuff->gen_mess.data[jjj]);
                printf("\n");
        }
}

/* Create cell address and swap bytes (the AB3418 cell addresses are little-endian)*/
unsigned short calc_cell_addr(unsigned short phase_1_cell_addr, unsigned char phase) {
	unsigned short temp;
	unsigned short temp2;
	temp = (((phase - 1) << 4) + phase_1_cell_addr);
	temp2 = ((temp << 8) & 0xFF00) + ((temp >> 8) & 0x00FF);
	return temp2;
}

int set_timing(db_timing_set_2070_t *db_timing_set_2070, int *msg_len, int fpin, int fpout, char verbose) {

        fd_set readfds;
        fd_set writefds;
        int selectval = 1000;
        struct timeval timeout;
	struct timeb timeptr_raw;
	struct tm time_converted;
	atsc_typ atsc;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";
	int ser_driver_retval;
	set_controller_timing_data_t set_controller_timing_data_mess;
	int wait_for_data = 1;
	gen_mess_typ readBuff;

	memset(&set_controller_timing_data_mess, 0, sizeof(set_controller_timing_data_t));
	/* Send the message to request GetLongStatus8 from 2070. */
	set_controller_timing_data_mess.start_flag = 0x7e;
	set_controller_timing_data_mess.address = 0x05;
	set_controller_timing_data_mess.control = 0x13;
	set_controller_timing_data_mess.ipi = 0xc0;
	set_controller_timing_data_mess.mess_type = 0x99;
	set_controller_timing_data_mess.num_cells = 2;
	set_controller_timing_data_mess.magic_num.cell_addr = 0x9201;
	set_controller_timing_data_mess.magic_num.data = 2;
	set_controller_timing_data_mess.magic_num.cell_addr = 0x0000;
	set_controller_timing_data_mess.magic_num.data = 0;
	set_controller_timing_data_mess.cell_addr_data.cell_addr = 
		((db_timing_set_2070->cell_addr_data.cell_addr << 8) & 0xFF00) + ((db_timing_set_2070->cell_addr_data.cell_addr >> 8) & 0x00FF);
	if(db_timing_set_2070->phase > 0)
		set_controller_timing_data_mess.cell_addr_data.cell_addr = 
			calc_cell_addr(db_timing_set_2070->cell_addr_data.cell_addr, db_timing_set_2070->phase);
	set_controller_timing_data_mess.cell_addr_data.data = 
		db_timing_set_2070->cell_addr_data.data;
	// Get time of day and save in the database. 
	ftime ( &timeptr_raw );
	localtime_r ( &timeptr_raw.time, &time_converted );
	atsc.ts.hour = time_converted.tm_hour;
	atsc.ts.min = time_converted.tm_min;
	atsc.ts.sec = time_converted.tm_sec;
	atsc.ts.millisec = timeptr_raw.millitm;

	if(verbose != 0) 
		printf("set_timing: %02d:%02d:%02d:%03d ",atsc.ts.hour,atsc.ts.min,
			atsc.ts.sec,atsc.ts.millisec );
	printf("cell_addr %#hhx data %hhu phase %hhu\n",
		set_controller_timing_data_mess.cell_addr_data.cell_addr, 
		set_controller_timing_data_mess.cell_addr_data.data, 
		db_timing_set_2070->phase);
	set_controller_timing_data_mess.FCSmsb = 0x00;
	set_controller_timing_data_mess.FCSlsb = 0x00;
	/* Now append the FCS. */
	*msg_len = sizeof(set_controller_timing_data_t) - 4;
	fcs_hdlc(*msg_len, &set_controller_timing_data_mess, verbose);
	FD_ZERO(&writefds);
	FD_SET(fpout, &writefds);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if( (selectval = select(fpout+1, NULL, &writefds, NULL, &timeout)) <=0) {
		perror("select 6");
		outportisset = (FD_ISSET(fpout, &writefds)) == 0 ? "no" : "yes";
		printf("set_timing 3: fpout %d selectval %d outportisset %s\n", fpout, selectval, outportisset);
		return -3;
	}
	write ( fpout, &set_controller_timing_data_mess, sizeof(set_controller_timing_data_t));
	fflush(NULL);
	sleep(2);

	ser_driver_retval = 100;

	if(wait_for_data) {
		FD_ZERO(&readfds);
		FD_SET(fpin, &readfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
			perror("select 7");
			inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
			printf("set_timing 4: fpin %d selectval %d inportisset %s\n", fpin, selectval, inportisset);
			return -2;
		}
		ser_driver_retval = ser_driver_read(&readBuff, fpin, verbose);
		if(ser_driver_retval == 0) {
			printf("set_timing 5: Lost USB connection\n");
			return -1;
		}
	}
	if(verbose != 0)
		printf("set_timing 6-end: fpin %d selectval %d inportisset %s fpout %d selectval %d outportisset %s ser_driver_retval %d\n", fpin, selectval, inportisset, fpout, selectval, outportisset, ser_driver_retval);
	return 0;
}

int get_request(unsigned char msg_type, unsigned char page, unsigned char block, int fpout, char verbose) {

	int msg_len;
	int selectval = 1000;
        fd_set writefds;
        struct timeval timeout;
        char *outportisset = "not yet initialized";
	get_block_request_t get_block_request;

        // Send the message to request GetLongStatus8 from 2070. 
        get_block_request.start_flag = 0x7e;
        get_block_request.address = 0x05;
        get_block_request.control = 0x13;
        get_block_request.ipi = 0xc0;
        get_block_request.mess_type = GET_REQUEST | msg_type;
        get_block_request.page = page;
        get_block_request.block = block;
        get_block_request.FCSmsb = 0x00;
        get_block_request.FCSlsb = 0x00;

        // Now append the FCS. 
        msg_len = sizeof(get_block_request_t) - 4;
	fcs_hdlc(msg_len, &get_block_request, verbose);
	FD_ZERO(&writefds);
	FD_SET(fpout, &writefds);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if( (selectval = select(fpout+1, NULL, &writefds, NULL, &timeout)) <=0) {
		perror("select 8");
		outportisset = (FD_ISSET(fpout, &writefds)) == 0 ? "no" : "yes";
		printf("get_block_request 3: fpout %d outportisset %s\n", fpout, outportisset);
		return -1;
	}
	if( write(fpout, &get_block_request, sizeof(get_block_request_t)) < 0) {
		perror("get_request write");
		return -2;
	}

	if(verbose != 0)
		printf("get_block_request: fpout %d selectval %d outportisset %s page %hhx block %hhx\n", fpout, selectval, outportisset, get_block_request.page, get_block_request.block);
	return 0;
}

int get_timing(db_timing_get_2070_t *db_timing_get_2070, int wait_for_data, phase_timing_t *phase_timing, int *fpin, int *fpout, char verbose) {

	int msg_len;
        fd_set readfds;
        fd_set writefds;
        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";
	int ser_driver_retval;
	get_controller_timing_data_request_t get_controller_timing_data_request_mess;
	gen_mess_typ readBuff;

        /* Send the message to request GetLongStatus8 from 2070. */
        get_controller_timing_data_request_mess.start_flag = 0x7e;
        get_controller_timing_data_request_mess.address = 0x05;
        get_controller_timing_data_request_mess.control = 0x13;
        get_controller_timing_data_request_mess.ipi = 0xc0;
        get_controller_timing_data_request_mess.mess_type = 0x89;
        get_controller_timing_data_request_mess.offset = calc_cell_addr(db_timing_get_2070->page + 0x10, db_timing_get_2070->phase);
        get_controller_timing_data_request_mess.num_bytes = 16;
        get_controller_timing_data_request_mess.FCSmsb = 0x00;
        get_controller_timing_data_request_mess.FCSlsb = 0x00;

        /* Now append the FCS. */
        msg_len = sizeof(get_controller_timing_data_request_t) - 4;
	fcs_hdlc(msg_len, &get_controller_timing_data_request_mess, verbose);
	FD_ZERO(&writefds);
	FD_SET(*fpout, &writefds);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if( (selectval = select(*fpout+1, NULL, &writefds, NULL, &timeout)) <=0) {
		perror("select 8");
		outportisset = (FD_ISSET(*fpout, &writefds)) == 0 ? "no" : "yes";
		printf("get_timing 3: *fpout %d selectval %d outportisset %s\n", *fpout, selectval, outportisset);
		return -3;
	}
	write ( *fpout, &get_controller_timing_data_request_mess, sizeof(get_controller_timing_data_request_t));
	fflush(NULL);

	ser_driver_retval = 100;

	if(wait_for_data) {
		FD_ZERO(&readfds);
		FD_SET(*fpin, &readfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if( (selectval = select(*fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
		    if(errno != EINTR) {
			perror("select 9");
			inportisset = (FD_ISSET(*fpin, &readfds)) == 0 ? "no" : "yes";
			printf("get_timing 4: *fpin %d selectval %d inportisset %s\n", *fpin, selectval, inportisset);
			return -2;
		    }
		}
		ser_driver_retval = ser_driver_read(&readBuff, *fpin, verbose);
		if(ser_driver_retval == 0) {
			printf("get_timing 5: Lost USB connection\n");
			return -1;
		}
		else {
			memcpy(phase_timing, &readBuff.data[8], 16);
			if(verbose) {
				printf("\nPhase %d timing parameters:\n", db_timing_get_2070->phase);
				printf("Walk_1\t\t%hhu sec\n", phase_timing->walk_1);
				printf("Don't walk\t%hhu sec\n", phase_timing->dont_walk);
				printf("Min Green\t%hhu sec\n", phase_timing->min_green);
				printf("Detector Limit\t%hhu sec\n", phase_timing->detector_limit);
				printf("Max Green 1\t%hhu sec\n", phase_timing->max_green1);
				printf("Max Green 2\t%hhu sec\n", phase_timing->max_green2);
				printf("Max Green 3\t%hhu sec\n", phase_timing->max_green3);
				printf("Extension\t%.1f sec\n", 0.1 * phase_timing->extension);
				printf("Max Gap\t\t%.1f sec\n", 0.1 * phase_timing->max_gap);
				printf("Min Gap\t\t%.1f sec\n", 0.1 * phase_timing->min_gap);
				printf("Add per veh\t%.1f sec\n", 0.1 * phase_timing->add_per_veh);
				printf("Reduce gap by\t%.1f sec\n", 0.1 * phase_timing->reduce_gap_by);
				printf("Reduce every\t%.1f sec\n", 0.1 * phase_timing->reduce_every);
				printf("Yellow\t\t%.1f sec\n", 0.1 * phase_timing->yellow);
				printf("All red\t\t%.1f sec\n", 0.1 * phase_timing->all_red);
			}
		}
	}
	if(verbose != 0)
		printf("get_timing 6-end: *fpin %d selectval %d inportisset %s *fpout %d selectval %d outportisset %s ser_driver_retval %d get_controller_timing_data_request_mess.offset %hx\n", *fpin, selectval, inportisset, *fpout, selectval, outportisset, ser_driver_retval, get_controller_timing_data_request_mess.offset);
	return 0;
}

int get_coord_params(plan_params_t *plan_params, int plan_num, int wait_for_data, int *fpout, int *fpin, char verbose) {

        fd_set readfds;
        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";
	int ser_driver_retval;
	get_controller_timing_data_request_t get_controller_timing_data_request_mess;
	gen_mess_typ readBuff;
	int retval;
	int i;

	if( (retval = get_request( 0x87, 4, plan_num, *fpout, verbose)) < 0) {
		printf("get_coord_params 1: get_request returned %d\n", retval);
		return -1;
	}

	if(wait_for_data) {
		FD_ZERO(&readfds);
		FD_SET(*fpin, &readfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if( (selectval = select(*fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
		    if(errno != EINTR) {
			perror("select 9");
			inportisset = (FD_ISSET(*fpin, &readfds)) == 0 ? "no" : "yes";
			printf("get_coord_params 2: *fpin %d selectval %d inportisset %s\n", *fpin, selectval, inportisset);
			return -2;
		    }
		}
		ser_driver_retval = ser_driver_read(&readBuff, *fpin, verbose);
		if(ser_driver_retval == 0) {
			printf("get_coord_params 3: Lost serial connection\n");
			return -3;
		}
		else {
			memcpy(plan_params, &readBuff.data[6], 24);

			if(verbose) 
			{
				printf("\nCoordination plan %d parameters:\n", plan_params->plan_num);
				printf("Cycle length\t\t%hhu sec\n", plan_params->cycle_length);
				for(i=0; i<8; i++)
					printf("GF %d %d\n", i+1, plan_params->green_factor[i]);
				printf("Multiplier\t%hhu sec\n", plan_params->multiplier);
				printf("Offset A\t%hhu\n", plan_params->offsetA);
				printf("Offset B\t%hhu\n", plan_params->offsetB);
				printf("Offset C\t%hhu\n", plan_params->offsetC);
				printf("Permissive\t%hhu\n", plan_params->permissive);
				printf("Lag Phases\t%#hhx\n", plan_params->lag_phases);
				printf("Sync Phases\t%#hhx\n", plan_params->sync_phases);
				printf("Hold Phases\t%#hhx\n", plan_params->hold_phases);
				printf("Omit Phases\t%#hhx\n", plan_params->omit_phases);
				printf("Vehicle Min Recall\t%#hhx\n", plan_params->veh_min_recall);
				printf("Vehicle Max Recall\t%#hhx\n", plan_params->veh_max_recall);
				printf("Ped Recall\t%#hhx\n", plan_params->ped_recall);
				printf("Bicycle Recall\t%#hhx\n", plan_params->bicycle_recall);
				printf("Force Off Flag\t%#hhx\n", plan_params->force_off_flag);
			}
		}
	}
	if(verbose) 
		printf("get_coord_params 6-end: *fpin %d selectval %d inportisset %s *fpout %d selectval %d outportisset %s ser_driver_retval %d get_controller_timing_data_request_mess.offset %hx\n", *fpin, selectval, inportisset, *fpout, selectval, outportisset, ser_driver_retval, get_controller_timing_data_request_mess.offset);

	return 0;
}

int set_coord_params(plan_params_t *plan_params, int plan_num, mschedule_t *mschedule, int wait_for_data, int fdout, int fdin, char verbose) {

	int msg_len;
        fd_set readfds;
        fd_set writefds;
        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";
	int ser_driver_retval;
	get_controller_timing_data_request_t get_controller_timing_data_request_mess;
	gen_mess_typ readBuff;
	int i;
	char coord_plan_msg_buf[37];

	//Set the header and tail
	coord_plan_msg_buf[0] = 0x7e;	//start flag
	coord_plan_msg_buf[1] = 0x05;	//controller address
	coord_plan_msg_buf[2] = 0x13;	//control
	coord_plan_msg_buf[3] = 0xC0;	//IPI
	coord_plan_msg_buf[4] = 0x96;	//set command
	coord_plan_msg_buf[5] = 0x04;	//page ID
	coord_plan_msg_buf[6] = plan_num; //aka block ID
	coord_plan_msg_buf[34] = 0x00;	//FCS MSB
	coord_plan_msg_buf[35] = 0x00;	//FCS LSB
	coord_plan_msg_buf[36] = 0x7e;	//end flag

	for(i=0; i<8; i++){
		if( (mschedule->phase_sequence.phase_sequence[i] > 8) || (mschedule->phase_sequence.phase_sequence[i] < 1) )
			return -1;
		if( mschedule->phase_duration.phase_duration[i] > plan_params->cycle_length )
			return -2;
		plan_params->green_factor[mschedule->phase_sequence.phase_sequence[i] - 1] = mschedule->phase_duration.phase_duration[i];	
	}

	plan_params->lag_phases = (1 << (mschedule->phase_sequence.phase_sequence[2] - 1)) |
				  (1 << (mschedule->phase_sequence.phase_sequence[3] - 1)) |
				  (1 << (mschedule->phase_sequence.phase_sequence[6] - 1)) |
				  (1 << (mschedule->phase_sequence.phase_sequence[7] - 1));

	memcpy(&coord_plan_msg_buf[6], plan_params, sizeof(plan_params_t));

	/* Now append the FCS. */
	msg_len = sizeof(coord_plan_msg_buf) - 4;
	fcs_hdlc(msg_len, coord_plan_msg_buf, verbose);
	FD_ZERO(&writefds);
	FD_SET(fdout, &writefds);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if( (selectval = select(fdout+1, NULL, &writefds, NULL, &timeout)) <=0) {
		perror("coord_params select 1");
		outportisset = (FD_ISSET(fdout, &writefds)) == 0 ? "no" : "yes";
		printf("set_coord_params 3: fdout %d selectval %d outportisset %s\n", fdout, selectval, outportisset);
		return -3;
	}
	write ( fdout, coord_plan_msg_buf, sizeof(coord_plan_msg_buf));
	fflush(NULL);

	ser_driver_retval = 100;

	if(wait_for_data) {
		FD_ZERO(&readfds);
		FD_SET(fdin, &readfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if( (selectval = select(fdin+1, &readfds, NULL, NULL, &timeout)) <=0) {
			perror("coord_params 2");
			inportisset = (FD_ISSET(fdin, &readfds)) == 0 ? "no" : "yes";
			printf("set_coord_params 4: fdin %d selectval %d inportisset %s\n", fdin, selectval, inportisset);
			return -4;
		}
		ser_driver_retval = ser_driver_read(&readBuff, fdin, verbose);
		if(ser_driver_retval == 0) {
			printf("set_coord_params 5: Lost serial connection\n");
			return -5;
		}
	}
	if(verbose != 0)
		printf("set_coord_params 6-end: fdin %d selectval %d inportisset %s fdout %d selectval %d outportisset %s ser_driver_retval %d\n", fdin, selectval, inportisset, fdout, selectval, outportisset, ser_driver_retval);
	return 0;
}

int get_status(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose) {
	int msg_len;
        fd_set readfds;
        fd_set writefds;
        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";
	int ser_driver_retval;
	get_long_status8_resp_mess_typ get_long_status8_request;
        struct timespec start_time;
        struct timespec end_time;
	int i;
	
	if(verbose != 0)
		printf("get_status 1: Starting get_status request\n");
	/* Send the message to request GetLongStatus8 from 2070. */
	get_long_status8_request.start_flag = 0x7e;
	get_long_status8_request.address = 0x05;
	get_long_status8_request.control = 0x13;
	get_long_status8_request.ipi = 0xc0;
	get_long_status8_request.mess_type = 0x8c;
	get_long_status8_request.FCSmsb = 0x00;
	get_long_status8_request.FCSlsb = 0x00;

	if(fpout <= 0)
		return -1;
	/* Now append the FCS. */
	msg_len = sizeof(get_long_status8_mess_typ) - 4;
	fcs_hdlc(msg_len, &get_long_status8_request, verbose);
	FD_ZERO(&writefds);
	FD_SET(fpout, &writefds);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if( (selectval = select(fpout+1, NULL, &writefds, NULL, &timeout)) <=0) {
		perror("select 10");
		outportisset = (FD_ISSET(fpout, &writefds)) == 0 ? "no" : "yes";
		printf("get_status 3: fpout %d selectval %d outportisset %s\n", fpout, selectval, outportisset);
		return -3;
	}
                if(verbose != 0) {
                        clock_gettime(CLOCK_REALTIME, &start_time);
                }
	write ( fpout, &get_long_status8_request, msg_len+4 );
	fflush(NULL);

	ser_driver_retval = 100;

	if(wait_for_data && readBuff) {
		FD_ZERO(&readfds);
		FD_SET(fpin, &readfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
		    if(errno != EINTR) {
			perror("select 11");
			inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
			printf("get_status 4: fpin %d selectval %d inportisset %s\n", fpin, selectval, inportisset);
			return -2;
		    }
		}
		ser_driver_retval = ser_driver_read(readBuff, fpin, verbose);
		if(ser_driver_retval == 0) {
			printf("get_status 5: Lost USB connection\n");
			return -1;
		}
	}
	if(verbose != 0) {
		clock_gettime(CLOCK_REALTIME, &end_time);
		printf("get_status: Time for function call %f sec\n",
			(end_time.tv_sec + (end_time.tv_nsec/1.0e9)) -
			(start_time.tv_sec + (start_time.tv_nsec/1.0e9))
		);
		printf("get_status data: ");
		for(i = 0; i < sizeof(get_long_status8_resp_mess_typ); i++)
			printf("#%02d %#hhx ", i, readBuff->data[i]);
		printf("\n");
		printf("get_status 6-end: fpin %d selectval %d inportisset %s fpout %d selectval %d outportisset %s ser_driver_retval %d\n", fpin, selectval, inportisset, fpout, selectval, outportisset, ser_driver_retval);
	}
	return 0;
}

int get_spat(int wait_for_data, raw_signal_status_msg_t *praw_signal_status_msg, int fpin, char verbose, char print_packed_binary) {
        fd_set readfds;
        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";
	int ser_driver_retval;
	gen_mess_typ *readBuff = (gen_mess_typ *)praw_signal_status_msg;

	ser_driver_retval = 100;

	if(wait_for_data && praw_signal_status_msg) {
		FD_ZERO(&readfds);
		FD_SET(fpin, &readfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
		    if(errno != EINTR) {
			perror("select 11");
			inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
			printf("get_status 4: fpin %d selectval %d inportisset %s\n", fpin, selectval, inportisset);
			return -2;
		    }
		}
		ser_driver_retval = ser_driver_read((gen_mess_typ *)praw_signal_status_msg, fpin, verbose);
		if(ser_driver_retval == 0) {
			printf("get_status 5: Lost USB connection\n");
			return -1;
		}
	}
	if(print_packed_binary != 0)
		write(STDOUT_FILENO, &readBuff->data[5], sizeof(raw_signal_status_msg_t) - 9);
	else
	if(verbose != 0) 
	{
//		printf("get_spat 6-end: fpin %d selectval %d inportisset %s fpout %d selectval %d outportisset %s ser_driver_retval %d\n", fpin, selectval, inportisset, fpout, selectval, outportisset, ser_driver_retval);
		printf("get_spat 7: %#hhx %#hhx  %#hhx %.1f %.1f %#hhx %#hhx %#hhx %hhu %hhu %hhu %#hhx %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu\n", 
			praw_signal_status_msg->active_phase,
			praw_signal_status_msg->interval_A,
			praw_signal_status_msg->interval_B,
			praw_signal_status_msg->intvA_timer/10.0,
			praw_signal_status_msg->intvB_timer/10.0,
			praw_signal_status_msg->next_phase,
			praw_signal_status_msg->ped_call,
			praw_signal_status_msg->veh_call,
			praw_signal_status_msg->plan_num,
			praw_signal_status_msg->local_cycle_clock,
			praw_signal_status_msg->master_cycle_clock,
			praw_signal_status_msg->preempt,
			praw_signal_status_msg->permissive[0],
			praw_signal_status_msg->permissive[1],
			praw_signal_status_msg->permissive[2],
			praw_signal_status_msg->permissive[3],
			praw_signal_status_msg->permissive[4],
			praw_signal_status_msg->permissive[5],
			praw_signal_status_msg->permissive[6],
			praw_signal_status_msg->permissive[7],
			praw_signal_status_msg->force_off_A,
			praw_signal_status_msg->force_off_B,
			praw_signal_status_msg->ped_permissive[0],
			praw_signal_status_msg->ped_permissive[1],
			praw_signal_status_msg->ped_permissive[2],
			praw_signal_status_msg->ped_permissive[3],
			praw_signal_status_msg->ped_permissive[4],
			praw_signal_status_msg->ped_permissive[5],
			praw_signal_status_msg->ped_permissive[6],
			praw_signal_status_msg->ped_permissive[7]
		);
	}


	return 0;
}

int get_overlap(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose) {
	int msg_len;
        fd_set readfds;
        fd_set writefds;
        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";
	int ser_driver_retval;
	tsmss_get_msg_request_t overlap_get_request;
	
	if(verbose != 0)
		printf("get_overlap 1: Starting get_overlap request\n");
	overlap_get_request.get_hdr.start_flag = 0x7e;
	overlap_get_request.get_hdr.address = 0x05;
	overlap_get_request.get_hdr.control = 0x13;
	overlap_get_request.get_hdr.ipi = 0xc0;
	overlap_get_request.get_hdr.mess_type = 0x87;
	overlap_get_request.get_hdr.page_id = 0x02;
	overlap_get_request.get_hdr.block_id = 0x04;
	overlap_get_request.get_tail.FCSmsb = 0x00;
	overlap_get_request.get_tail.FCSlsb = 0x00;

	/* Now append the FCS. */
	msg_len = sizeof(tsmss_get_msg_request_t) - 4;
	fcs_hdlc(msg_len, &overlap_get_request, verbose);

	FD_ZERO(&writefds);
	FD_SET(fpout, &writefds);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if( (selectval = select(fpout+1, NULL, &writefds, NULL, &timeout)) <=0) {
		perror("select 12");
		outportisset = (FD_ISSET(fpout, &writefds)) == 0 ? "no" : "yes";
		printf("get_overlap 2: fpout %d selectval %d outportisset %s\n", fpout, selectval, outportisset);
		return -3;
	}
	write ( fpout, &overlap_get_request, msg_len+4 );
	fflush(NULL);
	sleep(2);

	ser_driver_retval = 100;

	if(wait_for_data && readBuff) {
		FD_ZERO(&readfds);
		FD_SET(fpin, &readfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
			perror("select 13");
			inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
			printf("get_overlap 3: fpin %d selectval %d inportisset %s\n", fpin, selectval, inportisset);
			return -2;
		}
		ser_driver_retval = ser_driver_read(readBuff, fpin, verbose);
		if(ser_driver_retval == 0) {
			printf("get_overlap 4: Lost USB connection\n");
			return -1;
		}
	}
	if(verbose != 0)
		printf("get_overlap 5-end: fpin %d selectval %d inportisset %s fpout %d selectval %d outportisset %s ser_driver_retval %d\n", fpin, selectval, inportisset, fpout, selectval, outportisset, ser_driver_retval);
	return 0;
}

int set_overlap(overlap_msg_t *poverlap_set_request, int fpin, int fpout, char verbose) {
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
	char *tempbuf = (char *)poverlap_set_request;
	int i;

printf("set_overlap input message: ");
for(i=0; i<sizeof(overlap_msg_t); i++) 
	printf("%#hhx ", tempbuf[i]);
printf("\n");

	if(verbose != 0)
		printf("set_overlap 1: Starting set_overlap request\n");
	poverlap_set_request->overlap_hdr.start_flag = 0x7e;
	poverlap_set_request->overlap_hdr.address = 0x05;
	poverlap_set_request->overlap_hdr.control = 0x13;
	poverlap_set_request->overlap_hdr.ipi = 0xc0;
	poverlap_set_request->overlap_hdr.mess_type = 0x96;
	poverlap_set_request->overlap_hdr.page_id = 0x02;
	poverlap_set_request->overlap_hdr.block_id = 0x04;
	poverlap_set_request->overlap_tail.FCSmsb = 0x00;
	poverlap_set_request->overlap_tail.FCSlsb = 0x00;

	/* Now append the FCS. */
	msg_len = sizeof(overlap_msg_t) - 4;
	fcs_hdlc(msg_len, poverlap_set_request, verbose);

printf("set_overlap output message: ");
for(i=0; i<sizeof(overlap_msg_t); i++) 
	printf("%#hhx ", tempbuf[i]);
printf("\n");

	FD_ZERO(&writefds);
	FD_SET(fpout, &writefds);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if( (selectval = select(fpout+1, NULL, &writefds, NULL, &timeout)) <=0) {
		perror("select 14");
		outportisset = (FD_ISSET(fpout, &writefds)) == 0 ? "no" : "yes";
		printf("set_overlap 2: fpout %d selectval %d outportisset %s\n", fpout, selectval, outportisset);
		return -3;
	}
	write ( fpout, poverlap_set_request, sizeof(overlap_msg_t));
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
			printf("set_overlap 3: fpin %d selectval %d inportisset %s\n", fpin, selectval, inportisset);
			return -2;
		}
		ser_driver_retval = ser_driver_read(&readBuff, fpin, verbose);
		if(ser_driver_retval == 0) {
			printf("set_overlap 4: Lost USB connection\n");
			return -1;
		}
	}
	if(verbose != 0)
		printf("set_overlap 5-end: fpin %d selectval %d inportisset %s fpout %d selectval %d outportisset %s ser_driver_retval %d\n", fpin, selectval, inportisset, fpout, selectval, outportisset, ser_driver_retval);
	return 0;
}

int get_special_flags(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose) {
	int msg_len;
        fd_set readfds;
        fd_set writefds;
        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";
	int ser_driver_retval;
	tsmss_get_msg_request_t special_flags_get_request;
	
	if(verbose != 0)
		printf("get_special_flags 1: Starting get_special_flags request\n");
	special_flags_get_request.get_hdr.start_flag = 0x7e;
	special_flags_get_request.get_hdr.address = 0x05;
	special_flags_get_request.get_hdr.control = 0x13;
	special_flags_get_request.get_hdr.ipi = 0xc0;
	special_flags_get_request.get_hdr.mess_type = 0x87;
	special_flags_get_request.get_hdr.page_id = 0x02;
	special_flags_get_request.get_hdr.block_id = 0x02;
	special_flags_get_request.get_tail.FCSmsb = 0x00;
	special_flags_get_request.get_tail.FCSlsb = 0x00;

	/* Now append the FCS. */
	msg_len = sizeof(tsmss_get_msg_request_t) - 4;
	fcs_hdlc(msg_len, &special_flags_get_request, verbose);

	FD_ZERO(&writefds);
	FD_SET(fpout, &writefds);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if( (selectval = select(fpout+1, NULL, &writefds, NULL, &timeout)) <=0) {
		perror("select 12");
		outportisset = (FD_ISSET(fpout, &writefds)) == 0 ? "no" : "yes";
		printf("get_special_flags 2: fpout %d selectval %d outportisset %s\n", fpout, selectval, outportisset);
		return -3;
	}
	write ( fpout, &special_flags_get_request, msg_len+4 );
	fflush(NULL);
	sleep(2);

	ser_driver_retval = 100;

	if(wait_for_data && readBuff) {
		FD_ZERO(&readfds);
		FD_SET(fpin, &readfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
			perror("select 13");
			inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
			printf("get_special_flags 3: fpin %d selectval %d inportisset %s\n", fpin, selectval, inportisset);
			return -2;
		}
		ser_driver_retval = ser_driver_read(readBuff, fpin, verbose);
		if(ser_driver_retval == 0) {
			printf("get_special_flags 4: Lost USB connection\n");
			return -1;
		}
	}
	if(verbose != 0)
		printf("get_special_flags 5-end: fpin %d selectval %d inportisset %s fpout %d selectval %d outportisset %s ser_driver_retval %d\n", fpin, selectval, inportisset, fpout, selectval, outportisset, ser_driver_retval);
	return 0;
}

int set_special_flags(get_set_special_flags_t *pspecial_flags_set_request, int fpin, int fpout, char verbose) {
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
	char *tempbuf = (char *)pspecial_flags_set_request;
	int i;

printf("set_special_flags input message: ");
for(i=0; i<sizeof(get_set_special_flags_t); i++) 
	printf("%#hhx ", tempbuf[i]);
printf("\n");

	if(verbose != 0)
		printf("set_special_flags 1: Starting set_special_flags request\n");
	pspecial_flags_set_request->special_flags_hdr.start_flag = 0x7e;
	pspecial_flags_set_request->special_flags_hdr.address = 0x05;
	pspecial_flags_set_request->special_flags_hdr.control = 0x13;
	pspecial_flags_set_request->special_flags_hdr.ipi = 0xc0;
	pspecial_flags_set_request->special_flags_hdr.mess_type = 0x96;
	pspecial_flags_set_request->special_flags_hdr.page_id = 0x02;
	pspecial_flags_set_request->special_flags_hdr.block_id = 0x02;
	pspecial_flags_set_request->special_flags_tail.FCSmsb = 0x00;
	pspecial_flags_set_request->special_flags_tail.FCSlsb = 0x00;

	/* Now append the FCS. */
	msg_len = sizeof(get_set_special_flags_t) - 4;
	fcs_hdlc(msg_len, pspecial_flags_set_request, verbose);

printf("set_special_flags output message: ");
for(i=0; i<sizeof(get_set_special_flags_t); i++) 
	printf("%#hhx ", tempbuf[i]);
printf("\n");

	FD_ZERO(&writefds);
	FD_SET(fpout, &writefds);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if( (selectval = select(fpout+1, NULL, &writefds, NULL, &timeout)) <=0) {
		perror("select 14");
		outportisset = (FD_ISSET(fpout, &writefds)) == 0 ? "no" : "yes";
		printf("set_special_flags 2: fpout %d selectval %d outportisset %s\n", fpout, selectval, outportisset);
		return -3;
	}
	write ( fpout, pspecial_flags_set_request, sizeof(get_set_special_flags_t));
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
			printf("set_special_flags 3: fpin %d selectval %d inportisset %s\n", fpin, selectval, inportisset);
			return -2;
		}
		ser_driver_retval = ser_driver_read(&readBuff, fpin, verbose);
		if(ser_driver_retval == 0) {
			printf("set_special_flags 4: Lost USB connection\n");
			return -1;
		}
	}
	if(verbose != 0)
		printf("set_special_flags 5-end: fpin %d selectval %d inportisset %s fpout %d selectval %d outportisset %s ser_driver_retval %d\n", fpin, selectval, inportisset, fpout, selectval, outportisset, ser_driver_retval);
	return 0;
}

int check_and_reconnect_serial(int retval, int *fpin, int *fpout, char *port) {
        char *portcfg;
        static int j = 0;
        int is_usb;

        is_usb = strcmp( port, "/dev/ttyUSB") > 0 ? 1 : 0;

        fprintf(stderr,"check_and_reconnect_serial 1: function return value %d is_usb %d\n", retval, is_usb);
        switch(retval) {
                case 0:
                case -1:
                case -2:
                   if(is_usb) {
                        portcfg = "for x in /dev/ttyUSB*; do /bin/stty -F $x raw 38400;done";
                        if(retval<0) {
                                fprintf(stderr,"check_and_reconnect_serial 2: Lost USB connection. Will try to reconnect...\n");
                        }
                        else
                                fprintf(stderr,"check_and_reconnect_serial 3: Opening initial serial connection\n");
                        if(*fpin != 0)
                                close(*fpin);
                        if(*fpout != 0)
                                close(*fpout);
                        *fpin = 0;
                        *fpout = 0;
                        while(*fpin <= 0) {
                                sprintf(port, "/dev/ttyUSB%d", (j % 8));
                                fprintf(stderr,"check_and_reconnect_serial 4: Trying to open %s\n", port);
                                *fpin = open( port,  O_RDONLY );
                                if ( *fpin <= 0 ) {
                                        perror("check_and_reconnect_serial 5: serial inport open");
                                        fprintf(stderr, "check_and_reconnect_serial 6:Error opening device %s for input\n", port );
                                        j++;
                                }
                                sleep(1);
                        }
                        system(portcfg);
                        *fpout = open( port,  O_WRONLY );
                        if ( *fpout <= 0 ) {
                                perror("check_and_reconnect_serial 7: serial outport open");
                                fprintf(stderr, "check_and_reconnect_serial 8:Error opening device %s for output\n", port );
                                return -1;
                        }
                        break;
                    }
                    else {
                        portcfg = "for x in /dev/ttyS[01]; do /bin/stty -F $x raw 38400;done";
                        if(retval != 0 ) {
                                fprintf(stderr,"check_and_reconnect_serial 9: Lost RS232 connection. Will try to reconnect...\n");
                                close(*fpin);
                                close(*fpout);
                        }
                        else
                                fprintf(stderr,"check_and_reconnect_serial 10: Initial RS232 connection. Will try to connect...\n");
                        close(*fpin);
                        *fpin = -1;
                        while(*fpin < 0) {
                                fprintf(stderr,"check_and_reconnect_serial 11: Trying to open %s\n", port);
                                *fpin = open( port,  O_RDONLY );
                                sleep(2); //required to make flush work, for some reason
                                tcflush(*fpin, TCIFLUSH);
                                if ( *fpin < 0 ) {
                                        perror("check_and_reconnect_serial 12: serial inport open");
                                        fprintf(stderr, "check_and_reconnect_serial 13:Error opening device %s for input *fpin %d\n", port, *fpin );
                                }
                        }
                        system(portcfg);
                        *fpout = open( port,  O_WRONLY );
                        sleep(2); //required to make flush work, for some reason
                        tcflush(*fpin, TCOFLUSH);
                        if ( *fpout <= 0 ) {
                                perror("check_and_reconnect_serial 6: serial outport open");
                                fprintf(stderr, "check_and_reconnect_serial 7:Error opening device %s for output\n", port );
                                return -1;
                        }
                        break;

                    }
                default:
                        printf("check_and_reconnect_serial 8: default case\n");
                        return -2;
        }
        return 0;
}

int get_mem(unsigned short lomem, unsigned short num_bytes, int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose) {

	unsigned short temp;
	int msg_len;
        fd_set readfds;
        fd_set writefds;
        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";
	int ser_driver_retval;
	get_controller_timing_data_request_t get_controller_timing_data_request_mess;
        struct timespec start_time;
        struct timespec end_time;

	memset(&start_time, 0, sizeof(struct timespec));
	memset(&end_time, 0, sizeof(struct timespec));

	temp = ((lomem << 8) & 0xFF00) + ((lomem >> 8) & 0x00FF);
//printf("get_mem: lomem %#x temp %#x\n", lomem, temp);
        /* Send the message to request GetLongStatus8 from 2070. */
	memset(&get_controller_timing_data_request_mess, 0, sizeof(get_controller_timing_data_request_t));
        get_controller_timing_data_request_mess.start_flag = 0x7e;
        get_controller_timing_data_request_mess.address = 0x05;
        get_controller_timing_data_request_mess.control = 0x13;
        get_controller_timing_data_request_mess.ipi = 0xc0;
        get_controller_timing_data_request_mess.mess_type = 0x89;
        get_controller_timing_data_request_mess.offset = temp;
        get_controller_timing_data_request_mess.num_bytes = num_bytes;
        get_controller_timing_data_request_mess.FCSmsb = 0x00;
        get_controller_timing_data_request_mess.FCSlsb = 0x00;

        /* Now append the FCS. */
        msg_len = sizeof(get_controller_timing_data_request_t) - 4;
	fcs_hdlc(msg_len, &get_controller_timing_data_request_mess, verbose);
	FD_ZERO(&writefds);
	FD_SET(fpout, &writefds);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if( (selectval = select(fpout+1, NULL, &writefds, NULL, &timeout)) <=0) {
		perror("select 16");
		outportisset = (FD_ISSET(fpout, &writefds)) == 0 ? "no" : "yes";
		printf("get_mem 1: fpout %d selectval %d outportisset %s\n", fpout, selectval, outportisset);
		return -3;
	}
                if(verbose != 0) {
printf("Got to 1\n");
                        clock_gettime(CLOCK_REALTIME, &start_time);
                }

	write ( fpout, &get_controller_timing_data_request_mess, sizeof(get_controller_timing_data_request_t));
	fflush(NULL);
//	sleep(2);
//printf("get_mem: got to here get_controller_timing_data_request_mess.offset %hx \n", get_controller_timing_data_request_mess.offset);

	ser_driver_retval = 100;

	if(wait_for_data && readBuff) {
		FD_ZERO(&readfds);
		FD_SET(fpin, &readfds);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
			perror("select 17");
			inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
			printf("get_mem 2: fpin %d selectval %d inportisset %s\n", fpin, selectval, inportisset);
			return -2;
		}
		ser_driver_retval = ser_driver_read(readBuff, fpin, verbose);
		if(ser_driver_retval == 0) {
			printf("get_mem 3: Lost USB connection\n");
			return -1;
		}
	}
	if(verbose != 0)
                        clock_gettime(CLOCK_REALTIME, &end_time);
                        printf("get_mem: Time for function call %f sec\n",
                                (end_time.tv_sec + (end_time.tv_nsec/1.0e9)) -
                                (start_time.tv_sec + (start_time.tv_nsec/1.0e9))
                                );
printf("Got to 2 end_time.tv_sec %d end_time.tv_nsec/1.0e9 %f start_time.tv_sec %d start_time.tv_nsec/1.0e9 %f\n",
	end_time.tv_sec,
	end_time.tv_nsec/1.0e9,
	start_time.tv_sec,
	start_time.tv_nsec/1.0e9);
		printf("get_mem 4-end: fpin %d selectval %d inportisset %s fpout %d selectval %d outportisset %s ser_driver_retval %d\n", fpin, selectval, inportisset, fpout, selectval, outportisset, ser_driver_retval);
	return 0;
}

int get_short_status(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose) {
	int msg_len;
        fd_set readfds;
        fd_set writefds;
        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";
	int ser_driver_retval;
	get_short_status_request_t get_short_status_request;
	
	if(verbose != 0)
		printf("get_short_status 1: Starting get_short_status request\n");
	/* Send the message to request GetShortStatus from 2070. */
	get_short_status_request.start_flag = 0x7e;
	get_short_status_request.address = 0x05;
	get_short_status_request.control = 0x13;
	get_short_status_request.ipi = 0xc0;
	get_short_status_request.mess_type = 0x84;
	get_short_status_request.FCSmsb = 0x00;
	get_short_status_request.FCSlsb = 0x00;

	if(fpout <= 0)
		return -1;
	/* Now append the FCS. */
	msg_len = sizeof(get_short_status_request_t) - 4;
	fcs_hdlc(msg_len, &get_short_status_request, verbose);

	FD_ZERO(&writefds);
	FD_SET(fpout, &writefds);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if( (selectval = select(fpout+1, NULL, &writefds, NULL, &timeout)) <=0) {
		perror("select 18");
		outportisset = (FD_ISSET(fpout, &writefds)) == 0 ? "no" : "yes";
		printf("get_short_status 2: fpout %d selectval %d outportisset %s\n", fpout, selectval, outportisset);
		return -3;
	}
	write ( fpout, &get_short_status_request, msg_len + 4 );
	fflush(NULL);
	ser_driver_retval = 100;

	if(wait_for_data && readBuff) {
		FD_ZERO(&readfds);
		FD_SET(fpin, &readfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
		    if(errno != EINTR) {
			perror("select 19");
			inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
			printf("get_short_status 3: fpin %d selectval %d inportisset %s\n", fpin, selectval, inportisset);
			return -2;
		    }
		}
readBuff->data[5] = 0xff;
		ser_driver_retval = ser_driver_read(readBuff, fpin, verbose);
		if(ser_driver_retval == 0) {
			printf("get_short_status 4: Lost USB connection\n");
			return -1;
		}
	}
	if(verbose != 0)
		printf("get_short_status 5-end: fpin %d selectval %d inportisset %s fpout %d selectval %d outportisset %s ser_driver_retval %d\n", fpin, selectval, inportisset, fpout, selectval, outportisset, ser_driver_retval);
	return 0;
}

#define FREE	255
#define FLASH	254
#define RED	0
#define YELLOW	1
#define GREEN	2

#include <SPAT.h>

int build_spat(sig_plan_msg_t *sig_plan_msg, raw_signal_status_msg_t *ca_spat, phase_timing_t *phase_timing[], get_long_status8_resp_mess_typ *long_status8, plan_params_t *plan_params[], battelle_spat_t *battelle_spat, int verbose) {

	unsigned char Max_Green[8];
	unsigned char Min_Green[8];
	unsigned char active_phaseA;
	unsigned char active_phaseB;
	unsigned char start_index_phaseA;
	unsigned char start_index_phaseB;
	int i;
	int j;
	unsigned char interval_multiplier[] = {1,1,10,1,10,10,10,10,1,1,1,1,1,1,1,1};
	unsigned char greens[] =  { 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
	unsigned char yellows[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0};
	unsigned char colors[] = { GREEN, RED, GREEN, RED, GREEN, GREEN, GREEN, GREEN, RED, RED, RED, RED, YELLOW, YELLOW, RED, RED};
	unsigned char walks[] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	unsigned char flash_dont_walks[] = { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	unsigned char interval;
	unsigned char countdown_timer;
	unsigned char force_off;
	unsigned char curr_plan_num;
	int time_to_next_phase[MAX_PHASES];
	int total_time_to_next_phase[MAX_PHASES];
	timestamp_t ts;
	int tstemp;
	static int reduce_gap_start_time[MAX_PHASES] = {0,0,0,0,0,0,0,0};
	unsigned char phase_sequence[MAX_PHASES];
	unsigned char *offset_str[] = {"A", "B", "C"};
	static unsigned char msg_count = 0;
	unsigned char	*pbattelle_spat = battelle_spat;
	SPAT_t SPAT;

	memset(sig_plan_msg, 0, sizeof(sig_plan_msg_t));
	memset(time_to_next_phase, 0, sizeof(time_to_next_phase));
	memset(total_time_to_next_phase, 0, sizeof(total_time_to_next_phase));

	get_current_timestamp(&ts);
	tstemp = (1000 * ((3600 * ts.hour) + (60 * ts.min) + ts.sec)) + ts.millisec;

	if( (long_status8->pattern < 252) && (long_status8->pattern > 0) )
		curr_plan_num = ((long_status8->pattern - 1) / 3) + 1;
	else
		curr_plan_num = 0;

	sig_plan_msg->coordination_plan_params.coord_phase = ca_spat->active_phase;

	SPAT.msgID = 0x0d;					/* Byte 0 */
//	SPAT.intersections= 0x0d;					/* Byte 0 */

	battelle_spat->DSRCmsgID = 0x0d;					/* Byte 0 */
	battelle_spat->content_version = msg_count++;				/* Byte 1 */
	battelle_spat->size = sizeof(battelle_spat_t) - 6;			/* Byte 2-3 */
	battelle_spat->intersection_id.obj_id = 0x01;				/* Byte 4 */
	battelle_spat->intersection_id.size = 0x04;				/* Byte 5 */
	battelle_spat->intersection_id.intersection_id = 0x01;			/* Byte 6-9 */

	battelle_spat->intersection_status.obj_id = 0x02;			/* Byte 10 */
	battelle_spat->intersection_status.size = 0x01;				/* Byte 11 */
	if( (long_status8->pattern == 254) || (long_status8->status == 2) )
		battelle_spat->intersection_status.intersection_status = INTERSECTION_STATUS_CONFLICT_FLASH;	/* Byte 12 */
	else if(long_status8->preemption != 0)
		battelle_spat->intersection_status.intersection_status = INTERSECTION_STATUS_PREEMPT;	/* Byte 12 */
	else if( ((long_status8->interval & 0x0f) == 0x0a) || ((long_status8->interval & 0xf0) == 0xa0) )
		battelle_spat->intersection_status.intersection_status = INTERSECTION_STATUS_STOP_TIME;	/* Byte 12 */
	else 
		battelle_spat->intersection_status.intersection_status = INTERSECTION_STATUS_MANUAL;	/* Byte 12 */

	battelle_spat->message_timestamp.obj_id = 0x03;				/* Byte 13 */
	battelle_spat->message_timestamp.size = 0x05;				/* Byte 14 */
	battelle_spat->message_timestamp.timestamp_sec = (3600 * ts.hour) + (60 * ts.min) + ts.sec;	/* Byte 15-18 */
	battelle_spat->message_timestamp.timestamp_tenths = ts.millisec/100;	/* Byte 19 */


/* Calculation of "Time to change":
**	Red->Green, active phase = 1, nonactive phase = 4
**	Ring_A = 1,2,3,4 Ring_B = 5,6,7,8
*/
	if(verbose) {
		printf("build_spat: active_phase %#hhx next_phase %#hhx intvlA %#hhx intvlB %#hhx timerA %hhu timerB %hhu veh_call %#hhx master clock %hhu\n",
			ca_spat->active_phase,
			ca_spat->next_phase ,
			ca_spat->interval_A ,
			ca_spat-> interval_B,
			ca_spat-> intvA_timer,
			ca_spat-> intvB_timer,
			ca_spat-> veh_call,
			ca_spat-> master_cycle_clock
		);
	}
printf("build_spat: pattern %d curr_plan_num %d offset %s ca_spat->plan_num %d\n", long_status8->pattern, curr_plan_num, offset_str[(long_status8->pattern - 1)%3], ca_spat->plan_num);
    if(long_status8->pattern == FREE) 	
	{
/*
	    for(i=0; i<MAX_PHASES; i++) {
		j = 1 << i;


		interval = (i<4) ? ca_spat->interval_A : ca_spat->interval_B;
		Max_Green[i] = (ca_spat->veh_call & j) ? (phase_timing[i]->max_green1 + phase_timing[i]->min_green) : 0;
		Min_Green[i] = (ca_spat->veh_call & j) ? (phase_timing[i]->add_per_veh/10 + phase_timing[i]->min_green) : 0;

		if(ca_spat->active_phase & j) { //Active phase calculations
			battelle_spat->movement[i].obj_id = 0x04;
			battelle_spat->movement[i].lane_set.obj_id = 0x05;

			if(interval == 0x07) {
				if(reduce_gap_start_time[i] != 0)
					battelle_spat->movement[i].min_time_remaining.mintimeremaining = 
						(10 * phase_timing[i]->max_green1) - ((tstemp - reduce_gap_start_time[i])/100);
				else {
					battelle_spat->movement[i].min_time_remaining.mintimeremaining = 
						10 * phase_timing[i]->max_green1;
					reduce_gap_start_time[i] = tstemp;
				}
			}
			else {
				countdown_timer = (i<4) ? ca_spat->intvA_timer : ca_spat->intvB_timer;
				battelle_spat->movement[i].min_time_remaining.mintimeremaining = interval_multiplier[interval] * countdown_timer;
				battelle_spat->movement[i].max_time_remaining.maxtimeremaining = battelle_spat->movement[i].min_time_remaining.mintimeremaining;
				reduce_gap_start_time[i] = 0;
			}
printf("timer for movement %d %d interval %hhx plan %hhu\n", i+1, battelle_spat->movement[i].min_time_remaining.mintimeremaining, interval, curr_plan_num);
		
			Max_Green[i] = battelle_spat->movement[i].min_time_remaining.mintimeremaining; 
			Min_Green[i] = battelle_spat->movement[i].min_time_remaining.mintimeremaining; 
			if(verbose)
				printf("\n\nPhase %d min_time_remaining %hu intrvl %#hhx veh_call %#hhx ped_call %#hhx\n\n", 
					i+1, 
					battelle_spat->movement[i].min_time_remaining.mintimeremaining*interval_multiplier[interval], 
					interval, 
					ca_spat->veh_call, 
					ca_spat->ped_call
				);
		}
*/
/*
		else {
			battelle_spat->movement[i].max_time_remaining.maxtimeremaining = 

				( (10*max(Max_Green[(i+1)%4], Max_Green[(i+1)%8])) + 
				max(phase_timing[(i+1)%4]->yellow, phase_timing[(i+1)%8]->yellow) + 
				max(phase_timing[(i+1)%4]->all_red, phase_timing[(i+1)%8]->all_red) +

				(10*max(Max_Green[(i+2)%4], Max_Green[(i+2)%8])) + 
				max(phase_timing[(i+2)%4]->yellow, phase_timing[(i+2)%8]->yellow) + 
				max(phase_timing[(i+2)%4]->all_red, phase_timing[(i+2)%8]->all_red) +

				(10*max(Max_Green[(i+3)%4], Max_Green[(i+3)%8])) + 
				max(phase_timing[(i+3)%4]->yellow, phase_timing[(i+3)%8]->yellow) + 
				max(phase_timing[(i+3)%4]->all_red, phase_timing[(i+3)%8]->all_red) )/10;

			if(verbose) {
				printf("build_spat: battelle_spat->movement[%d].max_time_remaining.maxtimeremaining %d\n",
					i, battelle_spat->movement[i].max_time_remaining.maxtimeremaining); 
			}
	    	}
*/
/*
		if(verbose) {
			printf("veh_call %#hhx max_green1(%d) %hhu yellow(%d) %hhu all-red(%d) %hhu min_green(%d) %hhu add_per_veh(%d) %hhu Max_Green(%d) %hu Min_Green(%d) %hhu\n",
				ca_spat->veh_call, 
				i+1, phase_timing[i]->max_green1, 
				i+1, phase_timing[i]->yellow,
				i+1, phase_timing[i]->all_red,
				i+1, phase_timing[i]->min_green,
				i+1, phase_timing[i]->add_per_veh,
				i+1, Max_Green[i],
				i+1, Min_Green[i]
			);
			printf("   active_phase %#hhx intvA %#hhx intvB %#hhx\n",
				ca_spat->active_phase,
				ca_spat->interval_A,
				ca_spat->interval_B
			);
			printf("   tstemp %u %hu\n",
				tstemp
				,sig_plan_msg->ms_since_midnight
			);
		}
	}

*/
    }
    else{
//printf("Phase sequence ");
	//Determine phase sequence in pairs. 
	for(i=0; i<MAX_PHASES; i+=2) {
		Max_Green[i] = (ca_spat->veh_call & j) ? min(phase_timing[i]->max_green1+phase_timing[i]->min_green, plan_params[curr_plan_num]->green_factor[i] - phase_timing[i]->min_green) : 0;
		Max_Green[i+1] = (ca_spat->veh_call & j<<1) ? min(phase_timing[i+1]->max_green1+phase_timing[i+1]->min_green, plan_params[curr_plan_num]->green_factor[i+1] - phase_timing[i+1]->min_green) : 0;

//0 1 2 3 4 5 6 7
//2 1 4 3 5 6 8 7 
//active_phaseA 2
//start_index_phaseA 0
		//if i is a lag phase, then i+1 must be a leading phase, and vice versa
		j = 1 << i;
		if( (j & plan_params[curr_plan_num]->lag_phases) ) {
			phase_sequence[i] = i+2;
			phase_sequence[i+1] = i+1;
			if(ca_spat->active_phase & j) { //Active phase calculations
				if(i<4) {
					start_index_phaseA = i+1;
					active_phaseA = i+1;
				}
				else {
					start_index_phaseB = i+1;
					active_phaseB = i+1;
				}
			}
			else {
				j <<= 1;
				if(ca_spat->active_phase & j) { //Active phase calculations
					if(i<4){
						start_index_phaseA = i;
						active_phaseA = i+2;
					}
					else{
						start_index_phaseB = i;
						active_phaseB = i+2;
					}
				}
			}
		}
		else {
			phase_sequence[i] = i+1;
			phase_sequence[i+1] = i+2;
			if(ca_spat->active_phase & j) { //Active phase calculations
				if(i<4) {
					start_index_phaseA = i;
					active_phaseA = i+1;
				}
				else {
					start_index_phaseB = i;
					active_phaseB = i+1;
				}
			}
			else {
				j <<= 1;
				if(ca_spat->active_phase & j) { //Active phase calculations
					if(i<4){
						start_index_phaseA = i+1;
						active_phaseA = i+2;
					}
					else{
						start_index_phaseB = i+1;
						active_phaseB = i+2;
					}
				}
			}
		}
//printf("#%hhu %hhu #%hhu %hhu ", i, phase_sequence[i], i+1, phase_sequence[i+1]);
	}
//printf("start_index_phaseA %hhu active_phaseA %hhu start_index_phaseB %hhu active_phaseB %hhu ", start_index_phaseA, active_phaseA, start_index_phaseB, active_phaseB);
//printf("\n");

//printf("Time to next phase ");
	//Determine time_to_next_phase for all phases
	for(i=0; i<MAX_PHASES; i++) {
		j = 1 << i;
		if(ca_spat->active_phase & j) { //Active phase calculations
			if(i<4) {
				interval = ca_spat->interval_A;
				countdown_timer = ca_spat->intvA_timer;
				force_off = ca_spat->force_off_A;
			}
			else {
				interval = ca_spat->interval_B;
				countdown_timer = ca_spat->intvB_timer;
				force_off = ca_spat->force_off_B;
			}
			switch(colors[interval]) {
				case GREEN:


					time_to_next_phase[i] = 10*(force_off - ca_spat->local_cycle_clock) + 
						phase_timing[i]->yellow + 
						phase_timing[i]->all_red;
						reduce_gap_start_time[i] = 0;
					if(plan_params[i]->sync_phases & j) {
						time_to_next_phase[i] = 10*(plan_params[curr_plan_num]->cycle_length - ca_spat->local_cycle_clock) + 
							phase_timing[i]->yellow + 
							phase_timing[i]->all_red;
							reduce_gap_start_time[i] = 0;
					}
/*					if(interval == 0x07) {
						if(reduce_gap_start_time[i] == 0)
							reduce_gap_start_time[i] = tstemp;
						time_to_next_phase[i] = 
							(10 * Max_Green[i]) - 
							((tstemp - reduce_gap_start_time[i])/100) +
							phase_timing[i]->yellow + 
							phase_timing[i]->all_red;
//printf("REDUCE GAP %d yellow %hhu all-red %hhu Max_Green[%d] %d tstemp %d reduce_gap_start_time %d\n", ((10 * Max_Green[i]) - ((tstemp - reduce_gap_start_time[i])/100))/10, phase_timing[i]->yellow, phase_timing[i]->all_red, i+1, Max_Green[i],tstemp, reduce_gap_start_time);
					}
					else 
						if(plan_params[i]->sync_phases & j) {
							time_to_next_phase[i] = 10*(plan_params[curr_plan_num]->cycle_length - ca_spat->local_cycle_clock) + 
								phase_timing[i]->yellow + 
								phase_timing[i]->all_red;
								reduce_gap_start_time[i] = 0;
						}
					
					else {
						time_to_next_phase[i] = 10*countdown_timer + 
							phase_timing[i]->yellow + 
							phase_timing[i]->all_red;
							reduce_gap_start_time[i] = 0;
					}
*/
					break;
				case YELLOW:
					time_to_next_phase[i] = countdown_timer + phase_timing[i]->all_red;
					reduce_gap_start_time[i] = 0;
					break;
				case RED:
					time_to_next_phase[i] = countdown_timer;
					reduce_gap_start_time[i] = 0;
					break;
			}
//printf("#%hhu %hhu %hhu %hhu %hhu", i+1, time_to_next_phase[i]/10, countdown_timer, ca_spat->force_off_A, ca_spat->force_off_B);
		}
		else {
			time_to_next_phase[i] = (10 * Max_Green[i]) + phase_timing[i]->yellow + phase_timing[i]->all_red;
		}
//printf("#%hhu %hhu ", i+1, time_to_next_phase[i]/10);
}
//printf("\n");
				
/*
max_green1	50 50 50 50 50 50 50 50
GreenFactor	20 20 20 20 20 20 20 20
Yellow		 3  3  3  3  3  3  3  3
All-red		 2  2  2  2  2  2  2  2
Cycle time	110
lag phases	1,3,5,7
sync phases	2,6
Red-green change 1,5=6; 4,8=31; 3,7=56; 2,6=81
*/			
		
//	The phase sequence is now contained in phase_sequence[]. There are two active phases, one in ring A (phases 1-4) and one
//	in ring B (phases 5-8).  The indices of the active phases within this array are start_index_phaseA and start_index_phaseB,
//	respectively. So calculating the max and min remaining times of nonactive phases is a matter of cycling through the phase_sequence
//	array, starting with the currently active phases, to find progressively later remaining times for phases later in the sequence.

	battelle_spat->movement[active_phaseA].min_time_remaining.mintimeremaining = 
		interval_multiplier[ca_spat->interval_A] * ca_spat->intvA_timer;
	battelle_spat->movement[active_phaseA].max_time_remaining.maxtimeremaining =
		min(plan_params[curr_plan_num]->green_factor[active_phaseA], 
			phase_timing[phase_sequence[active_phaseA]]->max_green1);


//0 1 2 3 4 5 6 7
//2 1 4 3 5 6 8 7 
//printf("#%hhu %hhu ", start_index_phaseA, active_phaseA);
	for(i=0; i<MAX_PHASES/2 - 1; i++) {
		total_time_to_next_phase[phase_sequence[(start_index_phaseA+i+1)%4]] = 
			total_time_to_next_phase[phase_sequence[(start_index_phaseA+i)%4]] + time_to_next_phase[phase_sequence[(start_index_phaseA+i)%4]];
//printf("#%hhu %hhu %hhu %d sec ", start_index_phaseA, active_phaseA, phase_sequence[start_index_phaseA+i+1]%4, total_time_to_next_phase[phase_sequence[start_index_phaseA]+i]);
//printf("#%hhu %hhu sec ", (start_index_phaseA+i+1)%4, total_time_to_next_phase[phase_sequence[(start_index_phaseA+i+1)%4]]/10);

			battelle_spat->movement[i].max_time_remaining.maxtimeremaining = 
				( (10*max(Max_Green[(i+1)%4], Max_Green[(i+1)%8])) + 
				max(phase_timing[(i+1)%4]->yellow, phase_timing[(i+1)%8]->yellow) + 
				max(phase_timing[(i+1)%4]->all_red, phase_timing[(i+1)%8]->all_red) +

				(10*max(Max_Green[(i+2)%4], Max_Green[(i+2)%8])) + 
				max(phase_timing[(i+2)%4]->yellow, phase_timing[(i+2)%8]->yellow) + 
				max(phase_timing[(i+2)%4]->all_red, phase_timing[(i+2)%8]->all_red) +
	
//				(10*max(Max_Green[(i+3)%4], Max_Green[(i+3)%8])) + 
				max(phase_timing[(i+3)%4]->yellow, phase_timing[(i+3)%8]->yellow) + 
				max(phase_timing[(i+3)%4]->all_red, phase_timing[(i+3)%8]->all_red) )/10;
	}

//printf("local clock %hhu\n", ca_spat->local_cycle_clock);
    }
//printf("\n");

if(verbose) {
	printf("build_spat:\n");
	for(j=0; j<sizeof(battelle_spat_t); j+=10){
		printf("%d: ", j);
		for(i=0; i<10; i++)
			printf("%hhx ", pbattelle_spat[j+i]);
		printf("\n");
	}
}
printf("sizeof(battelle_spat_t) %d sizeof(movement_t) %d\n", sizeof(battelle_spat_t), sizeof(movement_t));
printf("sizeof(intersection_id_t) %d sizeof(intersection_status_t) %d\n", sizeof(intersection_id_t), sizeof(intersection_status_t));
printf("sizeof(message_timestamp_t) %d\n", sizeof(message_timestamp_t));
	return 0;
}


int build_sigplanmsg(sig_plan_msg_t *sig_plan_msg, phase_timing_t *phase_timing[], plan_params_t *current_plan_params,get_long_status8_resp_mess_typ *long_status8, int verbose) {

	timestamp_t ts;
	int tstemp;
	int i;
	char *buf = (char *)sig_plan_msg;
	unsigned char curr_plan_num;
	unsigned char curr_offset;

	memset(sig_plan_msg, 0, sizeof(sig_plan_msg_t));
	sig_plan_msg->mmitss_msg_hdr.msgid = MSG_ID_SIG_PLAN;

	get_current_timestamp(&ts);
	tstemp = (1000 * ((3600 * ts.hour) + (60 * ts.min) + ts.sec)) + ts.millisec;
	sig_plan_msg->mmitss_msg_hdr.cur_epoch_time = tstemp/10;

	sig_plan_msg->phase_timing_params.obj_id = 0x01;
	sig_plan_msg->phase_timing_params.size = 0x31;
//MUST FIX!! Permitted phases is contained in the GetPhaseFlags request and in memory address 0x1F0
	sig_plan_msg->phase_timing_params.permitted_phases = 0xFF;

	sig_plan_msg->coordination_plan_params.obj_id = 0x02;
	sig_plan_msg->coordination_plan_params.size = 0x0C;


	for(i=0; i<MAX_PHASES; i++) {
		sig_plan_msg->phase_timing_params.min_green[i] = phase_timing[i]->min_green;
		sig_plan_msg->phase_timing_params.max_green[i] = phase_timing[i]->max_green1;
		sig_plan_msg->phase_timing_params.ped_walk[i] = phase_timing[i]->walk_1;
		sig_plan_msg->phase_timing_params.ped_clr[i] = phase_timing[i]->dont_walk;
		sig_plan_msg->phase_timing_params.yellow[i] = phase_timing[i]->yellow;
		sig_plan_msg->phase_timing_params.red_clr[i] = phase_timing[i]->all_red;
		if(long_status8->pattern < 252)
			sig_plan_msg->coordination_plan_params.green_factor[i] = current_plan_params->green_factor[i];
	}

printf("build_sigplanmsg: Got to 1 long_status8->pattern %d\n", long_status8->pattern);

	if( (long_status8->pattern < 252) && (long_status8->pattern > 0)) {
		sig_plan_msg->coordination_plan_params.cycle_length = current_plan_params->cycle_length;
		sig_plan_msg->coordination_plan_params.coord_phase = current_plan_params->sync_phases;

		curr_plan_num = ((long_status8->pattern - 1) / 3) + 1;
		curr_offset = ((long_status8->pattern - 1) % 3);

printf("build_sigplanmsg: Got to 2 curr_plan_num %d curr_offset %d\n", curr_plan_num, curr_offset);
		switch(curr_offset) {
			case 0:
				sig_plan_msg->coordination_plan_params.offset = current_plan_params->offsetA;
				break;
			case 1:
				sig_plan_msg->coordination_plan_params.offset = current_plan_params->offsetB;
				break;
			case 2:
				sig_plan_msg->coordination_plan_params.offset = current_plan_params->offsetC;
				break;
		}
	}
	else
		curr_plan_num = 0;
	sig_plan_msg->coordination_plan_params.plan_num = curr_plan_num;

	if(verbose) {
		printf("build_sigplanmsg:");
		for(i=0; i<sizeof(sig_plan_msg_t); i++){ 
			if(!(i%10))
				printf("\n%d: ", i);
			printf("0x%.2hhx ", buf[i]);
		}
		printf("\n");
	}
	return 0;
}
