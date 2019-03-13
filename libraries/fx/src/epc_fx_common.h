#ifndef _EPC_FX_COMMON_H_
#define _EPC_FX_COMMON_H_

#include <set>
#include <map>
#include "fx_common.h"

// /**
// ENUM        :: EpcFxStationType
// DESCRIPTION :: Type of a station in EPC region
// **/
typedef enum
{
	EPC_FX_STATION_TYPE_XG,       // eNodeB
	EPC_FX_STATION_TYPE_SGWMME,    // SGW/MME (not distinguished in PHASE2)
} EpcFxStationType;

// /**
// ENUM        :: EpcFxMessageType
// DESCRIPTION :: Type of EPC message
// **/
typedef enum
{
	EPC_FX_MESSAGE_TYPE_ATTACH_UE,
	EPC_FX_MESSAGE_TYPE_DETACH_UE,
	EPC_FX_MESSAGE_TYPE_HANDOVER_REQUEST,
	EPC_FX_MESSAGE_TYPE_HANDOVER_REQUEST_ACKNOWLEDGE,
	EPC_FX_MESSAGE_TYPE_SN_STATUS_TRANSFER,
	EPC_FX_MESSAGE_TYPE_DATA_FORWARDING,
	EPC_FX_MESSAGE_TYPE_PATH_SWITCH_REQUEST,
	EPC_FX_MESSAGE_TYPE_PATH_SWITCH_REQUEST_ACK,
	EPC_FX_MESSAGE_TYPE_END_MARKER,
	EPC_FX_MESSAGE_TYPE_UE_CONTEXT_RELEASE,
	EPC_FX_MESSAGE_TYPE_HO_PREPARATION_FAILURE,
} EpcFxMessageType;

typedef std::set<fxRnti> EpcFxYhList;
typedef std::map<fxRnti, EpcFxYhList> EpcFxLocationInfo;

// /**
// STRUCT      :: EpcStatData
// DESCRIPTION :: Statistics Data
// **/
typedef struct struct_EPC_FX_stat_data {
	UInt32 numHandoverRequestSent;
	UInt32 numHandoverRequestReceived;
	UInt32 numHandoverRequestAckSent;
	UInt32 numHandoverRequestAckReceived;
	UInt32 numSnStatusTransferSent;
	UInt32 numSnStatusTransferReceived;
	UInt32 numPathSwitchRequestSent;
	UInt32 numPathSwitchRequestReceived;
	UInt32 numPathSwitchRequestAckSent;
	UInt32 numPathSwitchRequestAckReceived;
	UInt32 numEndMarkerSent;
	UInt32 numEndMarkerReceived;
	UInt32 numUeContextReleaseSent;
	UInt32 numUeContextReleaseReceived;
	UInt32 numHandoversCompleted;
	UInt32 numHandoversFailed;
public:
	struct_EPC_FX_stat_data() :
		numHandoverRequestSent(0),
		numHandoverRequestReceived(0),
		numHandoverRequestAckSent(0),
		numHandoverRequestAckReceived(0),
		numSnStatusTransferSent(0),
		numSnStatusTransferReceived(0),
		numPathSwitchRequestSent(0),
		numPathSwitchRequestReceived(0),
		numPathSwitchRequestAckSent(0),
		numPathSwitchRequestAckReceived(0),
		numEndMarkerSent(0),
		numEndMarkerReceived(0),
		numUeContextReleaseSent(0),
		numUeContextReleaseReceived(0),
		numHandoversCompleted(0),
		numHandoversFailed(0)
	{};
} EpcFxStatData;

// /**
// STRUCT      :: EpcData
// DESCRIPTION :: EPC Data
// **/
typedef struct struct_EPC_FX_data
{
	EpcFxStationType type;                 // my type
	int outgoingInterfaceIndex;          // I/F used to send/receive EPC msg
	fxRnti sgwmmeRnti;                  // SGW/MME's RNTI on the subnet
	EpcFxLocationInfo* locationInfo;    // used only on SGWMME
	EpcFxStatData statData;                // for statistics
} EpcFxData;

#endif  // _EPC_FX_COMMON_H_

