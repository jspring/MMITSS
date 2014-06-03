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
	int message_type;	//Set to 0x1234
	unsigned char phase;			// phase
	int max_green;
	int min_green;
	int yellow;
	int all_red;
	cell_addr_data_t cell_addr_data;	// Determines parameter to change
} IS_PACKED db_2070_timing_set_t;


typedef struct {
	int message_type;	//Set to 0x1235
	unsigned char phase;			// phase
	unsigned short page;	// timing settings=0x100, local plan settings=0x300
} IS_PACKED db_2070_timing_get_t;

#endif
