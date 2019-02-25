#ifndef LAYER2_FX_SAC_H
#define LAYER2_FX_SAC_H

#include <node.h>
#include "fx_common.h"

//--------------------------------------------------------------------------
// Enumulation
//--------------------------------------------------------------------------
typedef enum {
	SAC_FX_POWER_OFF,
	SAC_FX_POWER_ON,
	SAC_FX_STATUS_NUM
} SacFxState;

void FxFreeMsg(
	Node* node,
	Message** msg);

enum messageInfoType {
	INFO_TYPE_FxSacDiscardTimerInfo
};

// /**
// ENUM         :: SacFxHoStatus
// DESCRIPTION  :: ho status
// **/
typedef enum {
	SAC_FX_SOURCE_E_NB_IDLE,
	SAC_FX_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ,
	SAC_FX_SOURCE_E_NB_BUFFERING,
	SAC_FX_SOURCE_E_NB_FORWARDING,
	SAC_FX_SOURCE_E_NB_FORWARDING_END,
	SAC_FX_TARGET_E_NB_IDLE,
	SAC_FX_TARGET_E_NB_FORWARDING,
	SAC_FX_TARGET_E_NB_CONNECTED,
	SAC_FX_TARGET_E_NB_WAITING_END_MARKER,
	SAC_FX_UE_IDLE,
	SAC_FX_UE_BUFFERING
} SacFxHoStatus;



// /**
// ENUM         :: SacFxHoBufferType
// DESCRIPTION  :: buffer type for ho
// **/
typedef enum {
	SAC_FX_REORDERING,
	SAC_FX_RETRANSMISSION,
	SAC_FX_TRANSMISSION,
} SacFxHoBufferType;

//--------------------------------------------------------------------------
// Constant
//--------------------------------------------------------------------------
// /**
// CONSTANT    ::SAC_FX_SEQUENCE_LIMIT : 0xFFF(4095)
// DESCRIPTION :: sac sequence number limit
// **/

#define SAC_FX_SEQUENCE_LIMIT (0xFFF)
#define SAC_FX_REORDERING_WINDOW (2048) // ((0xFFF) / 2)
#define SAC_FX_INVALID_SAC_SN (SAC_FX_SEQUENCE_LIMIT + 1)

//  According to the specifications,
//  select from ms50, ms100, ms150, ms300, ms500, ms750, ms1500
#define SAC_FX_DEFAULT_DISCARD_TIMER_DELAY (500 * MILLI_SECOND)
#define SAC_FX_DEFAULT_BUFFER_BYTE_SIZE (50000) // 50KB

// parameter strings
#define FX_SAC_STRING_DISCARD_TIMER_DELAY "SAC-FX-DISCARD-TIMER-DELAY"
#define FX_SAC_STRING_BUFFER_BYTE_SIZE "SAC-FX-BUFFER-BYTE-SIZE"

// parameter LIMIT
#define FX_SAC_MIN_DISCARD_TIMER_DELAY (1*MILLI_SECOND)
#define FX_SAC_MAX_DISCARD_TIMER_DELAY (CLOCKTYPE_MAX)
#define FX_SAC_MIN_BUFFER_BYTE_SIZE (0)
#define FX_SAC_MAX_BUFFER_BYTE_SIZE (INT_MAX)

//--------------------------------------------------------------------------
// Struct
//--------------------------------------------------------------------------
// /**
// STRUCT:: FxSacHeader
// DESCRIPTION::Sac Header
// **/
typedef struct
{
	//unsigned char sacSN;         // uchar=8bit=XRRRSSSS
	// X=DetaOrCtrlFlag
	// S=PDCPSN R=RESERVED=0
	//unsigned char sacSNCont;
	UInt8 octet[2];
} FxSacHeader;



// /**
// STRUCT       ::SacFxReceiveStatus
// DESCRIPTION  :: for SN status transfer request
// **/
typedef struct SacFxReceiveStatus {
	// managed only the last submitted PDCP received SN
	// in place of the bitmap
	UInt16 lastSubmittedSacRxSn;

	SacFxReceiveStatus(
		UInt16 argLastSubmittedSacRxSn)
		: lastSubmittedSacRxSn(argLastSubmittedSacRxSn)
	{
	}

	// get last submitted PDCP rx SN
	UInt16 GetLastSubmittedSacRxSn(void) { return lastSubmittedSacRxSn; }
} SacFxReceiveStatus;



// /**
// STRUCT       :: SacFxSnStatusTransferItem
// DESCRIPTION  :: for sn status transfer request
// **/
typedef struct SacFxSnStatusTransferItem {
	SacFxReceiveStatus receiveStatus;
	const UInt16 nextSacRxSn;
	const UInt16 nextSacTxSn;

	SacFxSnStatusTransferItem(
		SacFxReceiveStatus argReceiveStatus,
		const UInt16 argNextSacRxSn,
		const UInt16 argNextSacTxSn)
		: receiveStatus(argReceiveStatus),
		nextSacRxSn(argNextSacRxSn),
		nextSacTxSn(argNextSacTxSn)
	{
	}

	SacFxSnStatusTransferItem()
		: receiveStatus(0),
		nextSacRxSn(0),
		nextSacTxSn(0)
	{
	}

	struct SacFxSnStatusTransferItem& operator=(
		const struct SacFxSnStatusTransferItem& another)
	{
		// For supressing warning C4512
		ERROR_Assert(FALSE, "SacFxSnStatusTransferItem::operator= cannot be called");
		return *this;
	}
} SacFxSnStatusTransferItem;



// /**
// STRUCT       :: SacFxDiscardTimerInfo
// DESCRIPTION  :: infomation for discard timer msg
// **/
typedef struct SacFxDiscardTimerInfo {
	const fxRnti dstRnti;
	const int bearerId;
	const UInt16 sn;

	SacFxDiscardTimerInfo(
		const fxRnti argDstRnti,
		const int argBearerId,
		const UInt16 argSn)
		: dstRnti(argDstRnti), bearerId(argBearerId), sn(argSn)
	{
	}

	struct SacFxDiscardTimerInfo& operator=(
		const struct SacFxDiscardTimerInfo& another)
	{
		// For supressing warning C4512
		ERROR_Assert(FALSE, "SacFxDiscardTimerInfo::operator= cannot be called");
		return *this;
	}
} SacFxDiscardTimerInfo;



// /**
// STRUCT       :: SacFxHoManager
// DESCRIPTION  :: sac lte ho manager
// **/
typedef struct SacFxHoManager
{
	SacFxHoStatus hoStatus;

	UInt16 lastSubmittedSacRxSn;
	UInt16 nextSacRxSn;

	// managed buffer
	list<Message*> reorderingBuffer;
	list<Message*> retransmissionBuffer;
	list<Message*> transmissionBuffer;
	list<Message*> holdingBuffer;

	map<UInt16, Message*> discardTimer;

	// statistics
	UInt16 numEnqueuedRetransmissionMsg;
	UInt32 byteEnqueuedRetransmissionMsg;

	SacFxHoManager()
		: // hoStatus(),
		lastSubmittedSacRxSn(SAC_FX_SEQUENCE_LIMIT),
		nextSacRxSn(0),
		numEnqueuedRetransmissionMsg(0),
		byteEnqueuedRetransmissionMsg(0)
	{
	}
} SacFxHoManager;



// /**
// STRUCT:: FxSacEntity
// DESCRIPTION:: PDCP Entity
// **/
typedef struct
{
	UInt16 sacSN; // Sequence Number

				  // For processing delayed PDCP PDU sent to upper layer
	clocktype lastSacSduSentToNetworkLayerTime;

	SacFxHoManager hoManager;

	// incremented next PDCP tx SN
	void SetNextSacTxSn(UInt16 nextSacTxSn) {
		ERROR_Assert(nextSacTxSn <= SAC_FX_SEQUENCE_LIMIT, "failed to SetNextSacTxSn()");
		sacSN = nextSacTxSn;
	}
	// get "next PDCP SN" and set to next value
	const UInt16 GetAndSetNextSacTxSn(void);
	// get next PDCP tx SN
	const UInt16 GetNextSacTxSn(void) { return sacSN; }
} FxSacEntity;

typedef struct
{
	clocktype txTime;
} FxSacTxInfo;



// /**
// STRUCT:: FxSacStats
// DESCRIPTION:: PDCP statistics
// **/
typedef struct
{
	UInt32 numPktsFromUpperLayer;// Number of data packets from Upper Layer
	UInt32 numPktsFromUpperLayerButDiscard; // Number of data packets from
											// Upper Layer
	UInt32 numPktsToLowerLayer; // Number of data packets to Lower Layer
	UInt32 numPktsFromLowerLayer; // Number of data packets from Lower Layer
	UInt32 numPktsToUpperLayer; // Number of data packets to Upper Layer
								// Number of data packets enqueued in retransmission buffer
	UInt32 numPktsEnqueueRetranamissionBuffer;
	// Number of data packets discarded
	// due to retransmission buffer overflow
	UInt32 numPktsDiscardDueToRetransmissionBufferOverflow;
	// Number of data packets discarded
	// due to RLC's tx buffer overflow
	UInt32 numPktsDiscardDueToRlcTxBufferOverflow;
	// Number of data packets discarded from retransmission buffer
	// due to discard timer expired
	UInt32 numPktsDiscardDueToDiscardTimerExpired;
	// Number of data packets dequeued from retransmission buffer
	UInt32 numPktsDequeueRetransmissionBuffer;
	// Number of data packets discarded due to ack received
	UInt32 numPktsDiscardDueToAckReceived;
	// Number of data packets enqueued in reordering buffer
	UInt32 numPktsEnqueueReorderingBuffer;
	// Number of data packets discarded due to already received
	UInt32 numPktsDiscardDueToAlreadyReceived;
	// Number of data packets dequeued from reordering buffer
	UInt32 numPktsDequeueReorderingBuffer;
	// Number of data packets discarded from reordering buffer
	// due to invalid PDCP SN received
	UInt32 numPktsDiscardDueToInvalidSacSnReceived;
} FxSacStats;

// /**
// STRUCT:: FxSacData
// DESCRIPTION:: PDCP sublayer structure
// **/
typedef struct struct_SAC_FX_data
{
	FxSacStats stats;

	SacFxState SacFxState;

	clocktype discardTimerDelay;
	UInt32 bufferByteSize;
} FxSacData;

//--------------------------------------------------------------------------
// Function
//--------------------------------------------------------------------------
// /**
// FUNCTION::       SacFxInit
// LAYER::          MAC LTE PDCP
// PURPOSE::        Sac Initalization
//
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
//    + nodeInput:  Input from configuration file
// RETURN::         NULL
// **/
void SacFxInit(Node* node,
	unsigned int interfaceIndex,
	const NodeInput* nodeInput);

// /**
// FUNCTION::       SacFxInitConfigurableParameters
// LAYER::          MAC LTE PDCP
// PURPOSE::        Initialize configurable parameters of LTE PDCP Layer.
//
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
//    + nodeInput:  Input from configuration file
// RETURN::         NULL
// **/
void SacFxInitConfigurableParameters(Node* node,
	unsigned int interfaceIndex,
	const NodeInput* nodeInput);

// /**
// FUNCTION::       SacFxFinalize
// LAYER::          MAC LTE PDCP
// PURPOSE::        Sac finalization function
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
// RETURN::         NULL
// **/
void SacFxFinalize(Node* node, unsigned int interfaceIndex);

// /**
// FUNCTION::       SacFxProcessEvent
// LAYER::          MAC LTE PDCP
// PURPOSE::        Sac event handling function
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
//    + message     Message to be handled
// RETURN::         NULL
// **/
void SacFxProcessEvent(Node* node,
	unsigned int interfaceIndex,
	Message* msg);
//yt
void SacFxSetDiscardTimer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId,
	map<UInt16, Message*>& discardTimer,
	const UInt16 sn);

// /**
// FUNCTION::       SacFxUpperLayerHasPacketToSend
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        Notify Upper layer has packet to send
// PARAMETERS::
//    + node           : pointer to the network node
//    + interfaceIndex : index of interface
//    + sacData       : pointer of PDCP Protpcol Data
// RETURN::         NULL
// **/
void SacFxUpperLayerHasPacketToSend(
	Node* node,
	int interfaceIndex,
	FxSacData* sacData);



// /**
// FUNCTION::       SacFxReceivePduFromRlc
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        Notify Upper layer has packet to send
// PARAMETERS::
//    + node           : pointer to the network node
//    + interfaceIndex : index of interface
//    + srcRnti        : Source RNTI
//    + bearerId       : Radio Bearer ID
//    + sacPdu        : PDCP PDU
// RETURN::         NULL
// **/
void SacFxReceivePduFromSlc(
	Node* node,
	int interfaceIndex,
	const fxRnti& srcRnti,
	const int bearerId,
	Message* sacPdu,
	BOOL isReEstablishment);



// /**
// FUNCTION   :: SacFxNotifyPowerOn
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: Power ON
// PARAMETERS ::
// + node               : Node*        : Pointer to node.
// + interfaceIndex     : int          : Interface index
// RETURN     :: void : NULL
// **/
void SacFxNotifyPowerOn(Node* node, int interfaceIndex);

// /**
// FUNCTION   :: SacFxNotifyPowerOn
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: Power OFF
// PARAMETERS ::
// + node               : Node*        : Pointer to node.
// + interfaceIndex     : int          : Interface index
// RETURN     :: void : NULL
// **/
void SacFxNotifyPowerOff(Node* node, int interfaceIndex);

// /**
// FUNCTION   :: SacFxSetState
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: Set state
// PARAMETERS ::
// + node               : Node*        : Pointer to node.
// + interfaceIndex     : int          : Interface index
// + state              : SacFxState : State
// RETURN     :: void : NULL
// **/
void SacFxSetState(Node* node, int interfaceIndex, SacFxState state);

// /**
// FUNCTION   :: SacFxInitSacEntity
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: Init PDCP Entity
// PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : int            : Interface index
// + rnti               : const fxRnti& : RNTI
// + sacEntiry         : FxSacEntity* : PDCP Entity
// + isTargetEnb        : BOOL           : whether target eNB or not
// RETURN     :: void : NULL
// **/
void SacFxInitSacEntity(
	Node* node, int interfaceIndex,
	const fxRnti& rnti, FxSacEntity* sacEntity, BOOL isTargetEnb);



// /**
// FUNCTION   :: SacFxFinalizeSacEntity
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: Finalize PDCP Entity
// PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : int            : Interface index
// + rnti               : const fxRnti& : RNTI
// + sacEntiry         : FxSacEntity* : PDCP Entity
// RETURN     :: void : NULL
// **/
void SacFxFinalizeSacEntity(
	Node* node, int interfaceIndex,
	const fxRnti& rnti, FxSacEntity* sacEntiry);



// /**
// FUNCTION   :: SacFxReceiveSacStatusReportFromRlc
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event3
//               cancel such discard timer
//               and dequeue retransmission buffer head PDCP PDU
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + srcRnti            : const fxRnti& : source RNTI
// + bearerId           : const int      : Radio Bearer ID
// + rxAckSn            : const UInt16   : SN of received msg
// RETURN     :: void : NULL
// **/
void SacFxReceiveSacStatusReportFromRlc(
	Node* node,
	const int interfaceIndex,
	const fxRnti& srcRnti,
	const int bearerId,
	const UInt16 rxAckSn);



// /**
// FUNCTION   :: SacFxGetSnStatusTransferItem
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event4
//               return sn status transfer item
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const fxRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// RETURN     :: SacFxSnStatusTransferItem : sn status transfer item
// **/
SacFxSnStatusTransferItem SacFxGetSnStatusTransferItem(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId);



// /**
// FUNCTION   :: SacFxSetSnStatusTransferItem
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event5
//               set arg to own sn status transfer item
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const fxRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// + item : SacFxSnStatusTransferItem& : set sn status transfer item
// RETURN     :: void : NULL
// **/
void SacFxSetSnStatusTransferItem(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId,
	SacFxSnStatusTransferItem& item);



// /**
// FUNCTION   :: SacFxForwardBuffer
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event6
//               make list of msgs in Buffer
//               and call function for forwarding
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const fxRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// RETURN     :: void : NULL
// **/
void SacFxForwardBuffer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId);



// /**
// FUNCTION   :: SacFxReceiveBuffer
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event7
//               enqueue forwarded msg in such buffer
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const fxRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// + forwardedMsg       : Message*       : forwarded msg from source eNB
// RETURN     :: void : NULL
// **/
void SacFxReceiveBuffer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId,
	Message* forwardedMsg);



// /**
// FUNCTION   :: SacFxNotifyRrcConnected
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event8
//               dequeue msg from buffer and send upper/lower layer
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const fxRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// RETURN     :: void : NULL
// **/
void SacFxNotifyRrcConnected(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId);



// /**
// FUNCTION   :: SacFxNotifyEndMarker
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event9
//               target eNB or UE dequeue msg from buffer
//               and send upper/lower layer
//               or source eNB is to be finalize
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const fxRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// RETURN     :: void : NULL
// **/
void SacFxNotifyEndMarker(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId);



// /**
// FUNCTION   :: SacFxNotifyRlcReset
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event11
//               discard msg in retransmission buffer
//               and send msg in reordering buffer to upper layer
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rRnti              : const fxRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// RETURN     :: void : NULL
// **/
void SacFxNotifyRlcReset(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId);



// /**
// FUNCTION   :: SacFxReEstablishment
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event12
//               begin buffering received msg
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const fxRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// RETURN     :: void : NULL
// **/
void SacFxReEstablishment(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId);
//yt
void SacFxPrintStat(
	Node* node,
	int interfaceIndex,
	FxSacData* sacData);

FxSacEntity* SacFxGetSacEntity(
	Node* node,
	int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId);

void SacFxProcessSacSdu(
	Node* node,
	int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId,
	Message* sacSdu);

void SacFxProcessReorderingBuffer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& srcRnti,
	const int bearerId,
	BOOL isReEstablishment,
	FxSacEntity* sacEntity,
	list<Message*>& reorderingBuffer,
	Message* sacPdu);

void SacFxAddHeader(
	Node* node,
	const int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId,
	Message* sacSdu,
	const UInt16 sn);

void SacFxSendSacPduToRlc(
	Node* node,
	const int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId,
	Message* sacPdu,
	UInt16& sn,
	list<Message*>& retransmissionBuffer);

Message* SacFxDequeueRetransmissionBuffer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId,
	SacFxHoManager& hoManager,
	list<Message*>& buffer);

Message* SacFxDequeueReorderingBuffer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& srcRnti,
	const int bearerId,
	SacFxHoManager& hoManager,
	list<Message*>& buffer,
	list<Message*>::iterator& itr);

void SacFxEnqueueReorderingBuffer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& srcRnti,
	const int bearerId,
	SacFxHoManager& hoManager,
	list<Message*>& buffer,
	list<Message*>::iterator& itr,
	Message* msg);

void SacFxEnqueueRetransmissionBuffer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId,
	SacFxHoManager& hoManager,
	list<Message*>& buffer,
	Message* msg);







//Added by WLH
void SacFxNotifyEndMarker(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId);



// /**
// STRUCT       ::FxSacReceiveStatus
// DESCRIPTION  :: for SN status transfer request
// **/
typedef struct FxSacReceiveStatus {
	// managed only the last submitted PDCP received SN
	// in place of the bitmap
	UInt16 lastSubmittedPdcpRxSn;

	FxSacReceiveStatus(
		UInt16 argLastSubmittedPdcpRxSn)
		: lastSubmittedPdcpRxSn(argLastSubmittedPdcpRxSn)
	{
	}

	// get last submitted PDCP rx SN
	UInt16 GetLastSubmittedPdcpRxSn(void) { return lastSubmittedPdcpRxSn; }
} FxSacReceiveStatus;

void SacFxReceiveBuffer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId,
	Message* forwardedMsg);
void SacFxSetSnStatusTransferItem(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId,
	SacFxSnStatusTransferItem& item);
#endif // _MAC_SAC_FX_H_
