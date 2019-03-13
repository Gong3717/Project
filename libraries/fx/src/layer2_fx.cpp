#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "layer2_fx.h"
#include "phy_fx.h"
#include "api.h"



FxStationType FxLayer2GetStationType(Node* node, int interfaceIndex)
{
	return ((Layer2DataFx*)(node->macData[interfaceIndex]->macVar))->stationType;
}

void  FxLayer2UpperLayerHasPacketToSend(
	Node* node,
	int interfaceIndex,
	Layer2DataFx* fx)
{
	SacFxUpperLayerHasPacketToSend(node, interfaceIndex, fx->fxSacData);
}

const fxRnti& FxLayer2GetRnti(Node* node, int interfaceIndex)
{
	return ((Layer2DataFx*)(node->macData[interfaceIndex]->macVar))->myRnti;
}



Layer2DataFx* FxLayer2GetLayer2DataFx(Node* node, int interfaceIndex)
{
	return (Layer2DataFx*)(node->macData[interfaceIndex]->macVar);
}

Layer3DataFx* FxLayer2GetLayer3DataFx(
	Node* node,
	int interfaceIndex) {
	return ((Layer2DataFx*)(node->macData[interfaceIndex]->macVar))->fxLayer3Data;
}


FxSacData* FxLayer2GetFxSacData(Node* node, int interfaceIndex)
{
	return ((Layer2DataFx*)(node->macData[interfaceIndex]->macVar))->fxSacData;
}

FxSlcData* FxLayer2GetFxSlcData(Node* node, int interfaceIndex)
{
	return ((Layer2DataFx*)(node->macData[interfaceIndex]->macVar))->fxSlcVar;
}

FxSmacData* FxLayer2GetFxSmacData(Node* node, int interfaceIndex)
{
	return ((Layer2DataFx*)(node->macData[interfaceIndex]->macVar))->fxMacData;
}



void FxLayer2Init(Node* node,
	UInt32 interfaceIndex,
	const NodeInput* nodeInput)
{
	// Layer2Data 基本可以代表这个空口协议了，包含了空口协议的绝大部分记录
	Layer2DataFx* fx = NULL;
	fx = (Layer2DataFx*)MEM_malloc(sizeof(Layer2DataFx));

	memset(fx, 0, sizeof(Layer2DataFx));

	// 因为macVar的类型并不能确定，可能是多种MAC协议，所以定义时用void*
	// void * 类型指针，这个类型指针指向了实实在在的存放数据的地址，但是该地址存放的数据的数据类型暂时不知道
	node->macData[interfaceIndex]->macVar = (void *)fx;

	// RRC
	fx->rrcConfig = new FxRrcConfig();
	fx->fxLayer3Data = new Layer3DataFx();

	// Layer2 sublayers
	fx->fxMacData = new FxSmacData();
	fx->fxSlcVar = new FxSlcData();
	fx->fxSacData = new FxSacData();


	fx->myMacData = node->macData[interfaceIndex];

	fx->myRnti.nodeId = node->nodeId;
	fx->myRnti.interfaceIndex = interfaceIndex;


	// Station Type
	char retChar[MAX_STRING_LENGTH] = { 0 };
	BOOL wasFound = false;

	NodeAddress interfaceAddress =
		MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
			node, node->nodeId, interfaceIndex);
	IO_ReadString(node->nodeId,
		interfaceAddress,
		nodeInput,
		"MAC-FX-STATION-TYPE",
		&wasFound,
		retChar);

	if (wasFound)
	{
		if (strcmp(retChar, "YH") == 0)
		{
			fx->stationType = FX_STATION_TYPE_YH;

			int phyIndex =
				FxGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);
			PhyData* thisPhy = node->phyData[phyIndex];
			PhyDataFx* phy_sphy = (PhyDataFx*)thisPhy->phyVar;
			if (phy_sphy->stationType == FX_STATION_TYPE_SAT)
			{
				return;
			}
		}
		else if (strcmp(retChar, "XG") == 0)
		{
			fx->stationType = FX_STATION_TYPE_XG;
			
			// ask the satellite interface in XGsubnet to inform the beamChannels to XG
			// temp solution
			int phyIndex =
				FxGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);
			PhyData* thisPhy = node->phyData[phyIndex];
			PhyDataFx* phy_sphy = (PhyDataFx*)thisPhy->phyVar;
			if (phy_sphy->stationType == FX_STATION_TYPE_SAT)
			{
				node->FLinterfaceIndex = phyIndex;
				FxPhySetBeamChannels(node, phyIndex);
				return;
			}
		}
	}

	// Init SMAC Layer
	SmacFxInit(node, interfaceIndex, nodeInput);

	// Init SLC Layer
	SlcFxInit(node, interfaceIndex, nodeInput);

	// Init SAC Layer
	SacFxInit(node, interfaceIndex, nodeInput);

	// Init Layer3
	FxLayer3Init(node, interfaceIndex, nodeInput);

}

void FxLayer2Finalize(Node * node, UInt32 interfaceIndex)
{
	Layer2DataFx* fx = NULL;
	fx = (Layer2DataFx*)MEM_malloc(sizeof(Layer2DataFx));

	int phyIndex =
		FxGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phy_sphy = (PhyDataFx*)thisPhy->phyVar;
	if (phy_sphy->stationType == FX_STATION_TYPE_SAT)
	{
		return;
	}
	
	delete GetFxRrcConfig(node, interfaceIndex);
	
	SmacFxFinalize(node, interfaceIndex);

	SlcFxFinalize(node, interfaceIndex);

	SacFxFinalize(node, interfaceIndex);

	FxLayer3Finalize(node, interfaceIndex);
	
}

void RrcFxNotifyCellSelectionResultsAtYH(Node * node, UInt32 interfaceIndex, BOOL isSuccess, const fxRnti & xgRnti)
{
	FxStationType stationType = FxLayer2GetStationType(node, interfaceIndex);
	assert(stationType == FX_STATION_TYPE_YH);

	if (isSuccess)
	{
		Layer3DataFx* layer3Data =
			FxLayer2GetLayer3DataFx(node, interfaceIndex);

		FxConnectionInfo* connectionInfo = Layer3FxGetConnectionInfo(node, interfaceIndex, xgRnti);
		if (!connectionInfo)
		{
			Layer3FxAddConnectionInfo(node, interfaceIndex, xgRnti, LAYER3_FX_CONNECTION_WAITING);
		}
		
		SmacFxStartRandomAccessAtYH(node, interfaceIndex);
	}

}

void RrcFxNotifyNetworkEntryFromYH(Node * node, int interfaceIndex, const fxRnti & yhRnti)
{
	FxConnectionInfo* connectionInfo = Layer3FxGetConnectionInfo(node, interfaceIndex, yhRnti);

	if (!connectionInfo)
		Layer3FxAddConnectionInfo(node, interfaceIndex, yhRnti, LAYER3_FX_CONNECTION_WAITING);

}

void RrcFxNotifyRandomAccessResultsAtYH(Node * node, UInt32 interfaceIndex, BOOL isSuccess, const fxRnti & xgRnti)
{

	//Layer3FxProcessRrcConnected(node, interfaceIndex, xgRnti,0);
		
}

Layer2DataFx * FxLayer2GetLayer2Data(Node * node, int interfaceIndex)
{
	return (Layer2DataFx*)(node->macData[interfaceIndex]->macVar);
}

const UInt16 FxLayer2GetSacSnFromSacHeader(const Message * sacPdu)
{
	const FxSacHeader* sacHeader = (FxSacHeader*)MESSAGE_ReturnPacket(sacPdu);
	const UInt16 sn = (sacHeader->octet[0] << 8) | sacHeader->octet[1];

	return sn;

}



void Layer3FxGetSchedulableListSortedByConnectedTime(Node * node, int interfaceIndex, vecFxRnti * store, int beamIndex)

{
	Layer3DataFx* layer3Data =
		FxLayer2GetLayer3DataFx(node, interfaceIndex);

	// temp buffer to sort
	std::multimap<clocktype, fxRnti> tempBuffer;

	// collect
	for (MapFxConnectionInfo::iterator it =
		layer3Data->connectionInfoMap.begin();
		it != layer3Data->connectionInfoMap.end();
		++it)
	{
		if (it->second.state == LAYER3_FX_CONNECTION_CONNECTED &&
			it->second.connectedInfo.isSchedulable)
		{
			MapSBCPInfo::iterator beamInfo = layer3Data->SBCPMap.find(beamIndex);
			fxRnti destRnti = it->first;

			if (beamInfo->second.find(destRnti.nodeId) == beamInfo->second.end()) continue;

			tempBuffer.insert(std::multimap<clocktype, fxRnti>::value_type(
				it->second.connectedInfo.connectedTime, it->first));
		}
	}
	// clear
	store->clear();
	// sort
	for (std::multimap<clocktype, fxRnti>::iterator it =
		tempBuffer.begin(); it != tempBuffer.end(); ++it)
	{
		store->push_back(it->second);
	}
}

void Layer3FxGetMultiBeamIndexVec(Node * node, int interfaceIndex, vector<int>& beams)
{
	Layer3DataFx* layer3Data =
		FxLayer2GetLayer3DataFx(node, interfaceIndex);

	MapSBCPInfo::iterator sbcp = layer3Data->SBCPMap.begin();
	while (sbcp != layer3Data->SBCPMap.end())
	{
		beams.push_back(sbcp->first);
		sbcp++;
	}

}

void FxLayer2ReceivePacketFromPhy(Node * node, int interfaceIndex, Layer2DataFx * fx, Message * msg)
{
	PhyFxPhyToMacInfo* info =
		(PhyFxPhyToMacInfo*)MESSAGE_ReturnInfo(
			msg,
			(unsigned short)INFO_TYPE_FxPhyFxPhyToMacInfo);

	assert(info != NULL);

	SmacFxReceiveTransportBlockFromPhy(node, interfaceIndex, info->srcRnti, msg);
}

void FxLayer2ProcessEvent(Node * node, int interfaceIndex, Message * msg)
{
	switch (msg->eventType)
	{
		//SMAC Event
	case MSG_MAC_FX_TtiTimerExpired:
	case MSG_MAC_FX_RaResponseWaitingTimerExpired:
	{
		SmacFxProcessEvent(node, interfaceIndex, msg);
		break;
	}

	// SAC
	case MSG_SAC_FX_DiscardTimerExpired:
	case MSG_SAC_FX_DelayedSacSdu:
	{

		break;
	}
		//RRC Event
	case MSG_RRC_FX_HandoverScheduleTimerExpired:
	case MSG_RRC_FX_XdHandoverScheduleTimerExpired:
	{
		Layer3FxProcessEvent(node, interfaceIndex, msg);
		break;
	}

	default:
	{
		char errBuf[MAX_STRING_LENGTH] = { 0 };
		sprintf(errBuf,
			"Event Type(%d) is not supported "
			"by FX Layer2.", msg->eventType);
		ERROR_ReportError(errBuf);
		break;
	}
	}
}

