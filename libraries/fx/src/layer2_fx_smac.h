#ifndef LAYER2_FX_MAC_H
#define LAYER2_FX_MAC_H

#include <node.h>
#include "fx_common.h"


#define SMAC_FX_DEFAULT_FRAME_DURATION (100*MILLI_SECOND)

#define SMAC_FX_DEFAULT_SLOT_DURATION (3120*MICRO_SECOND)
#define SMAC_FX_DEFAULT_INTER_FRAME_DURATION (160*MICRO_SECOND)
#define SMAC_FX_DEFAULT_RA_BACKOFF_TIME (50*MILLI_SECOND)
#define SMAC_FX_DEFAULT_RA_PREAMBLE_INITIAL_RECEIVED_TARGET_POWER (-90.0)

//Gss
#define SMAC_FX_RRELCIDFL_WITH_15BIT_SUBHEADER_SIZE (3)
#define SMAC_FX_DEFAULT_F_FIELD (1)
#define SMAC_FX_DEFAULT_LCID_FIELD (0)
#define SMAC_FX_DEFAULT_E_FIELD (0)
//


typedef enum {
	SMAC_FX_POWER_OFF,
	SMAC_FX_IDEL,
	SMAC_FX_RA_RSP_WAITING,
	SMAC_FX_RA_BACKOFF_WAITING,
	SMAC_FX_DEFAULT_STATUS,
	SMAC_FX_STATUS_NUM
} SmacFxState;

typedef struct {
	fxRnti ueRnti; // UE's RNTI
	int bufferSizeLevel; // [0-63]
	UInt32 bufferSizeByte; // Sendable size in TX and/or RE-TX Buffer in RLC
} FxBsrInfo;



typedef struct {
	UInt32 numResourceBlocks;
} SmacFxTxInfo;

//gss
typedef struct {
	UInt32 numberOfSduFromUpperLayer;
	UInt32 numberOfPduToLowerLayer;
	UInt32 numberOfPduFromLowerLayer;
	UInt32 numberOfSduToUpperLayer;
} FxSmacStatData;

typedef struct {

	// configurable parameter
	//FX_SCHEDULER_TYPE enbSchType;    //MAC-FX-ENB-SCHEDULER-TYPE
	//FX_SCHEDULER_TYPE ueSchType;    //MAC-FX-UE-SCHEDULER-TYPE

	// random seed
	RandomSeed seed;

	// State Variable

	UInt16 currentSuperFrameNum; // 超帧号
	UInt8 currentFrameNum; // 帧号
	UInt8 currentSlotNum; // 时隙号

	int preambleTransmissionCounter; // PREAMBLE_TRANSMISSION_COUNTER

	// Stat data   gss
	FxSmacStatData statData;

									 // State
	SmacFxState smacState;

	// Timer
	//FxMapMessage mapSmacTimer;

	// Last propagation delay
	clocktype lastPropDelay;

	// 竞争接入退避有关参数
	UInt8 RA_B;
	UInt8 RA_D;
	UInt8 RA_N;
} FxSmacData;


//Gss    
// DESCRIPTION :: R/R/E/LCID/F/L sub-header with 15-bit L field
typedef struct {
	UInt8 e;
	UInt8 lcid;
	UInt8 f;
	UInt16 l;
} FxSmacRrelcidflWith15bitSubHeader;


void SmacFxInit(Node* node,
	UInt32 interfaceIndex,
	const NodeInput* nodeInput);

void SmacFxFinalize(Node* node, UInt32 interfaceIndex);

void SmacFxProcessEvent(Node* node, UInt32 interfaceIndex, Message* msg);

void SmacFxStartSchedulingAtXG(
	Node* node, UInt32 interfaceIndex, Message* msg);

void SmacFxStartSchedulingAtYH(
	Node* node, UInt32 interfaceIndex, Message* msg);

void SmacFxReceiveTransportBlockFromPhy(
	Node* node, int interfaceIndex, fxRnti srcRnti,
	Message* transportBlockMsg, BOOL isErr);

void SmacFxNotifyPowerOn(Node* node, int interfaceIndex);

void SmacFxNotifyPowerOff(Node* node, int interfaceIndex);


void SmacFxAddDestinationInfo(Node* node,
	Int32 interfaceIndex,
	Message* msg,
	const fxRnti& oppositeRnti);

int FxGetPhyIndexFromMacInterfaceIndex(Node* node, int interfaceIndex);

UInt64 SmacFxGetSlotNum(Node* node, int interfaceIndex);

UInt64 SmacFxGetFrameNum(Node * node, int interfaceIndex);

void SmacFxStartRandomAccessAtYH(Node* node, int interfaceIndex);

void SmacFxNotifyReceivedRaPreambleAtXG(Node* node,
	Int32 interfaceIndex,
	const fxRnti& hyRnti);

void SmacFxNotifyReceivedRaGrantAtYH(Node* node,
	Int32 interfaceIndex);

void SmacFxNotifyReceivedRrcSetupAtYH(Node * node, Int32 interfaceIndex);

void SmacFxReceiveTransportBlockFromPhy(
	Node* node, int interfaceIndex, fxRnti srcRnti,
	Message* transportBlockMsg);

void SmacFxSetNextTtiTimer(Node* node, Int32 interfaceIndex);

#endif