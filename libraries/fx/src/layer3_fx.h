#ifndef LAYER3_FX_H
#define LAYER3_FX_H

#include <map>
#include <set>

#include "fx_rrc_config.h"
//Added by WLH
#include "fx_common.h"
#include "layer2_fx_smac.h"
#include "layer2_fx_slc.h"
#include "layer2_fx_sac.h"
#define FX_DEFAULT_BEARER_ID (0)

#define RRC_FX_DEFAULT_WAIT_RRC_CONNECTED_TIME (10*MILLI_SECOND)
#define RRC_FX_DEFAULT_WAIT_RRC_CONNECTED_RECONF_TIME (10*MILLI_SECOND)
#define RRC_FX_DEFAULT_RELOC_PREP_TIME                  ( 200*MILLI_SECOND)
#define RRC_FX_DEFAULT_X2_RELOC_OVERALL_TIME            (1000*MILLI_SECOND)
#define RRC_FX_DEFAULT_X2_WAIT_SN_STATUS_TRANSFER_TIME  ( 500*MILLI_SECOND)
#define RRC_FX_DEFAULT_WAIT_ATTCH_UE_BY_HO_TIME         ( 500*MILLI_SECOND)
#define RRC_FX_DEFAULT_X2_WAIT_END_MARKER_TIME          ( 200*MILLI_SECOND)
#define RRC_FX_DEFAULT_S1_WAIT_PATH_SWITCH_REQ_ACK_TIME ( 200*MILLI_SECOND)
//Added by WLH
#define FX_LIB_STATION_TYPE_GUARD(node, interfaceIndex, type, typeString) \
    ERROR_Assert(FxLayer2GetStationType(node, interfaceIndex) == type, \
        "This function should be called from only "typeString".");




typedef enum {
	LAYER3_FX_POWER_OFF,
	LAYER3_FX_POWER_ON,
	LAYER3_FX_RRC_IDLE,
	LAYER3_FX_RRC_CONNECTED,
	LAYER3_FX_STATUS_NUM
} Layer3FxState;


typedef enum {
	LAYER3_FX_CONNECTION_WAITING,
	LAYER3_FX_CONNECTION_CONNECTED,
	LAYER3_FX_CONNECTION_HANDOVER,
	LAYER3_FX_CONNECTION_STATUS_NUM
} Layer3FxConnectionState;


typedef struct struct_fx_radio_bearer
{
	FxSacEntity sacEntity;
	FxSlcEntity slcEntity;

} FxRadioBearer;


typedef std::map < int, FxRadioBearer > MapFxRadioBearer;
typedef std::pair < int, FxRadioBearer > PairFxRadioBearer;



typedef struct struct_fx_connected_info
{
	FxBsrInfo bsrInfo; // using only eNB, a UE connected to this eNB has a bsr info
	MapFxRadioBearer radioBearers; // Radio Bearers of this connection
	clocktype connectedTime; // connectedTime
	BOOL isSchedulable;
} FxConnectedInfo;



typedef struct struct_connection_info_fx
{
	Layer3FxConnectionState state;
	FxConnectedInfo connectedInfo;
	std::map < int, Message* >  mapLayer3Timer;

	// FXHandoverParticipator hoParticipator;    // used ony for handover


	struct_connection_info_fx(Layer3FxConnectionState _state)
		: state(_state)
	{}
} FxConnectionInfo;





typedef std::map < fxRnti, FxConnectionInfo > MapFxConnectionInfo;
typedef std::map < int, set<int>> MapSBCPInfo;

typedef struct struct_layer3_fx_str
{
	Layer3FxState layer3FxState;

	MapFxConnectionInfo  connectionInfoMap;

	clocktype waitRrcConnectedTime;
	clocktype waitRrcConnectedReconfTime; 
	MapSBCPInfo SBCPMap;

	// 用户：切换表<切换时间，目标波束>
	map<clocktype, UInt8>* handoverSchedule;

} Layer3DataFx;




FxConnectionInfo* Layer3FxGetConnectionInfo(
	Node* node, int interfaceIndex, const fxRnti& rnti);

void Layer3FxProcessSwitchBeamInfoAtXG(Node * node, int iface, const fxRnti & oppositeRnti, UInt8 currentBeam, UInt8 targetBeam);

void Layer3FxProcessHandoverRequestAtXG(Node * node, int iface, const fxRnti & yhRnti);

void Layer3FxProcessRrcReconfigurationAtYH(Node * node, int iface, const fxRnti & xgRnti);

void Layer3FxPowerOn(Node* node, int interfaceIndex);



void FxLayer3Init(Node* node,
	UInt32 interfaceIndex,
	const NodeInput* nodeInput);

void Layer3FxAddConnectionInfo(
	Node* node, int interfaceIndex, const fxRnti& rnti,
	Layer3FxConnectionState state);


const fxRnti Layer3FxGetRandomAccessingXG(Node* node, int iface);

void Layer3FxAddBeam(Node * node, int iface, int beamIndex);

void Layer3FxAddNodetoBeam(Node * node, int iface, int NodeId, int beamIndex);

int Layer3FxGetBeamIndexofYHAtXG(Node * node, int iface, int NodeId);

void Layer3FxProcessRrcConnected(Node* node, int iface, const fxRnti & oppositeRnti);

void Layer3FxChangeBeamIndexAtYH(Node * node, int iface, int targetBeamIndex);

void Layer3FxProcessHandoverCompleteAtXG(Node * node, int iface, const fxRnti & yhRnti);

void FxLayer3Finalize(Node* node,
	UInt32 interfaceIndex);


void Layer3FxSetTimerForRrc(
	Node* node, int interfaceIndex, int eventType,
	clocktype delay);

void Layer3FxProcessEvent(Node* node, int interfaceIndex, Message* msg);

//Added by WLH 2018/11/20
typedef struct struct_rrc_connection_reconfiguration_fx {
	FxRrcConfig rrcConfig;
	struct_rrc_connection_reconfiguration_fx()
	{}
	struct_rrc_connection_reconfiguration_fx(
		const struct_rrc_connection_reconfiguration_fx& other)
	{
		rrcConfig = other.rrcConfig;    // just memberwise copy
	}
} FXRrcConnectionReconfiguration;
void Layer3FxSetTimerForRrc(
	Node* node, int interfaceIndex, const fxRnti& rnti, int eventType,
	clocktype delay);

void Layer3FxCancelTimerForRrc(
	Node* node, int interfaceIndex, const fxRnti& rnti, int eventType);

BOOL Layer3FxCheckTimerForRrc(
	Node* node, int interfaceIndex, const fxRnti& rnti, int eventType);

FxConnectionInfo* Layer3FxGetHandoverInfo(
	Node* node, int interfaceIndex, const fxRnti& rnti);

FxConnectedInfo* Layer3FxGetConnectedInfo(
	Node* node, int interfaceIndex, const fxRnti& rnti);

BOOL Layer3FxAdmissionControl(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator);

void Layer3FxSetFXHandoverParticipator(
	Node* node, int interfaceIndex, const fxRnti& rnti,
	const FXHandoverParticipator& hoParticipator);

void Layer3FxSendRrcConnReconfInclMobilityControlInfomation(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator,
	const FXRrcConnectionReconfiguration& reconf);

void Layer3FxPrepareDataForwarding(
	Node* node, int interfaceIndex,
	const FXHandoverParticipator& hoParticipator);

void Layer3FxStartDataForwarding(Node* node, int interfaceIndex,
	const FXHandoverParticipator& hoParticipator);
void Layer3FxDetachUE(
	Node* node, int interfaceIndex, const fxRnti& oppositeRnti);
void Layer3FxDeleteRoute(
	Node* node, int interfaceIndex, const fxRnti& oppositeRnti);
BOOL Layer3FxRemoveConnectionInfo(
	Node* node, int interfaceIndex, const fxRnti& rnti);
void Layer3FxNotifyLostDetection(
	Node* node, int interfaceIndex, fxRnti oppositeRnti);



void Layer3FxReceiveDataForwarding(
	Node* node,
	int interfaceIndex,
	const FXHandoverParticipator& hoParticipator,
	int bearerId,
	Message* forwardingMsg);

void Layer3FxReceivePathSwitchRequestAck(
	Node* node,
	int interfaceIndex,
	const FXHandoverParticipator& hoParticipator,
	BOOL result);

void Layer3FxReceiveUeContextRelease(
	Node* node,
	int interfaceIndex,
	const FXHandoverParticipator& hoParticipator);

void Layer3FxReceiveHoPreparationFailure(
	Node* node,
	int interfaceIndex,
	const FXHandoverParticipator& hoParticipator);

void Layer3FxReceiveEndMarker(
	Node* node,
	int interfaceIndex,
	const FXHandoverParticipator& hoParticipator);

void Layer3FxReceiveHoReq(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator);

void Layer3FxReceiveHoReqAck(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator,
	const FXRrcConnectionReconfiguration& reconf);

void Layer3FxSendSnStatusTransfer(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator);

void Layer3FxReceiveSnStatusTransfer(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator,
	std::map<int, SacFxSnStatusTransferItem>& snStatusTransferItem);



void Layer3FxInitHandoverSchedule(
	Node* node,
	UInt32 interfaceIndex,
	const NodeInput* nodeInput);

#endif