

#include "api.h"
#include "layer2_fx.h"
#include "layer2_fx_slc.h"
#include "layer2_fx_sac.h"
#include <fstream>

// /**
// FUNCTION::       SlcFxCreateAmEntity
// LAYER::          FX LAYER2 SLC
// PURPOSE::        Initialize a SLC AM entity
// PARAMETERS::
//      +node:      Node pointer
//      +iface:     The interface index
//      +entity:    SLC entity
// RETURN::         NULL
// **/


// set header to the message's Info-field.

void SlcFxSetMessageSlcHeaderInfo(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg);

void SlcFxSetMessageSlcHeaderFixedInfo(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg);

void SlcFxSetMessageSlcHeaderSegmentFormatFixedInfo(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg);

void SlcFxSetMessageSlcHeaderExtensionInfo(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg);
void SetNackedRetxBuffer(
	Node* node,
	int iface,
	FxSlcAmEntity& amEntity,
	std::list < FxSlcAmReTxBuffer > &retxBuffer,
	std::multimap < UInt16, FxSlcStatusPduSoPair > &nackSoMap);

void SlcFxSetMessageStatusPduFormatFixedInfo(
	Node* node,
	int iface,
	FxSlcStatusPduFormatFixed& fixed,
	Message* msg);

void SlcFxSetMessageStatusPduFormatExtensionInfo(
	Node* node,
	int iface,
	std::multimap < UInt16, FxSlcStatusPduSoPair > &nackSoMap,
	Message* msg);

void SlcFxSetMessageStatusPduFormatInfo(Node* node,
	int iface,
	Message* msg,
	FxSlcStatusPduFormat& status);

// get header from message's Info-field.

void SlcFxGetMessageSlcHeaderFixedInfo(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg);

void SlcFxGetMessageSlcHeaderSegmentFormatFixedInfo(
	Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg);
void SlcFxPduToSduList(Node* node,
	Message* msg,
	BOOL freeOld,
	std::list < Message* > &msgList);

void SlcFxGetMessageSlcHeaderExtensionInfo(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg);
void SetNackedRetxBuffer(Node* node,
	int iface,
	FxSlcAmEntity& amEntity,
	std::list < FxSlcAmReTxBuffer > &retxBuffer,
	int sn,
	FxSlcStatusPduSoPair nacked);

void SlcFxAmReceiveStatusPdu(Node* node,
	int iface,
	const fxRnti& srcRnti,
	const int bearerId,
	FxSlcAmEntity* amEntity,
	Message* pduMsg);
// get PDU information

int SlcFxGetAmPduSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg);

int SlcFxGetAmPduHeaderSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader);

int SlcFxGetAmPduFormatSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader);

int SlcFxGetAmPduFormatFixedSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader);

int SlcFxGetAmPduLiAndESize(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	int size);

int SlcFxGetAmPduFormatExtensionSize(
	Node* node,
	int iface,
	FxSlcHeader& slcHeader);

int SlcFxGetAmPduSegmentFormatSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader);

int SlcFxGetAmPduSegmentFormatFixedSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader);

int SlcFxGetAmPduSegmentFormatExtensionSize(
	Node* node,
	int iface,
	FxSlcHeader& slcHeader);

int SlcFxGetAmPduDataSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg);

int SlcFxGetAmPduListDataSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msgList);

int SlcFxGetExpectedAmPduSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	BOOL reSegment,
	Message* msgList,
	//Message* addMsg);
	FxSlcAmTxBuffer& txBuffData);

int SlcFxGetExpectedAmPduHeaderSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	BOOL reSegment);

int SlcFxGetExpectedAmPduDataSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msgList,
	Message* addMsg);

void SlcFxAllocAmPduLi(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	UInt16 size);

void SlcFxAddAmPduLi(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg);
static
void SlcFxAmSDUsegmentReassemble(
	Node* node,
	int iface,
	FxSlcAmEntity* amEntity);
static
void SlcFxAmHandleAck(Node* node,
	int iface,
	const fxRnti& srcRnti,
	const int bearerId,
	FxSlcAmEntity* amEntity,
	FxSlcStatusPduFormat rxStatusPDU);

static
Int16 SlcFxAmGetSeqNum(const Message* msg);
static
void SlcFxExtractSduSegmentFromPdu(
	Node* node,
	int iface,
	FxSlcEntity* slcEntity,
	Message* pduMsg);
static
BOOL SlcFxAmPduSegmentReassemble(
	Node* node,
	int iface,
	FxSlcAmEntity* amEntity,
	unsigned int rcvdSeqNum,
	Message* pduMsg);

static
void SlcFxAmCreateStatusPdu(
	Node* node,
	int iface,
	FxSlcAmEntity* amEntity);
static
void SlcFxAmReceivePduFromSmac(
	Node* node,
	int iface,
	const fxRnti& srcRnti,
	const int bearerId,
	FxSlcAmEntity* amEntity,
	std::list < Message* > &msgList);
static
void SlcFxAmDeliverPduToSmac(
	Node* node,
	int iface,
	FxSlcAmEntity* amEntity,
	int pduSize,
	std::list < Message* >* pduList);
static
UInt32 SlcFxAmSeqNumDist(UInt32 sn1, UInt32 sn2);
static
BOOL SlcFxAmSeqNumInWnd(unsigned int sn,
	unsigned int snStart);
void SlcFxDelAmPduHdrObj(FxSlcHeader& slcHeader);
static
void SlcFxAmInitPollRetrunsmitTimer(Node* node,
	int iface,
	FxSlcAmEntity* amEntity);
static
void SlcFxAmInitResetTimer(Node* node,
	int iface,
	FxSlcAmEntity* amEntity,
	FxSlcEntityDirect direct);
static
void SlcFxAmInitPollRetrunsmitTimer(Node* node,
	int iface,
	FxSlcAmEntity* amEntity);
static
BOOL SlcFxSeqNumInWnd(unsigned int sn,
	unsigned int snStart,
	unsigned int wnd,
	unsigned int snBound);
static
BOOL SlcFxAmScreenRcvdPdu(Node* node,
	int iface,
	FxSlcAmEntity* amEntity,
	unsigned int rcvdSeqNum,
	Message* pduMsg);
void SlcFxCpyAmPduHdrObj(const FxSlcHeader& obj, FxSlcHeader& slcHeader);
int SlcFxGetExpectedAmStatusPduSize(FxSlcStatusPduFormat& statusPdu);


// /**
// FUNCTION   :: SlcFxAmGetCPT
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: get CPT from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
UInt8 SlcFxAmGetCPT(Message* msg)
{
	UInt8   ret = 0;

	FxSlcStatusPduFormatFixed* info =
		(FxSlcStatusPduFormatFixed*)MESSAGE_ReturnInfo(msg,
		(unsigned short)INFO_TYPE_FxSlcAmStatusPduPduFormatFixed);

	if (info != NULL)
	{
		ret = (UInt8)info->cpt;
	}
	else
	{
		ERROR_Assert(FALSE, "SlcFxAmGetCPT: no fixed header field!");
	}

	return ret;
}
// /**
// FUNCTION::           DeleteSnRetxBuffer
// LAYER::              FX LAYER2 SLC
// PURPOSE::            Delete ACKed PDU from retransmission buffer
// PARAMETERS::
//  + node       : Node*          : Node pointer
//  + iface      : int            : Interfase index
//  + amEntity   : FxSlcAmEntity : The SLC AM entity
//  + retxBuffer : std::list<FxSlcAmReTxBuffer>& : re-transmission buffer
// RETURN::    void:         NULL
// **/
void DeleteSnRetxBuffer(Node* node,
	int iface,
	FxSlcAmEntity* amEntity,
	std::list < FxSlcAmReTxBuffer > &retxBuffer)
{
	std::list < FxSlcAmReTxBuffer > ::iterator it = retxBuffer.begin();
	while (it != retxBuffer.end())
	{

		if (it->pduStatus == FX_SLC_AM_DATA_PDU_STATUS_ACK)
		{

			amEntity->retxBufNackSize -= it->getNackedSize(node, iface);
			amEntity->retxBufSize -= MESSAGE_ReturnPacketSize(it->message);
			MESSAGE_FreeList(node, it->message);
			SlcFxDelAmPduHdrObj(it->slcHeader);
			it = retxBuffer.erase(it);
		}
		else
		{
			it++;
		}
	}

	return;
}

// FUNCTION   :: SlcFxUnSerializeMessage
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: recover the orignal message from the buffer
// PARAMETERS ::
//  + node      : Node*    : Pointer to node.
//  + buffer    : const char* : The string buffer containing the message
//                           was serialzed into
//  + bufIndex  : int&  : the start position in the buffer pointing
//                           to the message updated to the end of
//                           the message after the unserialization.
// RETURN     :: Message* : Message pointer to be recovered
// **/
Message* SlcFxUnSerializeMessage(Node* node,
	const char* buffer,
	int& bufIndex)
{
	return MESSAGE_Unserialize(node->partitionData, buffer, bufIndex);
}

static
Message* SlcFxInitTimer(Node* node,
	int iface,
	int timerType,
	const fxRnti& rnti,
	UInt32 bearerId,
	FxSlcEntityDirect direction)
{
	Message* timerMsg = NULL;

	// allocate the timer message and send out
	timerMsg = MESSAGE_Alloc(node,
		MAC_LAYER,
		MAC_PROTOCOL_FX,
		timerType);

	MESSAGE_SetInstanceId(timerMsg, (short)iface);

	MESSAGE_InfoAlloc(node,
		timerMsg,
		sizeof(FxSlcTimerInfoHdr));

	char* infoHead = MESSAGE_ReturnInfo(timerMsg);

	FxSlcTimerInfoHdr* infoHdr = (FxSlcTimerInfoHdr*)infoHead;

	infoHdr->rnti = rnti;
	infoHdr->bearerId = (char)bearerId;
	infoHdr->direction = direction;

	return timerMsg;
}
/**ÓÃ¹ý
// FUNCTION::           SlcFxAmInitStatusProhibitTimer
// LAYER::              FX LAYER2 SLC
// PURPOSE::            Init StatusProhibit Timer
// PARAMETERS::
//  + node     : Node*           : Node pointer
//  + iface    : int             : Inteface index
//  + amEntity : FxSlcAmEntity* : The SLC entity
// RETURN::    void:         NULL
// **/
static
void SlcFxAmInitStatusProhibitTimer(Node* node,
	int iface,
	FxSlcAmEntity* amEntity)
{
	FxSlcEntity* slcEntity = (FxSlcEntity*)(amEntity->entityVar);
	if (amEntity->statusProhibitTimerMsg)
	{
		MESSAGE_CancelSelfMsg(node, amEntity->statusProhibitTimerMsg);
		amEntity->statusProhibitTimerMsg = NULL;
	}
	amEntity->statusProhibitTimerMsg =
		SlcFxInitTimer(node,
			iface,
			MSG_SLC_FX_StatusProhibitTimerExpired,
			slcEntity->oppositeRnti,
			slcEntity->bearerId,
			FX_SLC_ENTITY_RX);
	amEntity->waitExprioryTStatusProhibitFlg = FALSE;
	MESSAGE_Send(node,
		amEntity->statusProhibitTimerMsg,
		GetFxSlcConfig(node, iface)->tStatusProhibit *
		MILLI_SECOND);
}

// /**
// FUNCTION::           SlcFxSDUsegmentReassemble
// LAYER::              FX LAYER2 SLC
// PURPOSE::            Insert the SDU segments extracted
//                      from the newly received PDU
//                      into the SDU segment list (receiving buffer).
//                      After this,
//                      re-assembly shall take place if possible.
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         The interface index
//      +slcEntity:     The SLC entity
// RETURN::             NULL
// **/
static
void SlcFxSDUsegmentReassemble(
	Node* node,
	int iface,
	FxSlcEntity* slcEntity)
{
	FxSlcEntityType mode = slcEntity->entityType;
	if (mode == FX_SLC_ENTITY_AM)
	{
		SlcFxAmSDUsegmentReassemble(
			node,
			iface,
			(FxSlcAmEntity*)slcEntity->entityData);
	}
}
// /**
// FUNCTION   :: SlcFxAmGetRFBit
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: get RFbit data from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
BOOL SlcFxAmGetRFBit(Message* msg)
{
	BOOL   ret = FALSE;

	FxSlcAmPduFormatFixed* info =
		(FxSlcAmPduFormatFixed*)MESSAGE_ReturnInfo(msg,
		(unsigned short)INFO_TYPE_FxSlcAmPduFormatFixed);

	if (info != NULL)
	{
		ret = info->resegmentationFlag;
	}
	else
	{
		ERROR_Assert(FALSE, "SlcFxAmGetRFBit: no fixed header field!");
	}

	return ret;
}
void SlcFxSerializeMessage(Node* node,
	Message* msg,
	std::string& buffer)
{
	MESSAGE_Serialize(node, msg, buffer);
}

Message* SlcFxSduListToPdu(Node* node, Message* msgList)
{
	Message* nextMsg = msgList;
	Message* lastMsg;
	Message* newMsg;

	std::string buffer;
	int numMsgs = 0;
	int actualPktSize = 0;
	while (nextMsg != NULL)
	{
		actualPktSize += MESSAGE_ReturnPacketSize(nextMsg);

		SlcFxSerializeMessage(node, nextMsg, buffer);

		lastMsg = nextMsg;
		nextMsg = nextMsg->next;

		MESSAGE_Free(node, lastMsg);
		numMsgs++;
	}
	newMsg = MESSAGE_Alloc(node, 0, 0, 0);
	ERROR_Assert(newMsg != NULL, "SLC FX: Out of memory!");
	MESSAGE_PacketAlloc(node, newMsg, 0, TRACE_SLC_FX);
	int tmpSize = MESSAGE_ReturnVirtualPacketSize(newMsg);
	if (tmpSize > 0)
	{
		MESSAGE_RemoveVirtualPayload(node, newMsg, tmpSize);
	}
	MESSAGE_AddVirtualPayload(node, newMsg, actualPktSize);
	char* info = MESSAGE_AddInfo(node,
		newMsg,
		(int)buffer.size(),
		INFO_TYPE_FxSlcAmSduToPdu);
	ERROR_Assert(info != NULL, "MESSAGE_AddInfo is failed.");
	memcpy(info, buffer.data(), buffer.size());

	return newMsg;
}


/*
// FUNCTION   :: SlcFxAmNotifySAcOfSacStatusReport
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: if positive acknowledgements have been received for all AMD
//               PDUs associated with a transmitted SLC SDU, send an
//               indication to the upper layers of successful delivery of
//               the SLC SDU.
// PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const fxRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// + slcAmEntity        : FxSlcAmEntity : The SLC AM entity
// + receivedSlcAckSn   : const Int32    : SN of received SLC ACK
// RETURN     :: void     : NULL

static
void SlcFxAmNotifySAcOfSacStatusReport(
Node* node,
const int interfaceIndex,
const fxRnti& rnti,
const int bearerId,
FxSlcAmEntity* slcAmEntity,
const Int32 receivedSlcAckSn);*/
static
void SlcFxAmUpperLayerHasSduToSend(
	Node* node,
	int iface,
	FxSlcAmEntity* amEntity,
	Message* sduMsg);


static
FxSlcData* SlcFxGetSubLayerData(Node* node, int iface);
static



void SlcFxCreateAmEntity(Node* node,
	int iface,
	FxSlcEntity* entity)
{
	ERROR_Assert(entity != NULL,
		"SlcFxCreateAmEntity: entity == NULL");
	ERROR_Assert(entity->entityType == FX_SLC_ENTITY_AM,
		"SlcFxCreateAmEntity: entity->entityType != FX_SLC_ENTITY_AM");

	switch (entity->entityType)
	{
	case FX_SLC_ENTITY_AM:
	{
		FxSlcAmEntity* amEntity = new FxSlcAmEntity();
		
		// xyt 1229 设置buffer大小
		FxSlcData* fxSlcData = FxLayer2GetFxSlcData(node, iface);
		amEntity->setTxBufferSize(fxSlcData->txBufferMaxSize);

		entity->entityData = (void*)amEntity;
		ERROR_Assert(entity->entityData != NULL,
			"SLC FX: Out of memory!");

		amEntity->entityVar = NULL;
		break;
	}
	default:
		ERROR_Assert(FALSE,
			"SLC FX: entity type Error!");
	}


}


//**ÓÃ¹ý
// FUNCTION::           SlcFxAmInitReoderingTimer
// LAYER::              FX LAYER2 SLC
// PURPOSE::            Init Reordering Timer
// PARAMETERS::
//  + node     : Node*           : Node pointer
//  + iface    : int             : Inteface index
//  + amEntity : FxSlcAmEntity* : The SLC entity
// RETURN::    void:         NULL
// **/
static
void SlcFxAmInitReoderingTimer(Node* node,
	int iface,
	FxSlcAmEntity* amEntity)
{
	FxSlcEntity* slcEntity = (FxSlcEntity*)(amEntity->entityVar);
	if (amEntity->reoderingTimerMsg)
	{
		MESSAGE_CancelSelfMsg(node, amEntity->reoderingTimerMsg);
		amEntity->reoderingTimerMsg = NULL;
	}
	amEntity->reoderingTimerMsg =
		SlcFxInitTimer(node,
			iface,
			MSG_SLC_FX_ReorderingTimerExpired,
			slcEntity->oppositeRnti,
			slcEntity->bearerId,
			FX_SLC_ENTITY_RX);
	MESSAGE_Send(node,
		amEntity->reoderingTimerMsg,
		GetFxSlcConfig(node, iface)->tReordering * MILLI_SECOND);
};
// /**
// FUNCTION::       SlcFxAmDecSN
// LAYER::          FX Layer2 SLC
// PURPOSE::        increment a sequence number (AM)
// PARAMETERS::
//  + seqNum : UInt32: The sequence number which is renewed
// RETURN::   void:      NULL
// **/
inline
void SlcFxAmDecSN(UInt32& seqNum)
{
	if (seqNum == 0)
	{
		seqNum += FX_SLC_AM_SN_UPPER_BOUND;
	}
	--seqNum;
}
inline
void SlcFxAmIncSN(UInt32& seqNum)
{
	++seqNum;
	seqNum %= FX_SLC_AM_SN_UPPER_BOUND;
}

// /**
// FUNCTION   :: SlcFxAmGetNacklist
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: get NACK list from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
//  + nackSoMap : std::multimap< UInt16, FxSlcStatusPduSoPair >* : NACK-list
// RETURN     :: BOOL     : NULL
// **/
void SlcFxAmGetNacklist(Message* msg,
	std::multimap < UInt16, FxSlcStatusPduSoPair >* nackSoMap)
{
	FxSlcStatusPduFormatExtensionInfo* info =
		(FxSlcStatusPduFormatExtensionInfo*)MESSAGE_ReturnInfo(
			msg,
			(unsigned short)INFO_TYPE_FxSlcAmStatusPduPduFormatExtension);

	if ((info != NULL) && (info->nackInfoSize > 0))
	{
		FxSlcStatusPduFormatExtension nackInfoWork;
		for (size_t i = 0; i < info->nackInfoSize; i++)
		{
			memcpy(&nackInfoWork,
				&(info->nackInfo) + i,
				sizeof(FxSlcStatusPduFormatExtension));
			(*nackSoMap).insert(
				pair < UInt16, FxSlcStatusPduSoPair >
				(nackInfoWork.nackSn, nackInfoWork.soOffset));
		}
	}
	else
	{
		nackSoMap->clear();
	}
}

// /**
// FUNCTION::           SlcFxAmAddSduBuffer
// LAYER::              FX LAYER2 SLC
// PURPOSE::            Insert the SDU to buffer for send SAC.
// PARAMETERS::
//      +node:       Node pointer
//      +iface:      The interface index
//      +amEntity:   The AM receiving entity
//      +seqNum:     Sequens number
//      +rcvMsgList: The message list to be inserted
// RETURN::             NULL
// **/
static
void SlcFxAmAddSduBuffer(Node* node,
	int iface,
	FxSlcAmEntity* amEntity,
	unsigned int seqNum,
	std::list < Message* > rcvMsgList)
{
	std::list < Message* > ::iterator it = rcvMsgList.begin();
	while (it != rcvMsgList.end())
	{
		std::pair < unsigned int, Message* > sdu(seqNum, (*it));
		amEntity->rcvSlcSduList.insert(sdu);
		++it;
	}
}
static
FxSlcEntity* SlcFxGetSlcEntity(
	Node* node,
	int iface,
	const fxRnti& destRnti,
	const int bearerId)
{
	FxSlcEntity* ret = NULL;

	FxConnectionInfo* connectionInfo = Layer3FxGetConnectionInfo(node, iface, destRnti);

	if (connectionInfo == NULL ||
		connectionInfo->state == LAYER3_FX_CONNECTION_WAITING)
	{
		return NULL;
	}

	MapFxRadioBearer* radioBearers = &(connectionInfo->connectedInfo.radioBearers);

	MapFxRadioBearer::iterator itr;
	itr = radioBearers->find(bearerId);
	if (itr != radioBearers->end())
	{
		ret = &((*itr).second.slcEntity);
	}
	else
	{
		char errBuf[MAX_STRING_LENGTH] = { 0 };
		sprintf(errBuf, "Not found Radio Bearer. "
			"Node : %d Interface : %d Bearer : %d",
			destRnti.nodeId,
			destRnti.interfaceIndex,
			bearerId);
		ERROR_ReportError(errBuf);
	}

	return ret;
}

// /**
// FUNCTION   :: SlcFxAmGetLastSegflag
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: get Last segment flag from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
BOOL SlcFxAmGetLastSegflag(Message* msg)
{
	BOOL   ret = 0;

	FxSlcAmPduSegmentFormatFixed* info =
		(FxSlcAmPduSegmentFormatFixed*)MESSAGE_ReturnInfo(msg,
		(unsigned short)INFO_TYPE_FxSlcAmPduSegmentFormatFixed);

	if (info != NULL)
	{
		ret = info->lastSegmentFlag;
	}
	else
	{
		// do nothing
	}
	return ret;
}
// /**
// FUNCTION   :: SlcFxAmGetAckSn
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: get ACK-SN from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
Int16 SlcFxAmGetAckSn(Message* msg)
{
	Int16   ret = FX_SLC_INVALID_SEQ_NUM;

	FxSlcStatusPduFormatFixed* info =
		(FxSlcStatusPduFormatFixed*)MESSAGE_ReturnInfo(msg,
		(unsigned short)INFO_TYPE_FxSlcAmStatusPduPduFormatFixed);

	if (info != NULL)
	{
		ret = (Int16)info->ackSn;
	}
	else
	{
		ERROR_Assert(FALSE, "SlcFxAmGetAckSn: no fixed header field!");
	}

	return ret;
}

// /**
// FUNCTION::           findSnRetxBuffer
// LAYER::              FX LAYER2 SLC
// PURPOSE::            Insert fragmented PDU into transmission buffer
// PARAMETERS::
//  + node       : Node* : Node pointer
//  + ifac       : int   : interface id
//  + retxBuffer : std::list<FxSlcAmReTxBuffer>& : re-transmittion buffer
//  + sn         : int   : Sequens number
//  + it         : std::list<FxSlcAmReTxBuffer>::iterator&
//                       : re-transmittion buffer's itarator
// RETURN::    void:         NULL
// **/
BOOL findSnRetxBuffer(Node* node,
	int iface,
	std::list < FxSlcAmReTxBuffer > &retxBuffer,
	int sn,
	std::list < FxSlcAmReTxBuffer > ::iterator& it)
{
	for (it = retxBuffer.begin();
		it != retxBuffer.end();
		it++)
	{
		if (it->slcHeader.fixed->seqNum == sn)
		{
			return TRUE;
		}
	}

	return FALSE;
}

// /**
// FUNCTION   :: SlcFxAmGetSegOffset
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: get segment offset from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
UInt16 SlcFxAmGetSegOffset(Message* msg)
{
	UInt16   ret = 0;

	FxSlcAmPduSegmentFormatFixed* info =
		(FxSlcAmPduSegmentFormatFixed*)MESSAGE_ReturnInfo(msg,
		(unsigned short)INFO_TYPE_FxSlcAmPduSegmentFormatFixed);

	if (info != NULL)
	{
		ret = info->segmentOffset;
	}
	else
	{
		// do nothing
	}

	return ret;
}
// /**
// FUNCTION   :: SlcFxAmGetLIlist
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: get LI-list data from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
void SlcFxAmGetLIlist(Message* msg, std::vector < UInt16 >* liList)
{
	FxSlcAmPduFormatExtensionMsgInfo* info =
		(FxSlcAmPduFormatExtensionMsgInfo*)MESSAGE_ReturnInfo(msg,
		(unsigned short)INFO_TYPE_FxSlcAmPduFormatExtension);

	if ((info != NULL) && (info->numLengthIndicator > 0))
	{
		UInt16 tmpLiData;
		for (size_t i = 0; i < info->numLengthIndicator; i++)
		{
			memcpy(&tmpLiData,
				&(info->lengthIndicator) + i,
				sizeof(UInt16));
			liList->push_back(tmpLiData);
		}
	}
	else
	{
		liList->clear();
	}
}
// /**ÓÃ¹ý
// FUNCTION::       SlcFxSeqNumLessChkInWnd
// LAYER::          FX Layer2 SLC
// PURPOSE::        Return whether a sequence number A is less than B,
//                  within the window.
//                  snA is allow out of the window, that case is allways TRUE
//                  Test is, snA < snB.(in window)
// PARAMETERS::
//  + snA     : unsigned int : The sequence number A to be tested
//  + snB     : unsigned int : The sequence number B to be tested
//  + wStart  : unsigned int : The sequence number of window starting point
//  + wEnd    : unsigned int : The sequence number of window end point
//  + snBound : unsigned int : The upperbound of sequence number
// RETURN::    void:     BOOL
// **/
//
static
BOOL SlcFxSeqNumLessChkInWnd(unsigned int snA,
	unsigned int snB,
	unsigned int wStart,
	unsigned int wEnd,
	unsigned int snBound)
{
	//// Case (1)-(5) : snA is in the window
	//(1)
	//                  A,B
	//  0<===========================================>1023
	//       S<----------window---------->E
	//
	//(2)
	//                                        A,B
	//  0<===========================================>1023
	//  ---->E                            S<----------
	//
	//(3)
	//     A,B
	//  0<===========================================>1023
	//  -------->E                         S<---------
	//
	//(4)
	//     B                                      A
	//  0<===========================================>1023
	//  ------>E                            S<--------
	//
	//(5)
	//     A                                    B
	//  0<===========================================>1023
	//  ------>E                            S<--------

	//// Case (6),(7) : snA is out of the window
	//(6)
	//     A                 or                 A
	//  0<===========================================>1023
	//       S<----------window---------->E
	//(7)
	//                       A
	//  0<===========================================>1023
	//  ------>E                            S<--------

	if (wStart > wEnd) {
		if (wEnd >= snA && wStart <= snB) {    // case (5)
			return FALSE;
		}
		else if (wEnd >= snB && wStart <= snA) {    // case (4)
			return TRUE;
		}
		else if ((wEnd >= snA && wEnd >= snB)        // case (3)
			|| (wStart <= snB && wStart <= snA)) { // case (2)
			return (snA < snB);
		}
		else if (wEnd < snA && wStart > snA) {       // case (7)
			return TRUE;
		}
	}
	else if (wEnd < snA || wStart > snA) {             // case (6)
		return TRUE;
	}
	return (snA < snB);                                  // case (1)
}
// /**
// FUNCTION::       SlcFxSeqNumInRange
// LAYER::          FX Layer2 SLC
// PURPOSE::        Return whether a sequence number is within the range
//                  test is, rStart <= sn < rEnd.
// PARAMETERS::
//  + sn     : unsigned int : The sequence number to be tested
//  + rStart : unsigned int : The sequence number of range starting point
//  + rEnd   : unsigned int : The sequence number of range end point
// RETURN::    void:     BOOL
// **/
//
static
BOOL SlcFxSeqNumInRange(unsigned int sn,
	unsigned int rStart,
	unsigned int rEnd)
{
	if (rStart <= rEnd)
	{
		return (rStart <= sn && sn < rEnd);
	}
	else
	{
		return (rStart <= sn || sn < rEnd);
	}
}
static
void SlcFxInitConfigurableParameters(Node* node,
	int interfaceindex,
	const NodeInput* nodeInput)
{
	BOOL wasFound = false;
	char errBuf[MAX_STRING_LENGTH] = { 0 };
	FxStationType stationType =
		FxLayer2GetStationType(node, interfaceindex);

	clocktype retTime = 0;
	int retInt = 0;

	NodeAddress interfaceAddress =
		MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
			node, node->nodeId, interfaceindex);

	FxSlcConfig* slcConfig = GetFxSlcConfig(node, interfaceindex);

	// eNB settings
	if (stationType == FX_STATION_TYPE_XG)
	{
		// FxRrcConfig settings
		// FX_SLC_STRING_MAX_RETX_THRESHOLD
		IO_ReadInt(node->nodeId,
			interfaceAddress,
			nodeInput,
			FX_SLC_STRING_MAX_RETX_THRESHOLD,
			&wasFound,
			&retInt);

		if (wasFound)
		{
			if (FX_SLC_MIN_MAX_RETX_THRESHOLD <= retInt &&
				retInt <= FX_SLC_MAX_MAX_RETX_THRESHOLD)
			{
				slcConfig->maxRetxThreshold = (UInt8)retInt;
			}
			else
			{
				sprintf(errBuf,
					"FX-SLC: Max Retx Threshold "
					"should be %d to %d. "
					"are available.",
					FX_SLC_MIN_MAX_RETX_THRESHOLD,
					FX_SLC_MAX_MAX_RETX_THRESHOLD);
				ERROR_ReportError(errBuf);
			}
		}
		else
		{
			// default
			slcConfig->maxRetxThreshold = FX_SLC_DEFAULT_MAX_RETX_THRESHOLD;
		}

		// FX_SLC_STRING_POLL_PDU
		IO_ReadInt(node->nodeId,
			interfaceAddress,
			nodeInput,
			FX_SLC_STRING_POLL_PDU,
			&wasFound,
			&retInt);

		if (wasFound)
		{
			if (FX_SLC_MIN_POLL_PDU <= retInt) {
				if (retInt <= FX_SLC_MAX_POLL_PDU)
				{
					slcConfig->pollPdu = retInt;
				}
				else
				{
					slcConfig->pollPdu = FX_SLC_INFINITY_POLL_PDU;
				}
			}
			else
			{
				sprintf(errBuf,
					"FX-SLC: Poll pdu "
					"should be %d to Infinity. "
					"are available.",
					FX_SLC_MIN_POLL_PDU);
				ERROR_ReportError(errBuf);
			}
		}
		else
		{
			// default
			slcConfig->pollPdu = FX_SLC_DEFAULT_POLL_PDU;
		}

		// FX_SLC_STRING_POLL_BYTE
		IO_ReadInt(node->nodeId,
			interfaceAddress,
			nodeInput,
			FX_SLC_STRING_POLL_BYTE,
			&wasFound,
			&retInt);

		if (wasFound)
		{
			if (FX_SLC_MIN_POLL_BYTE <= retInt) {
				if (retInt <= FX_SLC_MAX_POLL_BYTE)
				{
					slcConfig->pollByte = retInt;
				}
				else
				{
					slcConfig->pollByte = FX_SLC_INFINITY_POLL_BYTE;
				}
			}
			else
			{
				sprintf(errBuf,
					"FX-SLC: Poll Byte "
					"should be %d to Infinity. "
					"are available.",
					FX_SLC_MIN_POLL_BYTE);
				ERROR_ReportError(errBuf);
			}
		}
		else
		{
			// default
			slcConfig->pollByte = FX_SLC_DEFAULT_POLL_BYTE;
		}

		// FX_SLC_STRING_POLL_RETRANSMIT
		IO_ReadTime(node->nodeId,
			interfaceAddress,
			nodeInput,
			FX_SLC_STRING_POLL_RETRANSMIT,
			&wasFound,
			&retTime);

		if (wasFound)
		{
			if (FX_SLC_MIN_POLL_RETRANSMIT <= retTime &&
				retTime <= FX_SLC_MAX_POLL_RETRANSMIT)
			{
				slcConfig->tPollRetransmit =
					(int)(retTime / MILLI_SECOND);
			}
			else
			{
				sprintf(errBuf,
					"FX-SLC: Poll Retransmit "
					"should be set to more than %" TYPES_64BITFMT "dMS.",
					FX_SLC_MIN_POLL_RETRANSMIT / MILLI_SECOND);
				ERROR_ReportError(errBuf);
			}
		}
		else
		{
			// default
			slcConfig->tPollRetransmit = FX_SLC_DEFAULT_POLL_RETRANSMIT;
		}

		// FX_SLC_STRING_T_REORDERING
		IO_ReadTime(node->nodeId,
			interfaceAddress,
			nodeInput,
			FX_SLC_STRING_T_REORDERING,
			&wasFound,
			&retTime);

		if (wasFound)
		{
			if (FX_SLC_MIN_T_REORDERING <= retTime &&
				retTime <= FX_SLC_MAX_T_REORDERING)
			{
				slcConfig->tReordering =
					(int)(retTime / MILLI_SECOND);
			}
			else
			{
				sprintf(errBuf,
					"FX-SLC: T Reordering "
					"should be set to more than %" TYPES_64BITFMT "dMS.",
					FX_SLC_MIN_T_REORDERING / MILLI_SECOND);
				ERROR_ReportError(errBuf);
			}
		}
		else
		{
			// default
			slcConfig->tReordering = FX_SLC_DEFAULT_T_REORDERING;
		}


		// FX_SLC_STRING_T_STATUS_PROHIBIT
		IO_ReadTime(node->nodeId,
			interfaceAddress,
			nodeInput,
			FX_SLC_STRING_T_STATUS_PROHIBIT,
			&wasFound,
			&retTime);

		if (wasFound)
		{
			if (FX_SLC_MIN_T_STATUS_PROHIBIT <= retTime &&
				retTime <= FX_SLC_MAX_T_STATUS_PROHIBIT)
			{
				slcConfig->tStatusProhibit =
					(int)(retTime / MILLI_SECOND);
			}
			else
			{
				sprintf(errBuf,
					"FX-SLC: T Status Prohibit "
					"should be set to more than %" TYPES_64BITFMT "dMS.",
					FX_SLC_MIN_T_STATUS_PROHIBIT / MILLI_SECOND);
				ERROR_ReportError(errBuf);
			}
		}
		else
		{
			// default
			slcConfig->tStatusProhibit = FX_SLC_DEFAULT_T_STATUS_PROHIBIT;
		}



	}
}


BOOL SlcFxAmGetDCBit(Message* msg);



void SlcFxInitAmEntity(
	Node* node,
	int iface,
	const fxRnti& rnti,
	const FxSlcData* fxSlc,
	const FxSlcEntityDirect& direct,
	FxSlcEntity* entity)
{
	SlcFxCreateAmEntity(node, iface, entity);
	return;
}
// /**
// FUNCTION   :: SlcFxGetOrgPduSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get a message size from the original list of messages
// PARAMETERS ::
//  + node      : Node*    : Pointer to node.
//  + msg       : Message* : Pointer to the supper msg containing list of msgs
// RETURN     :: Int16 orgMessageSize
// **/
Int16 SlcFxGetOrgPduSize(Node* node,
	Message* msg)
{
	std::list<Message*> msgList;
	msgList.clear();

	Message* newMsg;

	char* info;
	int infoSize;
	int msgSize;

	Int16 orgMessageSize = 0;

	info = MESSAGE_ReturnInfo(
		msg, (unsigned short)INFO_TYPE_FxSlcAmSduToPdu);
	infoSize = MESSAGE_ReturnInfoSize(
		msg, (unsigned short)INFO_TYPE_FxSlcAmSduToPdu);
	msgSize = 0;
	ERROR_Assert(info != NULL, "INFO_TYPE_FxSlcAmSduToPdu is not found.");
	while (msgSize < infoSize)
	{
		newMsg = SlcFxUnSerializeMessage(node,
			info,
			msgSize);
		// add new msg to msgList
		msgList.push_back(newMsg);
	}
	std::list<Message*>::iterator it = msgList.begin();
	while (it != msgList.end())
	{
		Message* tmpMsg = *it;
		orgMessageSize += (Int16)MESSAGE_ReturnPacketSize(tmpMsg);
		MESSAGE_Free(node, tmpMsg);
		it++;
	}
	return orgMessageSize;
}
// /**
// FUNCTION   :: Fx_SlcInitDynamicStats
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Initiate dynamic statistics
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32 : interface index of FX
// + slc       : Fx_SlcData* : Pointer to SLC data
// RETURN     :: void : NULL
// **/
void SlcFxInitDynamicStats(Node* node,
	UInt32 iface,
	FxSlcData* slc)
{
	if (!node->guiOption)
	{
		// do nothing at Phase 1
		return;
	}
	return;
}
void SlcFxNotifyPowerOn(Node* node, int iface)
{
	FxSlcData* slcData = FxLayer2GetFxSlcData(node, iface);
	slcData->status = FX_SLC_POWER_ON;

}

void SlcFxNotifyPowerOff(Node* node, int iface)
{
	FxSlcData* slcData = FxLayer2GetFxSlcData(node, iface);
	slcData->status = FX_SLC_POWER_OFF;
}


//gl
void SlcFxInit(Node * node, unsigned int iface, const NodeInput * nodeInput)
{

	Layer2DataFx* layer2DataFx =
		FxLayer2GetLayer2DataFx(node, iface);
	FxSlcData* fxSlcData
		= (FxSlcData*)MEM_malloc(sizeof(FxSlcData));//·ÖÅäÄÚ´æ
	ERROR_Assert(fxSlcData != NULL, "SLC FX: Out of memory!");
	memset(fxSlcData, 0, sizeof(FxSlcData));
	layer2DataFx->fxSlcVar = fxSlcData;
	//// init configureble parameters
	SlcFxInitConfigurableParameters(node, iface, nodeInput);

	// init dyanamic statistics
	SlcFxInitDynamicStats(node, iface, fxSlcData);

	//slcData->status = FX_SLC_POWER_OFF;

	// xyt 1229
	int maxTxBufferSize;
	BOOL wasFound;

	NodeAddress interfaceAddress =
		MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
			node, node->nodeId, iface);

	IO_ReadInt(
		node->nodeId,
		interfaceAddress,
		nodeInput,
		"SLC-FX-TX-BUFFER-SIZE",
		&wasFound,
		&maxTxBufferSize);
	if (wasFound) {
		fxSlcData->txBufferMaxSize = maxTxBufferSize;
	}
	else
	{
		fxSlcData->txBufferMaxSize = FX_SLC_DEF_MAXBUFSIZE;
	}

}

void SlcFxFinalize(Node * node, unsigned int iface)
{
	FxSlcData* slcData = FxLayer2GetFxSlcData(node, iface);

	delete slcData;
}

// /**
// FUNCTION   :: SlcFxAmGetHeaderSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: get SLC header size from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
UInt16 SlcFxAmGetHeaderSize(Message* msg)
{
	UInt16   ret = 0;

	FxSlcAmPduFormatFixed* info =
		(FxSlcAmPduFormatFixed*)MESSAGE_ReturnInfo(msg,
		(unsigned short)INFO_TYPE_FxSlcAmPduFormatFixed);

	if (info != NULL)
	{
		ret = (UInt16)info->headerSize;
	}
	else
	{
		ERROR_Assert(FALSE, "SlcFxAmGetHeaderSize: no fixed header field!");
	}

	return ret;
}

void SlcFxReceiveSduFromSac(
	Node* node,
	int iface,
	const fxRnti& dstRnti,
	const int bearerId,
	Message* slcSdu)
{
	ERROR_Assert(slcSdu, "SLC SDU from SAC is NULL");

	Message* dupMsg = MESSAGE_Duplicate(node, slcSdu);//
	MESSAGE_FreeList(node, slcSdu);  //

	char errorMsg[MAX_STRING_LENGTH] = "SlcFxReceiveSduFromSac: ";
	FxSlcData* fxSlc = FxLayer2GetFxSlcData(node, iface);
	FxSlcEntity* entity =
		SlcFxGetSlcEntity(node, iface, dstRnti, bearerId);

	if (!entity)
	{
		MESSAGE_Free(node, dupMsg);
		ERROR_Assert(FALSE, "SlcFxReceiveSduFromSac: entity error");
		return;
	}

	fxSlc->stats.numSdusReceivedFromUpperLayer++;

	switch (entity->entityType)
	{
	case FX_SLC_ENTITY_AM:
	{
		FxSlcAmEntity* amEntity =
			(FxSlcAmEntity*)entity->entityData;


		SlcFxAmUpperLayerHasSduToSend( //
			node,
			iface,
			amEntity,
			dupMsg);
		break;
	}
	default:
	{
		MESSAGE_Free(node, dupMsg);
		sprintf(errorMsg + strlen(errorMsg), "wrong SLC entity type.");
		ERROR_ReportError(errorMsg);
	}
	}
}

// 被SMAC调用
void SlcFxDeliverPduToSmac(Node * node, int interfaceIndex, const fxRnti& dstRnti, std::list<Message*>* sduList, int tbSize)
{
	// 根据目的rnti获取这个rnti对应的SLC实体
	FxSlcEntity* entity = SlcFxGetSlcEntity(node, interfaceIndex, dstRnti, FX_DEFAULT_BEARER_ID);

	switch (entity->entityType)
	{
	case FX_SLC_ENTITY_AM:
	{
		// 获取SLC AM实体
		FxSlcAmEntity* amEntity = (FxSlcAmEntity*)entity->entityData;

		// 获取AM实体的发送队列
		std::list < FxSlcAmTxBuffer >* txBuffer = &amEntity->txBuffer;
		
		if (txBuffer->size() == 0) return;

		// 数据包大小目前得小于tbSize
		int PktSize = MESSAGE_ReturnPacketSize((*txBuffer->begin()).message);
		if (PktSize > tbSize - 7)
		{
			char errorMsg[MAX_STRING_LENGTH] = "SlcFxDeliverPduToSmac: ";
			sprintf(errorMsg + strlen(errorMsg), "size of a single packet is greater than SMAC TB size which is %d byte.", tbSize);
			ERROR_ReportError(errorMsg);
		}

		int totalPktsize = 0, n=1;
		// 把SLC实体发送队列中的包转移到sduList里
		/*ofstream write;
		write.open("text1.txt", ios::app);*/
		
		while (txBuffer->size() > 0)
		{
			Message* thisMsg = (*txBuffer->begin()).message;
			int thisPktSize = MESSAGE_ReturnPacketSize(thisMsg);
			totalPktsize += thisPktSize;
			/*write << thisPktSize << "   "<< amEntity->txBufSize <<endl;*/

			// 如果这个包放不下了就不再继续
			if (totalPktsize > tbSize - 4 - 3 * n++) break;

			sduList->push_front((*txBuffer->begin()).message);
			txBuffer->erase(txBuffer->begin());
			amEntity->txBufSize -= thisPktSize;		
		}
		//write.close();
		return;
	}
	default:
		break;
	}



}

void SlcFxDeliverPduToSmac(
	Node* node,
	int iface,
	const fxRnti& dstRnti,
	const int bearerId,
	int size,
	std::list < Message* >* sduList)
{
	sduList->clear();

	char errorMsg[MAX_STRING_LENGTH] = "SlcFxDeliverPduToSmac ";
	FxSlcEntity* entity =
		SlcFxGetSlcEntity(node, iface, dstRnti, bearerId);

	sprintf(errorMsg + strlen(errorMsg), "Can't find existing SLC entity "
		"given NodeId: %u, iface: %d, bearerId: %u",
		dstRnti.nodeId, dstRnti.interfaceIndex, bearerId);
	ERROR_Assert(entity, errorMsg);

	switch (entity->entityType)
	{
	case FX_SLC_ENTITY_AM:
	{
		FxSlcAmEntity* amEntity =
			(FxSlcAmEntity*)entity->entityData;


		SlcFxAmDeliverPduToSmac(node,
			iface,
			amEntity,
			size,
			sduList);


		break;
	}
	default:
	{
		sprintf(errorMsg + strlen(errorMsg), "wrong SLC entity type.");
		ERROR_ReportError(errorMsg);
	}
	}
}

// /**
// FUNCTION   :: SlcFxAmGetPollBit
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: get Poll bit data from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
BOOL SlcFxAmGetPollBit(Message* msg)
{
	BOOL   ret = FALSE;

	FxSlcAmPduFormatFixed* info =
		(FxSlcAmPduFormatFixed*)MESSAGE_ReturnInfo(msg,
		(unsigned short)INFO_TYPE_FxSlcAmPduFormatFixed);

	if (info != NULL)
	{
		ret = info->pollingBit;
	}
	else
	{
		ERROR_Assert(FALSE, "SlcFxAmGetPollBit: no fixed header field!");
	}

	return ret;
}

void SlcFxReceivePduFromSmac(Node * node, int interfaceIndex, const fxRnti & srcRnti, const int bearerId, Message* msg)
{
	// 从下层SMAC来
	SacFxReceivePduFromSlc(node, interfaceIndex, srcRnti, bearerId, msg, FALSE);

}

void SlcFxReceivePduFromSmac(
	Node* node,
	int iface,
	const fxRnti& srcRnti,
	const int bearerId,
	std::list < Message* > &pduList)
{

	char errorMsg[MAX_STRING_LENGTH] = "SlcFxReceivePduFromSmac ";
	FxSlcData* fxSlc = FxLayer2GetFxSlcData(node, iface);
	FxSlcEntity* entity =
		SlcFxGetSlcEntity(node, iface, srcRnti, bearerId);

	switch (entity->entityType)
	{
	case FX_SLC_ENTITY_AM:
	{
		FxSlcAmEntity* amEntity =
			(FxSlcAmEntity*)entity->entityData;


		SlcFxAmReceivePduFromSmac(node,
			iface,
			srcRnti,
			bearerId,
			amEntity,
			pduList);

		if (amEntity->rcvSlcSduList.empty())
		{
			break;
		}

		// send to sac
		std::multimap < unsigned int, Message* > ::iterator it;
		UInt32 chkPduNum = SlcFxAmSeqNumDist(
			amEntity->oldRcvNextSn, amEntity->rcvNextSn);
		for (UInt32 i = 0; i < chkPduNum; i++)
		{
			std::multimap < unsigned int, Message* > ::iterator it =
				amEntity->rcvSlcSduList.lower_bound(
					amEntity->oldRcvNextSn);
			std::multimap < unsigned int, Message* > ::iterator it_end =
				amEntity->rcvSlcSduList.upper_bound(
					amEntity->oldRcvNextSn);

			while (it != it_end)
			{
 				Message* toSacMsg = MESSAGE_Duplicate(node, it->second);
				MESSAGE_Free(node, it->second);

				SacFxReceivePduFromSlc(node,
					iface,
					srcRnti,
					bearerId,
					toSacMsg,
					FALSE);
				fxSlc->stats.numSdusSentToUpperLayer++;
				amEntity->rcvSlcSduList.erase(it++);
			}
			SlcFxAmIncSN(amEntity->oldRcvNextSn);
		}
		// When Reassemble is finished, if there are remaining SDUsegment
		// SN is decremented because Reassemble
		if (amEntity->rcvSlcSduSegList.size() != 0)
		{
			SlcFxAmDecSN(amEntity->oldRcvNextSn);
		}
		break;
	}
	default:
	{
		sprintf(errorMsg + strlen(errorMsg), "wrong SLC entity type.");
		ERROR_ReportError(errorMsg);
	}
	}
}


void SlcFxInitSlcEntity(Node * node, int iface, const fxRnti & rnti, const FxSlcEntityType & type, const FxSlcEntityDirect& direct, FxSlcEntity * entity)
{
	FxSlcData* fxSlcData = FxLayer2GetFxSlcData(node, iface);

	entity->node = node;
	entity->bearerId = FX_DEFAULT_BEARER_ID;
	entity->oppositeRnti = rnti;
	entity->entityType = type;
	entity->entityData = NULL;

	switch (type)
	{
	case FX_SLC_ENTITY_AM:
	{

		SlcFxInitAmEntity(node,
			iface,
			rnti,
			fxSlcData,
			direct,
			entity);
		//FxSlcAmEntity* amEntity = new FxSlcAmEntity();
		//entity->entityData = (void*)amEntity;
	}
	default:
		break;
	}

}
static
void SlcFxAmUpperLayerHasSduToSend(
	Node* node,
	int iface,
	FxSlcAmEntity* amEntity,
	Message* sduMsg)
{
	// add info for management received SLC ACK
	FxSlcAmSacPduInfo pduInfo = FxSlcAmSacPduInfo(
		FxLayer2GetSacSnFromSacHeader(sduMsg));
	FxAddMsgInfo(node, iface, sduMsg, INFO_TYPE_FxSlcAmSacPduInfo,
		pduInfo);
	amEntity->store(node, iface, sduMsg);  //´
	return;
}

void FxSlcAmEntity::store(Node* node, int iface, Message* msg)
{
	FxSlcData* fxSlcData =
		FxLayer2GetFxSlcData(node, iface);


	if (txBufSize + MESSAGE_ReturnPacketSize(msg) > maxTransBufSize)
	{

		fxSlcData->stats.numSdusDiscardedByOverflow++;

		MESSAGE_Free(node, msg);
		return;
	}

	txBufSize += MESSAGE_ReturnPacketSize(msg);


	FxSlcAmTxBuffer txMsg;
	txMsg.store(node, iface, msg);

	txBuffer.push_back(txMsg);




	return;
}


// /**
// FUNCTION::           FxSlcEntity::restore
// LAYER::              FX LAYER2 SLC
// PURPOSE::            restore the SLC entity
// PARAMETERS::
//  + node  : Node* : Node Pointer
//  + iface : int   : Inteface index
//  + size  : int   : request size of TXOP Notification
//  + txMsg : std::list<Message*>& : SLC-PDU messages list
// RETURN::    void :         NULL
// **/
void FxSlcAmEntity::restore(Node* node,
	int iface,
	int size,
	std::list < Message* > &txMsg)
{
	int restSize = size;
	int txPduByte = 0;
	int retxPduByte = 0;
	int statusPduByte = 0;
	int pollPduByte = 0;
	int smacHeaderByte = SMAC_FX_RRELCIDFL_WITH_15BIT_SUBHEADER_SIZE;
	int numTxPdu = 0;
	int numRetxPdu = 0;
	int numStatusPdu = 0;

	if (resetFlg == TRUE)
	{
		return;
	}

	BOOL makedPdu = FALSE;
	BOOL makedPduSegment = FALSE;

	// 1.) Make Status-PDU
	int reqSizeWithoutSMACHeader = restSize - smacHeaderByte;
	reqSizeWithoutSMACHeader = restSize > smacHeaderByte
		? restSize - smacHeaderByte
		: 0;
	getStatusBuffer(node,
		iface,
		reqSizeWithoutSMACHeader,
		statusBuffer,
		txMsg,
		statusPduByte);
	numStatusPdu = txMsg.size();;
	restSize = statusPduByte > 0
		? reqSizeWithoutSMACHeader
		: restSize;



	// 2.) Make SLC-PDU from retransmission buffer.
	int availableNumPdu = getNumAvailableRetxPdu(node, iface, restSize);
	reqSizeWithoutSMACHeader = restSize > availableNumPdu * smacHeaderByte
		? restSize - availableNumPdu * smacHeaderByte
		: 0;
	makedPduSegment = getRetxBuffer(node,
		iface,
		reqSizeWithoutSMACHeader,
		retxBuffer,
		txMsg,
		retxPduByte);
	numRetxPdu = txMsg.size() - numStatusPdu;
	// if numRetxPdu is less than availableNumPdu,
	// restSize is adjusted by numRetxPdu and smacHeaderSize.
	restSize = reqSizeWithoutSMACHeader +
		(availableNumPdu - numRetxPdu) * smacHeaderByte;

	if (resetFlg == TRUE)
	{

		return;
	}


	// 3.) Make SLC-PDU from trunsmission buffer.
	//     The next SN must be in a window.
	if (!SlcFxAmSeqNumInWnd(this->sndNextSn, this->ackSn))
	{

	}
	else
	{
		reqSizeWithoutSMACHeader = restSize - smacHeaderByte  > 0
			? restSize - smacHeaderByte
			: 0;
		makedPdu = getTxBuffer(node,
			iface,
			reqSizeWithoutSMACHeader,
			txBuffer,
			txMsg,
			txPduByte);
		numTxPdu = txMsg.size() - numStatusPdu - numRetxPdu;
		restSize = txPduByte > 0
			? reqSizeWithoutSMACHeader
			: restSize;
	}



	// Polling function.
	polling(node, iface, restSize,
		makedPdu, makedPduSegment, txMsg, pollPduByte);
	return;
}


// /**
// FUNCTION   :: SlcFxAmGetFramingInfo
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: get FI data from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
FxSlcAmFramingInfo SlcFxAmGetFramingInfo(Message* msg)
{
	FxSlcAmFramingInfo   ret = FX_SLC_AM_FRAMING_INFO_NO_SEGMENT;

	FxSlcAmPduFormatFixed* info =
		(FxSlcAmPduFormatFixed*)MESSAGE_ReturnInfo(msg,
		(unsigned short)INFO_TYPE_FxSlcAmPduFormatFixed);

	if (info != NULL)
	{
		ret = info->FramingInfo;
	}
	else
	{
		ERROR_Assert(FALSE,
			"SlcFxAmGetFramingInfo: no fixed header field!");
	}

	return ret;
}
// /**
// FUNCTION::           FxSlcEntity::getBufferSize
// LAYER::              FX LAYER2 SLC
// PURPOSE::            get buffer size of SLC entity
// PARAMETERS::
//  + node  : Node* : Node Pointer
//  + iface : int   : Inteface index
// RETURN::    void :         NULL
// **/
int FxSlcAmEntity::getBufferSize(Node* node, int iface)
{
	UInt64 retSize = 0;
	retSize += txBufSize;
	retSize += retxBufNackSize;
	retSize += statusBufSize;

	return (int)retSize;
}

// /**
// FUNCTION::           FxSlcEntity::discardPdu
// LAYER::              FX LAYER2 SLC
// PURPOSE::            discard PDU by re-transmit upper limit.
// PARAMETERS::
//  + node        : Node* : Node Pointer
//  + iface       : int   : Inteface index
//  + it          : std::list<FxSlcAmReTxBuffer>::iterator&
//                        : re-transmittion buffer iterator
//  + buf         : std::list<FxSlcAmReTxBuffer>& : re-transmittion buffer
//  + txMsg       : std::list<Message*>& : SLC-PDU messages list
//  + retxPduByte : int&  : re-transmit PDU size
// RETURN::    void :         NULL
// **/
BOOL FxSlcAmEntity::discardPdu(
	Node* node,
	int iface,
	std::list < FxSlcAmReTxBuffer > ::iterator& it,
	std::list < FxSlcAmReTxBuffer > &buf,
	std::list < Message* > &pdu,
	int& retxPduByte)
{
	BOOL ret = FALSE;
	FxSlcData* fxSlc =
		FxLayer2GetFxSlcData(node, iface);
	fxSlc->stats.numDataPdusDiscardedByRetransmissionThreshold++;
	retxBufSize -= MESSAGE_ReturnPacketSize(it->message);
	MESSAGE_FreeList(node, it->message);
	it->message = NULL;
	SlcFxDelAmPduHdrObj(it->slcHeader);
	it = buf.erase(it);
	// Reset
	{
		resetAll(TRUE);
		SlcFxAmSetResetData(node, iface, this);
		std::list < Message* > ::iterator pduItr;
		for (pduItr = pdu.begin();
			pduItr != pdu.end();
			++pduItr)
		{
			MESSAGE_FreeList(node, (*pduItr));
		}
		pdu.clear();
		resetFlg = TRUE;
		sendResetFlg = TRUE;
		retxPduByte = 0;
		SlcFxAmInitResetTimer(node, iface, this, FX_SLC_ENTITY_TX);
		return ret;
	}
}

// /**
// FUNCTION::           FxSlcEntity::getRetxBuffer
// LAYER::              FX LAYER2 SLC
// PURPOSE::            get PDU from re-transmit buffer.
// PARAMETERS::
//  + node        : Node* : Node Pointer
//  + iface       : int   : Inteface index
//  + restSize    : int&  : request data size
//  + buf         : std::list<FxSlcAmReTxBuffer>& : re-transmittion buffer
//  + pdu         : std::list<Message*>& : SLC-PDU messages list
//  + retxPduByte : int&  : re-transmit PDU size
// RETURN::    void :         NULL
// **/
BOOL FxSlcAmEntity::getRetxBuffer(Node* node,
	int iface,
	int& restSize,
	std::list < FxSlcAmReTxBuffer > &buf,
	std::list < Message* > &pdu,
	int& retxPduByte)
{
	FxSlcData* fxSlc = FxLayer2GetFxSlcData(node, iface);
	FxSlcConfig* slcConfig = GetFxSlcConfig(node, iface);
	BOOL ret = FALSE;
	BOOL resegment = FALSE;
	std::list < FxSlcAmReTxBuffer > ::iterator it;
	for (it = buf.begin(); it != buf.end();)
	{
		// NackData size
		int nackDataSize = 0;
		std::map < int, FxSlcStatusPduSoPair > ::iterator nackIt;

		for (nackIt = it->nackField.begin();
			nackIt != it->nackField.end();)
		{
			resegment = FALSE;
			ERROR_Assert((*nackIt).second.soEnd >=
				(*nackIt).second.soStart,
				"SLC-FX SegmentOffset field: start > end !");
			nackDataSize = (*nackIt).second.soEnd
				- (*nackIt).second.soStart;
			if (nackDataSize == 0)
			{
				break;
			}



			if (nackDataSize == MESSAGE_ReturnPacketSize(it->message))
			{
				// no resegmentation
				int size = nackDataSize +
					SlcFxGetExpectedAmPduHeaderSize(node,
						iface,
						it->slcHeader,
						FALSE);



				// size less than request size
				// no resegment
				if (restSize > size)
				{
					restSize -= size;

					// this message is not included header size
					Message* dupMsg = MESSAGE_Duplicate(node, it->message);
					// add header size and set to info
					SlcFxSetMessageSlcHeaderInfo(node,
						iface,
						it->slcHeader,
						dupMsg);

					it->retxCount++;
					if (it->retxCount > slcConfig->maxRetxThreshold)
					{
						if (dupMsg) {
							MESSAGE_Free(node, dupMsg);
							dupMsg = NULL;
						}
						else {
							// do nothing
						}
						ret = discardPdu(
							node, iface, it, buf, pdu, retxPduByte);
						return ret;
					}

					pdu.push_back(dupMsg);
					retxPduByte += MESSAGE_ReturnPacketSize(dupMsg);
					//fxSlc->stats.numDataPdusSentToSMACSublayer++;
					ret = TRUE;

					// update nack size
					retxBufNackSize -= it->getNackedSize(node, iface);
					it->nackField.erase(nackIt++);



					if (it->nackField.empty())
					{
						it->pduStatus = FX_SLC_AM_DATA_PDU_STATUS_WAIT;
					}
					continue;
				}
				else
				{
					resegment = TRUE;
				}
			}
			else
			{
				resegment = TRUE;
			}

			if (resegment == TRUE)
			{
				int headerSize = SlcFxGetExpectedAmPduHeaderSize(node,
					iface,
					it->slcHeader,
					TRUE);
				int size = nackDataSize + headerSize;




				if (restSize >= size)
				{
					restSize -= size;
					FxSlcHeader txSlcHeader;
					SlcFxCpyAmPduHdrObj(it->slcHeader, txSlcHeader);
					if (txSlcHeader.segFixed == NULL)
					{
						txSlcHeader.segFixed =
							new FxSlcAmPduSegmentFormatFixed();
					}
					txSlcHeader.fixed->resegmentationFlag = TRUE;
					if (((*nackIt).second.soStart + nackDataSize)
						== MESSAGE_ReturnPacketSize(it->message))
					{
						txSlcHeader.segFixed->lastSegmentFlag = TRUE;
					}

					txSlcHeader.segFixed->segmentOffset
						= (UInt16)(*nackIt).second.soStart;

					// this message is not included header size
					Message* dupMsg = MESSAGE_Duplicate(node, it->message);
					int tmpSize = MESSAGE_ReturnVirtualPacketSize(dupMsg);
					if (tmpSize > 0)
					{
						MESSAGE_RemoveVirtualPayload(node, dupMsg, tmpSize);
					}
					MESSAGE_AddVirtualPayload(node, dupMsg, nackDataSize);
					// add header size and set to info
					SlcFxSetMessageSlcHeaderInfo(node,
						iface,
						txSlcHeader,
						dupMsg);

					SlcFxDelAmPduHdrObj(txSlcHeader);

					it->retxCount++;
					if (it->retxCount > slcConfig->maxRetxThreshold)
					{
						if (dupMsg) {
							MESSAGE_Free(node, dupMsg);
							dupMsg = NULL;
						}
						else {
							// do nothing
						}
						ret = discardPdu(
							node, iface, it, buf, pdu, retxPduByte);
						return ret;
					}

					pdu.push_back(dupMsg);
					retxPduByte += MESSAGE_ReturnPacketSize(dupMsg);
					//fxSlc->stats.numDataPdusSentToSMACSublayer++;
					ret = TRUE;

					// remove Nack parts
					retxBufNackSize -= it->getNackedSize(node, iface);
					it->nackField.erase(nackIt++);
					if (it->nackField.empty())
					{
						it->pduStatus = FX_SLC_AM_DATA_PDU_STATUS_WAIT;
					}
					else
					{
						retxBufNackSize += it->getNackedSize(node, iface);
					}
					continue;
				}
				// resegment
				else
				{
					if (restSize < headerSize + FX_SLC_AM_DATA_PDU_MIN_BYTE)
					{
						return ret;
					}
					else
					{
						retxBufNackSize -= it->getNackedSize(node, iface);
						size = restSize - headerSize;
						restSize -= (size + headerSize);

						FxSlcHeader txSlcHeader;
						SlcFxCpyAmPduHdrObj(it->slcHeader, txSlcHeader);
						if (txSlcHeader.segFixed == NULL)
						{
							txSlcHeader.segFixed =
								new FxSlcAmPduSegmentFormatFixed();
						}
						txSlcHeader.fixed->resegmentationFlag = TRUE;
						if (((*nackIt).second.soStart + size)
							== MESSAGE_ReturnPacketSize(it->message))
						{
							txSlcHeader.segFixed->lastSegmentFlag = TRUE;
						}

						txSlcHeader.segFixed->segmentOffset =
							(UInt16)(*nackIt).second.soStart;

						// this message is not included header size
						Message* dupMsg = MESSAGE_Duplicate(
							node, it->message);
						int tmpSize = MESSAGE_ReturnVirtualPacketSize(dupMsg);
						if (tmpSize > 0)
						{
							MESSAGE_RemoveVirtualPayload(node, dupMsg, tmpSize);
						}
						MESSAGE_AddVirtualPayload(node, dupMsg, size);
						// add header size and set to info
						SlcFxSetMessageSlcHeaderInfo(node,
							iface,
							txSlcHeader,
							dupMsg);

						SlcFxDelAmPduHdrObj(txSlcHeader);

						it->retxCount++;
						if (it->retxCount > slcConfig->maxRetxThreshold)
						{
							if (dupMsg) {
								MESSAGE_Free(node, dupMsg);
								dupMsg = NULL;
							}
							else {
								// do nothing
							}
							ret = discardPdu(
								node, iface, it, buf, pdu, retxPduByte);
							return ret;
						}

						pdu.push_back(dupMsg);
						retxPduByte += MESSAGE_ReturnPacketSize(dupMsg);
						//fxSlc->stats.numDataPdusSentToSMACSublayer++;
						ret = TRUE;


						// update OffsetStart
						nackIt->second.soStart += size;
						retxBufNackSize += it->getNackedSize(node, iface);

						return ret;
					}
				}
			}

			nackIt++;
		}
		if (it == buf.end()) {
			break;
		}
		it++;
	}

	return ret;
}


// /**
// FUNCTION::           SlcFxAmDeliverPduToSmac
// LAYER::              FX LAYER2 SLC
// PURPOSE::            Prepare SLC PDUs sent to SMAC through AM SAP,
//                      invoked for each TTI
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         the interface index
//      +amEntity:      The AM entity
//      +pduSize:       PDU size given by SMAC
//      +pduList:       A list containing the pdus to be sent
// RETURN::             NULL
// **/
static
void SlcFxAmDeliverPduToSmac(
	Node* node,
	int iface,
	FxSlcAmEntity* amEntity,
	int pduSize,
	std::list < Message* >* pduList)
{
	amEntity->restore(node, iface, pduSize, *pduList);
}

// /**
// FUNCTION::           FxSlcEntity::getStatusBuffer
// LAYER::              FX LAYER2 SLC
// PURPOSE::            get status PDU from statusPDU buffer.
// PARAMETERS::
//  + node        : Node* : Node Pointer
//  + iface       : int   : Inteface index
//  + restSize    : int&  : request data size
//  + buf         : std::list<FxSlcStatusPduFormat>& : statusPDU buffer
//  + pdu         : std::list<Message*>& : SLC-PDU messages list
//  + retxPduByte : int&  : transmit statusPDU size
// RETURN::    void :         NULL
// **/
void FxSlcAmEntity::getStatusBuffer(
	Node* node,
	int iface,
	int& restSize,
	std::list < FxSlcStatusPduFormat > &buf,
	std::list < Message* > &pdu,
	int& statusPduByte)
{
	FxSlcData* fxSlc = FxLayer2GetFxSlcData(node, iface);
	std::list < FxSlcStatusPduFormat > ::iterator it = buf.begin();
	while (it != buf.end())
	{
		Message* pduMsg = NULL;
		int size = SlcFxGetExpectedAmStatusPduSize(*it);

		// Status PDU size is less than request size
		if (restSize <= size)
		{

			size = restSize;
		}

		pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
		ERROR_Assert(pduMsg != NULL, "SLC FX: Out of memory!");
		MESSAGE_PacketAlloc(node,
			pduMsg,
			size,
			TRACE_SLC_FX);

		SlcFxSetMessageStatusPduFormatInfo(node,
			iface,
			pduMsg,
			(*it));

		statusBufSize -= size;
		it = buf.erase(it);
		restSize -= size;


		if (pduMsg != NULL)
		{

			pdu.push_back(pduMsg);
			statusPduByte += MESSAGE_ReturnPacketSize(pduMsg);
			fxSlc->stats.numAmStatusPdusSendToMacSublayer++;
		}
	}
}

// /**
// FUNCTION::           FxSlcEntity::getTxBuffer
// LAYER::              FX LAYER2 SLC
// PURPOSE::            get PDU from transmittion buffer.
// PARAMETERS::
//  + node        : Node* : Node Pointer
//  + iface       : int   : Inteface index
//  + restSize    : int&  : request data size
//  + buf         : std::list<FxSlcAmTxBuffer>& : transmittion buffer
//  + pdu         : std::list<Message*>& : SLC-PDU messages list
//  + retxPduByte : int&  : transmit PDU size
// RETURN::    void :         NULL
// **/
BOOL FxSlcAmEntity::getTxBuffer(Node* node,
	int iface,
	int& restSize,
	std::list < FxSlcAmTxBuffer > &buf,
	std::list < Message* > &pdu,
	int& txPduByte)
{
	FxSlcData* fxSlc = FxLayer2GetFxSlcData(node, iface);
	BOOL ret = FALSE;
	Message* pduMsg = NULL;
	FxSlcHeader slcHeader = FxSlcHeader(FX_SLC_AM_PDU_TYPE_PDU);
	BOOL loopFinalIsSegment = FALSE;

	std::list < FxSlcAmTxBuffer > ::iterator it;

	for (it = buf.begin(); it != buf.end();)
	{
		int size = SlcFxGetExpectedAmPduSize(node,
			iface,
			slcHeader,
			FALSE,
			pduMsg,
			*it);

		// Data PDU size less than request size
		// need concatenation
		if (restSize >= size)
		{
			txBufSize -= MESSAGE_ReturnPacketSize(it->message);
			Message* rtxPduMsg = it->message;

			if (slcHeader.firstSdu == TRUE)
			{
				// set flag & FI field
				slcHeader.firstSdu = FALSE;
				slcHeader.firstSduLi
					= (UInt16)MESSAGE_ReturnPacketSize(rtxPduMsg);
				if ((slcHeader.extension == NULL)
					&& ((*it).offset != 0))
				{
					SlcFxAllocAmPduLi(node,
						iface,
						slcHeader,
						slcHeader.firstSduLi);
					slcHeader.firstSduLi = 0;
					slcHeader.fixed->FramingInfo
						= FX_SLC_AM_FRAMING_INFO_IN_END_BYTE;
				}
				else
				{
					slcHeader.fixed->FramingInfo
						= FX_SLC_AM_FRAMING_INFO_NO_SEGMENT;
				}

				concatenationMessage(node,
					iface,
					pduMsg,
					rtxPduMsg);
			}
			else
			{
				// add LI of first SDU
				if (slcHeader.firstSduLi != 0)
				{
					SlcFxAllocAmPduLi(node,
						iface,
						slcHeader,
						slcHeader.firstSduLi);
					slcHeader.firstSduLi = 0;
				}

				concatenation(node,
					iface,
					slcHeader,
					pduMsg,
					rtxPduMsg);
			}

			loopFinalIsSegment = FALSE;

			it = buf.erase(it);
		}
		// need segmentation
		else
		{
			if (slcHeader.firstSdu == TRUE)
			{
				// set flag & FI field
				slcHeader.firstSdu = FALSE;
				if (it->offset == 0)
				{
					slcHeader.fixed->FramingInfo
						= FX_SLC_AM_FRAMING_INFO_NO_SEGMENT;
				}
				else
				{
					slcHeader.fixed->FramingInfo
						= FX_SLC_AM_FRAMING_INFO_IN_END_BYTE;
				}
			}
			// add LI of first SDU
			if (slcHeader.firstSduLi != 0)
			{
				SlcFxAllocAmPduLi(node,
					iface,
					slcHeader,
					slcHeader.firstSduLi);
				slcHeader.firstSduLi = 0;
			}

			// calculate remaining size which is enough to segment
			size = SlcFxGetAmPduListDataSize(node,
				iface,
				slcHeader,
				pduMsg);
			size += SlcFxGetExpectedAmPduHeaderSize(node,
				iface,
				slcHeader,
				FALSE);
			if (slcHeader.extension == NULL)
			{
				size += FX_SLC_AM_DATA_PDU_FORMAT_EXTENSION_MIN_BYTE_SIZE;
			}

			restSize -= size;
			if (restSize > FX_SLC_AM_DATA_PDU_MIN_BYTE)
			{
				Message* fragHead = MESSAGE_Alloc(node, 0, 0, 0);
				Message* fragTail = MESSAGE_Alloc(node, 0, 0, 0);

				BOOL divisonFlag = FALSE;
				divisonFlag = divisionMessage(node,
					iface,
					restSize,
					it->message,
					fragHead,
					fragTail,
					TRACE_SLC_FX,
					TRUE);
				if (divisonFlag == TRUE)
				{

					concatenation(node,
						iface,
						slcHeader,
						pduMsg,
						fragHead);
					// update SDU -> SDUsegment
					it->message = fragTail;
					it->offset += MESSAGE_ReturnPacketSize(fragHead);
					txBufSize -= MESSAGE_ReturnPacketSize(fragHead);
					restSize = 0;
				}
				else
				{
					MESSAGE_Free(node, fragHead);
					MESSAGE_Free(node, fragTail);

					ERROR_Assert(FALSE, "The SDU size check has mistaken.");
				}
				loopFinalIsSegment = TRUE;
			}

			break;
		}
	}

	if (loopFinalIsSegment == TRUE)
	{
		// segment & set FI
		if (slcHeader.fixed->FramingInfo
			& FX_SLC_AM_FRAMING_INFO_CHK_FIRST_BYTE)
		{
			slcHeader.fixed->FramingInfo
				= FX_SLC_AM_FRAMING_INFO_FRAGMENT;
		}
		else
		{
			slcHeader.fixed->FramingInfo
				= FX_SLC_AM_FRAMING_INFO_IN_FIRST_BYTE;
		}
	}

	if (pduMsg != NULL)
	{


		// set unreceived SLC ACK map
		// in oreder to notify SAC of received SAC ACK
		int slcSn = this->getSndSn();
		Message* current = pduMsg;
		while (current) {
			unreceivedSlcAckList.push_back(pair<UInt16, UInt16>(
				(UInt16)slcSn,
				(UInt16)((FxSlcAmSacPduInfo*)MESSAGE_ReturnInfo(
					current,
					INFO_TYPE_FxSlcAmSacPduInfo))->sacSn));

			current = current->next;
		}


		Message* newMsg = SlcFxSduListToPdu(node, pduMsg);


		slcHeader.FxSlcSetPduSeqNo(slcSn);

		Message* dupMsg = MESSAGE_Duplicate(node, newMsg);

		FxSlcAmReTxBuffer retx;
		retx.store(node, iface, slcHeader, dupMsg);
		retxBuffer.push_back(retx);
		retxBufSize += MESSAGE_ReturnPacketSize(dupMsg);

		if (latestPdu.message != NULL)
		{
			SlcFxDelAmPduHdrObj(latestPdu.slcHeader);
			MESSAGE_FreeList(entityVar->node, latestPdu.message);
			latestPdu.message = NULL;
		}
		SlcFxCpyAmPduHdrObj(slcHeader, latestPdu.slcHeader);
		latestPdu.message = MESSAGE_Duplicate(node, dupMsg);

		// Polling function.
		polling(node, iface, slcHeader, dupMsg);

		SlcFxSetMessageSlcHeaderInfo(node,
			iface,
			slcHeader,
			newMsg);

		pdu.push_back(newMsg);
		txPduByte += MESSAGE_ReturnPacketSize(newMsg);
		fxSlc->stats.numDataPdusSentToMacSublayer++;



		ret = TRUE;
	}
	else
	{
		SlcFxDelAmPduHdrObj(slcHeader);

		ret = FALSE;
	}


	return ret;
}

// /**
// FUNCTION::           FxSlcEntity::polling
// LAYER::              FX LAYER2 SLC
// PURPOSE::            polling function.
// PARAMETERS::
//  + node      : Node*         : Node Pointer
//  + iface     : int           : Inteface index
//  + slcHeader : FxSlcHeader& : SLC header data
//  + newMsg    : Message*      : SLC-PDU messages list
// RETURN::    void :         NULL
// **/
void FxSlcAmEntity::polling(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* newMsg)
{
	FxSlcConfig* slcConfig = GetFxSlcConfig(node, iface);
	std::list < FxSlcAmReTxBuffer > ::iterator it;

	pduWithoutPoll += 1;
	byteWithoutPoll += SlcFxGetAmPduSize(node,
		iface,
		slcHeader,
		newMsg);
	if ((pduWithoutPoll >= (UInt32)slcConfig->pollPdu)
		|| (byteWithoutPoll >= (UInt32)slcConfig->pollByte))
	{
		slcHeader.fixed->pollingBit = TRUE;
		pduWithoutPoll = 0;
		byteWithoutPoll = 0;
		pollSn = sndNextSn;
		SlcFxAmDecSN(pollSn);

		resetExpriory_t_PollRetransmit();
		if (pollRetransmitTimerMsg != NULL)
		{
			MESSAGE_CancelSelfMsg(node, pollRetransmitTimerMsg);
			pollRetransmitTimerMsg = NULL;
		}

		SlcFxAmInitPollRetrunsmitTimer(node,
			iface,
			this);
	}

}

// /**
// FUNCTION::           FxSlcEntity::polling
// LAYER::              FX LAYER2 SLC
// PURPOSE::            get PDU from transmittion buffer.
// PARAMETERS::
//  + node            : Node* : Node Pointer
//  + iface           : int   : Inteface index
//  + restSize        : int&  : request data size
//  + makedPdu        : BOOL  : flag of making PDU
//  + makedPduSegment : BOOL  : flag of making PDUsegment
//  + pdu             : std::list<Message*>& : SLC-PDU messages list
//  + pollPduByte     : int&  : transmit PDU size
// RETURN::    void :         NULL
// **/
void FxSlcAmEntity::polling(Node* node,
	int iface,
	int& restSize,
	BOOL makedPdu,
	BOOL makedPduSegment,
	std::list < Message* > &pdu,
	int& pollPduByte)
{
	FxSlcData* fxSlc = FxLayer2GetFxSlcData(node, iface);
	BOOL poolFlag = chkExpriory_t_PollRetransmit();

	// "not dequeued from tx&retx buffer" and
	// "Expriory-t-PollRetransmit" is expired
	if ((makedPdu != TRUE) && (makedPduSegment != TRUE)
		&& (poolFlag == TRUE))
	{
		UInt32 latestSn = sndNextSn;
		SlcFxAmDecSN(latestSn);

		if (latestPdu.message == NULL)
		{
			ERROR_ReportError("FX-SLC: Not found sendable PDU.");
		}
		else
		{
			FxSlcHeader txSlcHeader;
			SlcFxCpyAmPduHdrObj(latestPdu.slcHeader, txSlcHeader);
			Message* dupMsg = MESSAGE_Duplicate(node, latestPdu.message);

			txSlcHeader.fixed->pollingBit = TRUE;
			pduWithoutPoll = 0;
			byteWithoutPoll = 0;
			pollSn = latestSn;


			resetExpriory_t_PollRetransmit();
			if (pollRetransmitTimerMsg != NULL)
			{
				MESSAGE_CancelSelfMsg(node, pollRetransmitTimerMsg);
				pollRetransmitTimerMsg = NULL;
			}
			SlcFxAmInitPollRetrunsmitTimer(node,
				iface,
				this);


			SlcFxSetMessageSlcHeaderInfo(node,
				iface,
				txSlcHeader,
				dupMsg);

			SlcFxDelAmPduHdrObj(txSlcHeader);

			pdu.push_back(dupMsg);
			pollPduByte += MESSAGE_ReturnPacketSize((dupMsg));
			fxSlc->stats.numDataPdusSentToMacSublayer++;


		}
	}
}

// /**
// FUNCTION::           FxSlcEntity::concatenationMessage
// LAYER::              FX LAYER2 SLC
// PURPOSE::            concatenation message.
// PARAMETERS::
//  + node            : Node*     : Node Pointer
//  + iface           : int       : Inteface index
//  + prev            : Message*& : messages list (for base)
//  + next            : Message*  : messages list
// RETURN::    void :         NULL
// **/
void FxSlcAmEntity::concatenationMessage(Node* node,
	int iface,
	Message*& prev,
	Message* next)
{
	if (prev == NULL)
	{
		prev = next;
	}
	else
	{
		Message* org = prev;
		while (prev->next != NULL)
		{
			prev = prev->next;
		}
		prev->next = next;
		prev = org;
	}
}

// /**
// FUNCTION::           FxSlcEntity::concatenation
// LAYER::              FX LAYER2 SLC
// PURPOSE::            concatenation message, and add LI-info.
// PARAMETERS::
//  + node      : Node*         : Node Pointer
//  + iface     : int           : Inteface index
//  + slcHeader : FxSlcHeader& : SLC header data
//  + prev      : Message*&     : messages list (for base)
//  + next      : Message*      : messages list
// RETURN::    void :         NULL
// **/
void FxSlcAmEntity::concatenation(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message*& prev,
	Message* next)
{
	//Message
	concatenationMessage(node,
		iface,
		prev,
		next);

	SlcFxAddAmPduLi(node,
		iface,
		slcHeader,
		next);

}

// /**
// FUNCTION   :: FxSlcAmEntity::getSndSn
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: get next VT(S).
// RETURN     :: int     : next VT(S)
// **/
int FxSlcAmEntity::getSndSn()
{
	int sndSn = sndNextSn;
	sndNextSn++;
	sndNextSn = sndNextSn % FX_SLC_AM_SN_UPPER_BOUND;

	return sndSn;
}

// /**
// FUNCTION   :: FxSlcAmEntity::txWindowCheck
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: check SN, inside of window.
// RETURN     :: BOOL
// **/
BOOL FxSlcAmEntity::txWindowCheck()
{
	int sn = ackSn % FX_SLC_AM_SN_UPPER_BOUND;
	if ((0 <= sn) && (sn < FX_SLC_AM_WINDOW_SIZE))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// /**
// FUNCTION::           FxSlcEntity::divisionMessage
// LAYER::              FX LAYER2 SLC
// PURPOSE::            division message. (into two parts)
// PARAMETERS::
//  + node         : Node*             : Node Pointer
//  + iface        : int               : Inteface index
//  + offset       : int               : offset of dividing position
//  + msg          : Message*          : messages(orignal)
//  + fragHead     : Message*          : messages(divided first-part)
//  + fragTail     : Message*          : messages(divided second-part)
//  + protocolType : TraceProtocolType : add TraceProtocolType
//  + msgFreeFlag  : BOOL              : orignal message free
// RETURN::    void :         NULL
// **/
BOOL FxSlcAmEntity::divisionMessage(Node* node,
	int iface,
	int offset,
	Message* msg,
	Message* fragHead,
	Message* fragTail,
	TraceProtocolType protocolType,
	BOOL msgFreeFlag)
{
	int pktSize = MESSAGE_ReturnPacketSize(msg);

	if (pktSize <= offset)
	{
		return FALSE;
	}
	else
	{
		int realPayloadSize = MESSAGE_ReturnActualPacketSize(msg);
		int virtualPayloadSize = MESSAGE_ReturnVirtualPacketSize(msg);
		int realIndex = 0;
		int virtualIndex = 0;
		int fragRealSize;
		int fragVirtualSize;
		int i;

		int numFrags = 2;
		int divSize[2];

		divSize[0] = offset;
		divSize[1] = realPayloadSize + virtualPayloadSize - offset;

		Message* fragMsg[2] = { fragHead, fragTail };

		for (i = 0; i < numFrags; i++)
		{
			// copy info fields
			MESSAGE_CopyInfo(node, fragMsg[i], msg);

			fragRealSize = realPayloadSize - realIndex;
			if ((fragRealSize >= divSize[i]) && i == 0)
			{
				fragRealSize = divSize[i];
				fragVirtualSize = 0;
			}
			else
			{
				fragVirtualSize = virtualPayloadSize - virtualIndex;
				if (fragVirtualSize + fragRealSize > divSize[i])
				{
					fragVirtualSize = divSize[i] - fragRealSize;
				}
			}

			if (fragRealSize > 0)
			{
				MESSAGE_PacketAlloc(node,
					fragMsg[i],
					fragRealSize,
					protocolType);
				memcpy(MESSAGE_ReturnPacket(fragMsg[i]),
					MESSAGE_ReturnPacket(msg) + realIndex,
					fragRealSize);
			}
			else
			{
				MESSAGE_PacketAlloc(node, fragMsg[i], 0, protocolType);
			}

			if (fragVirtualSize > 0)
			{
				MESSAGE_AddVirtualPayload(node, fragMsg[i], fragVirtualSize);
			}

			realIndex += fragRealSize;
			virtualIndex += fragVirtualSize;
		}

		// copy other simulation specific information to the first fragment
		fragMsg[0]->sequenceNumber = msg->sequenceNumber;
		fragMsg[0]->originatingProtocol = msg->originatingProtocol;
		fragMsg[0]->numberOfHeaders = msg->numberOfHeaders;

		for (i = 0; i < msg->numberOfHeaders; i++)
		{
			fragMsg[0]->headerProtocols[i] = msg->headerProtocols[i];
			fragMsg[0]->headerSizes[i] = msg->headerSizes[i];
		}
		if (msgFreeFlag)
		{
			MESSAGE_Free(node, msg);
		}
	}
	return TRUE;
}

// /**
// FUNCTION::       SlcFxAmSeqNumInWnd
// LAYER::          FX Layer2 SLC
// PURPOSE::        Return whether a sequence number is within the window
// PARAMETERS::
//  + sn      : unsigned int : The sequence number to be tested
//  + snStart : unsigned int : The sequence number of window starting point
// RETURN::    void:     BOOL
// **/
//
static
BOOL SlcFxAmSeqNumInWnd(unsigned int sn,
	unsigned int snStart)
{
	return SlcFxSeqNumInWnd(sn,
		snStart,
		FX_SLC_AM_WINDOW_SIZE,
		FX_SLC_AM_SN_UPPER_BOUND);
}

// /**ÓÃ¹ý
// FUNCTION::           SlcFxGetExpectedAmStatusPduSize
// LAYER::              FX LAYER2 SLC
// PURPOSE::            get status PDU size(byte).
// PARAMETERS::
//  + statusPdu : FxSlcStatusPduFormat& : status PDU
// RETURN::    void :         NULL
// **/
int SlcFxGetExpectedAmStatusPduSize(FxSlcStatusPduFormat& statusPdu)
{
	int sizeBit = FX_SLC_AM_STATUS_PDU_FIXED_BIT_SIZE;

	if (statusPdu.nackSoMap.empty() != TRUE)
	{
		std::multimap < UInt16, FxSlcStatusPduSoPair >
			::iterator it;
		for (it = statusPdu.nackSoMap.begin();
			it != statusPdu.nackSoMap.end();
			it++)
		{
			sizeBit += FX_SLC_AM_STATUS_PDU_EXTENSION_E1_E2_BIT_SIZE;
			if ((*it).second.soStart !=
				FX_SLC_INVALID_STATUS_PDU_FORMAT_NACK_SEGMENT_OFFSET)
			{
				sizeBit += FX_SLC_AM_STATUS_PDU_EXTENSION_SO_BIT_SIZE;
			}
		}
	}

	return (int)(ceil((double)sizeBit / 8));
}



// /**
// FUNCTION   :: SlcFxSetMessageSlcHeaderInfo
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Add SLC-PDU (AM entity) header information
//               to the message. (as Info)¸øÏûÏ¢Ìí¼Ó±¨Í·
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.(source)
// + msg       : Message*      : target message pointer.
// RETURN     :: void : NULL
// **/
void SlcFxSetMessageSlcHeaderInfo(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg)
{
	int pduHdrSize = SlcFxGetAmPduHeaderSize(node,
		iface,
		slcHeader);
	slcHeader.fixed->headerSize = pduHdrSize;
	MESSAGE_AddVirtualPayload(node, msg, pduHdrSize);

	SlcFxSetMessageSlcHeaderFixedInfo(node,
		iface,
		slcHeader,
		msg);
	SlcFxSetMessageSlcHeaderSegmentFormatFixedInfo(node,
		iface,
		slcHeader,
		msg);
	SlcFxSetMessageSlcHeaderExtensionInfo(node,
		iface,
		slcHeader,
		msg);
}

// /**
// FUNCTION   :: SlcFxSetMessageSlcHeaderFixedInfo
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Add SLC-PDU (AM entity) header information
//               to the message. (as Info)
//               SLC-PDU fixed part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.(source)
// + msg       : Message*      : target message pointer.
// RETURN     :: void : NULL
// **/
void SlcFxSetMessageSlcHeaderFixedInfo(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg)
{
	if (slcHeader.fixed != NULL)
	{
		MESSAGE_AppendInfo(node,
			msg,
			sizeof(FxSlcAmPduFormatFixed),
			(unsigned short)INFO_TYPE_FxSlcAmPduFormatFixed);

		FxSlcAmPduFormatFixed* info =
			(FxSlcAmPduFormatFixed*)MESSAGE_ReturnInfo(
				msg,
				(unsigned short)INFO_TYPE_FxSlcAmPduFormatFixed);

		if (info != NULL)
		{
			//*info = *(slcHeader.fixed);
			memcpy(info,
				slcHeader.fixed,
				sizeof(FxSlcAmPduFormatFixed));
		}
	}
}

// /**
// FUNCTION   :: SlcFxSetMessageSlcHeaderSegmentFormatFixedInfo
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Add SLC-PDU (AM entity) header information
//               to the message. (as Info)
//               SLC-PDU resegment-information part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.(source)
// + msg       : Message*      : target message pointer.
// RETURN     :: void : NULL
// **/
void SlcFxSetMessageSlcHeaderSegmentFormatFixedInfo(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg)
{
	if (slcHeader.segFixed != NULL) {
		MESSAGE_AppendInfo(
			node,
			msg,
			sizeof(FxSlcAmPduSegmentFormatFixed),
			(unsigned short)INFO_TYPE_FxSlcAmPduSegmentFormatFixed);

		FxSlcAmPduSegmentFormatFixed* info =
			(FxSlcAmPduSegmentFormatFixed*)MESSAGE_ReturnInfo(
				msg,
				(unsigned short)INFO_TYPE_FxSlcAmPduSegmentFormatFixed);

		if (info != NULL)
		{
			//*info = *(slcHeader.segFixed);
			memcpy(info,
				slcHeader.segFixed,
				sizeof(FxSlcAmPduSegmentFormatFixed));
		}
	}
}

// /**
// FUNCTION   :: SlcFxSetMessageSlcHeaderExtensionInfo
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Add SLC-PDU (AM entity) header information
//               to the message. (as Info)
//               SLC-PDU extention(segment-information:LI) part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.(source)
// + msg       : Message*      : target message pointer.
// RETURN     :: void : NULL
// **/
void SlcFxSetMessageSlcHeaderExtensionInfo(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg)
{
	if (slcHeader.extension != NULL)
	{
		slcHeader.extension->numLengthIndicator
			= slcHeader.extension->lengthIndicator.size();
		size_t hdrExtSize = sizeof(size_t)
			+ sizeof(UInt16) * slcHeader.extension->numLengthIndicator;

		MESSAGE_AppendInfo(
			node,
			msg,
			hdrExtSize,
			(unsigned short)INFO_TYPE_FxSlcAmPduFormatExtension);

		FxSlcAmPduFormatExtensionMsgInfo* info =
			(FxSlcAmPduFormatExtensionMsgInfo*)MESSAGE_ReturnInfo(
				msg,
				(unsigned short)INFO_TYPE_FxSlcAmPduFormatExtension);

		if (info != NULL)
		{
			memcpy(info,
				&(slcHeader.extension->numLengthIndicator),
				sizeof(size_t));

			UInt16 tmpLiData;
			int i = 0;
			std::vector < UInt16 > ::iterator it
				= slcHeader.extension->lengthIndicator.begin();
			while (it != slcHeader.extension->lengthIndicator.end())
			{
				tmpLiData = *it;
				memcpy(&(info->lengthIndicator) + i,
					&tmpLiData,
					sizeof(UInt16));
				++i;
				++it;
			}
		}

	}
}

// /**
// FUNCTION   :: SlcFxSetMessageStatusPduFormatInfo
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Add SLC-statusPDU to the message. (as Info)
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + msg       : Message*      : target message pointer.
// + status    : FxSlcStatusPduFormat& : SLC-satusPDU structure. (source)
// RETURN     :: void : NULL
// **/
void SlcFxSetMessageStatusPduFormatInfo(
	Node* node,
	int iface,
	Message* msg,
	FxSlcStatusPduFormat& status)
{
	SlcFxSetMessageStatusPduFormatFixedInfo(node,
		iface,
		status.fixed,
		msg);
	SlcFxSetMessageStatusPduFormatExtensionInfo(node,
		iface,
		status.nackSoMap,
		msg);
}

// /**
// FUNCTION   :: SlcFxSetMessageStatusPduFormatFixedInfo
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Add SLC-statusPDU header information to the message.
//               (as Info) SLC-statusPDU fixed part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-statusPDU fixed part structure.(source)
// + msg       : Message*      : target message pointer.
// RETURN     :: void : NULL
// **/
void SlcFxSetMessageStatusPduFormatFixedInfo(
	Node* node,
	int iface,
	FxSlcStatusPduFormatFixed& fixed,
	Message* msg)
{
	MESSAGE_AppendInfo(node,
		msg,
		sizeof(FxSlcStatusPduFormatFixed),
		(unsigned short)INFO_TYPE_FxSlcAmStatusPduPduFormatFixed);
	FxSlcStatusPduFormatFixed* info =
		(FxSlcStatusPduFormatFixed*)MESSAGE_ReturnInfo(
			msg,
			(unsigned short)INFO_TYPE_FxSlcAmStatusPduPduFormatFixed);

	if (info != NULL)
	{
		//*info = fixed;
		memcpy(info,
			&fixed,
			sizeof(FxSlcStatusPduFormatFixed));
	}
}

// /**
// FUNCTION   :: SlcFxSetMessageStatusPduFormatExtensionInfo
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Add SLC-statusPDU header information to the message.
//               (as Info) SLC-statusPDU extention(nack-information) part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + nackSoMap : std::multimap< UInt16, FxSlcStatusPduSoPair >&
//                             : SLC-statusPDU's nack-SO part structure.
//                             : (source)
// + msg       : Message*      : target message pointer.
// RETURN     :: void : NULL
// **/
void SlcFxSetMessageStatusPduFormatExtensionInfo(
	Node* node,
	int iface,
	std::multimap < UInt16, FxSlcStatusPduSoPair > &nackSoMap,
	Message* msg)
{
	if (nackSoMap.empty() != TRUE)
	{
		size_t unitSize = sizeof(FxSlcStatusPduFormatExtension);
		size_t nackInfoSize = nackSoMap.size();

		int size = (int)(unitSize * nackInfoSize + sizeof(size_t));

		MESSAGE_AppendInfo(node,
			msg,
			size,
			(unsigned short)INFO_TYPE_FxSlcAmStatusPduPduFormatExtension);

		FxSlcStatusPduFormatExtensionInfo* info =
			(FxSlcStatusPduFormatExtensionInfo*)MESSAGE_ReturnInfo(
				msg,
				(unsigned short)INFO_TYPE_FxSlcAmStatusPduPduFormatExtension);

		if (info != NULL)
		{
			info->nackInfoSize = nackInfoSize;

			size_t pos = 0;
			FxSlcStatusPduFormatExtension extension;
			std::multimap < UInt16, FxSlcStatusPduSoPair >
				::iterator it;
			for (it = nackSoMap.begin();
				it != nackSoMap.end();
				it++)
			{
				extension.nackSn = it->first;
				extension.soOffset = it->second;
				memcpy(&(info->nackInfo) + pos, &extension, unitSize);
				++pos;
			}
		}
	}
}

// /**
// FUNCTION   :: SlcFxGetMessageSlcHeaderFixedInfo
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU (AM entity) header information
//               from the message info-field.
//               SLC-PDU fixed part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// + msg       : Message*      : target message pointer.(source)
// RETURN     :: void : NULL
// **/
void SlcFxGetMessageSlcHeaderFixedInfo(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg)
{
	if (slcHeader.fixed != NULL)
	{
		FxSlcAmPduFormatFixed* info =
			(FxSlcAmPduFormatFixed*)MESSAGE_ReturnInfo(
				msg,
				(unsigned short)INFO_TYPE_FxSlcAmPduFormatFixed);

		if (info != NULL)
		{
			//*(slcHeader.fixed) = *info;
			memcpy(slcHeader.fixed,
				info,
				sizeof(FxSlcAmPduFormatFixed));
		}
	}
}

// /**
// FUNCTION   :: SlcFxGetMessageSlcHeaderSegmentFormatFixedInfo
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU (AM entity) header information
//               from the message info-field.
//               SLC-PDU resegment-information part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// + msg       : Message*      : target message pointer.(source)
// RETURN     :: void : NULL
// **/
void SlcFxGetMessageSlcHeaderSegmentFormatFixedInfo(
	Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg)
{
	if (slcHeader.segFixed != NULL) {
		FxSlcAmPduSegmentFormatFixed* info =
			(FxSlcAmPduSegmentFormatFixed*)MESSAGE_ReturnInfo(
				msg,
				(unsigned short)INFO_TYPE_FxSlcAmPduSegmentFormatFixed);

		if (info != NULL)
		{
			//*(slcHeader.segFixed) = *info;
			memcpy(slcHeader.segFixed,
				info,
				sizeof(FxSlcAmPduSegmentFormatFixed));
		}
	}
}

// /**
// FUNCTION   :: SlcFxGetMessageSlcHeaderExtensionInfo
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU(AM entity) header information
//               from the message info-field.
//               SLC-PDU extension-information(segment information:LI) part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// + msg       : Message*      : target message pointer.(source)
// RETURN     :: void : NULL
// **/
void SlcFxGetMessageSlcHeaderExtensionInfo(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg)
{
	if (slcHeader.extension != NULL)
	{
		FxSlcAmPduFormatExtensionMsgInfo* info =
			(FxSlcAmPduFormatExtensionMsgInfo*)MESSAGE_ReturnInfo(
				msg,
				(unsigned short)INFO_TYPE_FxSlcAmPduFormatExtension);

		if ((info != NULL) && (info->numLengthIndicator > 0))
		{
			slcHeader.extension->numLengthIndicator
				= info->numLengthIndicator;
			UInt16 tmpLiData;
			for (size_t i = 0; i < info->numLengthIndicator; i++)
			{
				memcpy(&tmpLiData,
					&(info->lengthIndicator) + i,
					sizeof(UInt16));
				slcHeader.extension->lengthIndicator.push_back(tmpLiData);
			}

		}
		else
		{
			slcHeader.extension->numLengthIndicator = 0;
			slcHeader.extension->lengthIndicator.clear();
		}
	}
}

// /**
// FUNCTION   :: SlcFxGetAmPduSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU(AM entity) header + data size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// + msg       : Message*      : target message pointer.
// RETURN     :: int : header + data size
// **/
int SlcFxGetAmPduSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg)
{
	return (SlcFxGetAmPduHeaderSize(node, iface, slcHeader) +
		SlcFxGetAmPduDataSize(node, iface, slcHeader, msg));
}

// /**
// FUNCTION   :: SlcFxGetAmPduHeaderSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU(AM entity) header size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// RETURN     :: int : header size
// **/
int SlcFxGetAmPduHeaderSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader)
{
	int retSize = 0;

	if (slcHeader.fixed != NULL)
	{
		if (slcHeader.fixed->dcPduFlag)
		{
			// PDU format or PDU segment format
			if (slcHeader.fixed->resegmentationFlag == FALSE) {
				retSize = SlcFxGetAmPduFormatSize(node, iface, slcHeader);
			}
			else
			{
				retSize = SlcFxGetAmPduSegmentFormatSize(
					node, iface, slcHeader);
			}
		}
	}

	return retSize;
}

// /**
// FUNCTION   :: SlcFxGetAmPduFormatSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU(AM entity) format header size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// RETURN     :: int : PDU format header size
// **/
int SlcFxGetAmPduFormatSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader)
{
	int retSize = 0;

	if (slcHeader.fixed->resegmentationFlag == FALSE)
	{
		retSize += SlcFxGetAmPduFormatFixedSize(node, iface, slcHeader);
		if (slcHeader.extension != NULL) {
			if (!slcHeader.extension->lengthIndicator.empty())
			{
				retSize += SlcFxGetAmPduFormatExtensionSize(node,
					iface,
					slcHeader);
			}
		}
	}
	return retSize;
}


// /**
// FUNCTION   :: SlcFxGetAmPduFormatFixedSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU(AM entity) header size of no segmented.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// RETURN     :: int : header size
// **/
int SlcFxGetAmPduFormatFixedSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader)
{
	return FX_SLC_AM_DATA_PDU_FORMAT_FIXED_BYTE_SIZE;
}

// /**
// FUNCTION   :: SlcFxGetAmPduFormatFixedSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU(AM entity) header size of no segmented.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// + size      : int           : number of LI
// RETURN     :: int : header size
// **/
int SlcFxGetAmPduLiAndESize(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	int size)
{
	if ((size % 2) == 0)
	{
		return (size * FX_SLC_AM_DATA_PDU_FORMAT_EXTENSION_BIT_SIZE) / 8;
	}
	else
	{
		return ((size * FX_SLC_AM_DATA_PDU_FORMAT_EXTENSION_BIT_SIZE) +
			FX_SLC_AM_DATA_PDU_FORMAT_EXTENSION_PADDING_BIT_SIZE) / 8;
	}
}

// /**
// FUNCTION   :: SlcFxGetAmPduFormatExtensionSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU(AM entity) header size of segmented.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// RETURN     :: int : header size
// **/
int SlcFxGetAmPduFormatExtensionSize(
	Node* node,
	int iface,
	FxSlcHeader& slcHeader)
{
	if (slcHeader.extension != NULL)
	{
		return SlcFxGetAmPduLiAndESize(
			node,
			iface,
			slcHeader,
			slcHeader.extension->lengthIndicator.size());
	}
	else
	{
		return 0;
	}
}

// /**
// FUNCTION   :: SlcFxGetAmPduFormatExtensionSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU(AM entity) header size of re-segmented.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// RETURN     :: int : header size
// **/
int SlcFxGetAmPduSegmentFormatSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader)
{
	int retSize = 0;
	if (slcHeader.fixed->resegmentationFlag == TRUE)
	{
		retSize += SlcFxGetAmPduSegmentFormatFixedSize(node,
			iface,
			slcHeader);
		if (slcHeader.segFixed != NULL)
		{
			retSize += SlcFxGetAmPduSegmentFormatExtensionSize(
				node,
				iface,
				slcHeader);
		}
	}

	return retSize;
}

// /**
// FUNCTION   :: SlcFxGetAmPduFormatExtensionSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU(AM entity) header's fixed part size of
//               re-segmented.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// RETURN     :: int : header size
// **/
int SlcFxGetAmPduSegmentFormatFixedSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader)
{
	return (FX_SLC_AM_DATA_PDU_FORMAT_FIXED_BYTE_SIZE
		+ FX_SLC_AM_DATA_PDU_RESEGMENT_FORMAT_EXTENSIO_BYTE_SIZE);
}

// /**
// FUNCTION   :: SlcFxGetAmPduFormatExtensionSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU(AM entity) header's extention part size
//               re-segmented.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// RETURN     :: int : header size
// **/
int SlcFxGetAmPduSegmentFormatExtensionSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader)
{
	if (slcHeader.extension != NULL)
	{
		return SlcFxGetAmPduLiAndESize(
			node,
			iface,
			slcHeader,
			slcHeader.extension->lengthIndicator.size());
	}
	else
	{
		return 0;
	}
}

// /**
// FUNCTION   :: SlcFxGetAmPduDataSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU(AM entity) data size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// + msg       : Message*      : SLC-PDU messages
// RETURN     :: int : data size
// **/
int SlcFxGetAmPduDataSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg)
{
	return MESSAGE_ReturnPacketSize(msg);
}

// /**
// FUNCTION   :: SlcFxGetAmPduListDataSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU(AM entity) data size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// + msgList   : Message*      : SLC-PDU messages list
// RETURN     :: int : data size
// **/
int SlcFxGetAmPduListDataSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msgList)
{
	int size = 0;
	Message* tmp = msgList;

	while (tmp != NULL)
	{
		size += MESSAGE_ReturnPacketSize(tmp);
		tmp = tmp->next;
	}

	return size;
}

// /**
// FUNCTION   :: SlcFxGetExpectedAmPduSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU(AM entity) expected size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// + reSegment : BOOL          : flag of re-segmented
// + msgList   : Message*      : SLC-PDU messages list
// + txBuffData: FxSlcAmTxBuffer& : transmittion buffer
// RETURN     :: int : header + data size
// **/
int SlcFxGetExpectedAmPduSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	BOOL reSegment,
	Message* msgList,
	//Message* addMsg)
	FxSlcAmTxBuffer& txBuffData)
{
	int size = 0;

	// expect header size
	size += SlcFxGetExpectedAmPduHeaderSize(node,
		iface,
		slcHeader,
		reSegment);
	if ((slcHeader.extension == NULL) && (txBuffData.offset != 0))
	{
		size += FX_SLC_AM_DATA_PDU_FORMAT_EXTENSION_MIN_BYTE_SIZE;
	}

	// expect data size
	size += SlcFxGetExpectedAmPduDataSize(node,
		iface,
		slcHeader,
		msgList,
		//addMsg);
		txBuffData.message);

	return size;
}

// /**
// FUNCTION   :: SlcFxGetExpectedAmPduHeaderSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU(AM entity) header expected size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// + reSegment : BOOL          : flag of re-segmented
// RETURN     :: int : header size
// **/
int SlcFxGetExpectedAmPduHeaderSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	BOOL reSegment)
{
	int size = 0;
	int extSize = 0;

	if (reSegment == FALSE)
	{
		// fixed header size
		size += FX_SLC_AM_DATA_PDU_FORMAT_FIXED_BYTE_SIZE;

		if ((slcHeader.firstSdu != TRUE)
			&& (slcHeader.extension == NULL))
		{
			extSize = 1; // first sdu 12bit = 1.5 octet
			extSize++;   // current sdu 12bit = 1.5 octet
		}

		// expand header size
		if (slcHeader.extension != NULL)
		{
			extSize += slcHeader.extension->lengthIndicator.size();
			extSize++;
			size += SlcFxGetAmPduLiAndESize(node,
				iface,
				slcHeader,
				extSize);
		}
	}
	else
	{
		size = slcHeader.fixed->headerSize;

		if (slcHeader.segFixed == NULL)
		{
			size += FX_SLC_AM_DATA_PDU_RESEGMENT_FORMAT_EXTENSIO_BYTE_SIZE;
		}
	}

	return size;
}

// /**
// FUNCTION   :: SlcFxGetExpectedAmPduDataSize
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Get SLC-PDU(AM entity) data expected size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// + msgList   : Message*      : SLC-PDU messages list
// + addMsg    : Message*      : SLC-PDU messages
// RETURN     :: int : data size
// **/
int SlcFxGetExpectedAmPduDataSize(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msgList,
	Message* addMsg)
{
	int size = 0;
	Message* tmp = msgList;

	while (tmp != NULL)
	{
		size += MESSAGE_ReturnPacketSize(tmp);
		tmp = tmp->next;
	}

	size += MESSAGE_ReturnPacketSize(addMsg);
	return size;
}

// /**
// FUNCTION   :: SlcFxAllocAmPduLi
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: allocate LI field for SLC-PDU(AM entity) header.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// + size      : UInt16        : LI (size of massage)
// RETURN     :: void : NULL
// **/
void SlcFxAllocAmPduLi(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	UInt16 size)
{
	if (slcHeader.extension == NULL)
	{
		slcHeader.extension = new FxSlcAmPduFormatExtension();
	}

	slcHeader.extension->numLengthIndicator++;
	slcHeader.extension->lengthIndicator.push_back(size);

	return;
}


// /**
// FUNCTION   :: SlcFxAddAmPduLi
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: add LI to SLC-PDU(AM entity) header.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + slcHeader : FxSlcHeader  : SLC-PDU header structure.
// + msg       : Message*      : massage
// RETURN     :: void : NULL
// **/
void SlcFxAddAmPduLi(Node* node,
	int iface,
	FxSlcHeader& slcHeader,
	Message* msg)
{
	int size = MESSAGE_ReturnPacketSize(msg);

	SlcFxAllocAmPduLi(node, iface, slcHeader, (UInt16)size);

	return;
}

// /**
// FUNCTION   :: SlcFxDelAmPduHdrObj
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: delete the data-structure in SLC header
// PARAMETERS ::
//  + slcHeader : FxSlcHeader& : SLC header
// RETURN     :: void     : NULL
// **/
void SlcFxDelAmPduHdrObj(FxSlcHeader& slcHeader)
{
	if (slcHeader.fixed != NULL)
	{
		delete slcHeader.fixed;
		slcHeader.fixed = NULL;
	}

	if (slcHeader.segFixed != NULL)
	{
		delete slcHeader.segFixed;
		slcHeader.segFixed = NULL;
	}

	if (slcHeader.extension != NULL)
	{
		delete slcHeader.extension;
		slcHeader.extension = NULL;
	}
}


void SlcFxCpyAmPduHdrObj(const FxSlcHeader& obj, FxSlcHeader& slcHeader)
{
	if (obj.fixed != NULL)
	{
		slcHeader.fixed = new FxSlcAmPduFormatFixed();
		*slcHeader.fixed = *obj.fixed;
	}

	if (obj.segFixed != NULL)
	{
		slcHeader.segFixed = new FxSlcAmPduSegmentFormatFixed();
		*slcHeader.segFixed = *obj.segFixed;
	}

	if (obj.extension != NULL)
	{
		slcHeader.extension = new FxSlcAmPduFormatExtension();
		*slcHeader.extension = *obj.extension;
	}
	slcHeader.firstSdu = obj.firstSdu;
	slcHeader.firstSduLi = obj.firstSduLi;
}

// /**
// FUNCTION::           SlcFxAmInitPollRetrunsmitTimer
// LAYER::              FX LAYER2 SLC
// PURPOSE::            Init PollRetrunsmit Timer
// PARAMETERS::
//  + node     : Node*           : Node pointer
//  + iface    : int             : Inteface index
//  + amEntity : FxSlcAmEntity* : The SLC entity
// RETURN::    void:         NULL
// **/
static
void SlcFxAmInitPollRetrunsmitTimer(Node* node,
	int iface,
	FxSlcAmEntity* amEntity)
{
	FxSlcEntity* slcEntity = (FxSlcEntity*)(amEntity->entityVar);
	if (amEntity->pollRetransmitTimerMsg)
	{
		MESSAGE_CancelSelfMsg(node, amEntity->pollRetransmitTimerMsg);
		amEntity->pollRetransmitTimerMsg = NULL;
	}
	amEntity->pollRetransmitTimerMsg =
		SlcFxInitTimer(node,
			iface,
			MSG_SLC_FX_PollRetransmitTimerExpired,
			slcEntity->oppositeRnti,
			slcEntity->bearerId,
			FX_SLC_ENTITY_TX);
	amEntity->exprioryTPollRetransmitFlg = FALSE;
	MESSAGE_Send(node,
		amEntity->pollRetransmitTimerMsg,
		GetFxSlcConfig(node, iface)->tPollRetransmit *
		MILLI_SECOND);

}

// /**
// FUNCTION::       SlcFxSeqNumInWnd
// LAYER::          FX Layer2 SLC
// PURPOSE::        Return whether a sequence number is within the window
// PARAMETERS::
//  + sn      : unsigned int : The sequence number to be tested
//  + snStart : unsigned int : The sequence number of window starting point
//  + wnd     : unsigned int : The window size
//  + snBound : unsigned int : The upperbound of sequence number
// RETURN::    void:     BOOL
// **/
//
static
BOOL SlcFxSeqNumInWnd(unsigned int sn,
	unsigned int snStart,
	unsigned int wnd,
	unsigned int snBound)
{
	ERROR_Assert(wnd > 0 && wnd <= snBound / 2,
		"SlcFxSeqNumInWnd: window size must be larger than 0 "
		"and smaller than half of the maximum sequence number. ");
	unsigned int snEnd = (snStart + wnd) % snBound;
	if (snStart < snEnd)
	{
		return (snStart <= sn && sn < snEnd);
	}
	else
	{
		return (snStart <= sn || sn < snEnd);
	}
}

static
void SlcFxAmInitResetTimer(Node* node,
	int iface,
	FxSlcAmEntity* amEntity,
	FxSlcEntityDirect direct)
{
	ERROR_Assert(direct != FX_SLC_ENTITY_BI,
		"Wrong FxSlcEntityDirect(FX_SLC_ENTITY_BI).");

	FxSlcEntity* slcEntity = (FxSlcEntity*)(amEntity->entityVar);
	if (amEntity->resetTimerMsg)
	{
		MESSAGE_CancelSelfMsg(node, amEntity->resetTimerMsg);
		amEntity->resetTimerMsg = NULL;
	}
	amEntity->resetTimerMsg =
		SlcFxInitTimer(node,
			iface,
			MSG_SLC_FX_ResetTimerExpired,
			slcEntity->oppositeRnti,
			slcEntity->bearerId,
			direct);
	clocktype delay = (direct == FX_SLC_ENTITY_TX)
		? FX_SLC_RESET_TIMER_DELAY_MSEC * MILLI_SECOND
		: (FX_SLC_RESET_TIMER_DELAY_MSEC - 1) * MILLI_SECOND;

	MESSAGE_Send(node,
		amEntity->resetTimerMsg,
		delay);

}
int FxSlcAmReTxBuffer::getNackedSize(Node* node, int iface)
{
	UInt32 retSize = 0;
	std::map < int, FxSlcStatusPduSoPair > ::iterator
		it = nackField.begin();
	while (it != nackField.end())
	{
		ERROR_Assert((*it).second.soEnd >= (*it).second.soStart,
			"SLC-FX SegmentOffset field: start > end !");
		retSize = (*it).second.soEnd - (*it).second.soStart;
		it++;
	}
	return retSize;
}
void FxSlcAmEntity::resetAll(BOOL withoutResetProcess)
{

	// reset tx buffer
	std::for_each(
		txBuffer.begin(),
		txBuffer.end(),
		FxSlcFreeMessageData < FxSlcAmTxBuffer >(entityVar->node));
	txBuffer.clear();
	txBufSize = 0;

	// reset latestPdu
	SlcFxDelAmPduHdrObj(latestPdu.slcHeader);
	MESSAGE_FreeList(entityVar->node, latestPdu.message);
	latestPdu.message = NULL;

	// reset retx buffer
	std::list < FxSlcAmReTxBuffer > ::iterator
		it = retxBuffer.begin();
	while (it != retxBuffer.end())
	{
		SlcFxDelAmPduHdrObj((*it).slcHeader);
		++it;
	}
	std::for_each(
		retxBuffer.begin(),
		retxBuffer.end(),
		FxSlcFreeMessageData < FxSlcAmReTxBuffer >(entityVar->node));
	retxBuffer.clear();
	retxBufSize = 0;
	retxBufNackSize = 0;

	// reset status pdu buffer
	statusBuffer.clear();
	statusBufSize = 0;

	// reset rx sdu buffer
	std::multimap < unsigned int, Message* > ::iterator msgItr;
	for (msgItr = rcvSlcSduList.begin();
		msgItr != rcvSlcSduList.end();
		++msgItr)
	{
		MESSAGE_FreeList(entityVar->node, msgItr->second);
	}
	rcvSlcSduList.clear();
	std::list < FxSlcSduSegment > ::iterator sduItr;
	for (sduItr = rcvSlcSduSegList.begin();
		sduItr != rcvSlcSduSegList.end();
		++sduItr)
	{
		MESSAGE_FreeList(entityVar->node, (*sduItr).newMsg);
	}
	rcvSlcSduSegList.clear();
	for (sduItr = rcvSlcSduTempList.begin();
		sduItr != rcvSlcSduTempList.end();
		++sduItr)
	{
		MESSAGE_FreeList(entityVar->node, (*sduItr).newMsg);
	}
	rcvSlcSduTempList.clear();

	// reset unreceived ACK map
	unreceivedSlcAckList.clear();

	// reset rx pdu buffer
	std::map < unsigned int, FxSlcAmRcvPduData > ::iterator pduItr;
	for (pduItr = rcvPduBuffer.begin();
		pduItr != rcvPduBuffer.end();
		++pduItr)
	{
		pduItr->second.lengthIndicator.clear();
		MESSAGE_FreeList(entityVar->node, pduItr->second.message);
		pduItr->second.rcvdPduSegmentInfo.clear();
		pduItr->second.rcvNackedSegOffset.clear();
	}
	rcvPduBuffer.clear();

	// reset timer
	if (pollRetransmitTimerMsg != NULL) {
		MESSAGE_CancelSelfMsg(entityVar->node,
			pollRetransmitTimerMsg);
		pollRetransmitTimerMsg = NULL;
	}

	if (reoderingTimerMsg != NULL) {
		MESSAGE_CancelSelfMsg(entityVar->node,
			reoderingTimerMsg);
		reoderingTimerMsg = NULL;
	}

	if (statusProhibitTimerMsg != NULL) {
		MESSAGE_CancelSelfMsg(entityVar->node,
			statusProhibitTimerMsg);
		statusProhibitTimerMsg = NULL;
	}

	// Tx window
	ackSn = 0;
	sndWnd = FX_SLC_AM_WINDOW_SIZE;
	sndNextSn = 0;
	pollSn = 0;

	// Rx window
	rcvNextSn = 0;
	rcvWnd = FX_SLC_AM_WINDOW_SIZE;
	tReorderingSn = FX_SLC_INVALID_SEQ_NUM;
	maxStatusSn = 0;
	rcvSn = 0;

	oldRcvNextSn = 0;

	// reset variable
	pduWithoutPoll = 0;
	byteWithoutPoll = 0;
	retxCount = 0;

	// reset flag
	exprioryTPollRetransmitFlg = FALSE;
	waitExprioryTStatusProhibitFlg = FALSE;

	if (withoutResetProcess == FALSE)
	{
		if (resetTimerMsg != NULL) {
			MESSAGE_CancelSelfMsg(entityVar->node,
				resetTimerMsg);
			resetTimerMsg = NULL;
			resetFlg = FALSE;
			sendResetFlg = FALSE;
		}
	}
}
void SlcFxAmSetResetData(
	Node* node,
	int interfaceIndex,
	FxSlcAmEntity* amEntity)
{
	amEntity->resetFlg = TRUE;
}
UInt32 FxSlcAmEntity::getNumAvailableRetxPdu(
	Node* node,
	int interfaceIndex,
	int size)
{
	int ret = 0;
	std::list < FxSlcAmReTxBuffer > ::iterator itr = retxBuffer.begin();
	int tempSize = 0;

	while (itr != retxBuffer.end())
	{
		int nackedSize = itr->getNackedSize(node, interfaceIndex);
		tempSize += nackedSize;
		if (nackedSize > 0)
		{
			ret++;
		}
		if (tempSize > size)
		{
			break;
		}
		++itr;
	}

	return ret;
}

// /**
// FUNCTION::           SlcFxAmReceivePduFromSmac
// LAYER::              FX LAYER2 SLC
// PURPOSE::            Invoked when MAC submit a PDU to the entity
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         the interface index
//      +amEntity:      TM SLC AM entity
//      +msgList:       PDU message list
// RETURN::             NULL
// **/
static
void SlcFxAmReceivePduFromSmac(
	Node* node,
	int iface,
	const fxRnti& srcRnti,
	const int bearerId,
	FxSlcAmEntity* amEntity,
	std::list < Message* > &msgList)
{
	FxSlcData* slcData = SlcFxGetSubLayerData(node, iface);
	std::list < Message* > ::iterator it;
	for (it = msgList.begin(); it != msgList.end(); )
	{
		Message* pduMsg = *it;

		// inspect what type of PDU it is
		if (SlcFxAmGetDCBit(pduMsg))
		{
			if (amEntity->resetFlg == TRUE)
			{
				slcData->stats.
					numDataPdusReceivedFromMacSublayerButDiscardedByReset++;
				MESSAGE_Free(node, pduMsg);
				it = msgList.erase(it);
				continue;
			}
			slcData->stats.numDataPdusReceivedFromMacSublayer++;

			Message* dupMsg = MESSAGE_Duplicate(node, pduMsg);
			SlcFxExtractSduSegmentFromPdu(node,
				iface,
				amEntity->entityVar,
				dupMsg);
		}
		else
		{
			// Received an control PDU.
			// -> check the CPT(Control PDU Type).
			UInt8 controlPduType = SlcFxAmGetCPT(pduMsg);
			if (controlPduType == FX_SLC_AM_CONTROL_PDU_TYPE_STATUS)
			{
				if (amEntity->resetFlg == TRUE)
				{

					slcData->stats.
						numAmStatusPdusReceivedFromMacSublayerButDiscardedByReset++;
					MESSAGE_Free(node, pduMsg);
					it = msgList.erase(it);
					continue;
				}

				slcData->stats.numAmStatusPdusReceivedFromMacSublayer++;

				SlcFxAmReceiveStatusPdu(node,
					iface,
					srcRnti,
					bearerId,
					amEntity,
					pduMsg);

			}
			else
			{
				std::stringstream log;
				log.str("");
				log << "SlcFxAmReceivePduFromSmac:";
				log << "received an invalid type of control PDU.";

				ERROR_Assert(FALSE, log.str().c_str());
			}
		}

		MESSAGE_Free(node, pduMsg);
		it = msgList.erase(it);
	}
}

static
UInt32 SlcFxAmSeqNumDist(UInt32 sn1, UInt32 sn2)
{
	if (sn1 <= sn2)
	{
		return sn2 - sn1;
	}
	else
	{
		return sn2 + (FX_SLC_AM_SN_UPPER_BOUND - sn1);
	}
}

/**
// FUNCTION::           SlcFxExtractSduSegmentFromPdu
// LAYER::              FX LAYER2 SLC
// PURPOSE::            Separate segments belonging to various SDU so that
//                      they can be inserted into a
//                      list to facilate re-assembly
//                      once all segments of an SDU have arrived
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         the interface index
//      +slcEntity:     The slc entity doing the reassembly
//      +pduMsg:        The received PDU
// RETURN::             NULL
// **/
static
void SlcFxExtractSduSegmentFromPdu(
	Node* node,
	int iface,
	FxSlcEntity* slcEntity,
	Message* pduMsg)
{
	char errorMsg[MAX_STRING_LENGTH] = "SlcFxExtractSduSegmentFromPdu: ";

	FxSlcEntityType mode = slcEntity->entityType;
	Int16 rcvdSeqNum = 0;

	if (mode == FX_SLC_ENTITY_AM)
	{
		FxSlcAmEntity* amEntity =
			(FxSlcAmEntity*)slcEntity->entityData;
		ERROR_Assert(amEntity, errorMsg);
		rcvdSeqNum = SlcFxAmGetSeqNum(pduMsg);
		if (rcvdSeqNum < 0)
		{
			sprintf(errorMsg + strlen(errorMsg),
				"invalid Sequense number.");
			ERROR_Assert(FALSE, errorMsg);
		}
	}
	else
	{
		sprintf(errorMsg + strlen(errorMsg),
			"worng SLC entity type for Phase-1.");
		ERROR_Assert(FALSE, errorMsg);
	}

	BOOL screenRes = FALSE;

	if (mode == FX_SLC_ENTITY_AM)
	{
		FxSlcAmEntity* amEntity
			= (FxSlcAmEntity*)slcEntity->entityData;

		screenRes = SlcFxAmScreenRcvdPdu(node,
			iface,
			amEntity,
			rcvdSeqNum,
			pduMsg);
	}

	// get P field
	BOOL pollBitActive = SlcFxAmGetPollBit(pduMsg);

	// outside of window, or aleady received message.
	if (screenRes == FALSE)
	{

		MESSAGE_Free(node, pduMsg);
		return;
	}
	if (mode == FX_SLC_ENTITY_AM)
	{
		FxSlcAmEntity* amEntity
			= (FxSlcAmEntity*)slcEntity->entityData;
		// update reception window if received VR(R)'s SN completely
		if ((UInt32)rcvdSeqNum == amEntity->rcvNextSn)
		{
			std::map < unsigned int, FxSlcAmRcvPduData > ::iterator
				itSlcPduBuff;
			UInt32 chkNum = SlcFxAmSeqNumDist(amEntity->rcvNextSn,
				amEntity->rcvWnd);
			for (UInt32 i = 0; i < chkNum; i++)
			{
				itSlcPduBuff
					= amEntity->rcvPduBuffer.find(amEntity->rcvNextSn);
				if ((itSlcPduBuff == amEntity->rcvPduBuffer.end())
					|| (itSlcPduBuff->second.slcPduRcvFinished == FALSE))
				{
					break;
				}
				else
				{
					SlcFxAmIncSN(amEntity->rcvNextSn);
				}
			}

			amEntity->rcvWnd
				= (amEntity->rcvNextSn + FX_SLC_AM_WINDOW_SIZE)
				% FX_SLC_AM_SN_UPPER_BOUND;


			// reassemble slc-SDU(sac-PDU).
			SlcFxSDUsegmentReassemble(node,
				iface,
				slcEntity);
		}

		// check reorderingTimer
		if (amEntity->reoderingTimerMsg != NULL)
		{
			if ((amEntity->tReorderingSn == (Int32)amEntity->rcvNextSn)
				|| ((SlcFxAmSeqNumInWnd(rcvdSeqNum, amEntity->rcvNextSn)
					== FALSE)
					&& (amEntity->tReorderingSn != (Int32)amEntity->rcvWnd)))
			{
				MESSAGE_CancelSelfMsg(node, amEntity->reoderingTimerMsg);
				amEntity->reoderingTimerMsg = NULL;
				amEntity->tReorderingSn = FX_SLC_INVALID_SEQ_NUM;
			}
		}
		if (amEntity->reoderingTimerMsg == NULL)
		{
			SlcFxAmInitReoderingTimer(node, iface, amEntity);
			amEntity->tReorderingSn = amEntity->rcvSn;
		}

		// P field=1(TRUE) && SN < VR(MS) -> create statusPDU
		if (pollBitActive
			&& SlcFxSeqNumLessChkInWnd(rcvdSeqNum,
				amEntity->maxStatusSn,
				amEntity->rcvNextSn,
				amEntity->rcvWnd,
				FX_SLC_AM_SN_UPPER_BOUND))
		{

			SlcFxAmCreateStatusPdu(node, iface, amEntity);
		}
		else
		{

		}

	}
}
// /**
// FUNCTION   :: SlcFxAmGetDCBit
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: get D/Cbit data from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
BOOL SlcFxAmGetDCBit(Message* msg)
{
	BOOL   ret = FALSE;

	FxSlcAmPduFormatFixed* info =
		(FxSlcAmPduFormatFixed*)MESSAGE_ReturnInfo(msg,
		(unsigned short)INFO_TYPE_FxSlcAmPduFormatFixed);

	if (info != NULL)
	{
		ret = info->dcPduFlag;
	}
	else
	{
		// status pdu
	}

	return ret;
}
// /**
// FUNCTION::       SlcFxAmReceiveStatusPdu
// LAYER::          FX LAYER2 SLC
// PURPOSE::        Invoked when AM entity receives a STATUS PDU
// PARAMETERS::
//      +node:      The node pointer
//      +iface:     The interface index
//      +amEntity:  AM entity
//      +pduMsg:    The received STATUS PDU message
// RETURN::         NULL
// **/
//static
void SlcFxAmReceiveStatusPdu(Node* node,
	int iface,
	const fxRnti& srcRnti,
	const int bearerId,
	FxSlcAmEntity* amEntity,
	Message* pduMsg)
{
	FxSlcStatusPduFormat rxStatusPDU;
	rxStatusPDU.fixed.ackSn = SlcFxAmGetAckSn(pduMsg);
	SlcFxAmGetNacklist(pduMsg, &(rxStatusPDU.nackSoMap));
	SlcFxAmHandleAck(node, iface, srcRnti, bearerId, amEntity, rxStatusPDU);
}
// /**
// FUNCTION::       SlcFxGetSubLayerData
// LAYER::          FX LAYER2 SLC
// PURPOSE::        Get SLC sublayer pointer from node and interface Id
// PARAMETERS::
//  + node : Node* : node pointer
//  + ifac : int   : interface id
// RETURN::         fxSlcVar : SLC sublayer data
// **/
static
FxSlcData* SlcFxGetSubLayerData(Node* node, int iface)
{
	Layer2DataFx* layer2DataFx
		= FxLayer2GetLayer2DataFx(node, iface);
	return (FxSlcData*)layer2DataFx->fxSlcVar;
}
// /**
// FUNCTION::       SlcFxAmGetSeqNum
// LAYER::          FX Layer2 SLC
// PURPOSE::        Get a sequence number (AM)
// PARAMETERS::
//  + msg : Message* : the msg where the The sequence number to be get
// RETURN::   unsigned int:     the sequence number
// **/
static
Int16 SlcFxAmGetSeqNum(const Message* msg)
{
	Int16   seqNum = FX_SLC_INVALID_SEQ_NUM;

	FxSlcAmPduFormatFixed* info =
		(FxSlcAmPduFormatFixed*)MESSAGE_ReturnInfo(msg,
		(unsigned short)INFO_TYPE_FxSlcAmPduFormatFixed);

	if (info != NULL)
	{
		seqNum = info->seqNum;
	}
	else
	{
		ERROR_Assert(FALSE, "SlcFxAmGetSeqNum: no fixed header field!");
	}

	return seqNum;
}
// /**
// FUNCTION::           SlcFxAmScreenRcvdPdu
// LAYER::              FX LAYER2 SLC
// PURPOSE::            When receiving a new PDU, the AM entity
//                      decides to drop the PDU or drop some buffered
//                      PDU depend on the seq. number of received PDU and
//                      the state variables
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         The interface index
//      +amEntity:      The AM entity
//      +rcvdSeqNum:    The seqence number of the PDU, from which
//                      the sdu segments are extracted
//      +pduMsg:        The received PDU
// RETURN::             BOOL
// **/
static
BOOL SlcFxAmScreenRcvdPdu(Node* node,
	int iface,
	FxSlcAmEntity* amEntity,
	unsigned int rcvdSeqNum,
	Message* pduMsg)
{
	BOOL prepStatusPdu = FALSE;
	BOOL ret = FALSE;
	std::map < unsigned int, FxSlcAmRcvPduData > ::iterator
		itRcvdSn = amEntity->rcvPduBuffer.find(rcvdSeqNum);

	if (SlcFxAmSeqNumInWnd(rcvdSeqNum, amEntity->rcvNextSn) == FALSE)
	{
		// receved sequense number is outside reception window.

		prepStatusPdu = TRUE;
	}
	else
	{
		if ((itRcvdSn != amEntity->rcvPduBuffer.end())
			&& (itRcvdSn->second.slcPduRcvFinished == TRUE))
		{
			// Received SN(slc-PDU)

			prepStatusPdu = TRUE;
		}
	}

	// If there is a polling bit set, prepare status PDU
	if (prepStatusPdu == TRUE)
	{
		if (SlcFxAmGetPollBit(pduMsg))
		{


			SlcFxAmCreateStatusPdu(node, iface, amEntity);
		}

		return ret;
	}

	// if reached PDU, process received PDU
	ret = TRUE;

	FxSlcAmRcvPduData rcvData;
	rcvData.FramingInfo = SlcFxAmGetFramingInfo(pduMsg);
	SlcFxAmGetLIlist(pduMsg, &(rcvData.lengthIndicator));
	rcvData.orgMsgSize = SlcFxGetOrgPduSize(node, pduMsg);
	// first received SN
	if (itRcvdSn == amEntity->rcvPduBuffer.end())
	{
		amEntity->rcvPduBuffer.insert(
			make_pair(rcvdSeqNum, rcvData));
	}

	if (SlcFxAmGetRFBit(pduMsg) == TRUE)
	{
		ret = SlcFxAmPduSegmentReassemble(node,
			iface,
			amEntity,
			rcvdSeqNum,
			pduMsg);
	}
	// receives SLC-PDU.(not segment)
	else
	{
		amEntity->rcvPduBuffer[rcvdSeqNum].slcPduRcvFinished = TRUE;
		amEntity->rcvPduBuffer[rcvdSeqNum].message = pduMsg;
		amEntity->rcvPduBuffer[rcvdSeqNum].FramingInfo
			= rcvData.FramingInfo;
		amEntity->rcvPduBuffer[rcvdSeqNum].lengthIndicator
			= rcvData.lengthIndicator;

	}

	// update VR(H):Highest received state variable.
	if (SlcFxSeqNumLessChkInWnd(amEntity->rcvSn,
		rcvdSeqNum,
		amEntity->rcvNextSn,
		amEntity->rcvWnd,
		FX_SLC_AM_SN_UPPER_BOUND)
		|| (amEntity->rcvSn == rcvdSeqNum))
	{
		UInt32 tmpSN = (UInt32)rcvdSeqNum;
		SlcFxAmIncSN(tmpSN);

		amEntity->rcvSn = tmpSN;
	}

	// update VR(MS):Maximum STATUS transmit state variable.

	if (rcvdSeqNum == amEntity->maxStatusSn)
	{
		UInt32 chkSeqNum = amEntity->maxStatusSn;
		SlcFxAmIncSN(chkSeqNum);
		UInt32 chkNum =
			SlcFxAmSeqNumDist(chkSeqNum, amEntity->rcvWnd);
		BOOL isPduFound = FALSE;
		BOOL isPduSegFound = FALSE;
		BOOL isEmptyFound = FALSE;
		BOOL isTmpEmptyFound = FALSE;
		BOOL isMaxSatusSnUpdated = FALSE;
		UInt32 tmpMaxStatusSn = amEntity->maxStatusSn;
		for (UInt32 i = 0; i < chkNum; i++)
		{
			UInt32 curSn = chkSeqNum + i;
			itRcvdSn
				= amEntity->rcvPduBuffer.find(curSn);
			if (itRcvdSn == amEntity->rcvPduBuffer.end())
			{
				if (isMaxSatusSnUpdated == FALSE)
				{
					tmpMaxStatusSn = curSn;
					isMaxSatusSnUpdated = TRUE;
				}
				if (isEmptyFound == FALSE)
				{
					isTmpEmptyFound = TRUE;
				}
			}
			else if (itRcvdSn->second.slcPduRcvFinished == FALSE)
			{
				isPduSegFound = TRUE;
				if (isMaxSatusSnUpdated == FALSE)
				{
					tmpMaxStatusSn = curSn;
					isMaxSatusSnUpdated = TRUE;
				}
				if ((isEmptyFound == FALSE) && (isTmpEmptyFound == TRUE))
				{
					isEmptyFound = TRUE;
				}
			}
			else
			{
				isPduFound = TRUE;
				if ((isEmptyFound == FALSE) && (isTmpEmptyFound == TRUE))
				{
					isEmptyFound = TRUE;
				}
			}
		}
		// (Found PDU Segment or Found Toothless SN) and (maxStatusSn need be updated)
		// e.g.) 1,2,4 : 3 is Toothless SN
		if (((isPduSegFound == TRUE) || (isEmptyFound == TRUE))
			&& (isMaxSatusSnUpdated == TRUE))
		{
			amEntity->maxStatusSn = tmpMaxStatusSn;
		}
	}


	return ret;
}
// /**ÓÃ¹ý
// FUNCTION::       SlcFxAmCreateStatusPdu
// LAYER::          FX LAYER2 SLC
// PURPOSE::        Create a STATUS PDU message
//                  and stack to the status buffer.
// PARAMETERS::
//      +node:      The node pointer
//      +iface:     The interface index
//      +amEntity:  The AM entity
// RETURN::         NULL
// **/
static
void SlcFxAmCreateStatusPdu(
	Node* node,
	int iface,
	FxSlcAmEntity* amEntity)
{

	if (amEntity->statusProhibitTimerMsg != NULL)
	{
		amEntity->waitExprioryTStatusProhibitFlg = TRUE;
	}
	else
	{

		FxSlcStatusPduFormat statusPdu;
		std::map < unsigned int, FxSlcAmRcvPduData > ::iterator
			itSlcPduBuff;

		UInt32 chkNum = 0;
		BOOL isMaxStatusSnLessThanRcvNextSn =
			SlcFxSeqNumLessChkInWnd(
				amEntity->maxStatusSn,
				amEntity->rcvNextSn,
				amEntity->rcvNextSn,
				amEntity->rcvWnd,
				FX_SLC_AM_SN_UPPER_BOUND);
		// NACK settings VR(R):rcvNextSn ~ VR(MS):maxStatusSn
		chkNum = SlcFxAmSeqNumDist(amEntity->rcvNextSn,
			amEntity->maxStatusSn) + 1;
		BOOL isExistSlcPduSeg = FALSE;
		UInt32 lastSlcPduSegSn = amEntity->rcvNextSn;
		for (UInt32 i = 0; i < chkNum; i++)
		{
			UInt32 chkSn = (amEntity->rcvNextSn + i)
				% FX_SLC_AM_SN_UPPER_BOUND;
			itSlcPduBuff = amEntity->rcvPduBuffer.find(chkSn);
			if (itSlcPduBuff != amEntity->rcvPduBuffer.end())
			{
				isExistSlcPduSeg = TRUE;
				lastSlcPduSegSn = itSlcPduBuff->first;
			}

		}
		if (isMaxStatusSnLessThanRcvNextSn == FALSE
			&& isExistSlcPduSeg == TRUE)
		{
			//// NACK settings VR(R):rcvNextSn ~ VR(MS):maxStatusSn
			chkNum = SlcFxAmSeqNumDist(amEntity->rcvNextSn,
				lastSlcPduSegSn) + 1;
			for (UInt32 i = 0; i < chkNum; i++)
			{
				UInt32 chkSn = (amEntity->rcvNextSn + i)
					% FX_SLC_AM_SN_UPPER_BOUND;
				FxSlcStatusPduSoPair workSegInfo = { 0,0 };
				itSlcPduBuff = amEntity->rcvPduBuffer.find(chkSn);

				// whole parts is NACKed
				if (itSlcPduBuff == amEntity->rcvPduBuffer.end())
				{
					workSegInfo.soStart
						= FX_SLC_INVALID_STATUS_PDU_FORMAT_NACK_SEGMENT_OFFSET;
					workSegInfo.soEnd
						= FX_SLC_INVALID_STATUS_PDU_FORMAT_NACK_SEGMENT_OFFSET;

					statusPdu.nackSoMap.insert(
						pair < UInt16, FxSlcStatusPduSoPair >
						((UInt16)chkSn, workSegInfo));

				}
				else if (itSlcPduBuff->second.slcPduRcvFinished == FALSE)
				{
					UInt16 minRcvHead = 0;

					std::map < UInt16, FxSlcAmRcvPduSegmentPart > ::iterator
						itSegInfo
						= itSlcPduBuff->second.rcvdPduSegmentInfo.begin();
					while (itSegInfo
						!= itSlcPduBuff->second.rcvdPduSegmentInfo.end())
					{
						if (minRcvHead == 0)
						{
							if (itSegInfo->first != 0)
							{
								workSegInfo.soStart = 0;
								workSegInfo.soEnd
									= itSegInfo->first;

								statusPdu.nackSoMap.insert(
									pair < UInt16, FxSlcStatusPduSoPair >
									((UInt16)itSlcPduBuff->first, workSegInfo));

							}
						}
						else
						{
							workSegInfo.soEnd
								= itSegInfo->first;

							statusPdu.nackSoMap.insert(
								pair < UInt16, FxSlcStatusPduSoPair >
								((UInt16)itSlcPduBuff->first, workSegInfo));

						}
						// update for next processing
						minRcvHead = itSegInfo->first;
						workSegInfo.soStart
							= itSegInfo->second.rcvdTail;
						++itSegInfo;
					}
					// set status to NACKed
					if (workSegInfo.soStart < itSlcPduBuff->second.orgMsgSize)
					{
						workSegInfo.soEnd = itSlcPduBuff->second.orgMsgSize;

						statusPdu.nackSoMap.insert(
							pair < UInt16, FxSlcStatusPduSoPair >
							((UInt16)itSlcPduBuff->first, workSegInfo));

					}
				}
			}
		}

		// ACK settings
		chkNum = SlcFxAmSeqNumDist(amEntity->rcvNextSn, amEntity->rcvWnd);
		UInt32 snWork = amEntity->rcvNextSn;
		for (UInt32 i = 0; i < chkNum; i++)
		{
			itSlcPduBuff = amEntity->rcvPduBuffer.find(snWork);
			if ((itSlcPduBuff != amEntity->rcvPduBuffer.end())
				&& (itSlcPduBuff->second.slcPduRcvFinished == TRUE))
			{
				SlcFxAmIncSN(snWork);
			}
			else
			{
				statusPdu.fixed.ackSn = snWork;
				break;
			}
		}
		amEntity->statusBuffer.push_back(statusPdu);
		amEntity->statusBufSize +=
			SlcFxGetExpectedAmStatusPduSize(statusPdu);


		// StatusProhibit timer re-start.
		// (Clear waitExprioryTStatusProhibitFlg)
		SlcFxAmInitStatusProhibitTimer(node,
			iface,
			amEntity);
	}
}
// /**
// FUNCTION::       SlcFxAmHandleAck
// LAYER::          FX LAYER2 SLC
// PURPOSE::        Handle negative and positive acknowledgement
// PARAMETERS::
//      +node:      The node pointer
//      +iface:     The interface index
//      +amEntity:  The AM entity
//      +rxStatusPDU: The ACK sequence number returned in the ACK SUFI
// RETURN::         NULL
// **/
static
void SlcFxAmHandleAck(Node* node,
	int iface,
	const fxRnti& srcRnti,
	const int bearerId,
	FxSlcAmEntity* amEntity,
	FxSlcStatusPduFormat rxStatusPDU)
{
	std::list < FxSlcAmReTxBuffer > ::iterator itRetxBuff;


	//1.Process NACK_SN
	BOOL nackSnEqPollSn = FALSE;
	std::multimap < UInt16, FxSlcStatusPduSoPair > ::iterator
		itNackList = rxStatusPDU.nackSoMap.begin();
	while (itNackList != rxStatusPDU.nackSoMap.end())
	{
		if ((*itNackList).first == amEntity->pollSn)
		{
			nackSnEqPollSn = TRUE;
		}
		if (SlcFxSeqNumInRange((*itNackList).first,
			amEntity->ackSn,
			amEntity->sndNextSn)
			== FALSE)
		{
			rxStatusPDU.nackSoMap.erase(itNackList++);
			continue;
		}
		++itNackList;
	}

	SetNackedRetxBuffer(node,
		iface,
		*amEntity,
		amEntity->retxBuffer,
		rxStatusPDU.nackSoMap);

	//2.Process ACK_SN
	Int16 ackedSn = FX_SLC_INVALID_SEQ_NUM;
	itRetxBuff = amEntity->retxBuffer.begin();
	while (itRetxBuff != amEntity->retxBuffer.end())
	{
		Int16 chkSn = itRetxBuff->slcHeader.fixed->seqNum;
		if (((itRetxBuff->pduStatus
			== FX_SLC_AM_DATA_PDU_STATUS_WAIT) ||
			(itRetxBuff->pduStatus
				== FX_SLC_AM_DATA_PDU_STATUS_NACK))
			&& (SlcFxSeqNumInRange(chkSn,
				amEntity->ackSn,
				rxStatusPDU.fixed.ackSn)))
		{
			itRetxBuff->pduStatus = FX_SLC_AM_DATA_PDU_STATUS_ACK;
			ackedSn = chkSn;
		}
		++itRetxBuff;
	}

	//3. update transmission Window

	//  find not ACKed PDU from ackSn[VT(A)]<=SN<ACK_SN
	UInt32 chkNum = SlcFxAmSeqNumDist(amEntity->ackSn,
		rxStatusPDU.fixed.ackSn);
	for (UInt32 i = 0; i < chkNum; i++)
	{
		if (findSnRetxBuffer(node,
			iface,
			amEntity->retxBuffer,
			amEntity->ackSn,
			itRetxBuff))
		{
			if (itRetxBuff->pduStatus
				!= FX_SLC_AM_DATA_PDU_STATUS_ACK)
			{
				// stop VT(A) updating
				break;
			}
		}
		// update transmit window VT(A).
		SlcFxAmIncSN(amEntity->ackSn);
	}

	// Remove PDU which status is ACK
	DeleteSnRetxBuffer(node,
		iface,
		amEntity,
		amEntity->retxBuffer);
	/*SlcFxAmNotifySAcOfSacStatusReport(
	node, iface, srcRnti, bearerId,
	amEntity, rxStatusPDU.fixed.ackSn);*/


	// update transmit window VT(MS).
	amEntity->sndWnd = (amEntity->ackSn + FX_SLC_AM_WINDOW_SIZE)
		% FX_SLC_AM_SN_UPPER_BOUND;

	// check t-PollRetransmit
	if ((amEntity->pollRetransmitTimerMsg != NULL)
		&& ((rxStatusPDU.fixed.ackSn == (Int32)amEntity->pollSn)
			|| (nackSnEqPollSn == TRUE)))
	{
		MESSAGE_CancelSelfMsg(node, amEntity->pollRetransmitTimerMsg);
		amEntity->pollRetransmitTimerMsg = NULL;
	}

}

// /**
// FUNCTION::           SlcFxAmPduSegmentReassemble
// LAYER::              FX LAYER2 SLC
// PURPOSE::            Insert the PDU segments extracted
//                      from the newly received PDU segment
//                      into the PDU list (receiving buffer).
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         The interface index
//      +amEntity:      The AM entity
//      +rcvdSeqNum:    The seqence number of the PDU, from which
//                      the sdu segments are extracted
//      +pduMsg:        The received PDU
// RETURN::             BOOL
// **/
static
BOOL SlcFxAmPduSegmentReassemble(
	Node* node,
	int iface,
	FxSlcAmEntity* amEntity,
	unsigned int rcvdSeqNum,
	Message* pduMsg)
{
	BOOL doReassemble = TRUE;

	// Get current SLC-PDUsegment data info.
	FxSlcAmRcvPduData* currentPduData
		= &(amEntity->rcvPduBuffer[rcvdSeqNum]);

	int pduSegDataSize = MESSAGE_ReturnPacketSize(pduMsg)
		- SlcFxAmGetHeaderSize(pduMsg);

	FxSlcAmRcvPduSegmentPart currentSoInfo;
	currentSoInfo.rcvdHead = SlcFxAmGetSegOffset(pduMsg);
	currentSoInfo.rcvdTail = currentSoInfo.rcvdHead + (UInt16)pduSegDataSize;

	// check the overlap SLC-PDUsegments.
	// CH:currentSoInfo.rcvdHead, CT:currentSoInfo.rcvdTail
	// IH:it->second.rcvdHead, IT:it->second.rcvdTail
	std::map < UInt16, FxSlcAmRcvPduSegmentPart > ::iterator it
		= currentPduData->rcvdPduSegmentInfo.begin();
	while (it != currentPduData->rcvdPduSegmentInfo.end())
	{
		if (currentSoInfo.rcvdHead < it->second.rcvdHead)
		{
			if (currentSoInfo.rcvdTail >= it->second.rcvdHead)
			{
				if (currentSoInfo.rcvdTail <= it->second.rcvdTail)
				{
					// CurHead < ItHead < CurTail < ItTail
					// -> CurHead ~ ItTail
					currentSoInfo.rcvdTail = it->second.rcvdTail;
				}

				currentPduData->rcvdPduSegmentInfo.erase(it++);
				continue;
			}
		}
		else // if (currentSoInfo.rcvdHead >= it->second.rcvdHead)
		{
			if (currentSoInfo.rcvdHead <= it->second.rcvdTail)
			{
				if (currentSoInfo.rcvdTail >= it->second.rcvdTail)
				{
					// ItHead < CurHead < ItTail < CurTail
					// -> ItHead ~ CurTail
					currentSoInfo.rcvdHead = it->second.rcvdHead;
					currentPduData->rcvdPduSegmentInfo.erase(it++);
					continue;
				}
				else
				{
					// ItHead < CurHead < CurTail < ItTail
					// -> already received
					doReassemble = FALSE;
				}
			}
		}
		++it;
	}


	// Update end position of SLC-PDU.
	if (currentPduData->endPduPos < currentSoInfo.rcvdTail)
	{
		currentPduData->endPduPos = currentSoInfo.rcvdTail;
	}

	// Update received SLC-PDUsegments information.
	if (doReassemble)
	{
		currentPduData->rcvdPduSegmentInfo[currentSoInfo.rcvdHead]
			= currentSoInfo;
	}

	// received LastSegmentFlag -> Later, check reassembling completion.
	if (SlcFxAmGetLastSegflag(pduMsg))
	{
		currentPduData->gotLsf = TRUE;
	}

	if ((currentPduData->gotLsf == TRUE)
		&& (currentPduData->rcvdPduSegmentInfo.size() == 1)
		&& (currentPduData->rcvdPduSegmentInfo.find(0)
			!= currentPduData->rcvdPduSegmentInfo.end())
		&& doReassemble == TRUE)
	{
		ERROR_Assert(currentPduData->message == NULL,
			"SlcFxAmPduSegmentReassemble: current message is not NULL!");
		currentPduData->slcPduRcvFinished = TRUE;
		currentPduData->message = pduMsg;
	}

	return doReassemble;
}

// /**
// FUNCTION::           SlcFxAmSDUsegmentReassemble
// LAYER::              FX LAYER2 SLC
// PURPOSE::            Insert the SDU segments extracted from
//                      the newly received PDU
//                      into the SDU segment list (receiving buffer).
//                      After this,
//                      re-assembly shall take place if possible.
// PARAMETERS::
//      +node:           Node pointer
//      +iface:          The interface index
//      +FxSlcAmEntity: The AM receiving entity
// RETURN::             NULL
// **/
static
void SlcFxAmSDUsegmentReassemble(
	Node* node,
	int iface,
	FxSlcAmEntity* amEntity)
{
	// get SN which is less than rcvNext
	// -> reasemble rcvSlcPdu
	std::map < unsigned int, FxSlcAmRcvPduData > ::iterator
		itPduBuff = amEntity->rcvPduBuffer.begin();
	while (itPduBuff != amEntity->rcvPduBuffer.end())
	{
		if ((SlcFxAmSeqNumInWnd(itPduBuff->first,
			amEntity->rcvNextSn))
			|| (itPduBuff->second.slcPduRcvFinished == FALSE))
		{
			++itPduBuff;
			continue;
		}

		// Reassemble SDU/SDUsegment from received PDU
		std::list < Message* > rcvMsgList;
		SlcFxPduToSduList(node,
			itPduBuff->second.message,
			TRUE,
			rcvMsgList);

		// Number of Li is zero -> SDU.
		if (itPduBuff->second.lengthIndicator.empty())
		{
			SlcFxAmAddSduBuffer(node,
				iface,
				amEntity,
				itPduBuff->first,
				rcvMsgList);
		}
		// Number of Li is one.
		else if (itPduBuff->second.lengthIndicator.size() == 1)
		{
			ERROR_Assert(rcvMsgList.size() > 0, "rcvMsgList is empty!");
			FxSlcSduSegment workSdu;
			workSdu.seqNum = (Int16)itPduBuff->first;
			workSdu.FramingInfo = itPduBuff->second.FramingInfo;
			workSdu.newMsg = rcvMsgList.front();
			// FramingInfo is "IN_FIRST_BYTE"
			// -> SDUsegment(Head).
			if (itPduBuff->second.FramingInfo
				== FX_SLC_AM_FRAMING_INFO_IN_FIRST_BYTE)
			{
				amEntity->rcvSlcSduSegList.push_back(workSdu);
			}
			// FramingInfo is "IN_END_BYTE" or "FRAGMENT"
			// -> SDUsegment(tail), so puts on the temporary buffer.
			else
			{
				amEntity->rcvSlcSduTempList.push_back(workSdu);
			}
		}
		// Number of Li is two or more.
		else
		{
			FxSlcSduSegment workSdu;
			workSdu.seqNum = (Int16)itPduBuff->first;
			workSdu.FramingInfo = itPduBuff->second.FramingInfo;
			// FramingInfo is "IN_FIRST_BYTE" or "FRAGMENT",
			// and last element.
			// -> SDUsegment(Head).
			if ((itPduBuff->second.FramingInfo
				== FX_SLC_AM_FRAMING_INFO_IN_FIRST_BYTE)
				|| (itPduBuff->second.FramingInfo
					== FX_SLC_AM_FRAMING_INFO_FRAGMENT))
			{
				ERROR_Assert(rcvMsgList.size() > 0, "rcvMsgList is empty!");
				workSdu.newMsg = rcvMsgList.back();
				workSdu.FramingInfo = FX_SLC_AM_FRAMING_INFO_IN_FIRST_BYTE;
				amEntity->rcvSlcSduSegList.push_back(workSdu);
				rcvMsgList.pop_back();
			}
			// FramingInfo is "IN_END_BYTE" or "FRAGMENT",
			// and first element.
			// -> SDUsegment(tail), so puts on the temporary buffer.
			if ((itPduBuff->second.FramingInfo
				== FX_SLC_AM_FRAMING_INFO_IN_END_BYTE)
				|| (itPduBuff->second.FramingInfo
					== FX_SLC_AM_FRAMING_INFO_FRAGMENT))
			{
				ERROR_Assert(rcvMsgList.size() > 0, "rcvMsgList is empty!");
				workSdu.newMsg = rcvMsgList.front();
				workSdu.FramingInfo = FX_SLC_AM_FRAMING_INFO_IN_END_BYTE;
				amEntity->rcvSlcSduTempList.push_back(workSdu);
				rcvMsgList.pop_front();
			}

			// The remaining element is SDU.
			if (!rcvMsgList.empty())
			{
				SlcFxAmAddSduBuffer(node,
					iface,
					amEntity,
					itPduBuff->first,
					rcvMsgList);
			}
		}
		amEntity->rcvPduBuffer.erase(itPduBuff++);
	}
	// Reassemble SDUsegment
	std::list < FxSlcSduSegment > ::iterator itSduSeg;
	std::list < FxSlcSduSegment > ::iterator itSduTemp;

	for (itSduSeg = amEntity->rcvSlcSduSegList.begin();
		itSduSeg != amEntity->rcvSlcSduSegList.end();)
	{
		BOOL eraseItSduSeg = FALSE;
		for (itSduTemp = amEntity->rcvSlcSduTempList.begin();
			itSduTemp != amEntity->rcvSlcSduTempList.end();)
		{
			BOOL eraseItSduTemp = FALSE;
			UInt32 reAssySn = itSduSeg->seqNum;
			SlcFxAmIncSN(reAssySn);
			if (reAssySn == (UInt32)itSduTemp->seqNum)
			{
				Message* flagList[2]
					= { itSduSeg->newMsg, itSduTemp->newMsg };
				Message* reAssembleMsg
					= MESSAGE_ReassemblePacket(node,
						flagList,
						2,
						TRACE_SLC_FX);
				itSduSeg->newMsg = reAssembleMsg;
				if (itSduTemp->FramingInfo
					== FX_SLC_AM_FRAMING_INFO_FRAGMENT)
				{
					itSduSeg->seqNum = itSduTemp->seqNum;
				}
				else
				{
					std::pair < unsigned int, Message* >
						sdu(itSduSeg->seqNum, reAssembleMsg);
					amEntity->rcvSlcSduList.insert(sdu);

					eraseItSduSeg = TRUE;
				}
				eraseItSduTemp = TRUE;
			}

			if (eraseItSduTemp)
			{
				itSduTemp = amEntity->rcvSlcSduTempList.erase(itSduTemp);
			}
			else
			{
				++itSduTemp;
			}
		}

		if (eraseItSduSeg)
		{
			itSduSeg = amEntity->rcvSlcSduSegList.erase(itSduSeg);
		}
		else
		{
			++itSduSeg;
		}
	}
	// clean-up SDU list
	for (itSduSeg = amEntity->rcvSlcSduSegList.begin();
		itSduSeg != amEntity->rcvSlcSduSegList.end();)
	{
		UInt32 keepSn = amEntity->rcvNextSn;
		SlcFxAmDecSN(keepSn);
		if ((SlcFxAmSeqNumInWnd(itSduSeg->seqNum,
			amEntity->rcvNextSn) == FALSE)
			&& (itSduSeg->seqNum != (Int16)keepSn))
		{
			itSduSeg = amEntity->rcvSlcSduSegList.erase(itSduSeg);
		}
		else
		{
			++itSduSeg;
		}
	}
	for (itSduTemp = amEntity->rcvSlcSduTempList.begin();
		itSduTemp != amEntity->rcvSlcSduTempList.end();)
	{
		if (SlcFxAmSeqNumInWnd(itSduTemp->seqNum,
			amEntity->rcvNextSn) == FALSE)
		{
			itSduTemp = amEntity->rcvSlcSduTempList.erase(itSduTemp);
		}
		else
		{
			++itSduTemp;
		}
	}
}


// /**
// FUNCTION   :: SlcFxPduToSduList
// LAYER      :: FX Layer2 SLC
// PURPOSE    :: Unpack a message to the original list of messages
// PARAMETERS ::
//  + node      : Node*    : Pointer to node.
//  + msg      : Message* : Pointer to the supper msg containing list of msgs
//  + freeOld   : BOOL     : Whether the original message should be freed
//  + msgList   : list<Message*> : an empty stl list to be used to
//                contain unpacked messages
// RETURN     :: NULL
// **/
void SlcFxPduToSduList(Node* node,
	Message* msg,
	BOOL freeOld,
	std::list < Message* > &msgList)
{
	msgList.clear();

	Message* newMsg;

	char* info;
	int infoSize;
	int msgSize;

	info = MESSAGE_ReturnInfo(
		msg, (unsigned short)INFO_TYPE_FxSlcAmSduToPdu);
	infoSize = MESSAGE_ReturnInfoSize(
		msg, (unsigned short)INFO_TYPE_FxSlcAmSduToPdu);
	msgSize = 0;
	ERROR_Assert(info != NULL, "INFO_TYPE_FxSlcAmSduToPdu is not found.");
	while (msgSize < infoSize)
	{
		newMsg = SlcFxUnSerializeMessage(node,
			info,
			msgSize);

		// add new msg to msgList
		msgList.push_back(newMsg);
	}

	// free original message
	if (freeOld)
	{
		MESSAGE_Free(node, msg);
	}
}

// /**
// FUNCTION::           SetNackedRetxBuffer
// LAYER::              FX LAYER2 SLC
// PURPOSE::            Set PDU status=NACKed in the retransmission buffer
// PARAMETERS::
//  + node       : Node*         : Node pointer
//  + iface      : int           : Interface index
//  + slcEntity  : FxSlcEntity* :   The SLC entity
//  + retxBuffer : std::list<FxSlcAmReTxBuffer>& : retransmission buffer
//  + nackSoMap  : std::multimap< UInt16, FxSlcStatusPduSoPair >&
//                               : Nacked info - SN & SO(SOstart, SOend)
// RETURN::    void:         NULL
// **/
void SetNackedRetxBuffer(
	Node* node,
	int iface,
	FxSlcAmEntity& amEntity,
	std::list < FxSlcAmReTxBuffer > &retxBuffer,
	std::multimap < UInt16, FxSlcStatusPduSoPair > &nackSoMap)
{
	std::multimap < UInt16, FxSlcStatusPduSoPair > ::iterator it;

	for (it = nackSoMap.begin(); it != nackSoMap.end(); it++)
	{
		SetNackedRetxBuffer(node,
			iface,
			amEntity,
			retxBuffer,
			it->first,
			it->second);


	}
}
// /**
// FUNCTION::           SetNackedRetxBuffer
// LAYER::              FX LAYER2 SLC
// PURPOSE::            Set PDU status=NACKed in the retransmission buffer
// PARAMETERS::
//  + node       : Node*          : Node pointer
//  + iface      : int            : Interface index
//  + amEntity   : FxSlcAmEntity : The SLC AM entity
//  + retxBuffer : std::list<FxSlcAmReTxBuffer>& : retransmission buffer
//  + sn         : int            : Sequens number
//  + nacked     : FxSlcStatusPduSoPair : Nacked info (SOstart, SOend)
// RETURN::    void:           NULL
// **/
void SetNackedRetxBuffer(Node* node,
	int iface,
	FxSlcAmEntity& amEntity,
	std::list < FxSlcAmReTxBuffer > &retxBuffer,
	int sn,
	FxSlcStatusPduSoPair nacked)
{
	BOOL addFlag = TRUE;
	std::list < FxSlcAmReTxBuffer > ::iterator it;
	if (findSnRetxBuffer(node,
		iface,
		retxBuffer,
		sn,
		it))
	{
		// Whole parts is NACK.
		if (nacked.soStart
			== FX_SLC_INVALID_STATUS_PDU_FORMAT_NACK_SEGMENT_OFFSET)
		{
			nacked.soStart = 0;
			nacked.soEnd = MESSAGE_ReturnPacketSize((*it).message);
		}

		amEntity.retxBufNackSize -= it->getNackedSize(node, iface);
		std::map < int, FxSlcStatusPduSoPair > ::iterator nackIt;
		nackIt = it->nackField.lower_bound(nacked.soStart);
		BOOL first = TRUE;
		if (it->nackField.begin() != nackIt)
		{
			nackIt--;
			// check front
			while (TRUE)
			{
				if (nackIt->second.soEnd >= nacked.soStart)
				{
					if (nackIt->second.soEnd <= nacked.soEnd)
					{
						nackIt->second.soEnd = nacked.soEnd;
						addFlag = FALSE;

						if (first == TRUE)
						{
							first = FALSE;
						}
						else
						{
							nackIt++;
							it->nackField.erase(nackIt++);
						}
					}
					else
					{
						return;
					}

				}

				if (it->nackField.begin() == nackIt)
				{
					break;
				}

				if (it->nackField.begin() != nackIt)
				{
					nackIt--;
				}
			}
		}

		nackIt = it->nackField.lower_bound(nacked.soStart);
		if (first == FALSE)
		{
			if (it->nackField.begin() != nackIt) {
				nackIt--;
			}
			nacked.soStart = nackIt->second.soStart;
			nacked.soEnd = nackIt->second.soEnd;
			if (it->nackField.begin() == nackIt)
			{
				nackIt++;
			}

		}
		// check back
		while (it->nackField.end() != nackIt)
		{
			if (nackIt->second.soStart <= nacked.soEnd)
			{
				addFlag = FALSE;
				nackIt->second.soStart = nacked.soStart;

				if (nackIt->second.soEnd >= nacked.soEnd)
				{
					nacked.soEnd = nackIt->second.soEnd;
				}
				else
				{
					nackIt->second.soEnd = nacked.soEnd;
				}

				if (first == TRUE)
				{
					first = FALSE;
					it->nackField.erase(nackIt++);
					if (it->nackField.begin() != nackIt) {
						nackIt--;
						nackIt = it->nackField.insert(nackIt,
							std::pair < int, FxSlcStatusPduSoPair >
							(nacked.soStart, nacked));
						nackIt++;
					}
					else
					{
						nackIt = it->nackField.insert(nackIt,
							std::pair < int, FxSlcStatusPduSoPair >
							(nacked.soStart, nacked));
					}
				}
				else
				{
					nacked.soEnd = nackIt->second.soEnd;
					if (it->nackField.begin() != nackIt) {
						nackIt--;
						nackIt->second.soEnd = nacked.soEnd;
						nackIt++;
						it->nackField.erase(nackIt++);
					}
					else
					{
						nackIt->second.soEnd = nacked.soEnd;
						nackIt++;
					}
				}
			}
			else
			{
				nackIt++;
			}
		}

		if (addFlag == TRUE)
		{
			it->nackField.insert(std::pair < int, FxSlcStatusPduSoPair >
				(nacked.soStart, nacked));
		}

		amEntity.retxBufNackSize
			+= it->getNackedSize(node, iface);
		it->pduStatus = FX_SLC_AM_DATA_PDU_STATUS_NACK;
	}
}
//yt
UInt32 SlcFxSendableByteInTxQueue(
	Node* node,
	int iface,
	const fxRnti& dstRnti,
	const int bearerId)
{
	UInt32 retByte = 0;

	// Transmission Buffer size
	// (without Retransmission/Status PDU Buffer size)
	FxSlcEntity* entity =
		SlcFxGetSlcEntity(node, iface, dstRnti, bearerId);
	if (entity == NULL)
	{
		retByte = 0;
	}
	else
	{
		switch (entity->entityType)
		{
		case FX_SLC_ENTITY_AM:
		{
			FxSlcAmEntity* amEntity =
				(FxSlcAmEntity*)entity->entityData;
			retByte += (UInt32)amEntity->txBufSize;
			break;
		}
		default:
		{
			retByte = 0;
		}
		}
	}

	return retByte;
}
/*static
void SlcFxAmNotifySAcOfSacStatusReport(
Node* node,
const int interfaceIndex,
const fxRnti& rnti,
const int bearerId,
FxSlcAmEntity* slcAmEntity,
const Int32 receivedSlcAckSn)
{
ERROR_Assert(slcAmEntity, "slcAmEntity is NULL");
list<pair<UInt16, UInt16> >& unreceivedSlcAckList
= slcAmEntity->unreceivedSlcAckList;

// clear received SLC ACK SN of that ACK received prior
// from unreceivedSlcAckList, and managed that list(unRxSacAckSet).
set<UInt16> unRxSacAckSet;
UInt16 frontSn = FX_SLC_INVALID_SLC_SN; // front SN of receivedSlcAckSn
while (!unreceivedSlcAckList.empty()) {
frontSn = unreceivedSlcAckList.front().first;
if (((frontSn + FX_SLC_AM_WINDOW_SIZE < FX_SLC_AM_SN_UPPER_BOUND)
&& (frontSn < receivedSlcAckSn)
&& (receivedSlcAckSn
<= frontSn + FX_SLC_AM_WINDOW_SIZE))
|| ((frontSn + FX_SLC_AM_WINDOW_SIZE
>= FX_SLC_AM_SN_UPPER_BOUND)
&& ((frontSn < receivedSlcAckSn)
|| (receivedSlcAckSn + FX_SLC_AM_SN_UPPER_BOUND
<= frontSn + FX_SLC_AM_WINDOW_SIZE)))) {
unRxSacAckSet.insert(unreceivedSlcAckList.front().second);
unreceivedSlcAckList.pop_front();
} else {
break;
}
}
// if all SLC ACK for SAC PDU is received,
// notify SAC of SAC status report
UInt16 checkSacSn = FX_SLC_INVALID_SLC_SN;
list<pair<UInt16, UInt16> >::iterator itr
= unreceivedSlcAckList.begin();
list<pair<UInt16, UInt16> >::iterator itrEnd
= unreceivedSlcAckList.end();
while (!unRxSacAckSet.empty()) {
checkSacSn = *(unRxSacAckSet.begin());
unRxSacAckSet.erase(checkSacSn);
for (itr = unreceivedSlcAckList.begin();
itr != itrEnd  && itr->second != checkSacSn; ++itr) {
;
}
if (itr == itrEnd) {
SacFxReceiveSacStatusReportFromSlc(
node, interfaceIndex, rnti, bearerId, checkSacSn);
} else {
// do nothing
}
}
}*/




