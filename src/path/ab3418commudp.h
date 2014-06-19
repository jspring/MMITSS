//ab3418comm.h - prototypes and structs for ab3418_comm.c

#ifndef AB3418COMM_H
#define AB3418COMM_H

#include "msgs.h"

#define DB_2070_TIMING_SET_TYPE	5501
#define DB_2070_TIMING_SET_VAR	DB_2070_TIMING_SET_TYPE
#define DB_2070_TIMING_GET_TYPE	5502
#define DB_2070_TIMING_GET_VAR	DB_2070_TIMING_GET_TYPE
#define DB_TSCP_STATUS_TYPE	5503
#define DB_TSCP_STATUS_VAR	DB_TSCP_STATUS_TYPE
#define DB_SHORT_STATUS_TYPE	5504
#define DB_SHORT_STATUS_VAR	DB_SHORT_STATUS_TYPE
#define DB_PHASE_1_TIMING_TYPE	5601
#define DB_PHASE_1_TIMING_VAR	DB_PHASE_1_TIMING_TYPE
#define DB_PHASE_2_TIMING_TYPE	5602
#define DB_PHASE_2_TIMING_VAR	DB_PHASE_2_TIMING_TYPE
#define DB_PHASE_3_TIMING_TYPE	5603
#define DB_PHASE_3_TIMING_VAR	DB_PHASE_3_TIMING_TYPE
#define DB_PHASE_4_TIMING_TYPE	5604
#define DB_PHASE_4_TIMING_VAR	DB_PHASE_4_TIMING_TYPE
#define DB_PHASE_5_TIMING_TYPE	5605
#define DB_PHASE_5_TIMING_VAR	DB_PHASE_5_TIMING_TYPE
#define DB_PHASE_6_TIMING_TYPE	5606
#define DB_PHASE_6_TIMING_VAR	DB_PHASE_6_TIMING_TYPE
#define DB_PHASE_7_TIMING_TYPE	5607
#define DB_PHASE_7_TIMING_VAR	DB_PHASE_7_TIMING_TYPE
#define DB_PHASE_8_TIMING_TYPE	5608
#define DB_PHASE_8_TIMING_VAR	DB_PHASE_8_TIMING_TYPE
#define DB_PHASE_STATUS_TYPE    5620
#define DB_PHASE_STATUS_VAR     DB_PHASE_STATUS_TYPE

// Cell Addresses of various timing parameters
// These are the addresses for phase 1 parameters; 
// for phase N, change the second numeral to N.  So
// for "Flash Dont Walk" phase 2, change the cell address
// from 0x111 to 0x121. 
#define	WALK_1  	0x0110
#define FLASH_DONT_WALK 0x0111
#define MINIMUM_GREEN   0x0112
#define DETECTOR_LIMIT  0x0113
#define ADD_PER_VEH     0x0114
#define EXTENSION       0x0115
#define MAXIMUM_GAP     0x0116
#define MINIMUM_GAP     0x0117
#define MAXIMUM_GREEN_1 0x0118
#define MAXIMUM_GREEN_2 0x0119
#define MAXIMUM_GREEN_3 0x011a
#define MAXIMUM_INITIAL 0x011b
#define REDUCE_GAP_BY   0x011c
#define REDUCE_EVERY    0x011d
#define YELLOW          0x011e
#define ALL_RED		0x011f

#define	CYCLE_LENGTH	0x0310 
#define GREEN_FACTOR_1	0x0311 
#define GREEN_FACTOR_2	0x0312 
#define GREEN_FACTOR_3	0x0313 
#define GREEN_FACTOR_4	0x0314 
#define GREEN_FACTOR_5	0x0315 
#define GREEN_FACTOR_6	0x0316 
#define GREEN_FACTOR_7	0x0317 
#define GREEN_FACTOR_8	0x0318 
#define MULTIPLIER	0x0319 
#define PLAN_A          0x031a 
#define OFFSET_B        0x031b 
#define NO_LABEL_C      0x031c 

#define PAGE_TIMING	0X100
#define PAGE_LOCAL_PLAN	0X300

typedef struct {
	unsigned char walk_1;
	unsigned char dont_walk;
	unsigned char min_green;
	unsigned char detector_limit;
	unsigned char add_per_veh;
	unsigned char extension;
	unsigned char max_gap;
	unsigned char min_gap;
	unsigned char max_green1;
	unsigned char max_green2;
	unsigned char max_green3;
	unsigned char not_available;
	unsigned char reduce_gap_by;
	unsigned char reduce_every;
	unsigned char yellow;
	unsigned char all_red;
} IS_PACKED phase_timing_t; 

typedef struct {
	unsigned char plan_num; //aka block ID in TSMSS message
	unsigned char cycle_length;
	unsigned char green_factor[8];
	unsigned char multiplier;
	unsigned char offsetA;
	unsigned char offsetB;
	unsigned char offsetC;
	unsigned char permissive;
	unsigned char lag_phases;
	unsigned char sync_phases;
	unsigned char hold_phases;
	unsigned char omit_phases;
	unsigned char veh_min_recall;
	unsigned char veh_max_recall;
	unsigned char ped_recall;
	unsigned char bicycle_recall;
	unsigned char force_off_flag;
	unsigned char spare1;
	unsigned char spare2;
	unsigned char spare3;
} IS_PACKED plan_params_t; 

typedef struct {
	unsigned char obj_id; 	//Object ID; set to 0x01
	unsigned char size;	//Message size, not including obj_id and size; set to 0x31
	unsigned char permitted_phases;	//Bit-mapped with permitted phases
	unsigned char min_green[8];	//Minimum green, sec
	unsigned char max_green[8];	//Maximum green, sec
	unsigned char ped_walk[8];	//Pedestrian walk, sec
	unsigned char ped_clr[8];	//Pedestrian clear time (i.e. flash don't walk), sec
	unsigned char yellow[8];	//Phase yellow time, sec
	unsigned char red_clr[8];	//Phase all-red, sec
} IS_PACKED phase_timing_params_t;

typedef struct {
	unsigned char obj_id; 	//Object ID; set to 0x02
	unsigned char size;	//Message size, not including obj_id and size; set to 0x0C
	unsigned char plan_num;	//Coordination plan number 
	unsigned char cycle_length;	//Cycle length
	unsigned char offset;	//Current offset (A, B, or C)
	unsigned char coord_phase;	//Coordination phase; bit-mapped
	unsigned char green_factor[8];	//Green factor
} IS_PACKED  coordination_plan_params_t;
	
typedef struct {
	unsigned short 	internal_msg_header; 	// set to 0xffff
	unsigned char	msg_id;		    	// set to 0x01
	unsigned int	ms_since_midnight;	// time since midnight, 0.001 s resolution
	phase_timing_params_t phase_timing_params;
	coordination_plan_params_t coordination_plan_params;
} IS_PACKED sig_plan_msg_t;

//Battelle SPaT objects

typedef struct {
	unsigned char obj_id; //=0x01
	unsigned char size; //=0x04
	unsigned int intersection_id;
} IS_PACKED intersection_id_t;

typedef struct {
	unsigned char obj_id; //=0x02
	unsigned char size; //=0x01
	unsigned char intersection_status;
} IS_PACKED intersection_status_t;

typedef struct {
	unsigned char obj_id; //=0x03
	unsigned char size; //=0x05
	unsigned int timestamp_sec;
	unsigned char timestamp_tenths;
} IS_PACKED message_timestamp_t;

#define NUM_LANES	8
typedef struct {
	unsigned char obj_id; //=0x05
	unsigned char size; //=16 (variable, 2 bytes per lane described)
	unsigned char laneset[NUM_LANES][2]; //For now, lane=phase
} IS_PACKED lane_set_t;

typedef struct {
	unsigned char obj_id; //=0x06
	unsigned char size; //=32
	unsigned char currentstate[NUM_LANES][4];
} IS_PACKED current_state_t;

typedef struct {
	unsigned char obj_id; //=0x07
	unsigned char size; //=1
	unsigned short mintimeremaining;
} IS_PACKED min_time_remaining_t;

typedef struct {
	unsigned char obj_id; //=0x08
	unsigned char size; //=1
	unsigned short maxtimeremaining;
} IS_PACKED max_time_remaining_t;

typedef struct {
	unsigned char obj_id; //=0x09
	unsigned char size; //=32
	unsigned char yellowstate[NUM_LANES][4];
} IS_PACKED yellow_state_t;

#define MAX_PHASES	8
typedef struct {
	unsigned char obj_id; //=0x0A
	unsigned char size; //=16
	unsigned short yellowtime[MAX_PHASES]; //=yellow timing parameter
} IS_PACKED yellow_time_t;

typedef struct {
	unsigned char obj_id; //=0x0B
	unsigned char size; //=0x01
	unsigned char detect_flag;
} IS_PACKED ped_detected_t;

typedef struct {
	unsigned char obj_id; //=0x0C
	unsigned char size; //=0x01
	unsigned char ped_veh_count;
} IS_PACKED ped_veh_count_t;

//From the Battelle spec descriptions, it appears that
//the only movements to be included pertain to active
//phases. That is the only way that min/max remaining
//time would really make sense.
typedef struct {
	unsigned char obj_id; //=0x04
	lane_set_t lane_set;
	current_state_t current_state;
	min_time_remaining_t min_time_remaining;
	max_time_remaining_t max_time_remaining;
	yellow_state_t yellow_state;
	yellow_time_t yellow_time;
	ped_detected_t ped_detected;
	ped_veh_count_t ped_veh_count;
} IS_PACKED movement_t;

typedef struct {
	unsigned char intersection_id; 		//=0x01
	unsigned char intersection_id_size;	//=0x04
	message_timestamp_t message_timestamp;
	movement_t movement[MAX_PHASES];			//
	unsigned char end_of_blob;		//=0xFF
} IS_PACKED battelle_spat_t;

typedef struct {
	unsigned char	obj_id; //=0x01
	unsigned char	size;	//=0x08
	unsigned char	phase_sequence[8]; //0=skip?
} IS_PACKED phase_sequence_t;

typedef struct {
	unsigned char	obj_id; //=0x02
	unsigned char	size;	//=0x10
	unsigned char	phase_duration[8]; //0=skip?
} IS_PACKED phase_duration_t;

#define	SIGNAL_SCHED_MSG	0x22
typedef struct {
	unsigned short		internal_msg_header; //=0xFFFF
	unsigned char		sig_sche_msg_id; //=0x22
	unsigned int		ms_since_midnight;
	phase_sequence_t	phase_sequence; //Phase sequence is in the order:
						//{R1L, R2L, R1l, R2l, R1L, R2L, R1l, R2l}
						//where R#=ring number, L=lead, l=lag
	phase_duration_t	phase_duration;
} IS_PACKED mschedule_t;

typedef struct {
	int message_type;	//Set to 0x1234
	unsigned char phase;	//phase
	int max_green;
	int min_green;
	int yellow;
	int all_red;
	cell_addr_data_t cell_addr_data;	// Determines parameter to change
} IS_PACKED db_2070_timing_set_t;


typedef struct {
	int message_type;	//Set to 0x1235
	unsigned char phase;	// phase
	unsigned short page;	// timing settings=0x100, local plan settings=0x300
} IS_PACKED db_2070_timing_get_t;

#endif
