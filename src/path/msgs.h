/* FILE:  msgs.h  Header file for AB3418 format messages
 *
 * Copyright (c) 2012  Regents of the University of California
 *
 */

#ifndef MSGS_H
#define MSGS_H
#define DB_AB3418_CMD_TYPE	2222
#define DB_AB3418_CMD_VAR	DB_AB3418_CMD_TYPE

typedef struct {

	char	start_flag;	/* 0x7e */
	char	address;	/* 0x05 2070 controller */
	char	control;	/* 0x13 - unnumbered information, individual address */
	char	ipi;		/* 0xc0 - NTCIP Class B Protocol */
} ab3418_hdr_t;

typedef struct {

	ab3418_hdr_t ab3428_hdr;
	unsigned char msg_request;
	

} ab3418_frame_t;

/* Following is format for GetLongStatus8 message. */

/* Message sent from RSU to 2070 controller
 * has the following format. */
typedef struct
{
	char      start_flag;   /* 0x7e */
	char      address;      /* 0x05 2070 controller */
	char      control;      /* 0x13 - unnumbered information, individual address */
	char      ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char      mess_type;    /* 0x84 - get short status */
	char      FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
	char      FCSlsb;       /* FCS least significant byte */
	char      end_flag;     /* 0x7e */
} IS_PACKED get_short_status_request_t;

/* Message sent from 2070 to RSU has the following
 * format. */
typedef struct
{
	char          start_flag;   /* 0x7e */
	char          address;      /* 0x05 Requester PC */
	char          control;      /* 0x13 - unnumbered information, individual address */
	char          ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char          mess_type;    /* 0xc4 - get short status response */
	unsigned char greens;
	unsigned char status;       /* Bit 7 = critical alarm; bit 6 = non-critical alarm;
	                             * bit 5 = detector fault; bit 4 = coordination alarm;
	                             * bit 3 = local override; bit 2 = passed local zero;
	                             * bit 1 = cabinet flash; bit 0 = preempt. */
	unsigned char pattern;      /* Pattern (0-250, 251-253 reserved, 254 flash, 255 free) */
	unsigned char FCSmsb;        /* FCS (Frame Checking Sequence) MSB */
	unsigned char FCSlsb;        /* FCS least significant byte */
	char          end_flag;      /* 0x7e */
} IS_PACKED get_short_status_resp_t;

typedef struct
{
	char      start_flag;   /* 0x7e */
	char      address;      /* 0x05 2070 controller */
	char      control;      /* 0x13 - unnumbered information, individual address */
	char      ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char      mess_type;    /* 0x8c - get long status8 */
	char      FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
	char      FCSlsb;       /* FCS least significant byte */
	char      end_flag;     /* 0x7e */
} IS_PACKED get_long_status8_mess_typ;

/* Message sent from 2070 to RSU has the following
 * format. */
typedef struct
{
	char          start_flag;   /* 0x7e */
	char          address;      /* Byte 2 0x05 Requester PC */
	char          control;      /* Byte 3 0x13 - unnumbered information, individual address */
	char          ipi;          /* Byte 4 0xc0 - NTCIP Class B Protocol */
	char          mess_type;    /* Byte 5 0xcc - get long status8 response */
	unsigned char flags;        /* Byte 6 Additional flags; bit 0:focus (default 0 = no focus),
	                             * bits 1-7: reserved, unused. */
	unsigned char status;       /* Byte 7 Bit 7 = critical alarm; bit 6 = non-critical alarm;
	                             * bit 5 = detector fault; bit 4 = coordination alarm;
	                             * bit 3 = local override; bit 2 = passed local zero;
	                             * bit 1 = cabinet flash; bit 0 = preempt. */
	unsigned char pattern;      /* Byte 8 Pattern (0-250, 251-253 reserved, 254 flash, 255 free) */
	unsigned char green_yellow_overlap; /* Byte 9 Bits 0-3 green overlaps A-D;
	                              * bits 4-7 yellow overlaps A-D */
	unsigned char preemption;    /* Byte 10 Bits 0-1 EV A-D; bits 4-6 RR 1-2;
	                              * bit 6 = pattern transition; bit 7 unused */
	unsigned char phase_call;    /* Byte 11 Phase call 1-8; (bit 7 = phase 8; bit 0 = phase 1) */
	unsigned char ped_call;      /* Byte 12 Ped call 1-8; (bit 7 = ped 8; bit 0 = ped 1) */
	unsigned char active_phase;  /* Byte 13 Bits 0-7 -> phases 1-8; bit set true for phase active */
	unsigned char interval;      /* Byte 14 Bits 0-3: ring 0 interval; bits 4-7: ring 1 interval.
	                              * Interval encoding is as follows:
                                      * 0X00 = walk, 0x01 = don't walk, 0x02 = min green,
	                              * 0x03 = unused, 0x04 = added initial, 0x05 = passage -resting,
	                              * 0x06 = max gap, 0x07 = min gap, 0x08 = red rest,
	                              * 0x09 = preemption, 0x0a = stop time, 0x0b = red revert,
	                              * 0x0c = max termination, 0x0d = gap termination,
	                              * 0x0e = force off, 0x0f = red clearance */
	unsigned char presence1;     /* Byte 15 Bits 0-7: detector 1-8. Presence bits set true for
	                              * positive presence. */
	unsigned char presence2;     /* Byte 16 Bits 0-7: detector 9-16 */
	unsigned char presence3;     /* Byte 17 Bits 0-7: detector 17-24 */
	unsigned char presence4;     /* Byte 18 Bits 0-3: detector 25-28, bits 4-7 unused */
	unsigned char master_clock;  /* Byte 19 Master background cycle clock.  Counts up to cycle length */
	unsigned char local_clock;   /* Byte 20 Local cycle clock.  Counts up to cycle length. */
	unsigned char seq_number;    /* Byte 21 Sample sequence number */
	unsigned char volume1;       /* Byte 22 System detector 1 */
	unsigned char occupancy1;    /* Byte 23 System detector 1.  Value 0-200 = detector occupancy in
	                              * 0.5% increments, 201-209 = reserved, 210 = stuck ON fault,
	                              * 211 = stuck OFF fault, 212 = open loop fault,
	                              * 213 = shorted loop fault, 214 = excessive inductance fault,
	                              * 215 = overcount fault. */
	unsigned char volume2;       /* Byte 24 System detector 2 */
	unsigned char occupancy2;    /* Byte 25 system detector 2 */
	unsigned char volume3;       /* Byte 26 System detector 3 */
	unsigned char occupancy3;    /* Byte 27 system detector 3 */
	unsigned char volume4;       /* Byte 28 System detector 4 */
	unsigned char occupancy4;    /* Byte 29 system detector 4 */
	unsigned char volume5;       /* Byte 30 System detector 5 */
	unsigned char occupancy5;    /* Byte 31 system detector 5 */
	unsigned char volume6;       /* Byte 32 System detector 6 */
	unsigned char occupancy6;    /* Byte 33 system detector 6 */
	unsigned char volume7;       /* Byte 34 System detector 7 */
	unsigned char occupancy7;    /* Byte 35 system detector 7 */
	unsigned char volume8;       /* Byte 36 System detector 8 */
	unsigned char occupancy8;    /* Byte 37 system detector 8 */
	unsigned char FCSmsb;        /* Byte 38 FCS (Frame Checking Sequence) MSB */
	unsigned char FCSlsb;        /* Byte 39 FCS least significant byte */
	char          end_flag;      /* Byte 40 0x7e */
} IS_PACKED get_long_status8_resp_mess_typ;

typedef struct {
	unsigned short cell_addr;
	unsigned char data;
} IS_PACKED cell_addr_data_t;

/* Message sent from RSU to 2070 controller
 * has the following format. */
typedef struct
{
	char	start_flag;   /* 0x7e */
	char	address;      /* 0x05 2070 controller */
	char	control;      /* 0x13 - unnumbered information, individual address */
	char	ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char	mess_type;    /* 0x99 - set controller timing data*/
	char	num_cells;    /* 0x02 - number of cells to set */
        cell_addr_data_t magic_num;     // Magic number (=0x0192) that James 
                                        // Lau gave me. I still don't know what
                                        // it means, but it's necessary to get 
                                        // the parameters to change.
	cell_addr_data_t cell_addr_data; // Allow 1 parameter change at a time 
	char	FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
	char	FCSlsb;       /* FCS least significant byte */
	char	end_flag;     /* 0x7e */
} IS_PACKED set_controller_timing_data_t;

/* Message sent from RSU to 2070 controller
 * has the following format. */
typedef struct
{
	char	start_flag;   /* 0x7e */
	char	address;      /* 0x05 2070 controller */
	char	control;      /* 0x13 - unnumbered information, individual address */
	char	ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char	mess_type;    /* 0x89 - get controller timing data*/
	unsigned short offset;    /* Start address - 0x110 for timing settings, 0x310 for local plan settings*/
	char	num_bytes;    /* Number of bytes to get; 32 is the max, set to 16 for timing or plan settings for a single phase  */
	char	FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
	char	FCSlsb;       /* FCS least significant byte */
	char	end_flag;     /* 0x7e */
} IS_PACKED get_controller_timing_data_request_t;

typedef struct
{
	char	start_flag;   /* 0x7e */
	char	address;      /* 0x05 2070 controller */
	char	control;      /* 0x13 - unnumbered information, individual address */
	char	ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char	mess_type;    /* 0x89 - get controller timing data*/
	char	page;         /* Page number */
	char	block;        /* Block number */
	char	FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
	char	FCSlsb;       /* FCS least significant byte */
	char	end_flag;     /* 0x7e */
} IS_PACKED get_block_request_t;

/* Message sent from 2070 to RSU
 * has the following format. */
typedef struct
{
	char	start_flag;   /* 0x7e */
	char	address;      /* 0x05 2070 controller */
	char	control;      /* 0x13 - unnumbered information, individual address */
	char	ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char	mess_type;    /* 0xc9 - controller timing data response code*/
	unsigned short offset;    /* Start address - 0x110 for timing settings, 0x310 for local plan settings*/
	char	num_bytes;    /* Number of bytes received ; 32 is the max */
	char	data[16];     /* Data */
	char	FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
	char	FCSlsb;       /* FCS least significant byte */
	char	end_flag;     /* 0x7e */
} IS_PACKED get_controller_timing_data_response_t;

typedef struct
{
	/* Set for maximum length of expanded message after converting
	 * any 0x7e to the combo 0x7d, 0x5e. */
	unsigned char    data[200];
} gen_mess_typ;

typedef struct
{
	char	start_flag;	/* 0x7e */
	char	address;	/* 0x05 2070 controller */
	char	control;	/* 0x13 - unnumbered information, individual address */
	char	ipi;		/* 0xc0 - NTCIP Class B Protocol */
	char	mess_type;	/* 0x92 - set time request */
	char	day_of_week;	/* Day of week (1-7) 1=Sunday */	
	char	month;		/* Month (1-12) 1=January */
	char	day_of_month;	/* Day of month (1-31) */
	char	year;		/* Last two digits of year (00-99) 95=1995, 0=2000, 94=2094 */
	char 	hour;		/* Hour (0-23) */
	char 	minute;		/* Minute (0-59) */
	char 	second;		/* Second (0-59) */
	char 	tenths;		/* Tenth second (0-9) */
	unsigned char FCSmsb;        /* FCS (Frame Checking Sequence) MSB */
	unsigned char FCSlsb;        /* FCS least significant byte */
	char          end_flag;      /* 0x7e */
} IS_PACKED set_time_t;


/*************************************************************************************************
TSMSS Support for TSCP

05/20/2013 - JAS

TSMSS GET request, GET response, and SET request messages for different types of data, all have
the same message code - 0x87=GET request, 0xC7=GET response, 0x96=SET request. The payload 
type is differentiated by the page and block IDs.
*************************************************************************************************/
typedef struct {
	char	start_flag;   /* 0x7e */
	char	address;      /* 0x05 2070 controller drop address (0x05 = address #1)*/
	char	control;      /* 0x13 - unnumbered information, individual address */
	char	ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char	mess_type;    /* 0x87=GET request, 0xC7=GET response, 0x96=SET request overlap flags code*/
	char	page_id;	// 0x02 - overlap Page ID
	char	block_id;	// 0x04 - overlap Block ID
} IS_PACKED tsmss_get_set_hdr_t;

typedef struct {
	unsigned char FCSmsb;        /* FCS (Frame Checking Sequence) MSB */
	unsigned char FCSlsb;        /* FCS least significant byte */
	char          end_flag;      /* 0x7e */
} IS_PACKED tsmss_get_set_tail_t;

typedef struct {
	tsmss_get_set_hdr_t get_hdr; //get_hdr.mess_type = 0x87, page & block ID - see manual
	tsmss_get_set_tail_t get_tail;
} IS_PACKED tsmss_get_msg_request_t;

typedef struct {
	tsmss_get_set_hdr_t special_flags_hdr ; // special_flags_hdr.mess_type=0xC7 GET response
					 // special_flags_hdr.mess_type=0x96 SET request
					 // special_flags_hdr.page_id=0x02
					 // special_flags_hdr.block_id=0x02

	unsigned char call_to_phase_1;	// Bits 0-7 <-> phases 1-8
	unsigned char call_to_phase_2;	// Bits 0-7 <-> phases 1-8
	unsigned char call_to_phase_3;	// Bits 0-7 <-> phases 1-8
	unsigned char call_to_phase_4;	// Bits 0-7 <-> phases 1-8
	unsigned char call_to_phase_5;	// Bits 0-7 <-> phases 1-8
	unsigned char call_to_phase_6;	// Bits 0-7 <-> phases 1-8
	unsigned char call_to_phase_7;	// Bits 0-7 <-> phases 1-8
	unsigned char call_to_phase_8;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_1;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_2;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_3;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_4;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_5;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_6;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_7;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_8;	// Bits 0-7 <-> phases 1-8
	unsigned char yellow_flash_phases; // Bits 0-7 <-> phases 1-8
	unsigned char yellow_flash_overlaps; // Bits 0-7 <-> overlaps A-F
	unsigned char flash_in_red_phases; // Bits 0-7 <-> phases 1-8
	unsigned char flash_in_red_overlaps; // Bits 0-7 <-> overlapss A-F
	unsigned char single_exit_phases;	// Bits 0-7 <-> phases 1-8
	unsigned char driveway_signal_phases;	// Bits 0-7 <-> phases 1-8
	unsigned char driveway_signal_overlaps;	// Bits 0-7 <-> overlaps A-F
	unsigned char leading_ped_phases;	// Bits 0-7 <-> phases 1-8
	unsigned char protected_permissive;	//
	unsigned char cabinet_type;		//
	unsigned char cabinet_config;		//

	tsmss_get_set_tail_t special_flags_tail;

} IS_PACKED get_set_special_flags_t;

typedef struct {
	tsmss_get_set_hdr_t overlap_hdr; // overlap_hdr.mess_type=0xC7 GET response
					 // overlap_hdr.mess_type=0x96 SET request
					 // overlap_hdr.page_id=0x02
					 // overlap_hdr.block_id=0x04
	char	overlapA_parent;  //Overlap A – On With Phases bits 0-7 = phases 1-8
	char	overlapA_omit;	  //Overlap A – Omit Phases bits 0-7 = phases 1-8
	char	overlapA_nostart; //Overlap A – No Start Phases bits 0-7 = phases 1-8
	char	overlapA_not;	  //Overlap A – Not On With Phases bits 0-7 = phases 1-8
	char	overlapB_parent;  //Overlap B – On With Phases bits 0-7 = phases 1-8
	char	overlapB_omit;	  //Overlap B – Omit Phases bits 0-7 = phases 1-8
	char	overlapB_nostart; //Overlap B – No Start Phases bits 0-7 = phases 1-8
	char	overlapB_not;	  //Overlap B – Not On With Phases bits 0-7 = phases 1-8
	char	overlapC_parent;  //Overlap C – On With Phases bits 0-7 = phases 1-8
	char	overlapC_omit;	  //Overlap C – Omit Phases bits 0-7 = phases 1-8
	char	overlapC_nostart; //Overlap C – No Start Phases bits 0-7 = phases 1-8
	char	overlapC_not;	  //Overlap C – Not On With Phases bits 0-7 = phases 1-8
	char	overlapD_parent;  //Overlap D – On With Phases bits 0-7 = phases 1-8
	char	overlapD_omit;	  //Overlap D – Omit Phases bits 0-7 = phases 1-8
	char	overlapD_nostart; //Overlap D – No Start Phases bits 0-7 = phases 1-8
	char	overlapD_not;	  //Overlap D – Not On With Phases bits 0-7 = phases 1-8
	char	overlapE_parent;  //Overlap E – On With Phases bits 0-7 = phases 1-8
	char	overlapE_omit;	  //Overlap E – Omit Phases bits 0-7 = phases 1-8
	char	overlapE_nostart; //Overlap E – No Start Phases bits 0-7 = phases 1-8
	char	overlapE_not;	  //Overlap E – Not On With Phases bits 0-7 = phases 1-8
	char	overlapF_parent;  //Overlap F – On With Phases bits 0-7 = phases 1-8
	char	overlapF_omit;	  //Overlap F – Omit Phases bits 0-7 = phases 1-8
	char	overlapF_nostart; //Overlap F – No Start Phases bits 0-7 = phases 1-8
	char	overlapF_not;	  //Overlap F – Not On With Phases bits 0-7 = phases 1-8
	tsmss_get_set_tail_t overlap_tail;
} IS_PACKED overlap_msg_t;

typedef struct {
	//	Detector Attributes = Det Attrib
	//	Detector Configuration = Det Config
	//	X = detector 1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 41
	//	X+1 = detector 2, 6, 10, 14, 18, 22, 26, 30, 34, 38, 42
	//	X+2 = detector 3, 7, 11, 15, 19, 23, 27, 31, 35, 39, 43
	//	X+3 = detector 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44

    char det_type; //Byte 8 Det X Attrib – Detector Type
	//	0 = None
	//	1 = Count
	//	2 = Call
	//	3 = Extend
	//	4 = Count + Call
	//	5 = Call + Extend
	//	6 = Count + Call + Extend
	//	7 = Count + Extend
	//	8 = Limited
	//	9 = Bicycle
	//	10 = Pedestrian

    char phase_assignment; //Byte 9 Det X Attrib – Phases Assignment
	//	Phases that are assigned to the detector
	//	Bits 0-7 <-> phases 1-8

    char lock;	//Byte 10 Det X Attrib – Lock
	//	0 = No
	//	1 = Red
	//	2 = Yellow

    char delay_time;	//Byte 11 Det X Attrib – Delay Time 0-255
    char extend_time;	//Byte 11 Det X Attrib – Extend Time (0.1 sec) 0-255
    char recall_time;	//Byte 11 Det X Attrib – Recall Time 0-255
    char input_port;	//Byte 11 Det X Attrib – Input Port (0x13 = 1.3)

} IS_PACKED detector_attr_t;

typedef struct {
    tsmss_get_set_hdr_t detector_hdr; 
	// detector_hdr.mess_type=0xC7 GET response
	// detector_hdr.mess_type=0x96 SET request
	// detector_hdr.page_id=0x07
	// detector_hdr.block_id=0x01=det 1-4
	//	Block 	Block Description	Bytes	Timing Chart 
	//	ID#					Reference
	//	1 	Detector 1 to 4		38
	//	2 	Detector 5 to 8		38
	//	3 	Detector 9 to 12	38
	//	4 	Detector 12 to 16	38
	//	5 	Detector 17 to 20	38
	//	6 	Detector 21 to 24	38
	//	7 	Detector 25 to 28	38
	//	8 	Detector 29 to 32	38
	//	9 	Detector 33 to 36	38
	//	10 	Detector 37 to 40	38
	//	11 	Detector 41 to 44	38
	//	12	Failure Times		34	Failure Times (5-3)
	//		Failure Override		Failure Override (5-4)
	//		System Det. Assignment		System Detectory Assignment (5-5)
	//	13 	CIC Operation		35	CIC Operation (5-6-1)		
	//		CIC Values			CIC Values (5-6-2)
	//		Det.-to-Phase Ass't		Det.-to-Phase Ass't (5-6-3)

    detector_attr_t	detector_attr[4];
    tsmss_get_set_tail_t detector_tail;

} IS_PACKED detector_msg_t;

typedef struct {
  unsigned char start_flag;    // 0x7e
  unsigned char address;       // 0x01 
  unsigned char control;       // 0x13 - unnumbered information, individual address
  unsigned char ipi;           // 0xc0 - NTCIP Class B Protocol 
  unsigned char mess_type;     // 0xCE - raw spat message	
  unsigned char active_phase;  // Bits 1-8 <=> phase 1-8
  unsigned char interval_A;    // interval on ringA phase
  unsigned char interval_B;   // interval on ringB phase
  unsigned char intvA_timer;   // countdown timer for active ringA interval, in 10th sec
  unsigned char intvB_timer;   // countdown timer for active ringB interval, in 10th sec
  unsigned char next_phase;    // Bits 1-8 <==> phase 1 - 8 (same format as active phase)
  unsigned char ped_call;      // Bits 1-8 <=> Pedestrian call on phase 1-8
  unsigned char veh_call;      // Bits 1-8 <=> vehicle call on phase 1-8
  unsigned char plan_num;      // control plan
  unsigned char local_cycle_clock;	
  unsigned char master_cycle_clock;	
  unsigned char preempt;       // preemption
  unsigned char permissive[8]; // permissive period for phase 1 to 8
  unsigned char force_off_A;   // force-off point for ringA phase
  unsigned char force_off_B;   // force-off point for ringB phase	
  unsigned char ped_permissive[8]; // ped permissive period for phase 1 to 8
  unsigned char dummy;
  unsigned char FCSmsb;       // FCS (Frame Checking Sequence) MSB 
  unsigned char FCSlsb;       // FCS least significant byte
  unsigned char end_flag;     // 0x7e

} raw_signal_status_msg_t;

typedef union
{
	get_long_status8_mess_typ         get_long_status8_mess;
	get_long_status8_resp_mess_typ    get_long_status8_resp_mess;
	gen_mess_typ                      gen_mess;
	set_controller_timing_data_t      set_controller_timing_data_mess;
	set_time_t			  set_time_mess;
	get_controller_timing_data_request_t get_controller_timing_data_request_mess;
	get_controller_timing_data_response_t get_controller_timing_data_response_mess;
	overlap_msg_t			overlap_msg;
	get_short_status_request_t	get_short_status_resp;
	detector_msg_t			detector_msg_t;

} IS_PACKED mess_union_typ;

int *GetControllerID(void);
int *SetTime(void);
int *SetPattern(void);
int *GetShortStatus(void);
int *GetSystemDetectorData(void);
int *GetStatus8(void);
int *SetLoginAccess(void);
int *SetMasterPolling(void);
int *GetControllerTimingData(void);
int *SetControllerTimingData(void);
int *GetStatus16(void);
int *SetControllerTimingDataOffset(void);
int *GetLongStatus8(void);
int *SetMasterTrafficResponsive(void);



//long msg_struct_ptr_00; 		//0x00
long msg_struct_ptr_01; 		//0x01
long msg_struct_ptr_02; 		//0x02
long msg_struct_ptr_03; 		//0x03
long msg_struct_ptr_04; 		//0x04
long msg_struct_ptr_05; 		//0x05
long msg_struct_ptr_06; 		//0x06
long msg_struct_ptr_07; 		//0x07
long msg_struct_ptr_08; 		//0x08
long msg_struct_ptr_09; 		//0x09
long msg_struct_ptr_0A; 		//0x0A
long msg_struct_ptr_0B; 		//0x0B
long msg_struct_ptr_0C; 		//0x0C
long msg_struct_ptr_0D; 		//0x0D
long msg_struct_ptr_0E; 		//0x0E
long msg_struct_ptr_0F; 		//0x0F
long msg_struct_ptr_10; 		//0x10
long msg_struct_ptr_11; 		//0x11
long msg_struct_ptr_12; 		//0x12
long msg_struct_ptr_13; 		//0x13
long msg_struct_ptr_14; 		//0x14
long msg_struct_ptr_15; 		//0x15
long msg_struct_ptr_16; 		//0x16
long msg_struct_ptr_17; 		//0x17
long msg_struct_ptr_18; 		//0x18
long msg_struct_ptr_19; 		//0x19
long msg_struct_ptr_1A; 		//0x1A
long msg_struct_ptr_1B; 		//0x1B
long msg_struct_ptr_1C; 		//0x1C
long msg_struct_ptr_1D; 		//0x1D
long msg_struct_ptr_1E; 		//0x1E
long msg_struct_ptr_1F; 		//0x1F
long msg_struct_ptr_20; 		//0x20
long msg_struct_ptr_21; 		//0x21
long msg_struct_ptr_22; 		//0x22
long msg_struct_ptr_23; 		//0x23
long msg_struct_ptr_24; 		//0x24
long msg_struct_ptr_25; 		//0x25
long msg_struct_ptr_26; 		//0x26
long msg_struct_ptr_27; 		//0x27
long msg_struct_ptr_28; 		//0x28
long msg_struct_ptr_29; 		//0x29
long msg_struct_ptr_2A; 		//0x2A
long msg_struct_ptr_2B; 		//0x2B
long msg_struct_ptr_2C; 		//0x2C
long msg_struct_ptr_2D; 		//0x2D
long msg_struct_ptr_2E; 		//0x2E
long msg_struct_ptr_2F; 		//0x2F
long msg_struct_ptr_30; 		//0x30
long msg_struct_ptr_31; 		//0x31
long msg_struct_ptr_32; 		//0x32
long msg_struct_ptr_33; 		//0x33
long msg_struct_ptr_34; 		//0x34
long msg_struct_ptr_35; 		//0x35
long msg_struct_ptr_36; 		//0x36
long msg_struct_ptr_37; 		//0x37
long msg_struct_ptr_38; 		//0x38
long msg_struct_ptr_39; 		//0x39
long msg_struct_ptr_3A; 		//0x3A
long msg_struct_ptr_3B; 		//0x3B
long msg_struct_ptr_3C; 		//0x3C
long msg_struct_ptr_3D; 		//0x3D
long msg_struct_ptr_3E; 		//0x3E
long msg_struct_ptr_3F; 		//0x3F
long msg_struct_ptr_40; 		//0x40
long msg_struct_ptr_41; 		//0x41
long msg_struct_ptr_42; 		//0x42
long msg_struct_ptr_43; 		//0x43
long msg_struct_ptr_44; 		//0x44
long msg_struct_ptr_45; 		//0x45
long msg_struct_ptr_46; 		//0x46
long msg_struct_ptr_47; 		//0x47
long msg_struct_ptr_48; 		//0x48
long msg_struct_ptr_49; 		//0x49
long msg_struct_ptr_4A; 		//0x4A
long msg_struct_ptr_4B; 		//0x4B
long msg_struct_ptr_4C; 		//0x4C
long msg_struct_ptr_4D; 		//0x4D
long msg_struct_ptr_4E; 		//0x4E
long msg_struct_ptr_4F; 		//0x4F
long msg_struct_ptr_50; 		//0x50
long msg_struct_ptr_51; 		//0x51
long msg_struct_ptr_52; 		//0x52
long msg_struct_ptr_53; 		//0x53
long msg_struct_ptr_54; 		//0x54
long msg_struct_ptr_55; 		//0x55
long msg_struct_ptr_56; 		//0x56
long msg_struct_ptr_57; 		//0x57
long msg_struct_ptr_58; 		//0x58
long msg_struct_ptr_59; 		//0x59
long msg_struct_ptr_5A; 		//0x5A
long msg_struct_ptr_5B; 		//0x5B
long msg_struct_ptr_5C; 		//0x5C
long msg_struct_ptr_5D; 		//0x5D
long msg_struct_ptr_5E; 		//0x5E
long msg_struct_ptr_5F; 		//0x5F
long msg_struct_ptr_60; 		//0x60
long msg_struct_ptr_61; 		//0x61
long msg_struct_ptr_62; 		//0x62
long msg_struct_ptr_63; 		//0x63
long msg_struct_ptr_64; 		//0x64
long msg_struct_ptr_65; 		//0x65
long msg_struct_ptr_66; 		//0x66
long msg_struct_ptr_67; 		//0x67
long msg_struct_ptr_68; 		//0x68
long msg_struct_ptr_69; 		//0x69
long msg_struct_ptr_6A; 		//0x6A
long msg_struct_ptr_6B; 		//0x6B
long msg_struct_ptr_6C; 		//0x6C
long msg_struct_ptr_6D; 		//0x6D
long msg_struct_ptr_6E; 		//0x6E
long msg_struct_ptr_6F; 		//0x6F
long msg_struct_ptr_70; 		//0x70
long msg_struct_ptr_71; 		//0x71
long msg_struct_ptr_72; 		//0x72
long msg_struct_ptr_73; 		//0x73
long msg_struct_ptr_74; 		//0x74
long msg_struct_ptr_75; 		//0x75
long msg_struct_ptr_76; 		//0x76
long msg_struct_ptr_77; 		//0x77
long msg_struct_ptr_78; 		//0x78
long msg_struct_ptr_79; 		//0x79
long msg_struct_ptr_7A; 		//0x7A
long msg_struct_ptr_7B; 		//0x7B
long msg_struct_ptr_7C; 		//0x7C
long msg_struct_ptr_7D; 		//0x7D
long msg_struct_ptr_7E; 		//0x7E
long msg_struct_ptr_7F; 		//0x7F
long msg_struct_ptr_80; 		//0x80
long msg_struct_ptr_81; 		//0x81
long msg_struct_ptr_82; 		//0x82
long msg_struct_ptr_83; 		//0x83
long msg_struct_ptr_84; 		//0x84
long msg_struct_ptr_85; 		//0x85
long msg_struct_ptr_86; 		//0x86
long msg_struct_ptr_87; 		//0x87
long msg_struct_ptr_88; 		//0x88
long msg_struct_ptr_89; 		//0x89
long msg_struct_ptr_8A; 		//0x8A
long msg_struct_ptr_8B; 		//0x8B
long msg_struct_ptr_8C; 		//0x8C
long msg_struct_ptr_8D; 		//0x8D
long msg_struct_ptr_8E; 		//0x8E
long msg_struct_ptr_8F; 		//0x8F

typedef struct{
	int	size;
	void	*struct_ptr;
} IS_PACKED mmitss_msg_struct_ptr_t;

// Replace the first element with the size of the message and
// the second with a pointer to the message declaration
/*
mmitss_msg_struct_ptr_t mmitss_msg[]={
	{sizeof(int), 0}, 		//0x00
	{sizeof(int), 0}, 		//0x01
	{sizeof(int), 0}, 		//0x02
	{sizeof(int), 0}, 		//0x03
	{sizeof(int), 0}, 		//0x04
	{sizeof(int), 0}, 		//0x05
	{sizeof(int), 0}, 		//0x06
	{sizeof(int), 0}, 		//0x07
	{sizeof(int), 0}, 		//0x08
	{sizeof(int), 0}, 		//0x09
	{sizeof(int), 0}, 		//0x0A
	{sizeof(int), 0}, 		//0x0B
	{sizeof(int), 0}, 		//0x0C
	{sizeof(int), 0}, 		//0x0D
	{sizeof(int), 0}, 		//0x0E
	{sizeof(int), 0}, 		//0x0F
	{sizeof(raw_signal_status_msg_t), 0}, 		//0x10
	{sizeof(int), 0}, 		//0x11
	{sizeof(int), 0}, 		//0x12
	{sizeof(int), 0}, 		//0x13
	{sizeof(int), 0}, 		//0x14
	{sizeof(int), 0}, 		//0x15
	{sizeof(int), 0}, 		//0x16
	{sizeof(int), 0}, 		//0x17
	{sizeof(int), 0}, 		//0x18
	{sizeof(int), 0}, 		//0x19
	{sizeof(int), 0}, 		//0x1A
	{sizeof(int), 0}, 		//0x1B
	{sizeof(int), 0}, 		//0x1C
	{sizeof(int), 0}, 		//0x1D
	{sizeof(int), 0}, 		//0x1E
	{sizeof(int), 0}, 		//0x1F
	{sizeof(int), 0}, 		//0x20
	{sizeof(int), 0}, 		//0x21
	{sizeof(int), 0}, 		//0x22
	{sizeof(int), 0}, 		//0x23
	{sizeof(int), 0}, 		//0x24
	{sizeof(int), 0}, 		//0x25
	{sizeof(int), 0}, 		//0x26
	{sizeof(int), 0}, 		//0x27
	{sizeof(int), 0}, 		//0x28
	{sizeof(int), 0}, 		//0x29
	{sizeof(int), 0}, 		//0x2A
	{sizeof(int), 0}, 		//0x2B
	{sizeof(int), 0}, 		//0x2C
	{sizeof(int), 0}, 		//0x2D
	{sizeof(int), 0}, 		//0x2E
	{sizeof(int), 0}, 		//0x2F
	{sizeof(int), 0}, 		//0x30
	{sizeof(int), 0}, 		//0x31
	{sizeof(int), 0}, 		//0x32
	{sizeof(int), 0}, 		//0x33
	{sizeof(int), 0}, 		//0x34
	{sizeof(int), 0}, 		//0x35
	{sizeof(int), 0}, 		//0x36
	{sizeof(int), 0}, 		//0x37
	{sizeof(int), 0}, 		//0x38
	{sizeof(int), 0}, 		//0x39
	{sizeof(int), 0}, 		//0x3A
	{sizeof(int), 0}, 		//0x3B
	{sizeof(int), 0}, 		//0x3C
	{sizeof(int), 0}, 		//0x3D
	{sizeof(int), 0}, 		//0x3E
	{sizeof(int), 0}, 		//0x3F
	{sizeof(int), 0}, 		//0x40
	{sizeof(int), 0}, 		//0x41
	{sizeof(int), 0}, 		//0x42
	{sizeof(int), 0}, 		//0x43
	{sizeof(int), 0}, 		//0x44
	{sizeof(int), 0}, 		//0x45
	{sizeof(int), 0}, 		//0x46
	{sizeof(int), 0}, 		//0x47
	{sizeof(int), 0}, 		//0x48
	{sizeof(int), 0}, 		//0x49
	{sizeof(int), 0}, 		//0x4A
	{sizeof(int), 0}, 		//0x4B
	{sizeof(int), 0}, 		//0x4C
	{sizeof(int), 0}, 		//0x4D
	{sizeof(int), 0}, 		//0x4E
	{sizeof(int), 0}, 		//0x4F
	{sizeof(int), 0}, 		//0x50
	{sizeof(int), 0}, 		//0x51
	{sizeof(int), 0}, 		//0x52
	{sizeof(int), 0}, 		//0x53
	{sizeof(int), 0}, 		//0x54
	{sizeof(int), 0}, 		//0x55
	{sizeof(int), 0}, 		//0x56
	{sizeof(int), 0}, 		//0x57
	{sizeof(int), 0}, 		//0x58
	{sizeof(int), 0}, 		//0x59
	{sizeof(int), 0}, 		//0x5A
	{sizeof(int), 0}, 		//0x5B
	{sizeof(int), 0}, 		//0x5C
	{sizeof(int), 0}, 		//0x5D
	{sizeof(int), 0}, 		//0x5E
	{sizeof(int), 0}, 		//0x5F
	{sizeof(int), 0}, 		//0x60
	{sizeof(int), 0}, 		//0x61
	{sizeof(int), 0}, 		//0x62
	{sizeof(int), 0}, 		//0x63
	{sizeof(int), 0}, 		//0x64
	{sizeof(int), 0}, 		//0x65
	{sizeof(int), 0}, 		//0x66
	{sizeof(int), 0}, 		//0x67
	{sizeof(int), 0}, 		//0x68
	{sizeof(int), 0}, 		//0x69
	{sizeof(int), 0}, 		//0x6A
	{sizeof(int), 0}, 		//0x6B
	{sizeof(int), 0}, 		//0x6C
	{sizeof(int), 0}, 		//0x6D
	{sizeof(int), 0}, 		//0x6E
	{sizeof(int), 0}, 		//0x6F
	{sizeof(int), 0}, 		//0x70
	{sizeof(int), 0}, 		//0x71
	{sizeof(int), 0}, 		//0x72
	{sizeof(int), 0}, 		//0x73
	{sizeof(int), 0}, 		//0x74
	{sizeof(int), 0}, 		//0x75
	{sizeof(int), 0}, 		//0x76
	{sizeof(int), 0}, 		//0x77
	{sizeof(int), 0}, 		//0x78
	{sizeof(int), 0}, 		//0x79
	{sizeof(int), 0}, 		//0x7A
	{sizeof(int), 0}, 		//0x7B
	{sizeof(int), 0}, 		//0x7C
	{sizeof(int), 0}, 		//0x7D
	{sizeof(int), 0}, 		//0x7E
	{sizeof(int), 0}, 		//0x7F
	{sizeof(int), 0}, 		//0x80
	{sizeof(int), 0}, 		//0x81
	{sizeof(int), 0}, 		//0x82
	{sizeof(int), 0}, 		//0x83
	{sizeof(int), 0}, 		//0x84
	{sizeof(int), 0}, 		//0x85
	{sizeof(int), 0}, 		//0x86
	{sizeof(int), 0}, 		//0x87
	{sizeof(int), 0}, 		//0x88
	{sizeof(int), 0}, 		//0x89
	{sizeof(int), 0}, 		//0x8A
	{sizeof(int), 0}, 		//0x8B
	{sizeof(int), 0}, 		//0x8C
	{sizeof(int), 0}, 		//0x8D
	{sizeof(int), 0}, 		//0x8E
	{sizeof(int), 0}, 		//0x8F
};
*/

#endif
