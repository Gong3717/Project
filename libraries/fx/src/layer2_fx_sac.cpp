#include "api.h"
#include "layer2_fx.h"
#include "layer2_fx_slc.h"
#include "layer2_fx_sac.h"
#include "layer3_fx.h"
//#include <fstream>

void SacFxInit(Node* node,
	unsigned int interfaceIndex,
	const NodeInput* nodeInput)
{
	FxSacData* sacData = FxLayer2GetFxSacData(node, interfaceIndex);
	sacData->discardTimerDelay = SAC_FX_DEFAULT_DISCARD_TIMER_DELAY;
	sacData->bufferByteSize = SAC_FX_DEFAULT_BUFFER_BYTE_SIZE;

	// init parameter
	SacFxInitConfigurableParameters(node, interfaceIndex, nodeInput);

	// init stat
	sacData->stats.numPktsFromLowerLayer = 0;
	sacData->stats.numPktsFromUpperLayer = 0;
	sacData->stats.numPktsFromUpperLayerButDiscard = 0;
	sacData->stats.numPktsToLowerLayer = 0;
	sacData->stats.numPktsToUpperLayer = 0;
	// for retransmission buffer
	sacData->stats.numPktsEnqueueRetranamissionBuffer = 0;
	sacData->stats.numPktsDiscardDueToRetransmissionBufferOverflow = 0;
	sacData->stats.numPktsDiscardDueToRlcTxBufferOverflow = 0;
	sacData->stats.numPktsDiscardDueToDiscardTimerExpired = 0;
	sacData->stats.numPktsDequeueRetransmissionBuffer = 0;
	sacData->stats.numPktsDiscardDueToAckReceived;
	// for reordering buffer
	sacData->stats.numPktsEnqueueReorderingBuffer = 0;
	sacData->stats.numPktsDiscardDueToAlreadyReceived = 0;
	sacData->stats.numPktsDequeueReorderingBuffer = 0;
	sacData->stats.numPktsDiscardDueToInvalidSacSnReceived = 0;

	// init state
	sacData->SacFxState = SAC_FX_POWER_OFF;
}

// /**
// FUNCTION::       SacFxInitConfigurableParameters
// LAYER::          MAC FX SAC
// PURPOSE::        Initialize configurable parameters of FX SAC Layer.
//
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
//    + nodeInput:  Input from configuration file
// RETURN::         NULL
// **/
void SacFxInitConfigurableParameters(Node* node,
	unsigned int interfaceIndex,
	const NodeInput* nodeInput)
{
    BOOL wasFound = false;
    char errBuf[MAX_STRING_LENGTH] = {0};
    FxSacData* sacData = FxLayer2GetFxSacData(node, interfaceIndex);
    NodeAddress interfaceAddress =
        MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
            node, node->nodeId, interfaceIndex);
    clocktype retTime = 0;
    int retInt = 0;

	// FX_SAC_STRING_DISCARD_TIMER_DELAY
	IO_ReadTime(node->nodeId,
		interfaceAddress,
		nodeInput,
		FX_SAC_STRING_DISCARD_TIMER_DELAY,
		&wasFound,
		&retTime);

    if (wasFound)
    {
        if (retTime >= FX_SAC_MIN_DISCARD_TIMER_DELAY &&
            retTime < FX_SAC_MAX_DISCARD_TIMER_DELAY)
        {
            sacData->discardTimerDelay = retTime;
        }
        else
        {
            char timeBufMin[MAX_STRING_LENGTH] = {0};
            char timeBufMax[MAX_STRING_LENGTH] = {0};
            TIME_PrintClockInSecond(
                FX_SAC_MIN_DISCARD_TIMER_DELAY, timeBufMin);
            TIME_PrintClockInSecond(
                FX_SAC_MAX_DISCARD_TIMER_DELAY, timeBufMax);
            sprintf(errBuf,
                    "FX-SAC: Discard timer delay "
                    "should be %ssec to %ssec. ",
                    timeBufMin, timeBufMax);
            ERROR_ReportError(errBuf);
        }
    }
    else
    {
        // default
        sacData->discardTimerDelay = SAC_FX_DEFAULT_DISCARD_TIMER_DELAY;
    }

	// FX_SAC_STRING_DISCARD_TIMER_DELAY
	IO_ReadInt(node->nodeId,
		interfaceAddress,
		nodeInput,
		FX_SAC_STRING_BUFFER_BYTE_SIZE,
		&wasFound,
		&retInt);

	if (wasFound)
	{
		if (retInt >= FX_SAC_MIN_BUFFER_BYTE_SIZE &&
			retInt < FX_SAC_MAX_BUFFER_BYTE_SIZE)
		{
			sacData->bufferByteSize = retInt;
		}
		else
		{
			sprintf(errBuf,
				"FX-SAC: Buffer Size "
				"should be %d to %d.",
				FX_SAC_MIN_BUFFER_BYTE_SIZE,
				FX_SAC_MAX_BUFFER_BYTE_SIZE);
			ERROR_ReportError(errBuf);
		}
	}
	else
	{
		// default
		sacData->bufferByteSize = SAC_FX_DEFAULT_BUFFER_BYTE_SIZE;
	}

}

// /**
// FUNCTION::       SacFxFinalize
// LAYER::          MAC FX SAC
// PURPOSE::        Sac finalization function
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
// RETURN::         NULL
// **/
void SacFxFinalize(Node* node, unsigned int interfaceIndex)
{
	FxSacData* sacData = FxLayer2GetFxSacData(node, interfaceIndex);
	SacFxPrintStat(node, interfaceIndex, sacData);

	delete sacData;
}

// /**
// FUNCTION::       SacFxUpperLayerHasPacketToSend
// LAYER::          FX Layer2 SAC
// PURPOSE::        Notify Upper layer has packet to send
// PARAMETERS::
//    + node           : pointer to the network node
//    + interfaceIndex : index of interface
//    + sacData       : pointer of SAC Protpcol Data
// RETURN::         NULL
// **/
void SacFxUpperLayerHasPacketToSend(
	Node* node,
	int interfaceIndex,
	FxSacData* sacData)
{
	Message* sacSdu = NULL;
	NodeAddress curNextHopAddr;
	MacHWAddress destHWAddr, tempDestHWAddr;
	int packetType;
	int networkType;
	TosType priority;
	//ofstream write;
	//write.open("text.txt", ios::app);
	while (MAC_OutputQueueIsEmpty(node, interfaceIndex) == FALSE)
	{
		// Check top packet of IP Queue
		// xyt : why this function is check ?
		MAC_OutputQueueTopPacket(
			node,
			interfaceIndex,
			&sacSdu,
			&tempDestHWAddr,
			&networkType,
			&priority);
		
		// Get top packet of IP Queue
		MAC_OutputQueueDequeuePacketForAPriority(
			node,
			interfaceIndex,
			&priority,
			&sacSdu,
			&curNextHopAddr,
			&destHWAddr,
			&networkType,
			&packetType);
		//write << sacSdu->packetSize << endl;
		
		// Destination RNTI
		NodeAddress dstNodeId =
			MAPPING_GetNodeIdFromInterfaceAddress(
				node,
				curNextHopAddr);
		Int32 dstInterfaceIndex =
			MAPPING_GetInterfaceIndexFromInterfaceAddress(
				node,
				curNextHopAddr);
		fxRnti dstRnti(dstNodeId, dstInterfaceIndex);

		// Add Destination Info
		SmacFxAddDestinationInfo(node,
			interfaceIndex,
			sacSdu,
			dstRnti);


		if (sacData->SacFxState == SAC_FX_POWER_OFF)
		{

			//sacData->stats.numPktsFromUpperLayerButDiscard++;
			MESSAGE_Free(node, sacSdu);
			continue;
		}

		// // do not support broadcast address
		if (MAC_IsBroadcastHWAddress(&destHWAddr) == TRUE)
		{

			//sacData->stats.numPktsFromUpperLayerButDiscard++;
			MESSAGE_Free(node, sacSdu);

			continue;
		}


		FxSacEntity* sacEntity =
			SacFxGetSacEntity(
				node, interfaceIndex, dstRnti, FX_DEFAULT_BEARER_ID);

		if (sacEntity == NULL)
		{

			// pdcpData->stats.numPktsFromUpperLayerButDiscard++;
			MESSAGE_Free(node, sacSdu);

			continue;
		}

		// Process PDCP SDU
		SacFxProcessSacSdu(
			node, interfaceIndex, dstRnti,
			FX_DEFAULT_BEARER_ID, sacSdu);
	}
	//write.close();
}


// /**
// FUNCTION::       SacFxTransferPduFromRlcToUpperLayer
// LAYER::          MAC FX SAC
// PURPOSE::        Send PDU from RLC to SAC
//
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
//    + sacPdu:    SAC PDU
// RETURN::         NULL
// **/
static void SacFxTransferPduFromRlcToUpperLayer(
	Node* node,
	int interfaceIndex,
	Message* sacPdu)
{
	FxSacData* sacData = FxLayer2GetFxSacData(node, interfaceIndex);

	MAC_HandOffSuccessfullyReceivedPacket(
		node, interfaceIndex, sacPdu, ANY_ADDRESS);

	sacData->stats.numPktsToUpperLayer++;
}



// /**
// FUNCTION::       SacFxReceivePduFromRlc
// LAYER::          FX Layer2 SAC
// PURPOSE::        Notify Upper layer has packet to send
// PARAMETERS::
//    + node           : pointer to the network node
//    + interfaceIndex : index of interface
//    + srcRnti        : Source RNTI
//    + bearerId       : Radio Bearer ID
//    + sacPdu        : SAC PDU
// RETURN::         NULL
// **/
void SacFxReceivePduFromSlc(
	Node* node,
	int interfaceIndex,
	const fxRnti& srcRnti,
	const int bearerId,
	Message* sacPdu,
	BOOL isReEstablishment)
{
	FxSacData* sacData = FxLayer2GetFxSacData(node, interfaceIndex);

	sacData->stats.numPktsFromLowerLayer++;

	// event2: receive sac pdu from rlc am
	ERROR_Assert(sacPdu, "sac pdu is NULL");

	MESSAGE_RemoveHeader(
		node, sacPdu, sizeof(FxSacHeader), TRACE_SAC_FX);


	SacFxTransferPduFromRlcToUpperLayer(
		node, interfaceIndex, sacPdu);


	//FxSacEntity* sacEntity =
	//	SacFxGetSacEntity(node, interfaceIndex, srcRnti, bearerId);
	//ERROR_Assert(sacEntity, "sacEntity is NULL");
	//SacFxHoManager& hoManager = sacEntity->hoManager;
	//list<Message*>& reorderingBuffer = hoManager.reorderingBuffer;

	//switch (hoManager.hoStatus) {
	//case SAC_FX_SOURCE_E_NB_IDLE:
	//case SAC_FX_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ:
	//case SAC_FX_SOURCE_E_NB_BUFFERING:
	//case SAC_FX_TARGET_E_NB_WAITING_END_MARKER:
	//case SAC_FX_UE_IDLE:
	//case SAC_FX_UE_BUFFERING: {
	//	// enqueue in reorderingBuffer, reorder buffer,
	//	// dequeue and send msg to upper layer
	//	SacFxProcessReorderingBuffer(node, interfaceIndex, srcRnti,
	//		bearerId, isReEstablishment, sacEntity,
	//		reorderingBuffer, sacPdu);
	//	break;
	//}
	//}
}


// /**
// FUNCTION   :: SacFxNotifyPowerOn
// LAYER      :: FX Layer2 SAC
// PURPOSE    :: Power ON
// PARAMETERS ::
// + node               : Node*        : Pointer to node.
// + interfaceIndex     : int          : Interface index
// RETURN     :: void : NULL
// **/
void SacFxNotifyPowerOn(Node* node, int interfaceIndex)
{
	SacFxSetState(node, interfaceIndex, SAC_FX_POWER_ON);
}


// /**
// FUNCTION   :: SacFxSetState
// LAYER      :: FX Layer2 SAC
// PURPOSE    :: Set state
// PARAMETERS ::
// + node               : Node*        : Pointer to node.
// + interfaceIndex     : int          : Interface index
// + state              : SacFxState : State
// RETURN     :: void : NULL
// **/
void SacFxSetState(Node* node, int interfaceIndex, SacFxState state)
{
	FxSacData* sacData = FxLayer2GetFxSacData(node, interfaceIndex);

	sacData->SacFxState = state;

}



// /**
// FUNCTION   :: SacFxInitSacEntity
// LAYER      :: FX Layer2 SAC
// PURPOSE    :: Init SAC Entity
// PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : int            : Interface index
// + rnti               : const fxRnti& : RNTI
// + sacEntiry         : FxSacEntity* : SAC Entity
// + isTargetEnb        : BOOL           : whether target eNB or not
// RETURN     :: void : NULL
// **/
void SacFxInitSacEntity(
	Node* node,
	int interfaceIndex,
	const fxRnti& rnti,
	FxSacEntity* sacEntity,
	BOOL isTargetEnb)
{
	sacEntity->sacSN = 0;
	sacEntity->lastSacSduSentToNetworkLayerTime = 0;

	switch (FxLayer2GetStationType(node, interfaceIndex)) {
	case FX_STATION_TYPE_XG: {
		sacEntity->hoManager.hoStatus = isTargetEnb
			? SAC_FX_TARGET_E_NB_IDLE : SAC_FX_SOURCE_E_NB_IDLE;
		break;
	}
	case FX_STATION_TYPE_YH: {
		sacEntity->hoManager.hoStatus = SAC_FX_UE_IDLE;
		break;
	}
	case FX_STATION_TYPE_SAT: {
		break;
	}
	default: {
		ERROR_Assert(FALSE, "Invalid node type");
		break;
	}
	}
}






// /**
// FUNCTION::       FxSacEntity::GetAndSetNextSacTxSn
// LAYER::          FX Layer2 SAC
// PURPOSE::        get current "next SAC tx SN"
//                  and set next SAC tx SN to next value
// PARAMETERS::
// + void           :
// RETURN:: UInt16 : current "next SAC tx SN"
// **/
const UInt16 FxSacEntity::GetAndSetNextSacTxSn()
{
	ERROR_Assert(sacSN <= SAC_FX_SEQUENCE_LIMIT,
		"failed to GetAndSetNextSacTxSn()");
	// get current "next SAC tx SN"
	UInt16 returnVal = sacSN;
	// set next SAC tx SN to next value
	sacSN = (sacSN == SAC_FX_SEQUENCE_LIMIT) ? 0 : sacSN + 1;
	return returnVal;
}



//--------------------------------------------------------------------------
// Static function
//--------------------------------------------------------------------------
static
void SacFxPrintStat(
	Node* node,
	int interfaceIndex,
	FxSacData* sacData)
{
	char buf[MAX_STRING_LENGTH] = { 0 };
	sprintf(buf, "Number of packets from Upper Layer = %d",
		sacData->stats.numPktsFromUpperLayer);
	IO_PrintStat(node,
		"Layer2",
		"FX SAC",
		ANY_DEST,
		interfaceIndex,
		buf);

	sprintf(buf, "Number of packets from Upper Layer but discard = %d",
		sacData->stats.numPktsFromUpperLayerButDiscard);
	IO_PrintStat(node,
		"Layer2",
		"FX SAC",
		ANY_DEST,
		interfaceIndex,
		buf);

	sprintf(buf, "Number of packets to Lower Layer = %d",
		sacData->stats.numPktsToLowerLayer);
	IO_PrintStat(node,
		"Layer2",
		"FX SAC",
		ANY_DEST,
		interfaceIndex,
		buf);

	sprintf(buf, "Number of packets from Lower Layer = %d",
		sacData->stats.numPktsFromLowerLayer);
	IO_PrintStat(node,
		"Layer2",
		"FX SAC",
		ANY_DEST,
		interfaceIndex,
		buf);

	sprintf(buf, "Number of packets to Upper Layer = %d",
		sacData->stats.numPktsToUpperLayer);
	IO_PrintStat(node,
		"Layer2",
		"FX SAC",
		ANY_DEST,
		interfaceIndex,
		buf);

	sprintf(buf, "Number of data packets enqueued "
		"in retransmission buffer = %d",
		sacData->stats.numPktsEnqueueRetranamissionBuffer);
	IO_PrintStat(node,
		"Layer2",
		"FX SAC",
		ANY_DEST,
		interfaceIndex,
		buf);

	sprintf(buf, "Number of data packets discarded "
		"due to retransmission buffer overflow = %d",
		sacData->stats.numPktsDiscardDueToRetransmissionBufferOverflow);
	IO_PrintStat(node,
		"Layer2",
		"FX SAC",
		ANY_DEST,
		interfaceIndex,
		buf);

	sprintf(buf, "Number of data packets discarded "
		"due to RLC's tx buffer overflow = %d",
		sacData->stats.numPktsDiscardDueToRlcTxBufferOverflow);
	IO_PrintStat(node,
		"Layer2",
		"FX SAC",
		ANY_DEST,
		interfaceIndex,
		buf);

	sprintf(buf, "Number of data packets discarded "
		"from retransmission buffer due to discard timer expired = %d",
		sacData->stats.numPktsDiscardDueToDiscardTimerExpired);
	IO_PrintStat(node,
		"Layer2",
		"FX SAC",
		ANY_DEST,
		interfaceIndex,
		buf);

	sprintf(buf, "Number of data packets dequeued "
		"from retransmission buffer = %d",
		sacData->stats.numPktsDequeueRetransmissionBuffer);
	IO_PrintStat(node,
		"Layer2",
		"FX SAC",
		ANY_DEST,
		interfaceIndex,
		buf);

	sprintf(buf, "Number of data packets discarded"
		"due to ack received = %d",
		sacData->stats.numPktsDiscardDueToAckReceived);
	IO_PrintStat(node,
		"Layer2",
		"FX SAC",
		ANY_DEST,
		interfaceIndex,
		buf);

	sprintf(buf, "Number of data packets enqueued "
		"in reordering buffer = %d",
		sacData->stats.numPktsEnqueueReorderingBuffer);
	IO_PrintStat(node,
		"Layer2",
		"FX SAC",
		ANY_DEST,
		interfaceIndex,
		buf);

	sprintf(buf, "Number of data packets discarded "
		"due to already received = %d",
		sacData->stats.numPktsDiscardDueToAlreadyReceived);
	IO_PrintStat(node,
		"Layer2",
		"FX SAC",
		ANY_DEST,
		interfaceIndex,
		buf);

	sprintf(buf, "Number of data packets dequeued "
		"from reordering buffer = %d",
		sacData->stats.numPktsDequeueReorderingBuffer);
	IO_PrintStat(node,
		"Layer2",
		"FX SAC",
		ANY_DEST,
		interfaceIndex,
		buf);

	sprintf(buf, "Number of data packets discarded "
		"from reordering buffer = %d",
		sacData->stats.numPktsDiscardDueToInvalidSacSnReceived);
	IO_PrintStat(node,
		"Layer2",
		"FX SAC",
		ANY_DEST,
		interfaceIndex,
		buf);
}



// /**
// FUNCTION::       SacFxProcessSacSdu
// LAYER::          FX Layer2 SAC
// PURPOSE::        Process SAC SDU
//                  + add header
//                  + send to RLC
// PARAMETERS::
//    + node           : pointer to the network node
//    + interfaceIndex : index of interface
//    + dstRnti        : Destination RNTI
//    + bearerId       : Radio Bearer ID
//    + sacSdu        : SAC SDU
// RETURN::         NULL
// **/
static
void SacFxProcessSacSdu(
	Node* node,
	int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId,
	Message* sacSdu)
{
	FxSacData* sacData = FxLayer2GetFxSacData(node, interfaceIndex);

	FxSacEntity* sacEntity =
		SacFxGetSacEntity(node, interfaceIndex, dstRnti, bearerId);

	ERROR_Assert(sacEntity != NULL, "SacFxGetSacEntity return NULL!");

	sacData->stats.numPktsFromUpperLayer++;

	// event1: received first sent sac sdu from upper layer
	ERROR_Assert(sacSdu, "sac sdu is NULL");
	SacFxHoManager& hoManager = sacEntity->hoManager;


	switch (hoManager.hoStatus) {
	case SAC_FX_SOURCE_E_NB_IDLE:
	case SAC_FX_UE_IDLE: {
		UInt32 rlcTxBufferSize = SlcFxSendableByteInTxQueue(
			node, interfaceIndex, dstRnti, bearerId);
		UInt32 sacPduSize =
			MESSAGE_ReturnPacketSize(sacSdu) + sizeof(FxSacHeader);

		// check whether retransmission buffer is full
		if ((hoManager.numEnqueuedRetransmissionMsg
			< SAC_FX_REORDERING_WINDOW)
			&& hoManager.byteEnqueuedRetransmissionMsg
			+ MESSAGE_ReturnPacketSize(sacSdu)
			+ sizeof(FxSacHeader)
			<= sacData->bufferByteSize) {
			// get and set next SAC SN
			UInt16 sn = sacEntity->GetAndSetNextSacTxSn();
			// add sac pdu header
			SacFxAddHeader(node, interfaceIndex, dstRnti,
				bearerId, sacSdu, sn);
			// send SAC PDU to RLC,
			// enqueue it in retransmission buffer
			// and set discard timer
			SacFxSendSacPduToRlc(node, interfaceIndex, dstRnti,
				bearerId, sacSdu, sn,
				hoManager.retransmissionBuffer);
		}
		break;
	}
	}
}

static
FxSacEntity* SacFxGetSacEntity(
	Node* node,
	int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId)
{
	FxSacEntity* ret = NULL;

	FxConnectionInfo* connectionInfo = Layer3FxGetConnectionInfo(
		node, interfaceIndex, dstRnti);

	if (connectionInfo == NULL ||
		connectionInfo->state == LAYER3_FX_CONNECTION_WAITING)
	{
		return NULL;
	}

	MapFxRadioBearer* radioBearers =
		&(connectionInfo->connectedInfo.radioBearers);

	MapFxRadioBearer::iterator itr;
	itr = radioBearers->find(bearerId);
	if (itr != radioBearers->end())
	{
		ret = &((*itr).second.sacEntity);
	}
	else
	{
		char errBuf[MAX_STRING_LENGTH] = { 0 };
		sprintf(errBuf, "Not found Radio Bearer. "
			"Node : %d Interface : %d Bearer : %d",
			dstRnti.nodeId,
			dstRnti.interfaceIndex,
			bearerId);
		ERROR_ReportError(errBuf);
	}

	return ret;
}



// /**
// FUNCTION   :: SacFxReceiveSacStatusReportFromRlc
// LAYER      :: FX Layer2 SAC
// PURPOSE    :: event3
//               cancel such discard timer
//               and dequeue retransmission buffer head SAC PDU
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + srcRnti            : const fxRnti& : source RNTI
// + bearerId           : const int      : Radio Bearer ID
// + rxAckSn            : const UInt16   : SN of received SAC ACK
// RETURN     :: void : NULL
// **/
void SacFxReceiveSacStatusReportFromRlc(
	Node* node,
	const int interfaceIndex,
	const fxRnti& srcRnti,
	const int bearerId,
	const UInt16 rxAckSn)
{
	FxSacEntity* sacEntity = SacFxGetSacEntity(
		node, interfaceIndex, srcRnti, bearerId);
	ERROR_Assert(sacEntity, "sacEntity is NULL");
	SacFxHoManager& hoManager = sacEntity->hoManager;
	switch (hoManager.hoStatus) {
	case SAC_FX_SOURCE_E_NB_IDLE:
	case SAC_FX_TARGET_E_NB_WAITING_END_MARKER:
	case SAC_FX_UE_IDLE: {
		// cancel discard timer
		map<UInt16, Message*>& discardTimer = hoManager.discardTimer;
		map<UInt16, Message*>::iterator discardTimerItr
			= discardTimer.find(rxAckSn);
		//if (discardTimerItr != discardTimer.end()) {
		//    SacFxCancelDiscardTimer(node, interfaceIndex, srcRnti,
		//            bearerId, hoManager, discardTimer, discardTimerItr);
		//} else {
		//    // do nothing
		//    // discard timer is not managed
		//    // after ho execution or discard timer was expired.
		//}

		// dequeue msg until discard timer of retransmission
		// buffer head msg is not managed
		list<Message*>& retransmissionBuffer
			= hoManager.retransmissionBuffer;
		while (!retransmissionBuffer.empty()
			&& (discardTimer.find(FxLayer2GetSacSnFromSacHeader(
				retransmissionBuffer.front()))
				== discardTimer.end())) {
			// dequeue msg from retransmission buffer
			Message* dequeuedMsg = SacFxDequeueRetransmissionBuffer(
				node, interfaceIndex, srcRnti, bearerId, hoManager,
				retransmissionBuffer);
			// update statistics
			++(FxLayer2GetFxSacData(node, interfaceIndex)
				->stats.numPktsDiscardDueToAckReceived);

			// free msg
			FxFreeMsg(node, &dequeuedMsg);
		}
		break;
	}
	case SAC_FX_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ:
	case SAC_FX_SOURCE_E_NB_BUFFERING:
	case SAC_FX_SOURCE_E_NB_FORWARDING:
	case SAC_FX_UE_BUFFERING: {
		// do nothing
		break;
	}
	case SAC_FX_SOURCE_E_NB_FORWARDING_END:
	case SAC_FX_TARGET_E_NB_IDLE:
	case SAC_FX_TARGET_E_NB_FORWARDING:
	case SAC_FX_TARGET_E_NB_CONNECTED: {

		ERROR_Assert(FALSE, "Invalid state transition");
		break;
	}
	default: {
		ERROR_Assert(FALSE, "Invalid hoStatus");
		break;
	}
	}

}

// /**
// FUNCTION::       SacFxAddHeader
// LAYER::          FX Layer2 SAC
// PURPOSE::        add sac header to sac sdu
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : const int      : index of interface
// + dstRnti           : const fxRnti& : destination RNTI
// + bearerId       : const int      : Radio Bearer ID
// + sacSdu        : Message*       : sac sdu
// + sn             : const UInt16   : sac pdu sn
// RETURN:: void : NULL
// **/
static
void SacFxAddHeader(
	Node* node,
	const int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId,
	Message* sacSdu,
	const UInt16 sn)
{
	ERROR_Assert(sacSdu, "sacSdu is NULL");

	MESSAGE_AddHeader(node, sacSdu, sizeof(FxSacHeader), TRACE_SAC_FX);
	FxSacHeader* sacHeader
		= (FxSacHeader*)MESSAGE_ReturnPacket(sacSdu);
	ERROR_Assert(sn >> 12 == 0, "SAC");
	UInt8 high = sn >> 8;
	UInt8 low = sn & 0xFF;

	// Add SN to header
	sacHeader->octet[0] = high;
	sacHeader->octet[1] = low;
	// NOTE : Only SN is supported now.

	FxSacTxInfo* txInfo = (FxSacTxInfo*)MESSAGE_AddInfo(
		node, sacSdu, sizeof(FxSacTxInfo), INFO_TYPE_FxSacTxInfo);
	ERROR_Assert(txInfo != NULL, "MESSAGE_AddInfo is failed.");
	txInfo->txTime = getSimTime(node);
}



// /**
// FUNCTION::       SacFxSetDiscardTimer
// LAYER::          FX Layer2 SAC
// PURPOSE::        set discard Timer
// PARAMETERS::
// + node           : Node*                  : pointer to the network node
// + interfaceIndex : int                    : index of interface
// + dstRnti        : const fxRnti&         : destination RNTI
// + bearerId       : const int              : Radio Bearer ID
// + discardTimer   : map<UInt16, Message*>& : discard timer message
// + sacSdu        : Message*               : sac sdu
// + sn             : const UInt16           : sac pdu sn
// RETURN:: void : NULL
// **/
static
void SacFxSetDiscardTimer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId,
	map<UInt16, Message*>& discardTimer,
	const UInt16 sn)
{
	FxSacData* sacData = FxLayer2GetFxSacData(node, interfaceIndex);

	Message* discardTimerMsg = MESSAGE_Alloc(
		node, MAC_LAYER,
		MAC_PROTOCOL_FX, MSG_SAC_FX_DiscardTimerExpired);
	assert(discardTimerMsg);
	MESSAGE_SetInstanceId(discardTimerMsg, interfaceIndex);

	// add info
	SacFxDiscardTimerInfo tinfo = SacFxDiscardTimerInfo(
		dstRnti, bearerId, sn);
	FxAddMsgInfo(node, interfaceIndex, discardTimerMsg,
		INFO_TYPE_FxSacDiscardTimerInfo,
		tinfo);
	MESSAGE_Send(node, discardTimerMsg,
		sacData->discardTimerDelay);

	// manage discard timer
	if (!discardTimer.insert(
		pair<UInt16, Message*>(sn, discardTimerMsg)).second) {
		ERROR_Assert(FALSE, "discard timer is already managed");
	}
	else {
		// do nothing
	}

}



// /**
// FUNCTION::       SacFxIsUpdatedRxSnStatus
// LAYER::          FX Layer2 SAC
// PURPOSE::        check whether received SAC SN is in correct range,
//                  and update rx SN status
// PARAMETERS::
// + node           : Node*                  : pointer to the network node
// + interfaceIndex : int                    : index of interface
// + srcRnti        : const fxRnti&         : source RNTI
// + bearerId       : const int              : Radio Bearer ID
// + hoManager      : SacFxHoManager&      : ho manager
// + rxSn           : const UInt16           : sn of received message
// + isReEstablishment : BOOL : whether sac pdu received is re-establishment
// RETURN:: BOOL : is updated rx sn status : TRUE
//                  otherwise              : FALSE
// **/
static
BOOL SacFxIsUpdatedRxSnStatus(
	Node* node,
	const int interfaceIndex,
	const fxRnti& srcRnti,
	const int bearerId,
	SacFxHoManager& hoManager,
	const UInt16 rxSn,
	BOOL isReEstablishment)
{
	UInt16& lastSubmittedSacRxSn = hoManager.lastSubmittedSacRxSn;
	UInt16& nextSacRxSn = hoManager.nextSacRxSn;

	if (isReEstablishment) {
		// do nothing
	}
	else if ((rxSn - lastSubmittedSacRxSn > SAC_FX_REORDERING_WINDOW)
		|| ((lastSubmittedSacRxSn - rxSn >= 0)
			&& (lastSubmittedSacRxSn - rxSn
				< SAC_FX_REORDERING_WINDOW))) {
		return FALSE;
	}
	else if (nextSacRxSn - rxSn > SAC_FX_REORDERING_WINDOW) {
		nextSacRxSn = rxSn + 1;
	}
	else if (rxSn >= nextSacRxSn) {
		nextSacRxSn = (rxSn == SAC_FX_SEQUENCE_LIMIT) ? 0 : rxSn + 1;
	}
	else {
		// do nothing
	}

	return TRUE;
}






// /**
// FUNCTION::       SacFxSendSacSduToUpperLayer
// LAYER::          FX Layer2 SAC
// PURPOSE::        send sac sdu to upper layer
// PARAMETERS::
// + node           : Node*                  : pointer to the network node
// + interfaceIndex : int                    : index of interface
// + srcRnti        : const fxRnti&         : source RNTI
// + bearerId       : const int              : Radio Bearer ID
// + sacEntity     : const FxSacEntity&   : sac entity
// + reorderingBuffer : list<Message*>&      : reordering buffer
// + itr : list<Message*>::iterator  : iterator of reordering buffer
// RETURN:: void : NULL
// **/
static
void SacFxSendSacSduToUpperLayer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& srcRnti,
	const int bearerId,
	FxSacEntity* sacEntity,
	list<Message*>& reorderingBuffer,
	list<Message*>::iterator& itr)
{
	ERROR_Assert(sacEntity, "sacEntity is NULL");
	ERROR_Assert(itr != reorderingBuffer.end(),
		"msg is not found in reordering buffer");

	// check whether reordering buffer is empty
	if (reorderingBuffer.empty()) {
		return;
	}
	else {
		// do nothing
	}

	SacFxHoManager& hoManager = sacEntity->hoManager;
	UInt16& lastSubmittedSacRxSn = hoManager.lastSubmittedSacRxSn;

	// dequeue msg from reordering buffer
	Message* dequeuedMsg = SacFxDequeueReorderingBuffer(
		node, interfaceIndex, srcRnti, bearerId, hoManager,
		hoManager.reorderingBuffer, itr);
	// get sn from sac header
	UInt16 dequeuedSn = FxLayer2GetSacSnFromSacHeader(dequeuedMsg);
	// remove sac header
	MESSAGE_RemoveHeader(
		node, dequeuedMsg, sizeof(FxSacHeader), TRACE_SAC_FX);
	// send sac sdu to upper layer
	if (sacEntity->lastSacSduSentToNetworkLayerTime < getSimTime(node)) {
		sacEntity->lastSacSduSentToNetworkLayerTime = getSimTime(node);
		SacFxTransferPduFromRlcToUpperLayer(
			node, interfaceIndex, dequeuedMsg);
	}
	else {
		++sacEntity->lastSacSduSentToNetworkLayerTime;
		MESSAGE_SetLayer(dequeuedMsg, MAC_LAYER, bearerId);
		MESSAGE_SetEvent(dequeuedMsg, MSG_SAC_FX_DelayedSacSdu);
		MESSAGE_SetInstanceId(dequeuedMsg, interfaceIndex);
		MESSAGE_Send(node, dequeuedMsg,
			sacEntity->lastSacSduSentToNetworkLayerTime
			- getSimTime(node));
	}
	// update last submitted sac rx sn
	lastSubmittedSacRxSn = dequeuedSn;

}

// /**
// FUNCTION::       SacFxSendSacPduToRlc
// LAYER::          FX Layer2 SAC
// PURPOSE::        send sac pdu to rlc
// PARAMETERS::
// + node           : Node*                  : pointer to the network node
// + interfaceIndex : int                    : index of interface
// + dstRnti        : const fxRnti&         : Destination RNTI
// + bearerId       : const int              : Radio Bearer ID
// * sacPdu        : Message*               : sac pdu
// * sn             : UInt16&                : sn of sac pdu
// + retransmissionBuffer : list<Message*>&  : retransmission buffer
// RETURN:: void : NULL
// **/
static
void SacFxSendSacPduToRlc(
	Node* node,
	const int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId,
	Message* sacPdu,
	UInt16& sn,
	list<Message*>& retransmissionBuffer)
{
	ERROR_Assert(sacPdu, "sacPdu is NULL");
	FxSacEntity* sacEntity = SacFxGetSacEntity(node, interfaceIndex,
		dstRnti, bearerId);
	ERROR_Assert(sacEntity, "sacEntity is NULL");
	SacFxHoManager& hoManager = sacEntity->hoManager;

	// send sac pdu to slc
	SlcFxReceiveSduFromSac(node, interfaceIndex, dstRnti, bearerId,
		MESSAGE_Duplicate(node, sacPdu));
	FxSacData* sacData = FxLayer2GetFxSacData(node, interfaceIndex);
	++sacData->stats.numPktsToLowerLayer;

	// check whether discard timer is not managed
	ERROR_Assert(hoManager.discardTimer.find(sn)
		== hoManager.discardTimer.end(),
		"discard timer is already managed");

	// xyt : 1222 temp solution
	MESSAGE_Free(node, sacPdu);
	//// enqueue msg in retransmission buffer
	//SacFxEnqueueRetransmissionBuffer(node, interfaceIndex, dstRnti,
	//	bearerId, hoManager, retransmissionBuffer, sacPdu);
	//// set discard timer
	//SacFxSetDiscardTimer(node, interfaceIndex, dstRnti, bearerId,
	//	hoManager.discardTimer, sn);
}



// /**
// FUNCTION::       SacFxProcessReorderingBuffer
// LAYER::          FX Layer2 SAC
// PURPOSE::        discard sac pdu or enqueue in reordering buffer,
//                  reorder reordering bufferor,
//                  and dequeue and send msg to upper layer
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : int            : index of interface
// + srcRnti        : const fxRnti& : Source RNTI
// + bearerId       : const int      : Radio Bearer ID
// + isReEstablishment : BOOL : whether sac pdu received is re-establishment
// + sacEntity     : FxSacEntity* : pointer to sac entity
// + reorderingBuffer : list<Message*>& : reordering buffer
// + sacPdu        : Message*       : sac pdu 
// RETURN:: void : NULL
// **/
static
void SacFxProcessReorderingBuffer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& srcRnti,
	const int bearerId,
	BOOL isReEstablishment,
	FxSacEntity* sacEntity,
	list<Message*>& reorderingBuffer,
	Message* sacPdu)
{
	ERROR_Assert(sacEntity, "sacEntity is NULL");
	SacFxHoManager& hoManager = sacEntity->hoManager;
	// get sn
	const UInt16 rxSn = FxLayer2GetSacSnFromSacHeader(sacPdu);

	// check whether received SAC SN is in correct range,
	// and update rx SN status
	if (SacFxIsUpdatedRxSnStatus(node, interfaceIndex, srcRnti,
		bearerId, hoManager, rxSn, isReEstablishment)) {
		// find insert position
		UInt16& lastSubmittedSacRxSn = hoManager.lastSubmittedSacRxSn;
		list<Message*>::iterator itr = reorderingBuffer.begin();
		list<Message*>::iterator itrEnd = reorderingBuffer.end();
		UInt16 checkSn = SAC_FX_INVALID_SAC_SN;
		if (reorderingBuffer.empty()) {
			// do nothing
		}
		else {
			while (itr != itrEnd &&
				((((checkSn = FxLayer2GetSacSnFromSacHeader(*itr))
					< lastSubmittedSacRxSn)
					? (checkSn + SAC_FX_SEQUENCE_LIMIT + 1)
					: checkSn)
					< ((rxSn < lastSubmittedSacRxSn)
						? (rxSn + SAC_FX_SEQUENCE_LIMIT + 1)
						: rxSn))) {
				++itr;
			}

		}
		// check whether msg of this sn is already enqueued
		// in reordering buffer
		if (rxSn == checkSn) {
			// update statistics
			++(FxLayer2GetFxSacData(node, interfaceIndex)
				->stats.numPktsDiscardDueToAlreadyReceived);
			// discard received sac pdu
			FxFreeMsg(node, &sacPdu);
		}
		else {
			// enqueue msg in reordering buffer
			SacFxEnqueueReorderingBuffer(node, interfaceIndex, srcRnti,
				bearerId, hoManager, reorderingBuffer, itr, sacPdu);
		}

		// check whether this msg is re-establishment
		if (isReEstablishment) {
			// do nothing
		}
		else {
			// get lower bound SN
			checkSn = FxLayer2GetSacSnFromSacHeader(*itr);
			// dequeue msg less than received SN from reordering buffer
			itr = reorderingBuffer.begin();
			while (itr != reorderingBuffer.end()
				&& ((FxLayer2GetSacSnFromSacHeader(*itr)
					!= checkSn))) {
				// dequeue and send msg to upper layer
				SacFxSendSacSduToUpperLayer(
					node, interfaceIndex, srcRnti,
					bearerId, sacEntity, reorderingBuffer, itr);
			}
		}

		// dequeue msg consecutive sn from reordering buffer
		if (itr != reorderingBuffer.end() && (!isReEstablishment
			|| (isReEstablishment
				&& (rxSn == lastSubmittedSacRxSn + 1
					|| rxSn == lastSubmittedSacRxSn
					- SAC_FX_SEQUENCE_LIMIT)))) {
			do {
				// dequeue and send msg to upper layer
				SacFxSendSacSduToUpperLayer(
					node, interfaceIndex, srcRnti,
					bearerId, sacEntity, reorderingBuffer, itr);
			} while ((itr != reorderingBuffer.end())
				&& (((lastSubmittedSacRxSn
					== SAC_FX_SEQUENCE_LIMIT)
					? 0 : lastSubmittedSacRxSn + 1)
					== FxLayer2GetSacSnFromSacHeader(*itr)));
		}
		else {
			// do nothing
		}
	}
	else {
		// free sac sdu
		FxFreeMsg(node, &sacPdu);
		// update statistics
		++(FxLayer2GetFxSacData(node, interfaceIndex)
			->stats.numPktsDiscardDueToInvalidSacSnReceived);
	}
}



// /**
// FUNCTION::       SacFxEnqueueReorderingBuffer
// LAYER::          FX Layer2 SAC
// PURPOSE::        enqueue sac pdu in reordering buffer
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : int            : index of interface
// + srcRnti        : const fxRnti& : Source RNTI
// + bearerId       : const int      : Radio Bearer ID
// + hoManager      : SacFxHoManager& : ho manager
// + buffer         : list<Message*>& : reordering buffer
// + itr ; list<Message*>::iterator& : iterator of reordering buffer
// + msg            : Message*       : sac pdu
// RETURN:: void : NULL
// **/
static
void SacFxEnqueueReorderingBuffer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& srcRnti,
	const int bearerId,
	SacFxHoManager& hoManager,
	list<Message*>& buffer,
	list<Message*>::iterator& itr,
	Message* msg)
{
	ERROR_Assert(msg, "msg is NULL");
	// enqueue msg in reordering buffer
	itr = buffer.insert(itr, msg);
	ERROR_Assert(buffer.size() <= SAC_FX_REORDERING_WINDOW,
		"reordering buffer overflow");
}



// /**
// FUNCTION::       SacFxEnqueueRetransmissionBuffer
// LAYER::          FX Layer2 SAC
// PURPOSE::        enqueue sac pdu in retransmission buffer
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : int            : index of interface
// + rnti           : const fxRnti& : RNTI
// + bearerId       : const int      : Radio Bearer ID
// + hoManager      : SacFxHoManager& : ho manager
// + buffer         : list<Message*>& : retransmission buffer
// + msg            : Message*       : sac pdu
// RETURN:: void : NULL
// **/
void FxFreeMsg(
	Node* node,
	Message** msg)
{
	ERROR_Assert(*msg, "msg is NULL");
	MESSAGE_Free(node, *msg);
	*msg = NULL;
}

static
void SacFxEnqueueRetransmissionBuffer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId,
	SacFxHoManager& hoManager,
	list<Message*>& buffer,
	Message* msg)
{
	ERROR_Assert(msg, "msg is NULL");
	//enqueue in retransmission buffer
	buffer.push_back(msg);
	// update statistics
	++hoManager.numEnqueuedRetransmissionMsg;
	hoManager.byteEnqueuedRetransmissionMsg += MESSAGE_ReturnPacketSize(msg);
	++(FxLayer2GetFxSacData(node, interfaceIndex)
		->stats.numPktsEnqueueRetranamissionBuffer);

}



// /**
// FUNCTION::       SacFxDequeueReorderingBuffer
// LAYER::          FX Layer2 SAC
// PURPOSE::        dequeue sac pdu in reordering buffer
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + srcRnti        : const fxRnti&    : Source RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : SacFxHoManager& : ho manager
// + buffer         : list<Message*>&   : reordering buffer
// + msg            : list<Message*>::iterator : iterator of sac pdu
// RETURN:: Message* : dequeued message
// **/
static
Message* SacFxDequeueReorderingBuffer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& srcRnti,
	const int bearerId,
	SacFxHoManager& hoManager,
	list<Message*>& buffer,
	list<Message*>::iterator& itr)
{
	// dequeue msg from reordering buffer
	Message* dequeuedMsg = *itr;
	ERROR_Assert(dequeuedMsg, "dequeue msg is NULL");
	buffer.erase(itr++);
	return dequeuedMsg;
}



// /**
// FUNCTION::       SacFxDequeueRetransmissionBuffer
// LAYER::          FX Layer2 SAC
// PURPOSE::        dequeue sac pdu in retransmission buffer
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + rnti           : const fxRnti&    : RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : SacFxHoManager& : ho manager
// + buffer         : list<Message*>&   : retransmission buffer
// RETURN:: Message* : dequeued message
// **/
static
Message* SacFxDequeueRetransmissionBuffer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId,
	SacFxHoManager& hoManager,
	list<Message*>& buffer)
{
	// dequeue msg from retransmission buffer
	Message* dequeuedMsg = buffer.front();
	ERROR_Assert(dequeuedMsg, "dequeue msg is NULL");
	buffer.pop_front();
	// update statistics
	--hoManager.numEnqueuedRetransmissionMsg;
	hoManager.byteEnqueuedRetransmissionMsg
		-= MESSAGE_ReturnPacketSize(dequeuedMsg);
	++(FxLayer2GetFxSacData(node, interfaceIndex)
		->stats.numPktsDequeueRetransmissionBuffer);

	return dequeuedMsg;
}











//Added by WLH
void SacFxReceiveBuffer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId,
	Message* forwardedMsg) {

}

//Added by WLH
void SacFxNotifyEndMarker(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId) {

}
//Added by WLH
SacFxSnStatusTransferItem SacFxGetSnStatusTransferItem(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId) {
	return SacFxSnStatusTransferItem();
}
void SacFxSetSnStatusTransferItem(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId,
	SacFxSnStatusTransferItem& item) {

}
