#pragma	once

#define PRIO_REQ_OUTPORT		6000
#define PRIO_REQ_INPORT			6001
#define TRAJ_AWARE_OUTPORT		6002
#define TRAJ_AWARE_INPORT		6003
#define NOMADIC_TRAJ_AWARE_OUTPORT	6004
#define NOMADIC_TRAJ_AWARE_INPORT	6005
#define TRAF_CTL_OUTPORT		6006
#define TRAF_CTL_INPORT			6007
 #define MSG_ID_SIG_PLAN_POLL		'0'
 #define MSG_ID_SIG_PLAN		0x02
 #define MSG_ID_SPAT_POLL		'1'
 #define MSG_ID_SPAT			13
#define TRAF_CTL_IFACE_OUTPORT		6008
#define TRAF_CTL_IFACE_INPORT		6009
#define SPAT_BCAST_OUTPORT		6010
#define SPAT_BCAST_INPORT		6011
#define PERF_OBSRV_OUTPORT		6012
#define PERF_OBSRV_INPORT		6013
#define NOMADIC_DEV_OUTPORT		6014
#define NOMADIC_DEV_INPORT		6015

// The MMITSS message header convention is:
//   Header     InternalMsgHeader -- two bytes (0xFFFF)
//   msgID      MessageID -- one byte(0x01)
//   timeStamp  CurEpochTime --four bytes (0.01s)
typedef struct {
	unsigned short	InternalMsgHeader;
	unsigned char	msgid;
	unsigned int	cur_epoch_time;
} IS_PACKED mmitss_msg_hdr_t;
