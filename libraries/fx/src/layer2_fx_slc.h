#ifndef LAYER2_FX_SLC_H
#define LAYER2_FX_SLC_H


#include <node.h>
#include <deque>
#include "fx_common.h"

////////////////////////////////////////////////////////////////////////////
// const parameter
////////////////////////////////////////////////////////////////////////////

#define FX_SLC_SN_MAX         (1023) // 10bit
#define FX_SLC_RESET_TIMER_DELAY_MSEC (10)

// status PDU header
//
// D/C(1bit) + CPE(3bit) + ACK_SN(10bit) + E1(1bit)
#define FX_SLC_AM_STATUS_PDU_FIXED_BIT_SIZE (15)
// NACK_SN(10bit) + E1(1bit) + E2(1bit)
#define FX_SLC_AM_STATUS_PDU_EXTENSION_E1_E2_BIT_SIZE (12)
// SOstart(15bit) + SOend(15bit)
#define FX_SLC_AM_STATUS_PDU_EXTENSION_SO_BIT_SIZE (30)

// PDU header fixed part
//
// D/C(1bit) + RF(1bit) + P(1bit) + FI(2bit) + E(1bit) + SN(10bit)
#define FX_SLC_AM_DATA_PDU_FORMAT_FIXED_BIT_SIZE (16)
// PDU header fixed part size.(byte)
#define FX_SLC_AM_DATA_PDU_FORMAT_FIXED_BYTE_SIZE \
            (FX_SLC_AM_DATA_PDU_FORMAT_FIXED_BIT_SIZE / 8)

// PDU header extendion part
//
// E(1bit) + Li(11bit)
#define FX_SLC_AM_DATA_PDU_FORMAT_EXTENSION_BIT_SIZE (12)
// E+Li minimum length(1extension) = 12bit = 1.5octet = 2Byte
#define FX_SLC_AM_DATA_PDU_FORMAT_EXTENSION_MIN_BYTE_SIZE (2)
// E+Li padding = 4bit (If the number of Li is odd numbers)
#define FX_SLC_AM_DATA_PDU_FORMAT_EXTENSION_PADDING_BIT_SIZE (4)

// PDU header resegment part
//
// LSF(1bit) + SO(15bit)
#define FX_SLC_AM_DATA_PDU_RESEGMENT_FORMAT_EXTENSIO_BIT_SIZE (16)
// PDU header resegment part size.(byte)
#define FX_SLC_AM_DATA_PDU_RESEGMENT_FORMAT_EXTENSIO_BYTE_SIZE \
            (FX_SLC_AM_DATA_PDU_RESEGMENT_FORMAT_EXTENSIO_BIT_SIZE / 8)

// Minimum SLC-PDUsegment data size.(without header)
#define FX_SLC_AM_DATA_PDU_MIN_BYTE (0)

// const parameter AM
#define FX_SLC_AM_WINDOW_SIZE (512)
#define FX_SLC_AM_SN_UPPER_BOUND (FX_SLC_SN_MAX + 1)

// invalid
#define FX_SLC_INVALID_BEARER_ID   (-1)
#define FX_SLC_INVALID_SEQ_NUM (-1)
#define FX_SLC_INVALID_SLC_SN (FX_SLC_SN_MAX + 1)
#define FX_SLC_INVALID_STATUS_PDU_FORMAT_NACK_SEGMENT_OFFSET (-1)

// parameter infinity
#define FX_SLC_INFINITY_POLL_PDU   (-1)
#define FX_SLC_INFINITY_POLL_BYTE  (-1)

// parameter strings
#define FX_SLC_STRING_MAX_RETX_THRESHOLD \
        "SLC-FX-MAX-RETX-THRESHOLD"
#define FX_SLC_STRING_POLL_PDU          "SLC-FX-POLL-PDU"
#define FX_SLC_STRING_POLL_BYTE         "SLC-FX-POLL-BYTE"
#define FX_SLC_STRING_POLL_RETRANSMIT   "SLC-FX-T-POLL-RETRANSMIT"
#define FX_SLC_STRING_T_REORDERING      "SLC-FX-T-REORDERING"
#define FX_SLC_STRING_T_STATUS_PROHIBIT "SLC-FX-T-STATUS-PROHIBIT"

// default parameter
#define FX_SLC_DEFAULT_MAX_RETX_THRESHOLD (8)
#define FX_SLC_DEFAULT_POLL_PDU           (16)
#define FX_SLC_DEFAULT_POLL_BYTE          (250)
#define FX_SLC_DEFAULT_POLL_RETRANSMIT    (100) //msec
#define FX_SLC_DEFAULT_T_REORDERING       (100) //msec
#define FX_SLC_DEFAULT_T_STATUS_PROHIBIT  (12) //msec

// parameter LIMIT
#define FX_SLC_MIN_MAX_RETX_THRESHOLD (0)
#define FX_SLC_MAX_MAX_RETX_THRESHOLD (INT_MAX)
#define FX_SLC_MIN_POLL_PDU           (0)
#define FX_SLC_MAX_POLL_PDU           (INT_MAX)
#define FX_SLC_MIN_POLL_BYTE          (0)
#define FX_SLC_MAX_POLL_BYTE          (INT_MAX)
#define FX_SLC_MIN_POLL_RETRANSMIT    (1*MILLI_SECOND)
#define FX_SLC_MAX_POLL_RETRANSMIT    (CLOCKTYPE_MAX)
#define FX_SLC_MIN_T_REORDERING      (1*MILLI_SECOND)
#define FX_SLC_MAX_T_REORDERING      (CLOCKTYPE_MAX)
#define FX_SLC_MIN_T_STATUS_PROHIBIT  (1*MILLI_SECOND)
#define FX_SLC_MAX_T_STATUS_PROHIBIT  (CLOCKTYPE_MAX)


// /**
// CONSTANT    :: FX_SLC_DEF_MAXBUFSIZE
// DESCRIPTION :: Default value of the maximum buffer size
// **/
const UInt64 FX_SLC_DEF_MAXBUFSIZE = 50000;


////////////////////////////////////////////////////////////////////////////
// typedef
////////////////////////////////////////////////////////////////////////////

// FUNCTION   :: FxSlcDuplicateMessageList
// LAYER      :: ANY
// PURPOSE    :: Duplicate a list of message
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msgList   : Message* : Pointer to a list of messages
// RETURN     :: Message* : The duplicated message list header
// **/
Message* FxSlcDuplicateMessageList(
	Node* node,
	Message* msgList);


////////////////////////////////////////////////////////////////////////////
// Enumlation
////////////////////////////////////////////////////////////////////////////
// /**
// ENUM::       FxSlcStatus
// DESCRIPTION::SLC Status
// **/
typedef enum
{
	FX_SLC_POWER_OFF,
	FX_SLC_POWER_ON
} FxSlcStatus;

// /**
// ENUM::       FxSlcEntityDirect
// DESCRIPTION::Traffic direction of a SLC entity
// **/
typedef enum
{
	FX_SLC_ENTITY_TX,
	FX_SLC_ENTITY_RX,
	FX_SLC_ENTITY_BI
} FxSlcEntityDirect;

// /**
// ENUM:: FxSlcTimerType
// DESCRIPTION:: SLC timer Type
// **/
typedef enum
{
	FX_SLC_TIMER_RST,
	FX_SLC_TIMER_POLL_RETRANSEMIT,
	FX_SLC_TIMER_REODERING,
	FX_SLC_TIMER_STATUS_PROHIBIT,
}FxSlcTimerType;

// /**
// ENUM:: FxSlcEntityType
// DESCRIPTION:: SLC Entites
// **/
typedef enum
{
	FX_SLC_ENTITY_INVALID,
	FX_SLC_ENTITY_TM,
	FX_SLC_ENTITY_UM,
	FX_SLC_ENTITY_AM
} FxSlcEntityType;

// /**
// ENUM::FxSlcPduType
// DESCRIPTION:: SLC PDU states
//
typedef enum
{
	FX_SLC_AM_PDU_TYPE_STATUS,           // Status PDU
	FX_SLC_AM_PDU_TYPE_PDU,              // DATA without Segment
	FX_SLC_AM_PDU_TYPE_PDU_SEGMENTATION, // DATA with Segment
	FX_SLC_AM_PDU_TYPE_INVALID
} FxSlcPduType;

// /**
// ENUM::FxSlcPduState
// DESCRIPTION:: SLC PDU states
//
// **/
typedef enum
{
	FX_SLC_AM_FRAMING_INFO_NO_SEGMENT = 0x00, // 00
	FX_SLC_AM_FRAMING_INFO_IN_FIRST_BYTE = 0x01, // 01
	FX_SLC_AM_FRAMING_INFO_IN_END_BYTE = 0x02, // 10
	FX_SLC_AM_FRAMING_INFO_FRAGMENT = 0x03  // 11
} FxSlcAmFramingInfo;

#define FX_SLC_AM_FRAMING_INFO_CHK_FIRST_BYTE  0x02 //10(mask)
#define FX_SLC_AM_FRAMING_INFO_CHK_END_BYTE    0x01 //01(mask)

// /**
// ENUM::FxSlcPduState
// DESCRIPTION:: SLC PDU states
//
// **/
typedef enum
{
	FX_SLC_AM_CONTROL_PDU_TYPE_STATUS = 0x000,  // 00
	FX_SLC_AM_CONTROL_PDU_TYPE_LEN              // reserved 001-111
} FxSlcAmControlPduType;

// /**
// ENUM::
// DESCRIPTION:: SLC PDU states
//
// **/
typedef enum
{
	FX_SLC_AM_DATA_PDU_STATUS_WAIT,
	FX_SLC_AM_DATA_PDU_STATUS_NACK,
	FX_SLC_AM_DATA_PDU_STATUS_ACK
} FxSlcAmDataPduStatus;


////////////////////////////////////////////////////////////////////////////
// Structure
////////////////////////////////////////////////////////////////////////////


// /**
// STRUCT:: FxSlcStats
// DESCRIPTION:: SLC statistics
// **/
typedef struct
{
	UInt32 numSdusReceivedFromUpperLayer;
	UInt32 numSdusDiscardedByOverflow;
	UInt32 numSdusSentToUpperLayer;
	UInt32 numDataPdusSentToMacSublayer;
	UInt32 numDataPdusDiscardedByRetransmissionThreshold;
	UInt32 numDataPdusReceivedFromMacSublayer;
	UInt32 numDataPdusReceivedFromMacSublayerButDiscardedByReset;
	UInt32 numAmStatusPdusSendToMacSublayer;
	UInt32 numAmStatusPdusReceivedFromMacSublayer;
	UInt32 numAmStatusPdusReceivedFromMacSublayerButDiscardedByReset;
} FxSlcStats;

// /**
// STRUCT:: FxSlcTimerInfoHdr
// DESCRIPTION::
//      The first part of SLC timer info
// **/
typedef struct {
	fxRnti rnti;
	int bearerId;
	FxSlcEntityDirect direction;
	UInt32 infoLen;
} FxSlcTimerInfoHdr;

// /**
// STRUCT::     FxSlcFreeMessage
// DESCRIPTION::
//      Function object used to free Message pointer in STL container
// **/
class FxSlcFreeMessage
{
	Node* node;
public:
	explicit FxSlcFreeMessage(Node* ndPtr) :node(ndPtr) { }
	void operator() (Message* msg)
	{
		while (msg)
		{
			Message* nextMsg = msg->next;
			MESSAGE_Free(node, msg);
			msg = nextMsg;
		}
	}
};

// /**
// STRUCT::     FxSlcFreeMessageData
// DESCRIPTION::
//      Function object used to delete sduSegment in the list
// **/
template < typename T >
class FxSlcFreeMessageData
{
	Node* node;
public:
	explicit FxSlcFreeMessageData(Node* ndPtr) :node(ndPtr) { }
	void operator() (T messageData)
	{
		Message* msg = messageData.message;
		while (msg)
		{
			Message* nextMsg = msg->next;
			MESSAGE_Free(node, msg);
			msg = nextMsg;
		}
		//MEM_free(messageData);
	}
};

// /**
// STRUCT::     FxSlcAmPduFormatFixed
// DESCRIPTION:: SLC PDU header struct (nomal fixed part)
// **/
struct FxSlcAmPduFormatFixed
{
	// D/C(Data/Control)  TRUE:Data PDU  FALSE:Control PDU.
	BOOL dcPduFlag;

	// RF(resegmentation flag)   TRUE:PDU SEGMENT   FALSE:PDU.
	BOOL resegmentationFlag;

	// P(Polling Bit)  TRUE:Request status PDU  FALSE:NoRequest.
	BOOL pollingBit;

	// FI(Framing Info)
	FxSlcAmFramingInfo FramingInfo;

	// SN(Sequence Number)
	Int16 seqNum;

	// FX SLC Header Size (FX Lib Specific item)
	UInt32 headerSize;

	FxSlcAmPduFormatFixed()
	{
		dcPduFlag = FALSE;
		resegmentationFlag = FALSE;
		pollingBit = FALSE;
		FramingInfo = FX_SLC_AM_FRAMING_INFO_NO_SEGMENT;
		seqNum = FX_SLC_INVALID_SEQ_NUM;
		headerSize = 0;
	}

	FxSlcAmPduFormatFixed(FxSlcPduType& pduType)
	{
		switch (pduType)
		{
		case FX_SLC_AM_PDU_TYPE_PDU:
		{
			dcPduFlag = TRUE;
			resegmentationFlag = FALSE;
			pollingBit = FALSE;
			FramingInfo = FX_SLC_AM_FRAMING_INFO_NO_SEGMENT;
			seqNum = FX_SLC_INVALID_SEQ_NUM;
			headerSize = FX_SLC_AM_DATA_PDU_FORMAT_FIXED_BYTE_SIZE;
			break;
		}

		case FX_SLC_AM_PDU_TYPE_PDU_SEGMENTATION:
		{
			dcPduFlag = TRUE;
			resegmentationFlag = FALSE;
			pollingBit = FALSE;
			FramingInfo = FX_SLC_AM_FRAMING_INFO_FRAGMENT;
			seqNum = FX_SLC_INVALID_SEQ_NUM;
			headerSize = FX_SLC_AM_DATA_PDU_FORMAT_FIXED_BYTE_SIZE;
			break;
		}
		default:
		{
			ERROR_Assert(FALSE, "FxSlcAmPduFormatFixed:"
				"pduType No Support");
		}
		}
	}
};

// /**
// STRUCT::     FxSlcAmPduFormatExtension
// DESCRIPTION:: SLC PDU header struct (extentional part)
// **/
struct FxSlcAmPduFormatExtension
{
	// number of LI(Length Indicator)
	size_t numLengthIndicator;
	// LI(Length Indicator)  SLC SDU length
	std::vector < UInt16 > lengthIndicator;

	FxSlcAmPduFormatExtension()
	{
		numLengthIndicator = 0;
	}
};

// /**
// STRUCT::     FxSlcAmPduFormatExtension
// DESCRIPTION:: SLC PDU header struct (lengthIndicator-info part)
//               for massage info-field.
// **/
struct FxSlcAmPduFormatExtensionMsgInfo
{
	// number of LI(Length Indicator)
	size_t numLengthIndicator;
	// LI(Length Indicator)  SLC SDU length
	UInt16 lengthIndicator;
};

// /**
// STRUCT::     FxSlcAmPduSegmentFormatFixed
// DESCRIPTION:: SLC PDU header struct (extentional fixed part)
// **/
struct FxSlcAmPduSegmentFormatFixed
{
	// LSF(Last Segment Flag)
	// TRUE :PDU segment at the end of the last SLC PDU Byte Byte and match.
	// FALSE:do not match.
	BOOL lastSegmentFlag;

	// SO(Segment Offset).
	UInt16 segmentOffset;

	FxSlcAmPduSegmentFormatFixed()
	{
		lastSegmentFlag = FALSE;
		segmentOffset = 0;
	}

};

// /**
// STRUCT::     FxSlcStatusPduFormatFixed
// DESCRIPTION:: SLC status PDU header struct (fixed part)
// **/
struct FxSlcStatusPduFormatFixed
{
	// D/C(Data/Control)  TRUE:Data PDU  FALSE:Control PDU.
	BOOL dcPduFlag;

	// Control PDU type
	FxSlcAmControlPduType cpt;

	// Ack sequens number
	Int32 ackSn;

	FxSlcStatusPduFormatFixed()
	{
		dcPduFlag = FALSE;
		cpt = FX_SLC_AM_CONTROL_PDU_TYPE_STATUS;
		ackSn = FX_SLC_INVALID_SEQ_NUM;
	}
};

// /**
// STRUCT::     FxSlcStatusPduSoPair
// DESCRIPTION:: SLC status PDU header struct (nacked-info part)
// **/
struct FxSlcStatusPduSoPair
{
	// SOstart(Segment Offset Start)
	Int32 soStart;

	// SOend(Segment Offset End)
	Int32 soEnd;
};

// /**
// STRUCT::     FxSlcStatusPduFormatExtension
// DESCRIPTION:: SLC status PDU header struct (fixed part)
// **/
struct FxSlcStatusPduFormatExtension
{
	UInt16 nackSn;
	FxSlcStatusPduSoPair soOffset;
};

// /**
// STRUCT::     FxSlcStatusPduFormat
// DESCRIPTION:: SLC status PDU header struct
// **/
struct FxSlcStatusPduFormat
{
	// SLC status PDU header struct (fixed part)
	FxSlcStatusPduFormatFixed fixed;

	// Nack info
	//    first  : sequens numbers
	//    second : Segment Offset
	std::multimap < UInt16, FxSlcStatusPduSoPair > nackSoMap;
};

// /**
// STRUCT::     FxSlcStatusPduFormatExtensionInfo
// DESCRIPTION:: SLC status PDU header struct (nacked-info part)
//               for massage info-field.
// **/
struct FxSlcStatusPduFormatExtensionInfo
{
	// NACKed SO(Segment Offset) infomation-str size
	size_t nackInfoSize;
	// NACKed SO(Segment Offset) infomation-str
	FxSlcStatusPduFormatExtension nackInfo;
};

// /**
// STRUCT::     FxSlcHeader
// DESCRIPTION:: Slc Header
// **/
struct FxSlcHeader
{
	FxSlcAmPduFormatFixed*                fixed;
	FxSlcAmPduSegmentFormatFixed*         segFixed;
	FxSlcAmPduFormatExtension*            extension;

	BOOL firstSdu;
	UInt16 firstSduLi;

	FxSlcHeader()
	{
		fixed = NULL;
		segFixed = NULL;
		extension = NULL;

		firstSdu = TRUE;
		firstSduLi = 0;
	}

	FxSlcHeader(FxSlcPduType pduType)
	{

		switch (pduType)
		{
		case FX_SLC_AM_PDU_TYPE_PDU:
		{
			fixed = new FxSlcAmPduFormatFixed(pduType);
			segFixed = NULL;
			extension = NULL;

			firstSdu = TRUE;
			firstSduLi = 0;

			return;
		}
		case FX_SLC_AM_PDU_TYPE_STATUS:
		{
			fixed = NULL;
			segFixed = NULL;
			extension = NULL;

			firstSdu = FALSE;
			firstSduLi = 0;

			return;
		}
		default:
		{
			// TODO
			ERROR_ReportError("FxSlcHeader(): entity type error.\n");
			return;
		}
		}
	}

	void FxSlcSetPduSeqNo(int sn)
	{
		if (fixed != NULL)
		{
			fixed->seqNum = (Int16)sn;
		}
	};

};

// /**
// STRUCT::     FxSlcAmTxBuffer
// DESCRIPTION:: SLC Sdu struct
// **/
struct FxSlcAmTxBuffer
{
	Message*    message;
	int         offset;

	FxSlcAmTxBuffer()
	{
		message = NULL;
		offset = 0;
	}

	void store(Node* node, int interfaceIndex, Message* msg)
	{
		message = msg;
	}

	void restore(Node* node,
		int interfaceIndex,
		int seqNum,
		int size,
		FxSlcHeader& slcHeader,
		Message* msg)
	{
		message = msg;
		slcHeader.fixed->dcPduFlag = TRUE;
		slcHeader.fixed->pollingBit = 0;
		slcHeader.fixed->resegmentationFlag = FALSE;
		slcHeader.fixed->seqNum = (Int16)seqNum;
	}

	int getSduSize()
	{
		if (message == NULL)
		{
			return 0;
		}
		else
		{
			return MESSAGE_ReturnPacketSize(message);
		}
	}
};

// /**
// STRUCT::     FxSlcAmReTxBuffer
// DESCRIPTION:: SLC Sdu FxSlcAmReTxBuffer
// **/
struct FxSlcAmReTxBuffer
{
	Message*    message;
	int         offset;
	UInt8       retxCount;

	FxSlcHeader slcHeader;
	FxSlcAmDataPduStatus pduStatus;
	std::map < int, FxSlcStatusPduSoPair > nackField;

	FxSlcAmReTxBuffer()
	{
		message = NULL;
		retxCount = 0;
		offset = 0;
		pduStatus = FX_SLC_AM_DATA_PDU_STATUS_WAIT;
	}

	int getPduSize(Node* node, int interfaceIndex);
	int getNackedSize(Node* node, int interfaceIndex);

	void store(Node* node,
		int interfaceIndex,
		FxSlcHeader& header,
		Message* msg)
	{
		slcHeader = header;
		message = msg;
	}
};

// /**
// STRUCT::     FxSlcAmRcvPduSegmentData
// DESCRIPTION:: SLC-PDUsegment received data
// **/
struct FxSlcAmRcvPduSegmentData
{
	Message*    message;
	int         pduSize;    // Hdr+data
	int         pduDataSize;// data only
	BOOL        lastSegmentFlag;
};

// /**
// STRUCT::     FxSlcAmRcvPduSegmentData
// DESCRIPTION:: SLC-PDUsegment received data-part infomation
// **/
struct FxSlcAmRcvPduSegmentPart
{
	UInt16      rcvdHead;   // received data-part head position
	UInt16      rcvdTail;   // received data-part tail position
};

// /**
// STRUCT::     FxSlcAmRcvPduData
// DESCRIPTION:: SLC-PDU received data
// **/
struct FxSlcAmRcvPduData
{
	//FxSlcHeader    header;
	Message*        message;

	Int16           orgMsgSize;

	// flaged SLC-PDU receive finished. (positive ACK)
	BOOL        slcPduRcvFinished;

	// FI(Framing Info)
	FxSlcAmFramingInfo FramingInfo;

	// LI(Length Indicator) list
	std::vector < UInt16 > lengthIndicator;

	// SLC-PDUsegment received part infomation
	//    first  : SO(Segment Offset)start
	//    second : SLC-PDUsegment pair(SOstart,SOend)
	std::map < UInt16, FxSlcAmRcvPduSegmentPart > rcvdPduSegmentInfo;
	UInt16  endPduPos;  // current end-pos.
	BOOL    gotLsf;     // received LastSegmentFlag

	UInt32  reAssembledPos;

	// received Nacked part info (Segment Offset).
	std::deque < FxSlcStatusPduSoPair > rcvNackedSegOffset;

	FxSlcAmRcvPduData()
	{
		message = NULL;
		orgMsgSize = 0;
		slcPduRcvFinished = FALSE;
		FramingInfo = FX_SLC_AM_FRAMING_INFO_NO_SEGMENT;
		lengthIndicator.clear();
		rcvdPduSegmentInfo.clear();
		endPduPos = 0;
		gotLsf = FALSE;
		rcvNackedSegOffset.clear();
		reAssembledPos = 0;
	}

};

// /**
// STRUCT::     FxSlcSduSegment
// DESCRIPTION::
//      The sdu segment structure for re-assembly process
// **/
struct FxSlcSduSegment
{
	Int16               seqNum;
	FxSlcAmFramingInfo FramingInfo;
	Message*            newMsg;

	FxSlcSduSegment()
	{
		newMsg = NULL;
		seqNum = FX_SLC_INVALID_SEQ_NUM;
		FramingInfo = FX_SLC_AM_FRAMING_INFO_IN_FIRST_BYTE;
	}
};

// /**
// STRUCT:: FxSlcEntity
// DESCRIPTION::SLC entity structure
// **/
struct FxSlcEntity
{
	Node*              node;
	int                bearerId;
	fxRnti            oppositeRnti;
	FxSlcEntityType   entityType;
	void*              entityData;


	FxSlcEntity()
	{
		node = NULL;
		oppositeRnti = FX_INVALID_RNTI;
		bearerId = FX_SLC_INVALID_BEARER_ID;
		entityType = FX_SLC_ENTITY_INVALID;
		entityData = NULL;
	};

	void reset(Node* node);
	void store(Node* node, std::string& buffer);
	void restore(Node* node, const char* buffer, size_t bufSize);

	void setSlcEntityToXmEntity();
};

// /**
// STRUCT:: FxSlcAmEntity
// DESCRIPTION::SLC entity structure
// **/
struct FxSlcAmEntity
{
	FxSlcEntity*         entityVar;

	UInt64                maxTransBufSize;
	//UInt64                bufSize;
	UInt64                txBufSize;
	UInt64                retxBufSize;
	UInt64                retxBufNackSize;
	UInt64                statusBufSize;

	// The transmitting side of each AM SLC entity shall maintain
	// the following state variables:
	UInt32    ackSn;     // a) VT(A) . Acknowledgement state variable
	UInt32    sndWnd;    // b) VT(MS) . Maximum send state variable
	UInt32    sndNextSn; // c) VT(S) . Send state variable
	UInt32    pollSn;    // d) POLL_SN . Poll send state variable

						 // The transmitting side of each AM SLC entity shall maintain
						 // the following counters:
	UInt32    pduWithoutPoll;   //   a) PDU_WITHOUT_POLL . Counter
	UInt32    byteWithoutPoll;  //   b) BYTE_WITHOUT_POLL . Counter
	UInt32    retxCount;        //   c) RETX_COUNT . Counter

								// The receiving side of each AM SLC entity shall maintain
								// the following state variables:
	UInt32 rcvNextSn;     // a) VR(R) . Receive state variable
	UInt32 rcvWnd;   // b) VR(MR) . Maximum acceptable receive state variable
	Int32  tReorderingSn; // c) VR(X) . t-Reordering state variable
	UInt32 maxStatusSn; // d) VR(MS) . Maximum STATUS transmit state variable
	UInt32 rcvSn;         // e) VR(H) . Highest received state variable

	UInt32    oldRcvNextSn;  // for Reassemble SDU

							 // Timer
	Message* pollRetransmitTimerMsg;
	Message* reoderingTimerMsg;
	Message* statusProhibitTimerMsg;
	Message* resetTimerMsg;

	BOOL exprioryTPollRetransmitFlg;        // Expriory-t-PollRetransmit
	BOOL waitExprioryTStatusProhibitFlg;    // Wait_Expriory_T_satus_prohibit

	BOOL resetFlg; // ON : Waiting Reset timer is expired.
	BOOL sendResetFlg; // ON : Send Reset Msg.

					   // Transmission Buffer
	std::list < FxSlcAmTxBuffer > txBuffer;

	// Latest sending PDU
	FxSlcAmReTxBuffer latestPdu;

	// Retransmission Buffer
	std::list < FxSlcAmReTxBuffer > retxBuffer;

	// Status PDU Buffer
	std::list < FxSlcStatusPduFormat > statusBuffer;

	// Reception Buffer
	std::multimap < unsigned int, Message* > rcvSlcSduList;
	std::list < FxSlcSduSegment >   rcvSlcSduSegList;
	std::list < FxSlcSduSegment >   rcvSlcSduTempList;

	// SLC-PDU received buffer
	//    first  : SLC-PDU sequense numbser
	//    second : SLC-PDU received data
	std::map < unsigned int, FxSlcAmRcvPduData >    rcvPduBuffer;

	// unreceived SLC ACK map
	//  first:  SLC SN
	//  second: SAC SN
	list<pair<UInt16, UInt16> > unreceivedSlcAckList;

	void reset(Node* node);

	// Enqueue to Transmission Buffer
	void store(Node* node, int interfaceIndex, Message* msg);

	// Dequeue
	void restore(
		Node* node,
		int interfaceIndex,
		int size,
		std::list < Message* > &txMsg);

	FxSlcAmEntity()
	{
		txBufSize = 0;
		retxBufSize = 0;
		retxBufNackSize = 0;
		statusBufSize = 0;
		latestPdu.message = NULL;

		maxTransBufSize = FX_SLC_DEF_MAXBUFSIZE;
		entityVar = NULL;
		// The transmitting side of each AM SLC entity shall maintain
		// the following state variables:
		ackSn = 0;
		sndWnd = FX_SLC_AM_WINDOW_SIZE;
		sndNextSn = 0;
		pollSn = 0;

		// The transmitting side of each AM SLC entity shall maintain
		// the following counters:
		pduWithoutPoll = 0;
		byteWithoutPoll = 0;
		retxCount = 0;

		// The receiving side of each AM SLC entity shall maintain
		// the following state variables:
		rcvNextSn = 0;
		rcvWnd = FX_SLC_AM_WINDOW_SIZE;
		tReorderingSn = FX_SLC_INVALID_SEQ_NUM;
		maxStatusSn = 0;
		rcvSn = 0;

		oldRcvNextSn = 0;

		// Timer
		pollRetransmitTimerMsg = NULL;
		reoderingTimerMsg = NULL;
		statusProhibitTimerMsg = NULL;
		resetTimerMsg = NULL;

		exprioryTPollRetransmitFlg = FALSE;
		waitExprioryTStatusProhibitFlg = FALSE;
		resetFlg = FALSE;
		sendResetFlg = FALSE;
	}

	int getBufferSize(Node* node,
		int interfaceIndex);

	// xyt
	void setTxBufferSize(int size)
	{
		maxTransBufSize = size;
	}

	// test func
	int getTestTxBufferSize()
	{
		int size = 0;
		std::list < FxSlcAmTxBuffer > ::iterator it;
		for (it = txBuffer.begin(); it != txBuffer.end(); it++)
		{
			size += MESSAGE_ReturnPacketSize(it->message);
		}
		return size;
	}

	// test func
	int getTestRetxBufferSize()
	{
		int size = 0;
		std::list < FxSlcAmReTxBuffer > ::iterator it;
		for (it = retxBuffer.begin(); it != retxBuffer.end(); it++)
		{
			size += MESSAGE_ReturnPacketSize(it->message);
		}
		return size;
	}
	// test func
	int getTestNackedSize()
	{
		Node* node;
		int interfaceIndex;

		int size = 0;
		std::list < FxSlcAmReTxBuffer > ::iterator it;
		for (it = retxBuffer.begin(); it != retxBuffer.end(); it++)
		{
			size += it->getNackedSize(node, interfaceIndex);
		}
		return size;
	}
	// test func
	int getTestStatusSize();

	BOOL discardPdu(Node* node,
		int iface,
		std::list < FxSlcAmReTxBuffer > ::iterator& it,
		std::list < FxSlcAmReTxBuffer > &buf,
		std::list < Message* > &pdu,
		int& retxPduByte);

	BOOL getRetxBuffer(Node* node,
		int interfaceIndex,
		int& restSize,
		std::list < FxSlcAmReTxBuffer > &buf,
		std::list < Message* > &pdu,
		int& retxPduByte);

	void getStatusBuffer(Node* node,
		int interfaceIndex,
		int& restSize,
		std::list < FxSlcStatusPduFormat > &buf,
		std::list < Message* > &pdu,
		int& statusPduByte);

	BOOL getTxBuffer(Node* node,
		int interfaceIndex,
		int& restSize,
		std::list < FxSlcAmTxBuffer > &buf,
		std::list < Message* > &pdu,
		int& txPduByte);

	BOOL chkExpriory_t_PollRetransmit(void)
	{
		return exprioryTPollRetransmitFlg;
	}

	void resetExpriory_t_PollRetransmit(void)
	{
		exprioryTPollRetransmitFlg = FALSE;
	}

	void polling(Node* node,
		int interfaceIndex,
		FxSlcHeader& slcHeader,
		Message* newMsg);

	void polling(Node* node,
		int interfaceIndex,
		int& restSize,
		BOOL makedPdu,
		BOOL makedPduSegment,
		std::list < Message* > &pdu,
		int& pollPduByte);

	void concatenationMessage(Node* node,
		int interfaceIndex,
		Message*& prev,
		Message* next);

	void concatenation(Node* node,
		int interfaceIndex,
		FxSlcHeader& slcHeader,
		Message*& prev,
		Message* next);

	int getSndSn();

	BOOL txWindowCheck();

	BOOL divisionMessage(Node* node,
		int interfaceIndex,
		int offset,
		Message* msg,
		Message* fragHead,
		Message* fragTail,
		TraceProtocolType protocolType,
		BOOL msgFreeFlag);

	void resetAll(BOOL withoutResetProcess = FALSE);
	UInt32 getNumAvailableRetxPdu(
		Node* node,
		int interfaceIndex,
		int size);

	void dump(Node* node, int interfaceIndex, const char* procName);
};



// /**
// STRUCT::     FxSlcPackedPduInfo
// DESCRIPTION::
//      The information stored in the info field
//      of each PDU after packing
// **/
typedef struct
{
	unsigned int pduSize;
} FxSlcPackedPduInfo;

typedef std::list < FxSlcAmEntity* >  FxSlcEntityList;


// /**
// STRUCT:: FxSlcAmResetData
// DESCRIPTION:: Reset Information
//               If src's retx count is reached RetxThreshold,
//               src send this data to dst.
//               When dst receive this data, reset
// **/
typedef struct
{
	fxRnti src; // node reached retx threshold

	fxRnti dst; // node received this data
	int bearerId;
} FxSlcAmResetData;

// /**
// STRUCT:: Layer2FxSlcData
// DESCRIPTION:: SLC sublayer structure
// **/
typedef struct {
	FxSlcStatus status;
	int txBufferMaxSize;

	FxSlcStats stats;
} FxSlcData;

// /**
// STRUCT:: FxSlcAmSacPduInfo
// DESCRIPTION:: add info when store SLC SDU in transmission buffer
// **/
typedef struct FxSlcAmSacPduInfo {
	UInt16 sacSn;

	FxSlcAmSacPduInfo(const UInt16 argSacSn)
		: sacSn(argSacSn)
	{
	}
} FxSlcAmSacPduInfo;


////////////////////////////////////////////////////////////////////////////
// Function for SLC Layer
////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////
// eNB/UE - API
////////////////////////////////////////////////////////////////////////////

void SlcFxNotifyPowerOn(Node* node, int interfaceIndex);
void SlcFxNotifyPowerOff(Node* node, int interfaceIndex);
void SlcFxEstablishment(Node* node,
	int interfaceIndex,
	const fxRnti& srcRnti,
	const int bearerId);

FxSlcEntity* SlcFxGetSlcEntity(Node* node,
	int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId);

void SlcFxInitSlcEntity(Node* node,
	int interfaceIndex,
	const fxRnti& rnti,
	const FxSlcEntityType& type,
	const FxSlcEntityDirect& direct,
	FxSlcEntity* entity);

void SlcFxRelease(Node* node,
	int interfaceIndex,
	const fxRnti& srcRnti,
	const int bearerId);
// /**
// FUNCTION::       SlcFxInit
// LAYER::          FX Layer2 SLC
// PURPOSE::        SLC Initalization
//
// PARAMETERS::
//    + node:       Node* : pointer to the network node
//    + interfaceIndex: unsigned int : interdex of interface
//    + nodeInput:  const NodeInput* : Input from configuration file
// RETURN::         NULL
// **/
void SlcFxInit(Node* node,
	unsigned int interfaceIndex,
	const NodeInput* nodeInput);

// /**
// FUNCTION::       SlcFxFinalize
// LAYER::          FX Layer2 SLC
// PURPOSE::        SLC finalization function
// PARAMETERS::
//    + node:       Node* : pointer to the network node
//    + interfaceIndex: unsigned int : interdex of interface
// RETURN::         NULL
// **/
void SlcFxFinalize(Node* node, unsigned int interfaceIndex);

// /**
// FUNCTION::       SlcFxProcessEvent
// LAYER::          FX Layer2 SLC
// PURPOSE::        SLC event handling function
// PARAMETERS::
//    + node:       Node * : pointer to the network node
//    + interfaceIndex: unsigned int : interdex of interface
//    + message     Message* : Message to be handled
// RETURN::         NULL
// **/
void SlcFxProcessEvent(Node* node,
	unsigned int interfaceIndex,
	Message* message);

void SlcFxReceiveSduFromSac(
	Node* node,
	int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId,
	Message* slcSdu);
void SlcFxDeliverPduToSmac(
	Node * node, 
	int interfaceIndex, 
	const fxRnti& dstRnti,
	std::list<Message*>* sduList,
	int tbSize);
void SlcFxDeliverPduToSmac(
	Node* node,
	int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId,
	int size,
	std::list < Message* >* sduList);

UInt32 SlcFxSendableByteInQueue(
	Node* node,
	int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId);

UInt32 SlcFxSendableByteInTxQueue(
	Node* node,
	int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId);

void SlcFxReceivePduFromSmac(Node * node, int interfaceIndex, const fxRnti & srcRnti, const int bearerId, Message* msg);
/*void SlcFxReceivePduFromSmac(
Node* node,
int interfaceIndex,
const fxRnti& srcRnti,
const int bearerId,
std::list < Message* > &pduList);*/

void SlcFxReceivePduFromSmac(Node * node, int iface, const fxRnti & srcRnti, const int bearerId, std::list < Message* > &pduList);

int SlcFxCheckDeliverPduToMac(
	Node* node,
	int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId,
	int size,
	std::list < Message* >* sduList);

UInt32 SlcFxGetQueueSizeInByte(
	Node* node,
	int interfaceIndex,
	const fxRnti& dstRnti,
	const int bearerId);

void SlcFxAmIndicateResetWindow(
	Node* node,
	int interfaceIndex,
	const fxRnti& oppositeRnti,
	const int bearerId);

void SlcFxAmSetResetData(
	Node* node,
	int interfaceIndex,
	FxSlcAmEntity* amEntity);

void SlcFxAmGetResetData(
	Node* node,
	int interfaceIndex,
	std::vector < FxSlcAmResetData > &vecAmResetData);

void SlcFxLogPrintWindow(
	Node* node,
	int interfaceIndex,
	FxSlcEntity* slcEntity);



// /**
// FUNCTION:: SlcFxReEstablishment
// LAYER::    FX Layer2 SLC
// PURPOSE::  send all reassembled SLC SDUs to SAC
//
// PARAMETERS::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const fxRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// + targetRnti         : const fxRnti& : RNTI of target eNB
// RETURN::         void     : NULL
// **/
void SlcFxReEstablishment(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId,
	const fxRnti& targetRnti);



// FUNCTION:: SlcFxDiscardSlcSduFromTransmissionBuffer
// LAYER::    FX Layer2 SLC
// PURPOSE::  discard the indicated SLC SDU if no segment of the SLC SDU has
//            been mapped to a SLC data PDU yet.
// PARAMETERS::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const fxRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// + sacSn             : const UInt16&  : SAC SN
// RETURN::         void     : NULL
// **/
void SlcFxDiscardSlcSduFromTransmissionBuffer(
	Node* node,
	const int interfaceIndex,
	const fxRnti& rnti,
	const int bearerId,
	const UInt16& sacSn);
//yt
UInt32 SlcFxSendableByteInTxQueue(
	Node* node,
	int iface,
	const fxRnti& dstRnti,
	const int bearerId);

#endif //LAYER2_FX_SLC_H