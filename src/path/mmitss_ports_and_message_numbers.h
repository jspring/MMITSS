#pragma	once

#define PRIO_REQ_OUTPORT		56000
#define PRIO_REQ_INPORT			56001
#define TRAJ_AWARE_OUTPORT		56002
#define TRAJ_AWARE_INPORT		56003
#define TRAF_CTL_OUTPORT		56004
#define TRAF_CTL_INPORT			56005
#define NOMADIC_TRAJ_AWARE_OUTPORT	56006
#define NOMADIC_TRAJ_AWARE_INPORT	56007
#define TRAF_CTL_IFACE_OUTPORT		56008
#define TRAF_CTL_IFACE_INPORT		56009
#define SPAT_MAP_BCAST_OUTPORT		56010
#define SPAT_MAP_BCAST_INPORT		56011
#define PERF_OBSRV_OUTPORT		56012
#define PERF_OBSRV_INPORT		56013
#define NOMADIC_DEV_OUTPORT		56014
#define NOMADIC_DEV_INPORT		56015

#define MSG_ID_SIG_PLAN_POLL		0x11
#define MSG_ID_SIG_PLAN			0x02
#define MSG_ID_SPAT_POLL		'1'
#define MSG_ID_SPAT			13

// The MMITSS message header convention is:
//   Header     InternalMsgHeader -- two bytes (0xFFFF)
//   msgID      MessageID -- one byte(0x01)
//   timeStamp  CurEpochTime --four bytes (0.01s)
typedef struct {
	unsigned short	InternalMsgHeader;
	unsigned char	msgid;
	unsigned int	cur_epoch_time;
} IS_PACKED mmitss_msg_hdr_t;
