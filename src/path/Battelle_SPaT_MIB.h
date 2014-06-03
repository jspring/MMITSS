typedef struct {
	unsigned char	phase; 			
	unsigned short	VehMinTimeToChange;	
	unsigned short	VehMaxTimeToChange;
	unsigned short	PedMinTimeToChange;
	unsigned short	PedMaxTimeToChange;
	unsigned short	OvlpMinTimeToChange;
	unsigned short	OvlpMaxTimeToChange;
} IS_PACKED time_to_change_t;

typedef struct {
//byte 0: DynObj13 response byte (0xcd)
	unsigned char	msg_id;
//byte 1: number of phase/overlap blocks below (16) 
	unsigned char	num_phases;
//bytes 2-209:
	time_to_change_t	time_to_change[16];
//bytes 210-215:
	unsigned short	PhaseStatusReds;	//(2 bytes bit-mapped for phases 1-16)
	unsigned short	PhaseStatusYellow;	//(2 bytes bit-mapped for phases 1-16)
	unsigned short	PhaseStatusGreens;	//(2 bytes bit-mapped for phases 1-16)
//bytes 216-221:
	unsigned short	PhaseStatusDontWalks;	//(2 bytes bit-mapped for phases 1-16)
	unsigned short	PhaseStatusPedClears;	//(2 bytes bit-mapped for phases 1-16)
	unsigned short	PhaseStatusWalks;	//(2 bytes bit-mapped for phases 1-16)
//bytes 222-227:
	unsigned short	OverlapStatusReds;	//(2 bytes bit-mapped for overlaps 1-16)
	unsigned short	OverlapStatusYellows;	//(2 bytes bit-mapped for overlaps 1-16)
	unsigned short	OverlapStatusGreens;	//(2 bytes bit-mapped for overlaps 1-16)
//bytes 228-229:
	unsigned short	FlashingOutputPhaseStatus;	//(2 bytes bit-mapped for phases 1-16)
//bytes 230-231:
	unsigned short	FlashingOutputOverlapStatus;	//(2 bytes bit-mapped for overlaps 1-16)
//byte 232:
	unsigned char	IntersectionStatus;		// (1 byte)(bit-coded byte) 
//Byte 233:
	unsigned char	TimebaseAscActionStatus;	// (1 byte)(current action plan)                       
//byte 234:
	unsigned char	DiscontinuousChangeFlag;	// (1 byte)(upper 5 bits are msg version #2, 0b00010XXX)     
//byte 235:
	unsigned char	MessageSequenceCounter;		// (1 byte)(lower byte of up-time deciseconds) 
//Byte 236-238:
	unsigned char	SystemSeconds[3];;		// (3 byte)(sys-clock seconds in day 0-84600)                
//Byte 239-240:
	unsigned short	SystemMilliSeconds;		// (2 byte)(sys-clock milliseconds 0-999)  
//Byte 241-242:
	unsigned short	PedestrianDirectCallStatus;	// (2 byte)(bit-mapped phases 1-16)             
//Byte 243-244:
	unsigned short	PedestrianLatchedCallStatus;	// (2 byte)	(bit-mapped phases 1-16)             
} IS_PACKED spat_ntcip_mib_t;
