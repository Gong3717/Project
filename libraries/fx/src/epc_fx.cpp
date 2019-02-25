#include "epc_fx.h"
#include "epc_fx_app.h"
#include "network_ip.h"

// /**
// FUNCTION::       EpcFxPrintStat
// LAYER::          EPC
// PURPOSE::        Print EPC statistics
// PARAMETERS::
//    + node:       pointer to the network node
// RETURN::         NULL
// **/
static
void EpcFxPrintStat(
	Node* node)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	if (epc)
	{
		char buf[MAX_STRING_LENGTH] = { 0 };
		sprintf(buf, "Number of handover request sent = %u",
			epc->statData.numHandoverRequestSent);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);

		sprintf(buf, "Number of handover request received = %u",
			epc->statData.numHandoverRequestReceived);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);

		sprintf(buf, "Number of handover request acknowledgement sent = %u",
			epc->statData.numHandoverRequestAckSent);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);

		sprintf(buf, "Number of handover request acknowledgement received = %u",
			epc->statData.numHandoverRequestAckReceived);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);

		sprintf(buf, "Number of SN status transfer sent = %u",
			epc->statData.numSnStatusTransferSent);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);

		sprintf(buf, "Number of SN status transfer received = %u",
			epc->statData.numSnStatusTransferReceived);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);

		sprintf(buf, "Number of path switch request sent = %u",
			epc->statData.numPathSwitchRequestSent);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);

		sprintf(buf, "Number of path switch request received = %u",
			epc->statData.numPathSwitchRequestReceived);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);

		sprintf(buf, "Number of path switch request acknowledgement sent = %u",
			epc->statData.numPathSwitchRequestAckSent);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);

		sprintf(buf, "Number of path switch request acknowledgement received = %u",
			epc->statData.numPathSwitchRequestAckReceived);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);

		sprintf(buf, "Number of end marker sent = %u",
			epc->statData.numEndMarkerSent);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);

		sprintf(buf, "Number of end marker received = %u",
			epc->statData.numEndMarkerReceived);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);

		sprintf(buf, "Number of UE context release sent = %u",
			epc->statData.numUeContextReleaseSent);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);

		sprintf(buf, "Number of UE context release received = %u",
			epc->statData.numUeContextReleaseReceived);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);

		sprintf(buf, "Number of handovers completed = %u",
			epc->statData.numHandoversCompleted);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);

		sprintf(buf, "Number of handovers failed = %u",
			epc->statData.numHandoversFailed);
		IO_PrintStat(node,
			"application",
			"FX handover",
			ANY_DEST,
			-1,
			buf);
	}
}

// /**
// FUNCTION   :: EpcFxGetEpcFxData
// LAYER      :: EPC
// PURPOSE    :: get EPC data
// PARAMETERS ::
// + node      : Node*       : Pointer to node.
// RETURN     :: EpcFxData*    : EPC data on specified node
// **/
EpcFxData*
EpcFxGetEpcData(Node* node)
{
	return node->epcfxData;
}

// /**
// FUNCTION   :: EpcFxInit
// LAYER      :: EPC
// PURPOSE    :: initialize EPC
// PARAMETERS ::
// + node                   : Node*       : Pointer to node.
// + interfaceIndex         : int         : interface index
// + sgwmmeNodeId           : NodeAddress : node ID of SGW/MME
// + sgwmmeInterfaceIndex   : int         : interface index of SGW/MME
// + tagetNodeId      : NodeAddress : Node ID
// RETURN     :: void : NULL
// **/
void
EpcFxInit(Node* node, int interfaceIndex,
	NodeAddress sgwmmeNodeId, int sgwmmeInterfaceIndex)
{
	// already created
	if (node->epcfxData)
	{
		return;
	}

	node->epcfxData = new EpcFxData();

	EpcFxData* epc = EpcFxGetEpcData(node);

	// if sgwmmeNodeId and sgwmmeInterfaceIndex are mine, I'm a SGWMME.
	epc->type = (node->nodeId == sgwmmeNodeId
		&& interfaceIndex == sgwmmeInterfaceIndex) ?
		EPC_FX_STATION_TYPE_SGWMME : EPC_FX_STATION_TYPE_XG;

	// specify my own interface used by epc
	epc->outgoingInterfaceIndex = interfaceIndex;

	// set sgw/mme infomation
	epc->sgwmmeRnti.nodeId = sgwmmeNodeId;
	epc->sgwmmeRnti.interfaceIndex = sgwmmeInterfaceIndex;

	// if SGWMME, init location info
	if (epc->type == EPC_FX_STATION_TYPE_SGWMME)
	{
		epc->locationInfo = new EpcFxLocationInfo();
	}
	else
	{
		// if XG, add route to SGWMME as default gateway
		NodeAddress destAddr = 0;   // default route
		NodeAddress destMask = 0;   // default route
		NodeAddress nextHop =
			MAPPING_GetInterfaceAddressForInterface(
				node, epc->sgwmmeRnti.nodeId, epc->sgwmmeRnti.interfaceIndex);
		int outgoingInterfaceIndex = epc->outgoingInterfaceIndex;
		FxAddRouteToForwardingTable(node, destAddr, destMask,
			nextHop, outgoingInterfaceIndex);
	}
}

// /**
// FUNCTION   :: EpcFxFinalize
// LAYER      :: EPC
// PURPOSE    :: Finalize EPC
// PARAMETERS ::
// + node      : Node*       : Pointer to node.
// RETURN     :: void        : NULL
// **/
void
EpcFxFinalize(Node* node)
{
	/*EpcFxPrintStat(node);*/
	EpcFxData* epc = EpcFxGetEpcData(node);
	if (epc)
	{
		if (epc->type == EPC_FX_STATION_TYPE_SGWMME)
		{
			delete epc->locationInfo;
		}
		delete epc;
	}
}

// /**
// FUNCTION   :: EpcFxProcessAttachUE
// LAYER      :: EPC
// PURPOSE    :: process for message AttachUE
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + yhRnti    : const fxRnti& : YH
// + xgRnti   : const fxRnti& : XG
// RETURN     :: void : NULL
// **/
void
EpcFxProcessAttachUE(Node* node,
	const fxRnti& yhRnti, const fxRnti& xgRnti)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc->type == EPC_FX_STATION_TYPE_SGWMME,
		"This function should be called on SGW/MME");

	EpcFxLocationInfo* locationInfo = epc->locationInfo;

	EpcFxLocationInfo::iterator it = locationInfo->find(xgRnti);
	if (it == locationInfo->end())
	{
		// create new entry for this XG
		EpcFxYhList emptyList;
		locationInfo->insert(EpcFxLocationInfo::value_type(
			xgRnti, emptyList));
	}
	// traverse location info tree
	for (it = locationInfo->begin(); it != locationInfo->end(); it++)
	{
		EpcFxYhList* YhList = &it->second;
		EpcFxYhList::iterator it_yh = YhList->find(yhRnti);

		if (it_yh != YhList->end())
		{
			// the UE is already registered under the XG
			if (it->first == xgRnti)
			{
				// do nothing;
			}
			// the UE is registered under another XG
			else
			{
				// remove
				YhList->erase(it_yh);
				// refresh routing table
				NodeAddress destAddr =
					MAPPING_GetInterfaceAddressForInterface(
						node, yhRnti.nodeId, yhRnti.interfaceIndex);
				//NodeAddress destMask = MAPPING_GetSubnetMaskForInterface(
				//    node, yhRnti.nodeId, yhRnti.interfaceIndex);
				NodeAddress destMask = ANY_ADDRESS;
				FxDeleteRouteFromForwardingTable(node, destAddr, destMask);
			}
		}
		else
		{
			if (it->first == xgRnti)
			{
				// register
				YhList->insert(yhRnti);
				// refresh routing table
				NodeAddress destAddr =
					MAPPING_GetInterfaceAddressForInterface(
						node, yhRnti.nodeId, yhRnti.interfaceIndex);
				//NodeAddress destMask = MAPPING_GetSubnetMaskForInterface(
				//    node, yhRnti.nodeId, yhRnti.interfaceIndex);
				NodeAddress destMask = ANY_ADDRESS;
				NodeAddress nextHop =
					EpcFxAppGetNodeAddressOnEpcSubnet(node, xgRnti.nodeId);
				int outgoingInterfaceIndex = epc->outgoingInterfaceIndex;
				FxAddRouteToForwardingTable(node, destAddr, destMask,
					nextHop, outgoingInterfaceIndex);
				cout << "node" <<node->nodeId<< " : node" << yhRnti.nodeId << " attached to epc successfully at " << getSimTime(node) / (double)SECOND << " second." << endl;
			}
		}
	}
}

// /**
// FUNCTION   :: EpcFxProcessDetachUE
// LAYER      :: EPC
// PURPOSE    :: process for message DetachUE
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + yhRnti    : const fxRnti& : UE
// + xgRnti   : const fxRnti& : eNodeB
// RETURN     :: void : NULL
// **/
void
EpcFxProcessDetachUE(Node* node,
	const fxRnti& yhRnti, const fxRnti& xgRnti)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc->type == EPC_FX_STATION_TYPE_SGWMME,
		"This function should be called on SGW/MME");

	EpcFxLocationInfo* locationInfo = epc->locationInfo;

	EpcFxLocationInfo::iterator it = locationInfo->find(xgRnti);
	if (it == locationInfo->end())
	{
		// xgRnti's entry is not exist.
		// do nothing
		return;
	}
	else
	{
		EpcFxYhList* YhList = &it->second;
		EpcFxYhList::iterator it_yh = YhList->find(yhRnti);
		if (it_yh == YhList->end())
		{
			// yhRnti's entry is not exist.
			// do nothing
			return;
		}
		else
		{
			// remove
			YhList->erase(it_yh);
			// refresh routing table
			NodeAddress destAddr =
				MAPPING_GetInterfaceAddressForInterface(
					node, yhRnti.nodeId, yhRnti.interfaceIndex);
			NodeAddress destMask = ANY_ADDRESS;
			FxDeleteRouteFromForwardingTable(node, destAddr, destMask);
		}
	}
}

// /**
// FUNCTION   :: EpcFxProcessPathSwitchRequest
// LAYER      :: EPC
// PURPOSE    :: process for message DetachUE
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void
EpcFxProcessPathSwitchRequest(
	Node* node,
	const FXHandoverParticipator& hoParticipator)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc->type == EPC_FX_STATION_TYPE_SGWMME,
		"This function should be called on SGW/MME");

	// switch DL path
	BOOL result = EpcFxSwitchDLPath(node, hoParticipator);

	// send PathSwitchRequestAck
	EpcFxSendPathSwitchRequestAck(node, hoParticipator, result);

	// update stats
	epc->statData.numPathSwitchRequestReceived++;
}

// /**
// FUNCTION   :: EpcFxSwitchDLPath
// LAYER      :: EPC
// PURPOSE    :: switch DL path
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: BOOL : result
// **/
BOOL
EpcFxSwitchDLPath(
	Node* node,
	const FXHandoverParticipator& hoParticipator)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc->type == EPC_FX_STATION_TYPE_SGWMME,
		"This function should be called on SGW/MME");

	// result of this process
	BOOL result = TRUE;

	// send end marker to src XG
	EpcFxAppSend_EndMarker(
		node,
		epc->sgwmmeRnti.interfaceIndex,
		epc->sgwmmeRnti,
		hoParticipator.srcXgRnti,
		hoParticipator);

	const fxRnti& yhRnti = hoParticipator.yhRnti;
	EpcFxLocationInfo* locationInfo = epc->locationInfo;
	NodeAddress destAddr;
	NodeAddress destMask;
	NodeAddress nextHop;
	int outgoingInterfaceIndex;

	// src XG
	EpcFxLocationInfo::iterator it;
	it = locationInfo->find(hoParticipator.srcXgRnti);
	ERROR_Assert(it != locationInfo->end(),
		"src XG is not registered in loationInfo");

	// UE
	EpcFxYhList* YhListAtSrcXG = &it->second;
	EpcFxYhList::iterator it_yh =
		YhListAtSrcXG->find(yhRnti);
	ERROR_Assert(it_yh != YhListAtSrcXG->end(),
		"UE is not registered in loationInfo");

	// delete from src XG
	YhListAtSrcXG->erase(it_yh);
	// refresh routing table
	destAddr =
		MAPPING_GetInterfaceAddressForInterface(
			node, yhRnti.nodeId, yhRnti.interfaceIndex);
	destMask = ANY_ADDRESS;
	FxDeleteRouteFromForwardingTable(node, destAddr, destMask);

	// tgt XG (if it doesn't exist, create new info)
	it = locationInfo->find(hoParticipator.tgtXgRnti);
	if (it == locationInfo->end())
	{
		locationInfo->insert(EpcFxLocationInfo::value_type(
			hoParticipator.tgtXgRnti, EpcFxYhList()));
		it = locationInfo->find(hoParticipator.tgtXgRnti);
	}
	EpcFxYhList* YhListAtTgtXG = &it->second;

	// register to tgt XG
	YhListAtTgtXG->insert(yhRnti);
	// refresh routing table
	destAddr =
		MAPPING_GetInterfaceAddressForInterface(
			node, yhRnti.nodeId, yhRnti.interfaceIndex);
	destMask = ANY_ADDRESS;
	nextHop = EpcFxAppGetNodeAddressOnEpcSubnet(
		node, hoParticipator.tgtXgRnti.nodeId);
	outgoingInterfaceIndex = epc->outgoingInterfaceIndex;
	FxAddRouteToForwardingTable(node, destAddr, destMask,
		nextHop, outgoingInterfaceIndex);

	return result;
}

// /**
// FUNCTION   :: EpcFxSendPathSwitchRequestAck
// LAYER      :: EPC
// PURPOSE    :: send path switch request ack
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// + result           : BOOL     : path switch result
// RETURN     :: void : NULL
// **/
void
EpcFxSendPathSwitchRequestAck(
	Node* node,
	const FXHandoverParticipator& hoParticipator,
	BOOL result)
{
	EpcFxData* epc = EpcFxGetEpcData(node);
	ERROR_Assert(epc->type == EPC_FX_STATION_TYPE_SGWMME,
		"This function should be called on SGW/MME");

	// send EPC message PathSwitchRequestAck to tgt XG
	EpcFxAppSend_PathSwitchRequestAck(
		node, epc->sgwmmeRnti.interfaceIndex, hoParticipator, result);
}


