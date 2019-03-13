#ifndef _EPC_FX_APP_H_
#define _EPC_FX_APP_H_

#include <map>
#include "message.h"
#include "fileio.h"
#include "epc_fx_common.h"
#include "layer3_fx.h"
#include "layer3_lte.h"  //gss xd
#include "epc_lte_common.h" //gss xd
//#include "layer2_fx_sac.h"

#define EPC_FX_APP_CAT_EPC_APP    "EpcApp"
#define EPC_FX_APP_CAT_EPC_PROCESS     "EpcProcess"
#define EPC_FX_APP_DELAY               0

// /**
// STRUCT      :: EpcFxAppMessageArgument_AttachUE
// DESCRIPTION :: argument for AttachUE
// **/
typedef struct struct_epc_FX_app_message_argument_attach_ue
{
	fxRnti yhRnti;
	fxRnti xgRnti;
} EpcFxAppMessageArgument_AttachUE;

// /**
// STRUCT      :: EpcFxAppMessageArgument_DetachUE
// DESCRIPTION :: argument for DetachUE
// **/
typedef struct struct_epc_FX_app_message_argument_detach_ue
{
	fxRnti yhRnti;
	fxRnti xgRnti;
} EpcFxAppMessageArgument_DetachUE;

// /**
// STRUCT      :: EpcFxAppMessageArgument_HoReq
// DESCRIPTION :: argument for HoReq
// **/
typedef struct struct_epc_FX_app_message_argument_ho_req
{
	FXHandoverParticipator hoParticipator;
} EpcFxAppMessageArgument_HoReq;

// /**
// STRUCT      :: EpcFxAppMessageArgument_HoReq
// DESCRIPTION :: argument for HoReqAck
// **/
typedef struct struct_epc_FX_app_message_argument_ho_req_ack
{
	FXHandoverParticipator hoParticipator;
	FXRrcConnectionReconfiguration reconf;    // including moblityControlInfo
} EpcFxAppMessageArgument_HoReqAck;

//gss xd
typedef struct struct_epc_Xd_app_message_argument_ho_req
{
	XdHandoverParticipator xdhoParticipator;
} EpcXdAppMessageArgument_HoReq;

//gss xd
typedef struct struct_epc_Xd_app_message_argument_ho_req_ack
{
	XdHandoverParticipator xdhoParticipator;
} EpcXdAppMessageArgument_HoReqAck;


typedef struct struct_epc_FX_app_pair_bearerid_snstatus
{
	int bearerId;
	SacFxSnStatusTransferItem snStatusTransferItem;
} EpcFxAppPairBearerIdSnStatus;

// /**
// STRUCT      :: EpcFxAppMessageArgument_SnStatusTransfer
// DESCRIPTION :: argument for HoReqAck
// **/
typedef struct struct_epc_FX_app_message_argument_sn_status_transfer
{
	FXHandoverParticipator hoParticipator;
	int itemNum;
	EpcFxAppPairBearerIdSnStatus items[1];
} EpcFxAppMessageArgument_SnStatusTransfer;

// /**
// STRUCT      :: EpcFxAppMessageArgument_SnStatusTransfer
// DESCRIPTION :: argument for HoReqAck
// **/
typedef struct struct_epc_FX_app_message_argument_data_forwarding
{
	FXHandoverParticipator hoParticipator;
	int bearerId;
	Message* message;
} EpcFxAppMessageArgument_DataForwarding;

// /**
// STRUCT      :: EpcFxAppMessageArgument_PathSwitchRequest
// DESCRIPTION :: argument for HoReq
// **/
typedef struct struct_epc_FX_app_message_argument_path_switch_req
{
	FXHandoverParticipator hoParticipator;
} EpcFxAppMessageArgument_PathSwitchRequest;

// /**
// STRUCT      :: EpcFxAppMessageArgument_PathSwitchRequestAck
// DESCRIPTION :: argument for HoReqAck
// **/
typedef struct struct_epc_FX_app_message_argument_path_switch_req_ack
{
	FXHandoverParticipator hoParticipator;
	BOOL result;
} EpcFxAppMessageArgument_PathSwitchRequestAck;

// /**
// STRUCT      :: EpcFxAppMessageArgument_EndMarker
// DESCRIPTION :: argument for HoReq
// **/
typedef struct struct_epc_FX_app_message_argument_end_marker
{
	FXHandoverParticipator hoParticipator;
} EpcFxAppMessageArgument_EndMarker;

// /**
// STRUCT      :: EpcFxAppMessageArgument_UeContextRelease
// DESCRIPTION :: argument for HoReq
// **/
typedef struct struct_epc_FX_app_message_argument_ue_context_release
{
	FXHandoverParticipator hoParticipator;
} EpcFxAppMessageArgument_UeContextRelease;

// /**
// STRUCT      :: EpcFxAppMessageArgument_HoPreparationFailure
// DESCRIPTION :: argument for HoReq
// **/
typedef struct struct_epc_FX_app_message_argument_ho_preparation_failure
{
	FXHandoverParticipator hoParticipator;
} EpcFxAppMessageArgument_HoPreparationFailure;

// /**
// STRUCT      :: EpcFxAppMessageContainer
// DESCRIPTION :: common container for EPC message
// **/
typedef struct struct_epc_FX_app_message_container
{
	fxRnti src;
	fxRnti dst;
	EpcFxMessageType type;
	int length;
	char value[1];
} EpcFxAppMessageContainer;


// /**
// FUNCTION   :: EpcFxAppProcessEvent
// LAYER      :: EPC
// PURPOSE    :: process event for EPC
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + msg              : Message* : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void
EpcFxAppProcessEvent(Node *node, Message *msg);

// /**
// FUNCTION   :: EpcFxAppSend
// LAYER      :: EPC
// PURPOSE    :: common sender called by sender of each API
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + src              : const fxRnti& : source of message
// + dst              : const fxRnti& : dest of message
// + type             : EpcMessageType : message type
// + payloadSize      : int            : payload size
// + payload          : char*          : pointer to payload
// + virtualPacketSize: int            : virtual size of packet
//                                        when -1 specified, actual size used
// RETURN     :: void : NULL
// **/
void
EpcFxAppSend(Node* node,
	int interfaceIndex,
	const fxRnti& src,
	const fxRnti& dst,
	EpcFxMessageType type,
	int payloadSize,
	char *payload,
	int virtualPacketSize = -1);

//gss2019  xd
//发送星地切换请求确认时调用
void
EpcXdAppSend(Node* node,
	int interfaceIndex,
	const LteRnti& src,
	const fxRnti& dst,
	EpcMessageType type,
	int payloadSize,
	char *payload,
	int virtualPacketSize = -1);

//gss2019 xd
//发送星地切换请求时调用
void
EpcXdAppSend(Node* node,
	int interfaceIndex,
	const fxRnti& src,
	const LteRnti& dst,
	EpcMessageType type,
	int payloadSize,
	char *payload,
	int virtualPacketSize = -1);
//gss xd 
void
EpcXdAppSendRequried(Node* node,
	int interfaceIndex,
	const LteRnti& src,
	const LteRnti& dst,
	EpcMessageType type,
	int payloadSize,
	char *payload,
	int virtualPacketSize = -1);
// /**
// FUNCTION   :: EpcFxAppCommitToUdp
// LAYER      :: EPC
// PURPOSE    :: send to UDP layer
// PARAMETERS ::
// RETURN     :: void : NULL
// **/
void
EpcFxAppCommitToUdp(
	Node *node,
	AppType appType,
	NodeAddress sourceAddr,
	short sourcePort,
	NodeAddress destAddr,
	int outgoingInterface,
	char *payload,
	int payloadSize,
	TosType priority,
	clocktype delay,
	TraceProtocolType traceProtocol,
	int virtualPacketSize);

// /**
// FUNCTION   :: EpcFxAppSend_AttachUE
// LAYER      :: EPC
// PURPOSE    :: API to send message AttachUE
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + src              : const fxRnti& : UE
// + dst              : const fxRnti& : eNB
// RETURN     :: void : NULL
// **/
void
EpcFxAppSend_AttachUE(Node* node,
	int interfaceIndex,
	const fxRnti& yhRnti,
	const fxRnti& xgRnti);

// /**
// FUNCTION   :: EpcFxAppSend_DetachUE
// LAYER      :: EPC
// PURPOSE    :: API to send message DetachUE
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + src              : const fxRnti& : UE
// + dst              : const fxRnti& : eNB
// RETURN     :: void : NULL
// **/
void
EpcFxAppSend_DetachUE(Node* node,
	int interfaceIndex,
	const fxRnti& yhRnti,
	const fxRnti& xgRnti);

// /**
// FUNCTION   :: EpcFxAppSend_HandoverRequest
// LAYER      :: EPC
// PURPOSE    :: API to send message HandoverRequest
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const FXHandoverParticipator& : participators of H.O.
// RETURN     :: void : NULL
// **/
void EpcFxAppSend_HandoverRequest(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator);

// /**
// FUNCTION   :: EpcFxAppSend_HandoverRequestAck
// LAYER      :: EPC
// PURPOSE    :: API to send message HandoverRequestAck
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const FXHandoverParticipator& : participators of H.O.
// + reconf   : const FXRrcConnectionReconfiguration& : reconfiguration
// RETURN     :: void : NULL
// **/
void EpcFxAppSend_HandoverRequestAck(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator,
	const FXRrcConnectionReconfiguration& reconf);

// /**
// FUNCTION   :: EpcFxAppSend_SnStatusTransfer
// LAYER      :: EPC
// PURPOSE    :: API to send message SnStatusTransfer
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const FXHandoverParticipator& : participators of H.O.
// + snStatusTransferItem : std::map<int, SacFxSnStatusTransferItem>& : 
// RETURN     :: void : NULL
// **/
void EpcFxAppSend_SnStatusTransfer(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator,
	std::map<int, SacFxSnStatusTransferItem>& snStatusTransferItem);

// /**
// FUNCTION   :: EpcFxAppSend_DataForwarding
// LAYER      :: EPC
// PURPOSE    :: API to send message DataForwarding
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const FXHandoverParticipator& : participators of H.O.
// + bearerId         : int      : bearerId
// + forwardingMsg    : Message* : message
// RETURN     :: void : NULL
// **/
void EpcFxAppSend_DataForwarding(
	Node* node,
	int interfaceIndex,
	const FXHandoverParticipator& hoParticipator,
	int bearerId,
	Message* forwardingMsg);

// /**
// FUNCTION   :: EpcFxAppSend_PathSwitchRequest
// LAYER      :: EPC
// PURPOSE    :: API to send message PathSwitchRequest
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const FXHandoverParticipator& : participators of H.O.
// RETURN     :: void : NULL
// **/
void EpcFxAppSend_PathSwitchRequest(
	Node* node,
	int interfaceIndex,
	const FXHandoverParticipator&hoParticipator);

// /**
// FUNCTION   :: EpcFxAppSend_PathSwitchRequestAck
// LAYER      :: EPC
// PURPOSE    :: API to send message PathSwitchRequestAck
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const FXHandoverParticipator& : participators of H.O.
// + result           : BOOL     : path switch result
// RETURN     :: void : NULL
// **/
void EpcFxAppSend_PathSwitchRequestAck(
	Node* node,
	int interfaceIndex,
	const FXHandoverParticipator&hoParticipator,
	BOOL result);

// /**
// FUNCTION   :: EpcFxAppSend_EndMarker
// LAYER      :: EPC
// PURPOSE    :: API to send message EndMarker
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + src              : const fxRnti& : source of message
// + dst              : const fxRnti& : dest of message
// + hoParticipator   : const FXHandoverParticipator& : participators of H.O.
// RETURN     :: void : NULL
// **/
void EpcFxAppSend_EndMarker(
	Node *node,
	UInt32 interfaceIndex,
	const fxRnti& src,
	const fxRnti& dst,
	const FXHandoverParticipator& hoParticipator);

// /**
// FUNCTION   :: EpcFxAppSend_UeContextRelease
// LAYER      :: EPC
// PURPOSE    :: API to send message UeContextRelease
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const FXHandoverParticipator& : participators of H.O.
// RETURN     :: void : NULL
// **/
void EpcFxAppSend_UeContextRelease(
	Node* node,
	int interfaceIndex,
	const FXHandoverParticipator&hoParticipator);

// /**
// FUNCTION   :: EpcFxAppSend_HoPreparationFailure
// LAYER      :: EPC
// PURPOSE    :: API to send message HoPreparationFailure
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const FXHandoverParticipator& : participators of H.O.
// RETURN     :: void : NULL
// **/
void EpcFxAppSend_HoPreparationFailure(
	Node* node,
	int interfaceIndex,
	const FXHandoverParticipator&hoParticipator);

// /**
// FUNCTION   :: EpcFxAppGetNodeAddressOnEpcSubnet
// LAYER      :: EPC
// PURPOSE    :: API to send message HandoverRequestAck
// PARAMETERS ::
// + node             : Node*       : Pointer to node.
// + targetNodeId      : NodeAddress : Node ID
// RETURN     :: NodeAddress : address of specified node on EPC subnet
// **/
NodeAddress EpcFxAppGetNodeAddressOnEpcSubnet(
	Node* node, NodeAddress targetNodeId);

// /**
// FUNCTION   :: EpcFxAppIsNodeOnTheSameEpcSubnet
// LAYER      :: EPC
// PURPOSE    :: API to send message HandoverRequestAck
// PARAMETERS ::
// + node             : Node*       : Pointer to node.
// + targetNodeId      : NodeAddress : Node ID
// RETURN     :: BOOL : result
// **/
BOOL EpcFxAppIsNodeOnTheSameEpcSubnet(
	Node* node, NodeAddress targetNodeId);

void
FxDeleteRouteFromForwardingTable(
	Node *node,
	NodeAddress destAddr,
	NodeAddress destMask);
void
FxAddRouteToForwardingTable(
	Node *node,
	NodeAddress destAddr,
	NodeAddress destMask,
	NodeAddress nextHop,
	int outgoingInterfaceIndex);

//gss xd
void EpcXdAppSend_HandoverRequried(
	Node *node,
	UInt32 interfaceIndex,
	const LteRnti& ueRnti,
	const LteRnti& srcEnbRnti,
	Node * ueNode);
//gss xd
void EpcXdAppSend_HandoverCommand(
	Node *node,
	UInt32 interfaceIndex,
	const XdHandoverParticipator& xdhoParticipator);
//gss xd
void EpcXdAppSend_HandoverRequest(
	Node *node,
	UInt32 interfaceIndex,
	const XdHandoverParticipator& xdhoParticipator);
//gss xd
void EpcXdAppSend_HandoverRequestAck(
	Node *node,
	UInt32 interfaceIndex,
	const XdHandoverParticipator& xdhoParticipator);
#endif  // _EPC_FX_APP_H_

