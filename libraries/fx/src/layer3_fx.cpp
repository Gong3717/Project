#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network_ip.h"


#include "phy_fx.h"
#include "layer2_fx.h"
#include "epc_fx.h"
#include "epc_fx_app.h"//Added by WLH
#include "phy_lte.h"   //gss xd
#include "epc_Lte.h"  //gss xd




FxConnectionInfo* Layer3FxGetConnectionInfo(
	Node* node, int interfaceIndex, const fxRnti& rnti)
{
	// get the layer3Data stored in Layer2Data
	Layer3DataFx* layer3Data = ((Layer2DataFx*)(node->macData[interfaceIndex]->macVar))->fxLayer3Data;


	MapFxConnectionInfo::iterator it =
		layer3Data->connectionInfoMap.find(rnti);

	if (it != layer3Data->connectionInfoMap.end())
	{
		return &(it->second);
	}

	return NULL;
}


void Layer3FxProcessHandoverRequestAtXG(Node * node, int iface, const fxRnti & yhRnti)
{
	// 更改该用户RRCconnection的状态以暂停发包
	FxConnectionInfo* yhConnection = Layer3FxGetConnectionInfo(node, iface, yhRnti);
	yhConnection->state = LAYER3_FX_CONNECTION_HANDOVER;
}



void Layer3FxProcessSwitchBeamInfoAtXG(Node * node, int iface, const fxRnti & yhRnti, UInt8 currentBeam, UInt8 targetBeam)
{
	Layer3DataFx* layer3Data = FxLayer2GetLayer3DataFx(node, iface);

	auto sbcp = layer3Data->SBCPMap.find(currentBeam);
	assert(sbcp != layer3Data->SBCPMap.end());

	assert(sbcp->second.find(yhRnti.nodeId) != sbcp->second.end());

	// 删掉在目前所属波束中的记录，添加到目标波束中
	sbcp->second.erase(yhRnti.nodeId);

	sbcp = layer3Data->SBCPMap.find(targetBeam);
	assert(sbcp != layer3Data->SBCPMap.end());

	sbcp->second.insert(yhRnti.nodeId);
}



void FxLayer3Init(Node* node,
	UInt32 interfaceIndex,
	const NodeInput* nodeInput)
{

	Layer3DataFx* layer3Data = FxLayer2GetLayer3DataFx(node, interfaceIndex);

	layer3Data->layer3FxState = LAYER3_FX_POWER_OFF;
	layer3Data->connectionInfoMap.clear();

	// Power On
	Layer3FxPowerOn(node, interfaceIndex);


	// 信关站处理切换文件
	FxStationType stationType =
		FxLayer2GetStationType(node, interfaceIndex);
	if (stationType == FX_STATION_TYPE_YH) {
		Layer3FxInitHandoverSchedule(node, interfaceIndex, nodeInput);
		//Layer3FxInitXdHandover(node, interfaceIndex);
	}

}

void Layer3FxAddConnectionInfo(Node * node, int interfaceIndex, const fxRnti & rnti, Layer3FxConnectionState state)
{
	Layer3DataFx* layer3Data =
		FxLayer2GetLayer3DataFx(node, interfaceIndex);

	MapFxConnectionInfo* connMap = &layer3Data->connectionInfoMap;

	FxConnectionInfo newInfo(LAYER3_FX_CONNECTION_WAITING);

	connMap->insert(MapFxConnectionInfo::value_type(
		rnti, newInfo));

}

const fxRnti Layer3FxGetRandomAccessingXG(Node * node, int iface)
{
	Layer3DataFx* layer3Data =
		FxLayer2GetLayer3DataFx(node, iface);

	MapFxConnectionInfo::iterator it = layer3Data->connectionInfoMap.begin();

	for (; it != layer3Data->connectionInfoMap.end(); it++)
	{
		if (it->second.state == LAYER3_FX_CONNECTION_WAITING)
			return it->first;
	}

	return FX_INVALID_RNTI;

}

void Layer3FxAddRoute(Node * node, int interfaceIndex, const fxRnti & oppositeRnti)
{
	NodeAddress destAddr;
	NodeAddress destMask;
	NodeAddress nextHop;
	int outgoingInterfaceIndex;

	FxStationType stationType = FxLayer2GetStationType(node, interfaceIndex);
	if (stationType == FX_STATION_TYPE_XG)
	{
		// 信关站加的路由项直接针对这个IP地址，掩码全1
		destAddr =
			MAPPING_GetInterfaceAddressForInterface(
				node, oppositeRnti.nodeId, oppositeRnti.interfaceIndex);
		destMask = ANY_ADDRESS;
		nextHop =
			MAPPING_GetInterfaceAddressForInterface(
				node, oppositeRnti.nodeId, oppositeRnti.interfaceIndex);
		outgoingInterfaceIndex = interfaceIndex;
	}
	else
	{
		// 用户站添加默认路由
		destAddr = 0;   // default route
		destMask = 0;   // default route
		//destAddr =
		//	MAPPING_GetInterfaceAddressForInterface(
		//		node, oppositeRnti.nodeId, oppositeRnti.interfaceIndex);  //YH - XG
		//destAddr = 3187672578;   //YH - CN
		//destMask = ANY_ADDRESS;
		nextHop =
			MAPPING_GetInterfaceAddressForInterface(
				node, oppositeRnti.nodeId, oppositeRnti.interfaceIndex);
		outgoingInterfaceIndex = interfaceIndex;
	}

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

void Layer3FxCreateRadioBearer(
	Node* node, int interfaceIndex,
	const fxRnti& rnti, FxConnectedInfo* connectedInfo,
	BOOL isTargetEnb)
{
	PairFxRadioBearer radioBearerPair(
		FX_DEFAULT_BEARER_ID, FxRadioBearer());
	// FxRadioBearer 就是SAC和SLC的实体

	// Init SAC Entity
	SacFxInitSacEntity(
		node, interfaceIndex, rnti, &(radioBearerPair.second.sacEntity),
		isTargetEnb);

	// Init SLC Entity
	FxSlcEntityType rlcEntityType = FX_SLC_ENTITY_AM;
	//Added by WLH
	FxSlcEntityDirect rlcEntityDirect = FX_SLC_ENTITY_BI;

	SlcFxInitSlcEntity(node,
		interfaceIndex,
		rnti,
		rlcEntityType,
		rlcEntityDirect,
		&(radioBearerPair.second.slcEntity));

	std::pair<MapFxRadioBearer::iterator, bool> insertResult =
		connectedInfo->radioBearers.insert(radioBearerPair);

	// set RLC Entity address
	// insertResult.first->second.slcEntity.setSlcEntityToXmEntity();
}



void Layer3FxProcessHandoverCompleteAtXG(Node * node, int iface, const fxRnti & yhRnti)
{
	Layer3DataFx* layer3Data =
		FxLayer2GetLayer3DataFx(node, iface);

	FxConnectionInfo* conInfo = Layer3FxGetConnectionInfo(node, iface, yhRnti);
	conInfo->state = LAYER3_FX_CONNECTION_CONNECTED;
}


// 添加波束信息
void Layer3FxAddBeam(Node * node, int iface, int beamIndex)
{
	Layer3DataFx* layer3Data =
		FxLayer2GetLayer3DataFx(node, iface);

	MapSBCPInfo::iterator sbcp = layer3Data->SBCPMap.find(beamIndex);
	assert(sbcp == layer3Data->SBCPMap.end());

	set<int> nodes;
	layer3Data->SBCPMap.insert(make_pair(beamIndex, nodes));
}


// 添加节点到波束
void Layer3FxAddNodetoBeam(Node * node, int iface, int NodeId, int beamIndex)
{
	Layer3DataFx* layer3Data =
		FxLayer2GetLayer3DataFx(node, iface);

	FxStationType stationType = FxLayer2GetStationType(node, iface);
	
	MapSBCPInfo::iterator sbcp = layer3Data->SBCPMap.find(beamIndex);
	if (sbcp == layer3Data->SBCPMap.end())
	{
		assert(stationType == FX_STATION_TYPE_YH);

		set<int> nodes;
		layer3Data->SBCPMap.insert(make_pair(beamIndex, nodes));
		sbcp = layer3Data->SBCPMap.find(beamIndex);
	}

	assert(sbcp != layer3Data->SBCPMap.end());

	sbcp->second.insert(NodeId);
}

int Layer3FxGetBeamIndexofYHAtXG(Node * node, int iface, int NodeId)
{
	Layer3DataFx* layer3Data =
		FxLayer2GetLayer3DataFx(node, iface);

	FxStationType stationType = FxLayer2GetStationType(node, iface);
	assert(stationType == FX_STATION_TYPE_XG);

	auto sbcp = layer3Data->SBCPMap.begin();
	while (sbcp != layer3Data->SBCPMap.end())
	{
		if (sbcp->second.find(NodeId) != sbcp->second.end())
			return sbcp->first;
		sbcp++;
	}

	return -1;
}


void Layer3FxChangeBeamIndexAtYH(Node * node, int iface, int targetBeamIndex)
{
	// 用户站上的SBCPMap应该只有一项，内容是<波束号，<信关站号>>
	Layer3DataFx* layer3Data =
		FxLayer2GetLayer3DataFx(node, iface);

	FxStationType stationType = FxLayer2GetStationType(node, iface);
	assert(stationType == FX_STATION_TYPE_YH);
	
	auto sbcp = layer3Data->SBCPMap.begin();	
	set<int> copy = sbcp->second;
	layer3Data->SBCPMap.insert(make_pair(targetBeamIndex, copy));

	layer3Data->SBCPMap.erase(sbcp->first);
}


void Layer3FxProcessRrcConnected(Node * node, int iface, const fxRnti & oppositeRnti)
{
	cout << "node" << node->nodeId << " : made RRC connection with node " << oppositeRnti.nodeId
		<< " at " << getSimTime(node) / (double)SECOND << " second." << endl;

	Layer3DataFx* layer3Data =
		FxLayer2GetLayer3DataFx(node, iface);

	FxConnectionInfo* conInfo = Layer3FxGetConnectionInfo(node, iface, oppositeRnti);
	conInfo->state = LAYER3_FX_CONNECTION_CONNECTED;

	// 给该连接创建无线承载
	FxStationType stationType = FxLayer2GetStationType(node, iface);

	FxConnectionInfo* connectionInfo =
		Layer3FxGetConnectionInfo(node, iface, oppositeRnti);
	connectionInfo->connectedInfo.connectedTime = getSimTime(node);
	connectionInfo->connectedInfo.bsrInfo.bufferSizeByte = 0;
	connectionInfo->connectedInfo.bsrInfo.bufferSizeLevel = 0;
	connectionInfo->connectedInfo.bsrInfo.ueRnti =
		stationType == FX_STATION_TYPE_XG ? oppositeRnti : FX_INVALID_RNTI;
	connectionInfo->connectedInfo.isSchedulable = TRUE;

	Layer3FxCreateRadioBearer(node, iface, oppositeRnti,
		&connectionInfo->connectedInfo, FALSE);
		
	
	// Notify to PHY Layer
	int phyIndex =
		FxGetPhyIndexFromMacInterfaceIndex(node, iface);
	PhyFxRrcConnectedNotification(node, phyIndex);


	// add route
	Layer3FxAddRoute(node, iface, oppositeRnti);

	if (stationType == FX_STATION_TYPE_XG)
	{
		// 信关attach用户站到核心网
		EpcFxAppSend_AttachUE(node, iface, oppositeRnti, FxLayer2GetRnti(node, iface));
	}

}


void FxLayer3Finalize(Node * node, UInt32 interfaceIndex)
{
	Layer3DataFx* layer3Data =
		FxLayer2GetLayer3DataFx(node, interfaceIndex);
	layer3Data->connectionInfoMap.clear();

	delete layer3Data;

}

void Layer3FxSetTimerForRrc(Node * node, int interfaceIndex, int eventType, clocktype delay)
{

	Message* timerMsg = MESSAGE_Alloc(node, MAC_LAYER, MAC_PROTOCOL_FX, eventType);

	MESSAGE_Send(node, timerMsg, delay);
}

void Layer3FxProcessEvent(Node * node, int interfaceIndex, Message * msg)
{
	switch (msg->eventType)
	{
	case MSG_RRC_FX_HandoverScheduleTimerExpired:
	{
		// 目前在用户站上被调用
		Layer3DataFx* layer3Data =
			FxLayer2GetLayer3DataFx(node, interfaceIndex);

		auto scheduleEntry = layer3Data->handoverSchedule->begin();
		UInt8 currentBeamIndex = scheduleEntry->second;

		layer3Data->handoverSchedule->erase(scheduleEntry->first);

		scheduleEntry = layer3Data->handoverSchedule->begin();
		UInt8 nextBeamIndex = scheduleEntry->second;

		// 把当前和信关站的connection设为在切换中
		assert(layer3Data->connectionInfoMap.size() == 1);
		FxConnectionInfo* connection = &(layer3Data->connectionInfoMap.begin()->second);
		connection->state = LAYER3_FX_CONNECTION_HANDOVER;

		// 通知物理层
		int phyIndex =
			FxGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

		PhyFxRrcHandoverRequestNotificationAtYH(node, phyIndex, nextBeamIndex);

		// 设置下次切换的定时器		
		scheduleEntry++;
		if (scheduleEntry != layer3Data->handoverSchedule->end())
			Layer3FxSetTimerForRrc(node, interfaceIndex, MSG_RRC_FX_HandoverScheduleTimerExpired, scheduleEntry->first - getSimTime(node));
	}
	case MSG_RRC_FX_XdHandoverScheduleTimerExpired: 
	{
		PhyFxTxInfo* txInfo = (PhyFxTxInfo*)MESSAGE_ReturnInfo(msg, INFO_TYPE_FxPhyTxInfo);
		Layer3DataFx* layer3Data = FxLayer2GetLayer3DataFx(node, interfaceIndex);
		// Add to ConnectedList
		// 把当前和信关站的connection设为在切换中
		assert(layer3Data->connectionInfoMap.size() == 1);
		FxConnectionInfo* connection = &(layer3Data->connectionInfoMap.begin()->second);
		//connection->state = LAYER3_FX_CONNECTION_XDHANDOVER;
		//Layer3XdHandOverDecision(node, interfaceIndex, txInfo->destRNTI);
		Layer3FxSetTimerForRrc(node, interfaceIndex, MSG_RRC_FX_XdHandoverScheduleTimerExpired, RRC_FX_DEFAULT_XD_HANDOVER_TIME);
	}
	default:
		break;
	}

}


void Layer3FxProcessRrcReconfigurationAtYH(Node * node, int iface, const fxRnti & xgRnti)
{
	Layer3DataFx* layer3Data =
		FxLayer2GetLayer3DataFx(node, iface);

	FxConnectionInfo* connection = Layer3FxGetConnectionInfo(node, iface, xgRnti);
	assert(connection != NULL);

	connection->state = LAYER3_FX_CONNECTION_CONNECTED;

	// 通知物理层回复complete
	int phyIndex =
		FxGetPhyIndexFromMacInterfaceIndex(node, iface);

	PhyFxRrcReconfCompleteNotificationAtYH(node, phyIndex);
}



void Layer3FxPowerOn(Node* node, int interfaceIndex)
{
	Layer3DataFx* layer3Data =
		FxLayer2GetLayer3DataFx(node, interfaceIndex);
	FxStationType stationType =
		FxLayer2GetStationType(node, interfaceIndex);
	int phyIndex =
		FxGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);


	switch (stationType)
	{
	case FX_STATION_TYPE_YH:
	{
		layer3Data->layer3FxState = LAYER3_FX_RRC_IDLE;
		break;
	}
	case FX_STATION_TYPE_XG:
	{
		layer3Data->layer3FxState = LAYER3_FX_POWER_ON;
		break;
	}
	default:
		break;
	}

	SacFxNotifyPowerOn(node, interfaceIndex);
	SlcFxNotifyPowerOn(node, interfaceIndex);
	SmacFxNotifyPowerOn(node, interfaceIndex);
	PhyFxNotifyPowerOn(node, phyIndex);

	if (stationType == FX_STATION_TYPE_YH)
		PhyFxStartXGSearchAtHY(node, phyIndex);


}
//Added by WLH
void Layer3FxDetachUE(
	Node* node, int interfaceIndex, const fxRnti& oppositeRnti) {
	// station type guard
	FX_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
		FX_STATION_TYPE_XG, FX_STRING_STATION_TYPE_XG);
	int phyIndex =
		FxGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

#ifdef LTE_LIB_LOG
	{
		lte::LteLog::InfoFormat(node, interfaceIndex,
			LTE_STRING_LAYER_TYPE_RRC,
			LTE_STRING_CATEGORY_TYPE_LOST_DETECTION","
			LTE_STRING_FORMAT_RNTI","
			"[detach UE]",
			oppositeRnti.nodeId, oppositeRnti.interfaceIndex);
	}
#endif

	// remove route to the RNTI lost
	Layer3FxDeleteRoute(node, interfaceIndex, oppositeRnti);
	// DetachUE process must be ahead of Disconnect and Remove process
	EpcFxAppSend_DetachUE(node, interfaceIndex,
		oppositeRnti, FxLayer2GetRnti(node, interfaceIndex));
	// detach at phy layer
	PhyFxIndicateDisconnectToUe(node, phyIndex, oppositeRnti);
	// remove connection info
	Layer3FxRemoveConnectionInfo(node, interfaceIndex, oppositeRnti);
}
//Added by WLH
void Layer3FxDeleteRoute(
	Node* node, int interfaceIndex, const fxRnti& oppositeRnti)
{
	NodeAddress destAddr;
	NodeAddress destMask;

	FxStationType stationType =
		FxLayer2GetStationType(node, interfaceIndex);
	if (stationType == FX_STATION_TYPE_XG)
	{
		destAddr =
			MAPPING_GetInterfaceAddressForInterface(
				node, oppositeRnti.nodeId, oppositeRnti.interfaceIndex);
		//destMask = MAPPING_GetSubnetMaskForInterface(
		//    node, oppositeRnti.nodeId, oppositeRnti.interfaceIndex);
		destMask = ANY_ADDRESS;
	}
	else
	{
		destAddr = 0;   // default route
		destMask = 0;   // default route
	}
	FxDeleteRouteFromForwardingTable(node, destAddr, destMask);
}
//Added by WLH
void Layer3FxReceiveDataForwarding(
	Node* node,
	int interfaceIndex,
	const FXHandoverParticipator& hoParticipator,
	int bearerId,
	Message* forwardingMsg)
{
	// station type guard
	FX_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
		FX_STATION_TYPE_XG, FX_STRING_STATION_TYPE_XG);

#ifdef Fx_LIB_LOG
	{
		Fx::FxLog::InfoFormat(node, interfaceIndex,
			Fx_STRING_LAYER_TYPE_RRC,
			LAYER3_Fx_MEAS_CAT_HANDOVER","
			Fx_STRING_FORMAT_RNTI","
			"[receive forwarding data list],"
			Fx_STRING_FORMAT_HANDOVER_PARTICIPATOR,
			Fx_INVALID_RNTI.nodeId, Fx_INVALID_RNTI.interfaceIndex,
			hoParticipator.yhRnti.nodeId,
			hoParticipator.yhRnti.interfaceIndex,
			hoParticipator.srcXgRnti.nodeId,
			hoParticipator.srcXgRnti.interfaceIndex,
			hoParticipator.tgtXgRnti.nodeId,
			hoParticipator.tgtXgRnti.interfaceIndex);
	}
#endif

	FxConnectionInfo* connInfo =
		Layer3FxGetConnectionInfo(
			node, interfaceIndex, hoParticipator.yhRnti);

	if (connInfo != NULL &&
		connInfo->state == LAYER3_FX_CONNECTION_HANDOVER)
	{
		SacFxReceiveBuffer(
			node,
			interfaceIndex,
			hoParticipator.yhRnti,
			bearerId,
			forwardingMsg);
	}
	else
	{
		MESSAGE_Free(node, forwardingMsg);
		forwardingMsg = NULL;
	}
}

//Added by WLH
void Layer3FxReceivePathSwitchRequestAck(
	Node* node,
	int interfaceIndex,
	const FXHandoverParticipator& hoParticipator,
	BOOL result)
{
	// station type guard
	FX_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
		FX_STATION_TYPE_XG, FX_STRING_STATION_TYPE_XG);

	// cancel timer TS1WAIT_PATH_SWITCH_REQ_ACK
	Layer3FxCancelTimerForRrc(node, interfaceIndex, hoParticipator.yhRnti,
		MSG_MAC_FX_S1_WaitPathSwitchReqAck);

	BOOL isX2WaitEndMarkerRunning =
		Layer3FxCheckTimerForRrc(
			node, interfaceIndex, hoParticipator.yhRnti,
			MSG_MAC_FX_X2_WaitEndMarker);
	if (isX2WaitEndMarkerRunning == FALSE)
	{
#ifdef Fx_LIB_LOG
		Fx::FxLog::InfoFormat(
			node, interfaceIndex,
			Fx_STRING_LAYER_TYPE_RRC,
			LAYER3_Fx_MEAS_CAT_HANDOVER","
			Fx_STRING_FORMAT_RNTI","
			"HoEnd,"
			Fx_STRING_FORMAT_HANDOVER_PARTICIPATOR","
			"isSuccess=,1",
			Fx_INVALID_RNTI.nodeId,
			Fx_INVALID_RNTI.interfaceIndex,
			hoParticipator.yhRnti.nodeId,
			hoParticipator.yhRnti.interfaceIndex,
			hoParticipator.srcXgRnti.nodeId,
			hoParticipator.srcXgRnti.interfaceIndex,
			hoParticipator.tgtXgRnti.nodeId,
			hoParticipator.tgtXgRnti.interfaceIndex);
#endif
		// update stats
		EpcFxData* epc = EpcFxGetEpcData(node);
		epc->statData.numHandoversCompleted++;
	}

	// check result
	ERROR_Assert(result, "Switch DL path process should success in phase 2");

#ifdef Fx_LIB_LOG
	{
		Fx::FxLog::InfoFormat(node, interfaceIndex,
			Fx_STRING_LAYER_TYPE_RRC,
			LAYER3_Fx_MEAS_CAT_HANDOVER","
			Fx_STRING_FORMAT_RNTI","
			"[receive path switch request ack],"
			Fx_STRING_FORMAT_HANDOVER_PARTICIPATOR","
			"result,%s",
			Fx_INVALID_RNTI.nodeId, Fx_INVALID_RNTI.interfaceIndex,
			hoParticipator.yhRnti.nodeId,
			hoParticipator.yhRnti.interfaceIndex,
			hoParticipator.srcXgRnti.nodeId,
			hoParticipator.srcXgRnti.interfaceIndex,
			hoParticipator.tgtXgRnti.nodeId,
			hoParticipator.tgtXgRnti.interfaceIndex,
			result ? "TRUE" : "FALSE");
	}
#endif

	// send ue context release to src eNB
	EpcFxAppSend_UeContextRelease(
		node, interfaceIndex, hoParticipator);

	// update stats
	EpcFxData* epc = EpcFxGetEpcData(node);
	epc->statData.numPathSwitchRequestAckReceived++;
}

//Added by WLH
void Layer3FxReceiveEndMarker(
	Node* node,
	int interfaceIndex,
	const FXHandoverParticipator& hoParticipator)
{
	// station type guard
	FX_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
		FX_STATION_TYPE_XG, FX_STRING_STATION_TYPE_XG);

#ifdef Fx_LIB_LOG
	{
		Fx::FxLog::InfoFormat(node, interfaceIndex,
			Fx_STRING_LAYER_TYPE_RRC,
			LAYER3_Fx_MEAS_CAT_HANDOVER","
			Fx_STRING_FORMAT_RNTI","
			"[receive end marker],"
			Fx_STRING_FORMAT_HANDOVER_PARTICIPATOR,
			Fx_INVALID_RNTI.nodeId, Fx_INVALID_RNTI.interfaceIndex,
			hoParticipator.yhRnti.nodeId,
			hoParticipator.yhRnti.interfaceIndex,
			hoParticipator.srcXgRnti.nodeId,
			hoParticipator.srcXgRnti.interfaceIndex,
			hoParticipator.tgtXgRnti.nodeId,
			hoParticipator.tgtXgRnti.interfaceIndex);
	}
#endif

	FxConnectedInfo* connInfo = NULL;
	if (FxLayer2GetRnti(node, interfaceIndex) == hoParticipator.srcXgRnti)
	{
		// get connectedInfo
		FxConnectionInfo* handoverInfo = Layer3FxGetHandoverInfo(
			node, interfaceIndex, hoParticipator.yhRnti);
		ERROR_Assert(handoverInfo, "unexpectedd error");
		connInfo = &handoverInfo->connectedInfo;
		ERROR_Assert(connInfo, "unexpectedd error");

		// Notify to PDCP
		for (MapFxRadioBearer::iterator it =
			connInfo->radioBearers.begin();
			it != connInfo->radioBearers.end(); ++it)
		{
			SacFxNotifyEndMarker(
				node, interfaceIndex, hoParticipator.yhRnti, it->first);
		}

		// transfer end marker to tgt eNB
		EpcFxAppSend_EndMarker(
			node,
			interfaceIndex,
			hoParticipator.srcXgRnti,
			hoParticipator.tgtXgRnti,
			hoParticipator);
	}
	else
		if (FxLayer2GetRnti(node, interfaceIndex) == hoParticipator.tgtXgRnti)
		{
			// cancel timer TX2WAIT_END_MARKER
			Layer3FxCancelTimerForRrc(node, interfaceIndex,
				hoParticipator.yhRnti,
				MSG_MAC_FX_X2_WaitEndMarker);

			BOOL isS1WaitPathSwitchReqAck =
				Layer3FxCheckTimerForRrc(
					node, interfaceIndex, hoParticipator.yhRnti,
					MSG_MAC_FX_S1_WaitPathSwitchReqAck);
			if (isS1WaitPathSwitchReqAck == FALSE)
			{
#ifdef Fx_LIB_LOG
				Fx::FxLog::InfoFormat(
					node, interfaceIndex,
					Fx_STRING_LAYER_TYPE_RRC,
					LAYER3_Fx_MEAS_CAT_HANDOVER","
					Fx_STRING_FORMAT_RNTI","
					"HoEnd,"
					Fx_STRING_FORMAT_HANDOVER_PARTICIPATOR","
					"isSuccess=,1",
					Fx_INVALID_RNTI.nodeId,
					Fx_INVALID_RNTI.interfaceIndex,
					hoParticipator.yhRnti.nodeId,
					hoParticipator.yhRnti.interfaceIndex,
					hoParticipator.srcXgRnti.nodeId,
					hoParticipator.srcXgRnti.interfaceIndex,
					hoParticipator.tgtXgRnti.nodeId,
					hoParticipator.tgtXgRnti.interfaceIndex);
#endif
				// update stats
				EpcFxData* epc = EpcFxGetEpcData(node);
				epc->statData.numHandoversCompleted++;
			}

			// get connectedInfo
			connInfo = Layer3FxGetConnectedInfo(
				node, interfaceIndex, hoParticipator.yhRnti);
			ERROR_Assert(connInfo, "unexpectedd error");

			// Notify to PDCP
			for (MapFxRadioBearer::iterator it =
				connInfo->radioBearers.begin();
				it != connInfo->radioBearers.end(); ++it)
			{
				SacFxNotifyEndMarker(
					node, interfaceIndex, hoParticipator.yhRnti, it->first);
			}
		}
		else
		{
			ERROR_Assert(FALSE, "unexpectedd error");
		}

	// update stats
	EpcFxData* epc = EpcFxGetEpcData(node);
	epc->statData.numEndMarkerReceived++;
}

void Layer3FxReceiveUeContextRelease(
	Node* node,
	int interfaceIndex,
	const FXHandoverParticipator& hoParticipator) {

}

void Layer3FxReceiveHoPreparationFailure(
	Node* node,
	int interfaceIndex,
	const FXHandoverParticipator& hoParticipator) {

}
//Added by WLH
void Layer3FxReceiveHoReq(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator)
{
	// station type guard
	FX_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
		FX_STATION_TYPE_XG, FX_STRING_STATION_TYPE_XG);
	// never receive H.O. req of the UE which is handingover from this eNB
	// to another eNB.
	FxConnectionInfo* connInfo = Layer3FxGetConnectionInfo(
		node, interfaceIndex, hoParticipator.yhRnti);
	BOOL isAcceptHoReq = connInfo == NULL ? TRUE : FALSE;
	BOOL admissionResult = Layer3FxAdmissionControl(
		node, interfaceIndex, hoParticipator);

	if (admissionResult == TRUE && isAcceptHoReq == TRUE)
	{
		// get connection config
		Layer2DataFx* layer2 =
			FxLayer2GetLayer2DataFx(node, interfaceIndex);
		FXRrcConnectionReconfiguration reconf;
		reconf.rrcConfig = *layer2->rrcConfig;

		// send HandoverRequestAck to source eNB
		EpcFxAppSend_HandoverRequestAck(
			node, interfaceIndex, hoParticipator, reconf);

		// prepare UE's connection info at handover info
		Layer3FxAddConnectionInfo(
			node, interfaceIndex, hoParticipator.yhRnti,
			LAYER3_FX_CONNECTION_HANDOVER);
		Layer3FxSetFXHandoverParticipator(
			node, interfaceIndex, hoParticipator.yhRnti, hoParticipator);

		// set timer TX2WAIT_SN_STATUS_TRANSFER
		// (this should be after prepare UE's connction info)
		Layer3FxSetTimerForRrc(node, interfaceIndex, hoParticipator.yhRnti,
			MSG_MAC_FX_X2_WaitSnStatusTransfer,
			RRC_FX_DEFAULT_X2_WAIT_SN_STATUS_TRANSFER_TIME);
		// set timer TWAIT_ATTACH_UE_BY_HO
		Layer3FxSetTimerForRrc(node, interfaceIndex, hoParticipator.yhRnti,
			MSG_MAC_FX_WaitAttachUeByHo,
			RRC_FX_DEFAULT_WAIT_ATTCH_UE_BY_HO_TIME);

	}
	else
	{
		EpcFxAppSend_HoPreparationFailure(
			node, interfaceIndex, hoParticipator);
	}

	// update stats
	EpcFxData* epc = EpcFxGetEpcData(node);
	epc->statData.numHandoverRequestReceived++;
}

//Added by WLH
void Layer3FxReceiveHoReqAck(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator,
	const FXRrcConnectionReconfiguration& reconf)
{
	// station type guard
	FX_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
		FX_STATION_TYPE_XG, FX_STRING_STATION_TYPE_XG);

	// cancel timer TRELOCprep
	Layer3FxCancelTimerForRrc(node, interfaceIndex, hoParticipator.yhRnti,
		MSG_MAC_FX_RELOCprep);

#ifdef Fx_LIB_LOG
	{
		Fx::FxLog::InfoFormat(node, interfaceIndex,
			Fx_STRING_LAYER_TYPE_RRC,
			LAYER3_Fx_MEAS_CAT_HANDOVER","
			Fx_STRING_FORMAT_RNTI","
			"[receive handover request ack],"
			Fx_STRING_FORMAT_HANDOVER_PARTICIPATOR,
			Fx_INVALID_RNTI.nodeId, Fx_INVALID_RNTI.interfaceIndex,
			hoParticipator.yhRnti.nodeId,
			hoParticipator.yhRnti.interfaceIndex,
			hoParticipator.srcEnbRnti.nodeId,
			hoParticipator.srcEnbRnti.interfaceIndex,
			hoParticipator.tgtEnbRnti.nodeId,
			hoParticipator.tgtEnbRnti.interfaceIndex);
	}
#endif

	// send reconf to UE
	Layer3FxSendRrcConnReconfInclMobilityControlInfomation(
		node, interfaceIndex, hoParticipator, reconf);

	// prepare for data forwarding
	Layer3FxPrepareDataForwarding(node, interfaceIndex, hoParticipator);

	// send SN status transfer
	Layer3FxSendSnStatusTransfer(node, interfaceIndex, hoParticipator);

	// start data forwarding
	Layer3FxStartDataForwarding(node, interfaceIndex, hoParticipator);

	// update stats
	EpcFxData* epc = EpcFxGetEpcData(node);
	epc->statData.numHandoverRequestAckReceived++;
}

//Added by WLH
void Layer3FxSendSnStatusTransfer(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator)
{
	// station type guard
	FX_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
		FX_STATION_TYPE_XG, FX_STRING_STATION_TYPE_XG);

	// send SN Status Transfer
	FxConnectionInfo* handoverInfo = Layer3FxGetHandoverInfo(
		node, interfaceIndex, hoParticipator.yhRnti);
	ERROR_Assert(handoverInfo, "unexpected error");
	FxConnectedInfo* connInfo = &handoverInfo->connectedInfo;
	MapFxRadioBearer* bearers = &connInfo->radioBearers;
	std::map<int, SacFxSnStatusTransferItem> snStatusMap;
	for (MapFxRadioBearer::iterator it = bearers->begin();
		it != bearers->end(); ++it)
	{
		// get SnStatusTransferItem
		SacFxSnStatusTransferItem snStatusTransferItem =
			SacFxGetSnStatusTransferItem(
				node, interfaceIndex, hoParticipator.yhRnti, it->first);
		// store
		snStatusMap.insert(std::map<int, SacFxSnStatusTransferItem>
			::value_type(it->first, snStatusTransferItem));

#ifdef Fx_LIB_LOG
		{
			Fx::FxLog::InfoFormat(node, interfaceIndex,
				Fx_STRING_LAYER_TYPE_RRC,
				LAYER3_Fx_MEAS_CAT_HANDOVER","
				Fx_STRING_FORMAT_RNTI","
				"[store sn status],"
				Fx_STRING_FORMAT_HANDOVER_PARTICIPATOR","
				"bearerId,%d, nextPdcpRxSn,%d, nextPdcpTxSn,%d",
				Fx_INVALID_RNTI.nodeId, Fx_INVALID_RNTI.interfaceIndex,
				hoParticipator.yhRnti.nodeId,
				hoParticipator.yhRnti.interfaceIndex,
				hoParticipator.srcEnbRnti.nodeId,
				hoParticipator.srcEnbRnti.interfaceIndex,
				hoParticipator.tgtEnbRnti.nodeId,
				hoParticipator.tgtEnbRnti.interfaceIndex,
				it->first,
				snStatusTransferItem.nextPdcpRxSn,
				snStatusTransferItem.nextPdcpTxSn);
		}
#endif
	}

#ifdef Fx_LIB_LOG
	{
		Fx::FxLog::InfoFormat(node, interfaceIndex,
			Fx_STRING_LAYER_TYPE_RRC,
			LAYER3_Fx_MEAS_CAT_HANDOVER","
			Fx_STRING_FORMAT_RNTI","
			"[send sn status transfer],"
			Fx_STRING_FORMAT_HANDOVER_PARTICIPATOR,
			Fx_INVALID_RNTI.nodeId, Fx_INVALID_RNTI.interfaceIndex,
			hoParticipator.yhRnti.nodeId,
			hoParticipator.yhRnti.interfaceIndex,
			hoParticipator.srcEnbRnti.nodeId,
			hoParticipator.srcEnbRnti.interfaceIndex,
			hoParticipator.tgtEnbRnti.nodeId,
			hoParticipator.tgtEnbRnti.interfaceIndex);
	}
#endif

	// send SnStatusTransferItems
	EpcFxAppSend_SnStatusTransfer(
		node, interfaceIndex, hoParticipator, snStatusMap);
}

//Added by WLH
void Layer3FxReceiveSnStatusTransfer(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator,
	std::map<int, SacFxSnStatusTransferItem>& snStatusTransferItem)
{
	// station type guard
	FX_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
		FX_STATION_TYPE_XG, FX_STRING_STATION_TYPE_XG);

	// cancel timer TX2WAIT_SN_STATUS_TRANSFER
	Layer3FxCancelTimerForRrc(node, interfaceIndex, hoParticipator.yhRnti,
		MSG_MAC_FX_X2_WaitSnStatusTransfer);

#ifdef Fx_LIB_LOG
	{
		Fx::FxLog::InfoFormat(node, interfaceIndex,
			Fx_STRING_LAYER_TYPE_RRC,
			LAYER3_Fx_MEAS_CAT_HANDOVER","
			Fx_STRING_FORMAT_RNTI","
			"[receive sn status transfer],"
			Fx_STRING_FORMAT_HANDOVER_PARTICIPATOR,
			Fx_INVALID_RNTI.nodeId, Fx_INVALID_RNTI.interfaceIndex,
			hoParticipator.yhRnti.nodeId,
			hoParticipator.yhRnti.interfaceIndex,
			hoParticipator.srcEnbRnti.nodeId,
			hoParticipator.srcEnbRnti.interfaceIndex,
			hoParticipator.tgtEnbRnti.nodeId,
			hoParticipator.tgtEnbRnti.interfaceIndex);
	}
#endif

	// notify SnStatusTransfer to PDCP
	for (std::map<int, SacFxSnStatusTransferItem>::iterator it =
		snStatusTransferItem.begin(); it != snStatusTransferItem.end();
		++it)
	{
		SacFxSetSnStatusTransferItem(node, interfaceIndex,
			hoParticipator.yhRnti, it->first, it->second);

#ifdef Fx_LIB_LOG
		{
			Fx::FxLog::InfoFormat(node, interfaceIndex,
				Fx_STRING_LAYER_TYPE_RRC,
				LAYER3_Fx_MEAS_CAT_HANDOVER","
				Fx_STRING_FORMAT_RNTI","
				"[notify sn status to PDCP],"
				Fx_STRING_FORMAT_HANDOVER_PARTICIPATOR","
				"bearerId,%d, nextPdcpRxSn,%d, nextPdcpTxSn,%d",
				Fx_INVALID_RNTI.nodeId, Fx_INVALID_RNTI.interfaceIndex,
				hoParticipator.yhRnti.nodeId,
				hoParticipator.yhRnti.interfaceIndex,
				hoParticipator.srcEnbRnti.nodeId,
				hoParticipator.srcEnbRnti.interfaceIndex,
				hoParticipator.tgtEnbRnti.nodeId,
				hoParticipator.tgtEnbRnti.interfaceIndex,
				it->first,
				it->second.nextPdcpRxSn,
				it->second.nextPdcpTxSn);
		}
#endif
	}

	// set isSchedulable flag to ON
	Layer3FxGetConnectedInfo(node, interfaceIndex, hoParticipator.yhRnti)
		->isSchedulable = TRUE;

	// update stats
	EpcFxData* epc = EpcFxGetEpcData(node);
	epc->statData.numSnStatusTransferReceived++;
}

void Layer3FxInitHandoverSchedule(Node * node, UInt32 interfaceIndex, const NodeInput * nodeInput)
{
	Layer3DataFx* layer3Data = FxLayer2GetLayer3DataFx(
		node, interfaceIndex);


	char retChar[MAX_STRING_LENGTH] = { 0 };
	BOOL wasFound = false;

	NodeAddress interfaceAddress =
		MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
			node, node->nodeId, interfaceIndex);
	IO_ReadString(node->nodeId,
		interfaceAddress,
		nodeInput,
		"HANDOVER-SCHEDULE-FILE",
		&wasFound,
		retChar);
	if (wasFound)
	{
		if (strcmp(retChar, "NONE.schedule") == 0) return;
	}

	char    buf[MAX_STRING_LENGTH];

	NodeInput memberInput;
	BOOL retVal;


	IO_ReadCachedFile(node->nodeId, interfaceAddress, nodeInput,
		"HANDOVER-SCHEDULE-FILE", &retVal,
		&memberInput);

	if (retVal == FALSE)
	{
		return;
	}

	layer3Data->handoverSchedule = new map<clocktype, UInt8>;

	BOOL isNodeId;
	UInt8 nodeId;
	UInt8 beamIndex;
	clocktype startTime;

	char nodeIdString[MAX_STRING_LENGTH];
	char beamIndexString[MAX_STRING_LENGTH];
	char startTimeString[MAX_STRING_LENGTH];

	for (int i = 0; i < memberInput.numLines; i++)
	{
		int numValues = sscanf(memberInput.inputStrings[i],
			"%s %s %s",
			nodeIdString,
			beamIndexString,
			startTimeString);

		if (numValues != 3)
		{
			char errStr[MAX_STRING_LENGTH];
			sprintf(errStr, "Application: Wrong configuration format!\n");
			ERROR_Assert(FALSE, errStr);
		}

		nodeId = nodeIdString[0] - '0';
		assert(nodeId == node->nodeId); //temp

		beamIndex = beamIndexString[0] - '0';
		startTime = TIME_ConvertToClock(startTimeString);

		layer3Data->handoverSchedule->insert(make_pair(startTime, beamIndex));
	}
	
	auto scheduleEntry = layer3Data->handoverSchedule->begin();
	if (scheduleEntry->first == 0)
		scheduleEntry++;

	Layer3FxSetTimerForRrc(node, interfaceIndex, MSG_RRC_FX_HandoverScheduleTimerExpired, scheduleEntry->first);

}

//gss xd
//void Layer3XdHandOverDecision(
//	Node *node,
//	UInt32 interfaceIndex,
//	const fxRnti& targetRnti) 
//{
//	Layer3FxAddRoute(node, interfaceIndex, targetRnti);
//	Layer3FxSetTimerForRrc(node, interfaceIndex, MSG_RRC_FX_XdHandoverScheduleTimerExpired, RRC_FX_DEFAULT_XD_HANDOVER_TIME);
//}

//gss xd
void Layer3FxReceiveXdHoReq(
	Node *node,
	UInt32 interfaceIndex,
	const XdHandoverParticipator& xdhoParticipator)
{
	// send HandoverRequest to tgt XG
	EpcXdAppSend_HandoverRequestAck(node, interfaceIndex, xdhoParticipator);
	// update stats
	EpcData* epc = EpcLteGetEpcData(node);
	epc->statData.numXdHandoverRequestReceived++;
	cout << "node" << node->nodeId << " : receive xd handover request at " << getSimTime(node) / (double)SECOND << " second." << endl;
}


//gss xd
void Layer3FxReceiveXdHoReqAck(
	Node *node,
	UInt32 interfaceIndex,
	const XdHandoverParticipator& xdhoParticipator)
{
	EpcXdAppSend_HandoverCommand(node, interfaceIndex, xdhoParticipator);
	// update stats
	EpcData* epc = EpcLteGetEpcData(node);
	epc->statData.numXdHandoverRequestAckReceived++;
	cout << "node" << node->nodeId << " : receive xd handover request acknowledgment at " << getSimTime(node) / (double)SECOND << " second." << endl;
}


//Added by WLH
void Layer3FxSetTimerForRrc(
	Node* node, int interfaceIndex, const fxRnti& rnti, int eventType,
	clocktype delay) {
}

void Layer3FxCancelTimerForRrc(
	Node* node, int interfaceIndex, const fxRnti& rnti, int eventType) {
}

BOOL Layer3FxCheckTimerForRrc(
	Node* node, int interfaceIndex, const fxRnti& rnti, int eventType) {
	return false;
}

FxConnectionInfo* Layer3FxGetHandoverInfo(
	Node* node, int interfaceIndex, const fxRnti& rnti) {
	Layer3DataFx* layer3Data = FxLayer2GetLayer3DataFx(
		node, interfaceIndex);

	MapFxConnectionInfo::iterator it =
		layer3Data->connectionInfoMap.find(rnti);

	if (it != layer3Data->connectionInfoMap.end())
	{
		return &(it->second);
	}

	return NULL;
}

FxConnectedInfo* Layer3FxGetConnectedInfo(
	Node* node, int interfaceIndex, const fxRnti& rnti) {
	FxConnectionInfo* connectionInfo = Layer3FxGetConnectionInfo(
		node, interfaceIndex, rnti);
	ERROR_Assert(connectionInfo, "tried to get ConnectedInfo"
		" of unknown connectonInfo");

	return &(connectionInfo->connectedInfo);
}

BOOL Layer3FxAdmissionControl(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator) {
	return false;
}

void Layer3FxSetFXHandoverParticipator(
	Node* node, int interfaceIndex, const fxRnti& rnti,
	const FXHandoverParticipator& hoParticipator) {

}

void Layer3FxSendRrcConnReconfInclMobilityControlInfomation(
	Node *node,
	UInt32 interfaceIndex,
	const FXHandoverParticipator& hoParticipator,
	const FXRrcConnectionReconfiguration& reconf) {

}

void Layer3FxPrepareDataForwarding(
	Node* node, int interfaceIndex,
	const FXHandoverParticipator& hoParticipator) {

}

void Layer3FxStartDataForwarding(Node* node, int interfaceIndex,
	const FXHandoverParticipator& hoParticipator) {

}


//Added by WLH
BOOL Layer3FxRemoveConnectionInfo(
	Node* node, int interfaceIndex, const fxRnti& rnti)
{
	BOOL isSuccess = FALSE;

	Layer3DataFx* layer3Data =
		FxLayer2GetLayer3DataFx(node, interfaceIndex);
	MapFxConnectionInfo* connInfoMap = &layer3Data->connectionInfoMap;
	FxConnectionInfo* connInfo =
		Layer3FxGetConnectionInfo(node, interfaceIndex, rnti);

	if (connInfo)
	{
#ifdef Fx_LIB_LOG
		{
			Fx::FxLog::InfoFormat(node, interfaceIndex,
				Fx_STRING_LAYER_TYPE_RRC,
				Fx_STRING_CATEGORY_TYPE_CONNECTION_INFO","
				Fx_STRING_FORMAT_RNTI","
				"[remove],state,%d",
				rnti.nodeId, rnti.interfaceIndex,
				connInfo->state);
		}
#endif

		//// cancel all timer
		//Layer3FxCancelAllTimerForRrc(node, interfaceIndex, rnti);

		//switch (Layer3FxGetConnectionState(node, interfaceIndex, rnti))
		//{
		//case LAYER3_FX_CONNECTION_WAITING:
		//{
		//	break;
		//}
		//case LAYER3_FX_CONNECTION_CONNECTED:
		//{
		//	// remove from ConnectedInfoMap & delete radio bearer
		//	Layer3FxDeleteRadioBearer(
		//		node, interfaceIndex, rnti, &connInfo->connectedInfo);

		//	// Notify layer2 scheduler the detach of node
		//	FxLayer2GetScheduler(node, interfaceIndex)->notifyDetach(rnti);

		//	break;
		//}
		//case LAYER3_FX_CONNECTION_HANDOVER:
		//{
		//	// remove from ConnectedInfoMap & delete radio bearer
		//	Layer3FxDeleteRadioBearer(
		//		node, interfaceIndex, rnti, &connInfo->connectedInfo);

		//	break;
		//}
		//default:
		//	ERROR_Assert(FALSE, "unexpected error");
		//}

		//// erase
		//connInfoMap->erase(rnti);

		//isSuccess = TRUE;
	}
	else
	{
		// do nothing
	}

	return isSuccess;

}
//Added by WLH
void Layer3FxNotifyLostDetection(
	Node* node, int interfaceIndex, fxRnti oppositeRnti) {
	FxStationType stationType =
		FxLayer2GetStationType(node, interfaceIndex);

	if (stationType == FX_STATION_TYPE_XG)
	{
		Layer3FxDetachUE(node, interfaceIndex, oppositeRnti);
	}
	else if (stationType == FX_STATION_TYPE_YH)
	{
		// remove route to the RNTI lost
		Layer3FxDeleteRoute(node, interfaceIndex, oppositeRnti);
		// restart UE
		//Layer3FxRestart(node, interfaceIndex);
	}
	else
	{
		ERROR_Assert(FALSE, "invalid station type");
	}
}
