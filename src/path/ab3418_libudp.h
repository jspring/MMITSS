#ifndef AB3418_LIB_H
#define AB3418_LIB_H

#include "msgs.h"
#include "timestamp.h"
#include "ab3418commudp.h"

#define	FALSE	0
#define	TRUE	1

#define MAX_PHASES	8
#define MAX_PLANS	10

typedef int bool_typ;

typedef struct {
        int message_type;       //Set to 0x1234
        unsigned char phase;                      // phase
        int max_green;
        int min_green;
        int yellow;
        int all_red;
        cell_addr_data_t cell_addr_data;     // Determines parameters to change
} IS_PACKED db_timing_set_2070_t;


typedef struct {
        int message_type;       //Set to 0x1235
        unsigned char phase;                      // phase
        unsigned short page;    // timing settings=0x100, local plan settings=0x300
} IS_PACKED db_timing_get_2070_t;

typedef struct {
        timestamp_t ts;
        unsigned char greens;
        unsigned char yellows;
        unsigned char reds;
        unsigned char phase_status_colors[8];
        unsigned char barrier_flag;
} IS_PACKED phase_status_t;

extern bool_typ ser_driver_read( gen_mess_typ *pMessagebuff, int fpin, char verbose);
extern int get_timing(db_timing_get_2070_t *db_timing_get_2070, int wait_for_data, phase_timing_t *phase_timing, int *fpin, int *fpout, char verbose);
extern int get_mem(unsigned short lomem, unsigned short num_bytes, int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose);
extern int get_status(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose);
extern int get_short_status(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose);
extern int get_overlap(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose);
int get_special_flags(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose);
int check_and_reconnect_serial(int retval, int *fpin, int *fpout, char *port);
extern int set_timing(db_timing_set_2070_t *db_timing_set_2070, int *msg_len, int fpin, int fpout, char verbose);
extern int print_status(get_long_status8_resp_mess_typ *status);
extern void fcs_hdlc(int msg_len, void *msgbuf, char verbose);
extern int get_spat(int wait_for_data, raw_signal_status_msg_t *praw_signal_status_msg, int fpin, char verbose, char print_packed_binary);
extern int build_spat(sig_plan_msg_t *sig_plan_msg, raw_signal_status_msg_t *ca_spat, phase_timing_t *phase_timing[8], get_long_status8_resp_mess_typ *long_status8, plan_params_t *plan_params[10], battelle_spat_t *battelle_spat, int verbose);
int get_coord_params(plan_params_t *plan_params, int plan_num, int wait_for_data, int *fpout, int *fpin, char verbose);
extern int set_coord_params(plan_params_t *plan_params, int plan_num, mschedule_t *mschedule, int wait_for_data, int fdout, int fdin, char verbose);

#endif
