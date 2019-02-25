
#ifndef LAYER2_FX_H
#define LAYER2_FX_H


#include <node.h>
#include <list>


#include "layer2_fx_sac.h"
#include "layer2_fx_slc.h"
#include "layer2_fx_smac.h"
#include "layer3_fx.h"



typedef struct struct_layer2_data_fx
{

	MacData* myMacData;     // Mac Layer
	RandomSeed randSeed;    // Random Object
	fxRnti myRnti;         // Identifier

	// RRC Config
	FxRrcConfig* rrcConfig;

	// Sublayer, Scheduler, Layer3
	FxSmacData* fxMacData; // layer2 layer fx struct pointer
	FxSlcData* fxSlcVar; // RLC sublayer struct pointer
	FxSacData* fxSacData; // PDCP sublayer struct pointer
	// class FxScheduler* fxScheduler; // SCH sublayer struct pointer
	Layer3DataFx* fxLayer3Data;

	// configurable parameter
	FxStationType stationType;

}Layer2DataFx;


typedef struct struct_smac_fx_msg_destination_info_str
{
	fxRnti dstRnti;
} SmacFxMsgDestinationInfo;


FxStationType FxLayer2GetStationType(Node* node, int interfaceIndex);
const fxRnti& FxLayer2GetRnti(Node* node, int interfaceIndex);

Layer2DataFx* FxLayer2GetLayer2DataFx(Node* node, int interfaceIndex);

FxSacData* FxLayer2GetFxSacData(Node* node, int interfaceIndex);
FxSlcData* FxLayer2GetFxSlcData(Node* node, int interfaceIndex);
FxSmacData* FxLayer2GetFxSmacData(Node* node, int interfaceIndex);


void  FxLayer2UpperLayerHasPacketToSend(
	Node* node,
	int interfaceIndex,
	Layer2DataFx* fx);


Layer3DataFx* FxLayer2GetLayer3DataFx(
	Node* node,
	int interfaceIndex);


void FxLayer2Init(Node* node,
	UInt32 interfaceIndex,
	const NodeInput* nodeInput);

void FxLayer2Finalize(Node* node,
	UInt32 interfaceIndex);

void RrcFxNotifyCellSelectionResultsAtYH(Node* node,
	UInt32 interfaceIndex, BOOL isSuccess, const fxRnti& xgRnti);

void RrcFxNotifyNetworkEntryFromYH(
	Node* node, int interfaceIndex, const fxRnti& xgRnti);


void RrcFxNotifyRandomAccessResultsAtYH(Node* node,
	UInt32 interfaceIndex, BOOL isSuccess, const fxRnti& xgRnti);


Layer2DataFx* FxLayer2GetLayer2Data(Node* node, int interfaceIndex);

const UInt16 FxLayer2GetSacSnFromSacHeader(const Message* sacPdu);


void Layer3FxGetSchedulableListSortedByConnectedTime(
	Node* node, int interfaceIndex, vecFxRnti* store, int beamIndex);


void Layer3FxGetMultiBeamIndexVec(Node* node, int interfaceIndex, vector<int>& beams);


void FxLayer2ReceivePacketFromPhy(Node* node,
	int interfaceIndex,
	Layer2DataFx* fx,
	Message* msg);

void FxLayer2ProcessEvent(Node* node,
	int interfaceIndex,
	Message* msg);
#endif