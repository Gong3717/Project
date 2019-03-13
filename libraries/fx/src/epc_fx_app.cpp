#include "api.h"
#include "partition.h"
#include "app_util.h"
#include "network_ip.h"
#include "EPC_fx_common.h"
#include "EPC_fx.h"
#include "EPC_fx_app.h"
#include "epc_lte.h"  //gss xd
#include "epc_lte_app.h" //gss xd
#include "epc_lte_common.h" //gss xd

//#include "layer3_fx.h"

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
EpcFxAppProcessEvent(Node *node, Message *msg)
{
	// get container
	EpcFxAppMessageContainer* container =
		(EpcFxAppMessageContainer*)MESSAGE_ReturnInfo(
			msg, INFO_TYPE_FxEpcAppContainer);

	// switch process by message type
	switch (container->type)
	{
	case EPC_FX_MESSAGE_TYPE_ATTACH_UE:
	{
		EpcFxData* epc = EpcFxGetEpcData(node);
		ERROR_Assert(node->nodeId == epc->sgwmmeRnti.nodeId,
			"received unexpected EPC message");
		EpcFxAppMessageArgument_AttachUE arg;
		memcpy(&arg, container->value,
			sizeof(EpcFxAppMessageArgument_AttachUE));
		EpcFxProcessAttachUE(node, arg.yhRnti, arg.xgRnti);
		break;
	}
	case EPC_FX_MESSAGE_TYPE_DETACH_UE:
	{
		EpcFxData* epc = EpcFxGetEpcData(node);
		ERROR_Assert(node->nodeId == epc->sgwmmeRnti.nodeId,
			"received unexpected EPC message");
		EpcFxAppMessageArgument_DetachUE arg;
		memcpy(&arg, container->value,
			sizeof(EpcFxAppMessageArgument_DetachUE));
		EpcFxProcessDetachUE(node, arg.yhRnti, arg.xgRnti);
		break;
	}
	case EPC_FX_MESSAGE_TYPE_HANDOVER_REQUEST:
	{
		EpcFxAppMessageArgument_HoReq* arg =
			(EpcFxAppMessageArgument_HoReq*)container->value;
		ERROR_Assert(node->nodeId == arg->hoParticipator.tgtXgRnti.nodeId,
			"received unexpected EPC message");
		Layer3FxReceiveHoReq(node,
			arg->hoParticipator.tgtXgRnti.interfaceIndex,
			arg->hoParticipator);
		break;
	}
	case EPC_FX_MESSAGE_TYPE_HANDOVER_REQUEST_ACKNOWLEDGE:
	{
		EpcFxAppMessageArgument_HoReqAck* arg =
			(EpcFxAppMessageArgument_HoReqAck*)container->value;
		ERROR_Assert(node->nodeId == arg->hoParticipator.srcXgRnti.nodeId,
			"received unexpected EPC message");
		Layer3FxReceiveHoReqAck(node,
			arg->hoParticipator.srcXgRnti.interfaceIndex,
			arg->hoParticipator,
			arg->reconf);
		break;
	}
	case EPC_FX_MESSAGE_TYPE_SN_STATUS_TRANSFER:
	{
		EpcFxAppMessageArgument_SnStatusTransfer* arg =
			(EpcFxAppMessageArgument_SnStatusTransfer*)container->value;
		ERROR_Assert(node->nodeId == arg->hoParticipator.tgtXgRnti.nodeId,
			"received unexpected EPC message");
		// restore
		std::map<int, SacFxSnStatusTransferItem> snStatusMap;
		for (int i = 0; i < arg->itemNum; i++)
		{
			snStatusMap.insert(std::map<int, SacFxSnStatusTransferItem>
				::value_type(arg->items[i].bearerId,
					arg->items[i].snStatusTransferItem));
		}
		Layer3FxReceiveSnStatusTransfer(node,
			arg->hoParticipator.tgtXgRnti.interfaceIndex,
			arg->hoParticipator,
			snStatusMap);
		break;
	}
	case EPC_FX_MESSAGE_TYPE_DATA_FORWARDING:
	{
		EpcFxAppMessageArgument_DataForwarding* arg =
			(EpcFxAppMessageArgument_DataForwarding*)container->value;
		ERROR_Assert(node->nodeId == arg->hoParticipator.tgtXgRnti.nodeId,
			"received unexpected EPC message");
		Layer3FxReceiveDataForwarding(node,
			arg->hoParticipator.tgtXgRnti.interfaceIndex,
			arg->hoParticipator,
			arg->bearerId,
			arg->message);
		break;
	}
	case EPC_FX_MESSAGE_TYPE_PATH_SWITCH_REQUEST:
	{
		EpcFxAppMessageArgument_PathSwitchRequest* arg =
			(EpcFxAppMessageArgument_PathSwitchRequest*)container->value;
		EpcFxData* epc = EpcFxGetEpcData(node);
		ERROR_Assert(node->nodeId == epc->sgwmmeRnti.nodeId,
			"received unexpected EPC message");
		EpcFxProcessPathSwitchRequest(node, arg->hoParticipator);
		break;
	}
	case EPC_FX_MESSAGE_TYPE_PATH_SWITCH_REQUEST_ACK:
	{
		EpcFxAppMessageArgument_PathSwitchRequestAck* arg =
			(EpcFxAppMessageArgument_PathSwitchRequestAck*)container->value;
		ERROR_Assert(node->nodeId == arg->hoParticipator.tgtXgRnti.nodeId,
			"received unexpected EPC message");
		Layer3FxReceivePathSwitchRequestAck(
			node, arg->hoParticipator.tgtXgRnti.interfaceIndex,
			arg->hoParticipator, arg->result);
		break;
	}
	case EPC_FX_MESSAGE_TYPE_END_MARKER:
	{
		EpcFxAppMessageArgument_EndMarker* arg =
			(EpcFxAppMessageArgument_EndMarker*)container->value;
		ERROR_Assert(node->nodeId == arg->hoParticipator.srcXgRnti.nodeId
			|| node->nodeId == arg->hoParticipator.tgtXgRnti.nodeId,
			"received unexpected EPC message");
		Layer3FxReceiveEndMarker(
			node, arg->hoParticipator.srcXgRnti.interfaceIndex,
			arg->hoParticipator);
		break;
	}
	case EPC_FX_MESSAGE_TYPE_UE_CONTEXT_RELEASE:
	{
		EpcFxAppMessageArgument_UeContextRelease* arg =
			(EpcFxAppMessageArgument_UeContextRelease*)container->value;
		ERROR_Assert(node->nodeId == arg->hoParticipator.srcXgRnti.nodeId,
			"received unexpected EPC message");
		Layer3FxReceiveUeContextRelease(
			node, arg->hoParticipator.srcXgRnti.interfaceIndex,
			arg->hoParticipator);
		break;
	}
	case EPC_FX_MESSAGE_TYPE_HO_PREPARATION_FAILURE:
	{
		EpcFxAppMessageArgument_HoPreparationFailure* arg =
			(EpcFxAppMessageArgument_HoPreparationFailure*)container->value;
		ERROR_Assert(node->nodeId == arg->hoParticipator.srcXgRnti.nodeId,
			"received unexpected EPC message");
		Layer3FxReceiveHoPreparationFailure(
			node, arg->hoParticipator.srcXgRnti.interfaceIndex,
			arg->hoParticipator);
		break;
	}
	default:
	{
		ERROR_Assert(false, "received EPC message of unknown type");
		break;
	}
	}

	// delete message
	MESSAGE_Free(node, msg);
}

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
	int virtualPacketSize)
{
	EpcFxData* epc = EpcFxGetEpcData(node);

	// pack EpcMessageType and payload to container
	int containerSize = sizeof(EpcFxAppMessageContainer) + payloadSize;
	EpcFxAppMessageContainer* container =
		(EpcFxAppMessageContainer*)malloc(containerSize);
	container->src = src;
	container->dst = dst;
	container->type = type;
	container->length = payloadSize;
	if (payloadSize > 0)
	{
		memcpy(container->value, payload, payloadSize);
	}

	// resolve source and destination address for EPC subnet
	NodeAddress sourceAddr = EpcFxAppGetNodeAddressOnEpcSubnet(
		node, node->nodeId);
	NodeAddress destAddr = EpcFxAppGetNodeAddressOnEpcSubnet(
		node, dst.nodeId);

	EpcFxAppCommitToUdp(
		node,
		APP_EPC_FX,
		sourceAddr,
		APP_EPC_FX,
		destAddr,
		epc->outgoingInterfaceIndex,
		(char*)container,
		containerSize,
		APP_DEFAULT_TOS,
		EPC_FX_APP_DELAY,
		TRACE_EPC_FX,
		virtualPacketSize);

	// delete container
	free(container);

}

//gss xd  
//发送星地切换请求确认时调用
void
EpcXdAppSend(Node* node,
	int interfaceIndex,
	const LteRnti& src,
	const fxRnti& dst,
	EpcMessageType type,
	int payloadSize,
	char *payload,
	int virtualPacketSize)
{
	EpcData* epc = EpcLteGetEpcData(node);
	// pack EpcMessageType and payload to container
	int containerSize = sizeof(EpcLteAppMessageContainer) + payloadSize;
	EpcLteAppMessageContainer* container =
		(EpcLteAppMessageContainer*)malloc(containerSize);
	container->src = src;
	container->tgt = dst;
	container->type = type;
	container->length = payloadSize;
	if (payloadSize > 0)
	{
		memcpy(container->value, payload, payloadSize);
	}

	// resolve source and destination address for EPC subnet
	/*NodeAddress sourceAddr = EpcFxAppGetNodeAddressOnEpcSubnet(
		node, node->nodeId);*/
	NodeAddress sourceAddr = 3187672577; //sgwmme的Uint地址 暂时写死
	NodeAddress destAddr = EpcFxAppGetNodeAddressOnEpcSubnet(
		node, dst.nodeId);

	EpcLteAppCommitToUdp(
		node,
		APP_EPC_LTE,
		sourceAddr,
		APP_EPC_LTE,
		destAddr,
		epc->outgoingInterfaceIndex,
		(char*)container,
		containerSize,
		APP_DEFAULT_TOS,
		EPC_LTE_APP_DELAY,
		TRACE_EPC_LTE,
		virtualPacketSize);

	// delete container
	free(container);
}

//gss xd 
//发送星地切换请求时调用
void
EpcXdAppSend(Node* node,
	int interfaceIndex,
	const fxRnti& src,
	const LteRnti& dst,
	EpcMessageType type,
	int payloadSize,
	char *payload,
	int virtualPacketSize)
{
	EpcData* epc = EpcLteGetEpcData(node);
	// pack EpcMessageType and payload to container
	int containerSize = sizeof(EpcLteAppMessageContainer) + payloadSize;
	EpcLteAppMessageContainer* container =
		(EpcLteAppMessageContainer*)malloc(containerSize);
	container->tgt = src;
	container->dst = dst;
	container->type = type;
	container->length = payloadSize;
	if (payloadSize > 0)
	{
		memcpy(container->value, payload, payloadSize);
	}

	// resolve source and destination address for EPC subnet
	NodeAddress sourceAddr = EpcLteAppGetNodeAddressOnEpcSubnet(
		node, node->nodeId);
	/*NodeAddress destAddr = EpcLteAppGetNodeAddressOnEpcSubnet(
		node, dst.nodeId);*/
	NodeAddress destAddr = 3187672577; //sgwmme的Uint地址 暂时写死

	EpcLteAppCommitToUdp(
		node,
		APP_EPC_LTE,
		sourceAddr,
		APP_EPC_LTE,
		destAddr,
		epc->outgoingInterfaceIndex,
		(char*)container,
		containerSize,
		APP_DEFAULT_TOS,
		EPC_LTE_APP_DELAY,
		TRACE_EPC_LTE,
		virtualPacketSize);

	// delete container
	free(container);
}

//gss xd 发送星地请求需求时调用
void
EpcXdAppSendRequried(Node* node,
	int interfaceIndex,
	const LteRnti& src,
	const LteRnti& dst,
	EpcMessageType type,
	int payloadSize,
	char *payload,
	int virtualPacketSize)
{
	//EpcFxData* epc = EpcFxGetEpcData(node);
	EpcData* epc = EpcLteGetEpcData(node);
	// pack EpcMessageType and payload to container
	int containerSize = sizeof(EpcLteAppMessageContainer) + payloadSize;
	EpcLteAppMessageContainer* container =
		(EpcLteAppMessageContainer*)malloc(containerSize);
	container->src = src;
	container->dst = dst;
	container->type = type;
	container->length = payloadSize;
	if (payloadSize > 0)
	{
		memcpy(container->value, payload, payloadSize);
	}

	// resolve source and destination address for EPC subnet
	NodeAddress sourceAddr = EpcLteAppGetNodeAddressOnEpcSubnet(
		node, node->nodeId);
	NodeAddress destAddr = EpcLteAppGetNodeAddressOnEpcSubnet(
		node, dst.nodeId);

	EpcLteAppCommitToUdp(
		node,
		APP_EPC_LTE,
		sourceAddr,
		APP_EPC_LTE,
		destAddr,
		epc->outgoingInterfaceIndex,
		(char*)container,
		containerSize,
		APP_DEFAULT_TOS,
		EPC_LTE_APP_DELAY,
		TRACE_EPC_LTE,
		virtualPacketSize);

	// delete container
	free(container);
}

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
	int virtualPacketSize)
{
	Message *msg;
	AppToUdpSend *info;
	ActionData acnData;

	msg = MESSAGE_Alloc(
		node,
		TRANSPORT_LAYER,
		TransportProtocol_UDP,
		MSG_TRANSPORT_FromAppSend);

	if (virtualPacketSize < 0)
	{
		virtualPacketSize = payloadSize;
	}

	// allocate virtual packet size
	MESSAGE_PacketAlloc(node, msg, 0, traceProtocol);
	MESSAGE_AddVirtualPayload(node, msg, virtualPacketSize);

	// empty payload
	//memcpy(MESSAGE_ReturnPacket(msg), payload, payloadSize);

	MESSAGE_InfoAlloc(node, msg, sizeof(AppToUdpSend));
	info = (AppToUdpSend*)MESSAGE_ReturnInfo(msg);

	SetIPv4AddressInfo(&info->sourceAddr, sourceAddr);
	info->sourcePort = sourcePort;

	SetIPv4AddressInfo(&info->destAddr, destAddr);
	info->destPort = (short)appType;

	info->priority = priority;
	info->outgoingInterface = outgoingInterface;

	info->ttl = IPDEFTTL;

	// set container
	char* infoContainer = MESSAGE_AppendInfo(
		node, msg, payloadSize, INFO_TYPE_FxEpcAppContainer);
	memcpy(infoContainer, payload, payloadSize);

	//Trace Information
	acnData.actionType = SEND;
	acnData.actionComment = NO_COMMENT;
	TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
		PACKET_OUT, &acnData);

	MESSAGE_Send(node, msg, delay);
}

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
	const fxRnti& XgRnti)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	// set argument
	EpcFxAppMessageArgument_AttachUE arg;
	arg.yhRnti = yhRnti;
	arg.xgRnti = XgRnti;

	// send
	EpcFxAppSend(node,
		interfaceIndex,
		XgRnti,
		epc->sgwmmeRnti,
		EPC_FX_MESSAGE_TYPE_ATTACH_UE,
		sizeof(EpcFxAppMessageArgument_AttachUE),
		(char*)&arg);
}

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
	const fxRnti& XgRnti)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	// set argument
	EpcFxAppMessageArgument_DetachUE arg;
	arg.yhRnti = yhRnti;
	arg.xgRnti = XgRnti;

	// send
	EpcFxAppSend(node,
		interfaceIndex,
		XgRnti,
		epc->sgwmmeRnti,
		EPC_FX_MESSAGE_TYPE_DETACH_UE,
		sizeof(EpcFxAppMessageArgument_DetachUE),
		(char*)&arg);
}

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
	const FXHandoverParticipator& hoParticipator)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	EpcFxAppMessageArgument_HoReq arg;

	// set participators
	arg.hoParticipator = hoParticipator;

	// send
	EpcFxAppSend(node,
		interfaceIndex,
		arg.hoParticipator.srcXgRnti,
		arg.hoParticipator.tgtXgRnti,
		EPC_FX_MESSAGE_TYPE_HANDOVER_REQUEST,
		sizeof(EpcFxAppMessageArgument_HoReq),
		(char*)&arg);

	// update stats
	epc->statData.numHandoverRequestSent++;
}

// /**
// FUNCTION   :: EpcFxAppSend_HandoverRequestAck
// LAYER      :: EPC
// PURPOSE    :: API to send message HandoverRequestAck
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const FXHandoverParticipator& : participators of H.O.
// + reconf   : const RrcConnectionReconfiguration& : reconfiguration
// RETURN     :: void : NULL
// **/
void EpcFxAppSend_HandoverRequestAck(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator,
	const FXRrcConnectionReconfiguration& reconf)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	EpcFxAppMessageArgument_HoReqAck arg;
	// set participators
	arg.hoParticipator = hoParticipator;
	// set rrc config
	arg.reconf = reconf;  // copy

						  // send
	EpcFxAppSend(node,
		interfaceIndex,
		arg.hoParticipator.tgtXgRnti,
		arg.hoParticipator.srcXgRnti,
		EPC_FX_MESSAGE_TYPE_HANDOVER_REQUEST_ACKNOWLEDGE,
		sizeof(EpcFxAppMessageArgument_HoReqAck),
		(char*)&arg);

	// update stats
	epc->statData.numHandoverRequestAckSent++;
}

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
	std::map<int, SacFxSnStatusTransferItem>& snStatusTransferItem)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	int msgSize = sizeof(EpcFxAppMessageArgument_SnStatusTransfer) +
		snStatusTransferItem.size() * sizeof(EpcFxAppPairBearerIdSnStatus);
	EpcFxAppMessageArgument_SnStatusTransfer* arg =
		(EpcFxAppMessageArgument_SnStatusTransfer*)malloc(msgSize);

	// store
	arg->hoParticipator = hoParticipator;
	arg->itemNum = snStatusTransferItem.size();
	int itemIndex = 0;
	for (std::map<int, SacFxSnStatusTransferItem>::iterator it =
		snStatusTransferItem.begin(); it != snStatusTransferItem.end();
		++it)
	{
		arg->items[itemIndex].bearerId = it->first;
		memcpy(&arg->items[itemIndex].snStatusTransferItem,
			&it->second, sizeof(SacFxSnStatusTransferItem));
		itemIndex++;
	}

	// send
	EpcFxAppSend(node,
		interfaceIndex,
		arg->hoParticipator.srcXgRnti,
		arg->hoParticipator.tgtXgRnti,
		EPC_FX_MESSAGE_TYPE_SN_STATUS_TRANSFER,
		msgSize,
		(char*)arg);

	// del
	free(arg);

	// update stats
	epc->statData.numSnStatusTransferSent++;
}

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
	Message* forwardingMsg)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	EpcFxAppMessageArgument_DataForwarding arg;

	// store
	arg.hoParticipator = hoParticipator;
	arg.bearerId = bearerId;
	arg.message = forwardingMsg;

	int virtualPacketSize = sizeof(FXHandoverParticipator)
		+ sizeof(int) + MESSAGE_ReturnPacketSize(forwardingMsg);

	// send
	EpcFxAppSend(node,
		interfaceIndex,
		arg.hoParticipator.srcXgRnti,
		arg.hoParticipator.tgtXgRnti,
		EPC_FX_MESSAGE_TYPE_DATA_FORWARDING,
		virtualPacketSize,
		(char*)&arg);
}

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
	const FXHandoverParticipator& hoParticipator)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	EpcFxAppMessageArgument_PathSwitchRequest arg;
	// set participators
	arg.hoParticipator = hoParticipator;

	// send
	EpcFxAppSend(node,
		interfaceIndex,
		hoParticipator.tgtXgRnti,
		epc->sgwmmeRnti,
		EPC_FX_MESSAGE_TYPE_PATH_SWITCH_REQUEST,
		sizeof(EpcFxAppMessageArgument_PathSwitchRequest),
		(char*)&arg);

	// update stats
	epc->statData.numPathSwitchRequestSent++;
}

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
	BOOL result)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	EpcFxAppMessageArgument_PathSwitchRequestAck arg;

	// store
	arg.hoParticipator = hoParticipator;
	arg.result = result;

	// send
	EpcFxAppSend(node,
		interfaceIndex,
		epc->sgwmmeRnti,
		hoParticipator.tgtXgRnti,
		EPC_FX_MESSAGE_TYPE_PATH_SWITCH_REQUEST_ACK,
		sizeof(EpcFxAppMessageArgument_PathSwitchRequestAck),
		(char*)&arg);

	// update stats
	epc->statData.numPathSwitchRequestAckSent++;
}

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
	const FXHandoverParticipator& hoParticipator)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	EpcFxAppMessageArgument_EndMarker arg;
	// set participators
	arg.hoParticipator = hoParticipator;

	// send
	EpcFxAppSend(node,
		interfaceIndex,
		src,
		dst,
		EPC_FX_MESSAGE_TYPE_END_MARKER,
		sizeof(EpcFxAppMessageArgument_EndMarker),
		(char*)&arg);

	// update stats
	epc->statData.numEndMarkerSent++;
}

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
	const FXHandoverParticipator&hoParticipator)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	EpcFxAppMessageArgument_UeContextRelease arg;
	// set participators
	arg.hoParticipator = hoParticipator;

	// send
	EpcFxAppSend(node,
		interfaceIndex,
		hoParticipator.tgtXgRnti,
		hoParticipator.srcXgRnti,
		EPC_FX_MESSAGE_TYPE_UE_CONTEXT_RELEASE,
		sizeof(EpcFxAppMessageArgument_UeContextRelease),
		(char*)&arg);

	// update stats
	epc->statData.numUeContextReleaseSent++;
}

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
	const FXHandoverParticipator&hoParticipator)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	EpcFxAppMessageArgument_HoPreparationFailure arg;
	// set participators
	arg.hoParticipator = hoParticipator;

	// send
	EpcFxAppSend(node,
		interfaceIndex,
		hoParticipator.tgtXgRnti,
		hoParticipator.srcXgRnti,
		EPC_FX_MESSAGE_TYPE_HO_PREPARATION_FAILURE,
		sizeof(EpcFxAppMessageArgument_HoPreparationFailure),
		(char*)&arg);
}

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
	Node* node, NodeAddress targetNodeId)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	const fxRnti& rntiOnEpcSubnet = epc->sgwmmeRnti;

	NodeAddress subnetAddress = MAPPING_GetSubnetAddressForInterface(
		node, rntiOnEpcSubnet.nodeId, rntiOnEpcSubnet.interfaceIndex);
	NodeAddress subnetMask = MAPPING_GetSubnetMaskForInterface(
		node, rntiOnEpcSubnet.nodeId, rntiOnEpcSubnet.interfaceIndex);
	int numHostBits = AddressMap_ConvertSubnetMaskToNumHostBits(subnetMask);

	NodeAddress addr = MAPPING_GetInterfaceAddressForSubnet(
		node, targetNodeId, subnetAddress, numHostBits);

	// invalid address returned
	if (addr == ANY_ADDRESS)
	{
		char errStr[MAX_STRING_LENGTH];
		sprintf(errStr,
			"cannot get valid address on EPC subnet (0x%x) for Node%d.",
			subnetAddress, targetNodeId);
		ERROR_Assert(FALSE, errStr);
	}

	return addr;
}

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
	Node* node, NodeAddress targetNodeId)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	const fxRnti& rntiOnEpcSubnet = epc->sgwmmeRnti;

	NodeAddress subnetAddress = MAPPING_GetSubnetAddressForInterface(
		node, rntiOnEpcSubnet.nodeId, rntiOnEpcSubnet.interfaceIndex);
	NodeAddress subnetMask = MAPPING_GetSubnetMaskForInterface(
		node, rntiOnEpcSubnet.nodeId, rntiOnEpcSubnet.interfaceIndex);
	int numHostBits = AddressMap_ConvertSubnetMaskToNumHostBits(subnetMask);

	NodeAddress addr = MAPPING_GetInterfaceAddressForSubnet(
		node, targetNodeId, subnetAddress, numHostBits);

	// valid address or not
	return (addr != ANY_ADDRESS);
}
// /**
// FUNCTION   :: FxAddRouteToForwardingTable
// LAYER      :: FX
// PURPOSE    :: add route to forwarding table of QualNet (call QualNet API)
// PARAMETERS ::
// + node                  : Node*         : pointer to Node
// + destAddr              : NodeAddress   : destination address
// + destMask              : NodeAddress   : destination mask
// + nextHop               : NodeAddress   : next hop
// + outgoingInterfaceIndex: int           : outgoing interface index
// RETURN     :: void : NULL
// **/
void
FxAddRouteToForwardingTable(
	Node *node,
	NodeAddress destAddr,
	NodeAddress destMask,
	NodeAddress nextHop,
	int outgoingInterfaceIndex)
{
	NetworkRoutingProtocolType type = ROUTING_PROTOCOL_STATIC;
	int cost = 0;

	NetworkUpdateForwardingTable(
		node,
		destAddr,
		destMask,
		nextHop,
		outgoingInterfaceIndex,
		cost,
		type);
}

// /**
// FUNCTION   :: FxDeleteRouteFromForwardingTable
// LAYER      :: FX
// PURPOSE    :: delete route from forwarding table of QualNet
//                                              (modify the table directly)
// PARAMETERS ::
// + node                  : Node*         : pointer to Node
// + destAddr              : NodeAddress   : destination address
// + destMask              : NodeAddress   : destination mask
// RETURN     :: void : NULL
// **/
void
FxDeleteRouteFromForwardingTable(
	Node *node,
	NodeAddress destAddr,
	NodeAddress destMask)
{

	NetworkRoutingProtocolType type = ROUTING_PROTOCOL_STATIC;
	NetworkDataIp *ip = (NetworkDataIp *)node->networkData.networkVar;
	NetworkForwardingTable *rt = &ip->forwardTable;

	int i = 0;

	// Go through the routing table...

	while (i < rt->size)
	{
		// Delete entries that corresponds to the routing protocol used
		if (rt->row[i].protocolType == type
			&& rt->row[i].destAddress == destAddr
			&& rt->row[i].destAddressMask == destMask)
		{
			int j = i + 1;

			// Move all other entries down
			while (j < rt->size)
			{
				rt->row[j - 1] = rt->row[j];
				j++;
			}

			// Update forwarding table size.
			rt->size--;
		}
		else
		{
			i++;
		}
	}
}

//gss xd
void EpcXdAppSend_HandoverRequried(
	Node *node,
	UInt32 interfaceIndex,
	const LteRnti& ueRnti,
	const LteRnti& srcEnbRnti,
	Node *ueNode)
{
	EpcData* epc = EpcLteGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");
	EpcXdAppMessageArgument_HoRequried requried;
	requried.xdhoParticipator.srcEnbRnti = srcEnbRnti;
	requried.xdhoParticipator.sgwmmeRnti= epc->sgwmmeRnti;
	requried.xdhoParticipator.ueRnti = ueRnti;
	requried.xdhoParticipator.node = ueNode;
	//add ue node
	EpcXdAppSendRequried(node,
		interfaceIndex,
		srcEnbRnti,
		requried.xdhoParticipator.sgwmmeRnti,
		EPC_XD_MESSAGE_TYPE_HANDOVER_REQURIED,
		sizeof(EpcXdAppMessageArgument_HoRequried),
		(char*)&requried, 0);
	// update stats
	epc->statData.numXdHandoverRequiredSent++;
	cout << "node" << node->nodeId << " : sent xd handover require at " << getSimTime(node) / (double)SECOND << " second." << endl;
	
}

//gss xd
void EpcXdAppSend_HandoverCommand(
	Node *node,
	UInt32 interfaceIndex,
	const XdHandoverParticipator& xdhoParticipator) {
	EpcData* epc = EpcLteGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	EpcXdAppMessageArgument_HoCommand arg;
	arg.xdhoParticipator = xdhoParticipator;
	// send
	EpcLteAppSendXdCommand(node,
		interfaceIndex,
		arg.xdhoParticipator.sgwmmeRnti,
		arg.xdhoParticipator.srcEnbRnti,
		EPC_XD_MESSAGE_TYPE_HANDOVER_COMMAND,
		sizeof(EpcXdAppMessageArgument_HoCommand),
		(char*)&arg);
	// update stats
	epc->statData.numXdHandoverCommandSent++;
	cout << "node" << node->nodeId << " : sent xd handover command at " << getSimTime(node) / (double)SECOND << " second." << endl;
}
//gss xd
void EpcXdAppSend_HandoverRequest(
	Node *node,
	UInt32 interfaceIndex,
	const XdHandoverParticipator& xdhoParticipator)
{
	EpcData* epc = EpcLteGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	EpcXdAppMessageArgument_HoReq arg;

	// set participators
	arg.xdhoParticipator = xdhoParticipator;

	// send
	//SGWMME向目标XG发送切换请求
	EpcXdAppSend(node,
		interfaceIndex,
		arg.xdhoParticipator.sgwmmeRnti,
		arg.xdhoParticipator.tgtxgRnti,
		EPC_XD_MESSAGE_TYPE_HANDOVER_REQUEST,
		sizeof(EpcXdAppMessageArgument_HoReq),
		(char*)&arg);

	// update stats
	epc->statData.numXdHandoverRequestSent++;
	cout << "node" << node->nodeId << " : sent xd handover request at " << getSimTime(node) / (double)SECOND << " second." << endl;
}


//gss xd
void EpcXdAppSend_HandoverRequestAck(
	Node *node,
	UInt32 interfaceIndex,
	const XdHandoverParticipator& xdhoParticipator)
{
	EpcData* epc = EpcLteGetEpcData(node);
	ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

	EpcXdAppMessageArgument_HoReqAck arg;
	// set participators
	arg.xdhoParticipator = xdhoParticipator;

	//目标XG向SGWMME发送切换请求确认
	EpcXdAppSend(node,
		interfaceIndex,
		arg.xdhoParticipator.tgtxgRnti,
		arg.xdhoParticipator.sgwmmeRnti,
		EPC_XD_MESSAGE_TYPE_HANDOVER_REQUEST_ACKNOWLEDGE,
		sizeof(EpcXdAppMessageArgument_HoReqAck),
		(char*)&arg);

	// update stats
	epc->statData.numXdHandoverRequestAckSent++;
	cout << "node" << node->nodeId << " : sent xd handover request acknowledgment at " << getSimTime(node) / (double)SECOND << " second." << endl;
}