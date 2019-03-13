#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "api.h"
#include "antenna.h"
#include "antenna_global.h"

#include "antenna_switched.h" //For switched beam antenna model
#include "antenna_steerable.h" //For steerable antenna model
#include "antenna_patterned.h" //For patterned antenna model

#include "phy_fx.h"
#include "layer2_fx.h"
//gss xd
#include <fstream>
#include<iostream>
#include <set>


static
void PhySphyReportStatusToMac(
	Node *node,
	int phyNum,
	PhyStatusType status,
	clocktype receiveDuration,
	const Message* potentialIncomingPacket)
{
	return;
}





static //inline//
BOOL PhySphyCheckRxPacketError(Node* node,
	PhyDataFx* phy_sphy,
	double rxPowermW,
	double interferencePowermW)
{
	BOOL packetError = FALSE;
	double noise = phy_sphy->thisPhy->noise_mW_hz * 30000000;
	double sinr = rxPowermW / (interferencePowermW + noise);

	phy_sphy->latest_rxSinr_db = IN_DB(sinr);

	if (phy_sphy->thisPhy->phyRxModel == SNR_THRESHOLD_BASED)
	{
		if (sinr < phy_sphy->thisPhy->phyRxSnrThreshold)
		{
			packetError = TRUE;
		}
		else {
			packetError = FALSE;
		}
	}

	return packetError;
}



BOOL PhySphySetCqiReportInfoAtYH(Node* node, PhyDataFx* phy_sphy, Message* msg)
{

	double snr = phy_sphy->latest_rxSinr_db;

	MESSAGE_AppendInfo(node, msg, sizeof(PhySphyCqi), (unsigned short)INFO_TYPE_FxPhyCqiInfo);

	PhySphyCqi* info = (PhySphyCqi*)MESSAGE_ReturnInfo(msg, (unsigned short)INFO_TYPE_FxPhyCqiInfo);

	if (info != NULL)
	{

		return TRUE;
	}

	return FALSE;
}


BOOL PhySphySetMIBInfoAtXG(Node* node, int phyIndex, Message* msg)
{
	BOOL anyMIB = FALSE;

	int ifaceIndex = node->phyData[phyIndex]->macInterfaceIndex;
	UInt8 currentslot = SmacFxGetSlotNum(node, ifaceIndex);
	UInt8 currentFrame = SmacFxGetFrameNum(node, ifaceIndex);

	// 偶数帧10、11号时隙发送MIB1，星历信息
	if (currentFrame % 2 == 0 && (currentslot == 10 || currentslot == 11))
	{
		MIB1* info = (MIB1*)MESSAGE_AppendInfo(node, msg, sizeof(MIB1), INFO_TYPE_FxPhyMIB1);
		anyMIB = TRUE;

		PhyFxTxInfo* txInfo = (PhyFxTxInfo*)MESSAGE_ReturnInfo(msg, INFO_TYPE_FxPhyTxInfo);
		txInfo->destRNTI = FX_INVALID_RNTI;
	}

	// 0、16号时隙重复发送MIB2
	// 星号、信关站号、帧号，上行窄带信道配置掩码、上行信道配置、波束号，随机接入超时定时器
	if (currentslot == 0 || currentslot == 16)
	{
		MIB2* info = (MIB2*)MESSAGE_AppendInfo(node, msg, sizeof(MIB2), INFO_TYPE_FxPhyMIB2);
		info->frame_num = currentFrame;
		info->slot_num = currentslot;
		info->xgId = node->nodeId;
		anyMIB = TRUE;

		PhyFxTxInfo* txInfo = (PhyFxTxInfo*)MESSAGE_ReturnInfo(msg, INFO_TYPE_FxPhyTxInfo);
		txInfo->destRNTI = FX_INVALID_RNTI;
	}

	// 每帧24、25及26号时隙发送本秒，下1秒，下2秒的卫星位置信息
	if (currentslot == 24)
	{
		MIB3* info = (MIB3*)MESSAGE_AppendInfo(node, msg, sizeof(MIB3), INFO_TYPE_FxPhyMIB3);
		anyMIB = TRUE;

		PhyFxTxInfo* txInfo = (PhyFxTxInfo*)MESSAGE_ReturnInfo(msg, INFO_TYPE_FxPhyTxInfo);
		txInfo->destRNTI = FX_INVALID_RNTI;
	}

	return anyMIB;
}




BOOL PhySphySetRrcconnectionSetupInfoAtXG(Node* node, int phyIndex, Message* msg)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	if (phyFx->RRCsetups->size() > 0)
	{
		RRCConnectionSetup* rrcSetup = (RRCConnectionSetup*)MESSAGE_AppendInfo(node, msg, sizeof(RRCConnectionSetup), (unsigned short)INFO_TYPE_FxPhyRrcConnectionSetup);

		if (rrcSetup != NULL)
		{
			fxRnti yhRnti = *phyFx->RRCsetups->begin();

			int interfaceIndex =
				node->phyData[phyIndex]->macInterfaceIndex;
			int beamIndex = Layer3FxGetBeamIndexofYHAtXG(node, interfaceIndex, yhRnti.nodeId);
			assert(beamIndex != -1);

			PhyFxBeamInfo* beamInfo = (PhyFxBeamInfo*)MESSAGE_ReturnInfo(msg, (unsigned short)INFO_TYPE_FxPhyBeamInfo);
			if (beamIndex != beamInfo->beamIndex) return FALSE;

			phyFx->RRCsetups->erase(phyFx->RRCsetups->begin());

			rrcSetup->MacAddr = yhRnti.nodeId;

			PhyFxTxInfo* txInfo = (PhyFxTxInfo*)MESSAGE_ReturnInfo(msg, INFO_TYPE_FxPhyTxInfo);
			txInfo->destRNTI = yhRnti;
			return TRUE;
		}
	}

	return FALSE;
}



BOOL PhySphySetRrcconnectionReconfigurationInfoAtXG(Node* node, int phyIndex, Message* msg)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	if (phyFx->rrcReconfs->size() > 0)
	{		
		int destNodeId = phyFx->rrcReconfs->begin()->first;
		UInt8 targetBeam = phyFx->rrcReconfs->begin()->second;

		int interfaceIndex =
			node->phyData[phyIndex]->macInterfaceIndex;
		int beamIndex = Layer3FxGetBeamIndexofYHAtXG(node, interfaceIndex, destNodeId);
		assert(beamIndex != -1);

		PhyFxBeamInfo* beamInfo = (PhyFxBeamInfo*)MESSAGE_ReturnInfo(msg, (unsigned short)INFO_TYPE_FxPhyBeamInfo);
		if (beamIndex != beamInfo->beamIndex) return FALSE;

		RRCConnectionReconfiguration* reconf = (RRCConnectionReconfiguration*)
			MESSAGE_AppendInfo(node, msg, sizeof(RRCConnectionReconfiguration), INFO_TYPE_FxPhyRrcConnectionReconfiguration);
		reconf->targetBeamIndex = targetBeam;

		auto record = phyFx->beamChannel->find(targetBeam);
		reconf->ulChannelIndex = record->second.first;
		reconf->dlChannelIndex = record->second.second;

		PhyFxTxInfo* txInfo = (PhyFxTxInfo*)MESSAGE_ReturnInfo(msg, INFO_TYPE_FxPhyTxInfo);
		txInfo->destRNTI = fxRnti(destNodeId, 0);

		phyFx->rrcReconfs->erase(*phyFx->rrcReconfs->begin());
		return TRUE;
	}

	return FALSE;
}


BOOL PhySphySetRrcConnectionReconfigurationCompleteAtYH(Node* node, int phyIndex, Message* msg)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	if (phyFx->rrcReconfCompleteFlag)
	{
		RRCConnectionReconfigurationComplete* info = (RRCConnectionReconfigurationComplete*)
			MESSAGE_AppendInfo(node, msg, sizeof(RRCConnectionReconfigurationComplete), INFO_TYPE_FxPhyRrcConnectionReconfigurationComplete);
		info->MacAddr = node->nodeId;

		phyFx->rrcReconfCompleteFlag = FALSE;

		return TRUE;
	}

	return FALSE;
}


BOOL PhySphySetRAREQInfoAtYH(Node* node, int phyIndex, Message* msg)
{
	
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	if (phyFx->RAREQcount == -1) return FALSE;

	// 竞争接入在1-31时隙
	int ifaceIndex = node->phyData[phyIndex]->macInterfaceIndex;
	UInt8 currentslot = SmacFxGetSlotNum(node, ifaceIndex);

	if (currentslot == 0) return FALSE;

	if (phyFx->RAREQcount == 0)
	{
		RAREQ* rarequest = (RAREQ*)MESSAGE_AppendInfo(node, msg, sizeof(RAREQ), INFO_TYPE_FxPhyRandamAccessRequest);
		rarequest->MacAddr = node->nodeId;

		phyFx->RAREQcount--;

		return TRUE;
	}

	phyFx->RAREQcount--;
	return FALSE;
}


BOOL PhySphySetRrcconnectionRequestInfoAtYH(Node* node, int phyIndex, Message* msg)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	if (phyFx->randomAccessCompleteFlag)
	{
		RRCConnectionRequest* rrcRequest = (RRCConnectionRequest*)MESSAGE_AppendInfo(node, msg, sizeof(RRCConnectionRequest), INFO_TYPE_FxPhyRrcConnectionRequest);
		rrcRequest->MacAddr = node->nodeId;

		phyFx->randomAccessCompleteFlag = FALSE;
		return TRUE;
	}
	return FALSE;
}





BOOL PhySphySetRARSPInfoAtXG(Node* node, int phyIndex, Message* msg)
{

	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	if (phyFx->raGrants->size() > 0)
	{
		RARSP* raresponse = (RARSP*)MESSAGE_AppendInfo(node, msg, sizeof(RARSP), (unsigned short)INFO_TYPE_FxPhyRandamAccessResponse);
		
		if (raresponse != NULL)
		{
			fxRnti yhRnti = *phyFx->raGrants->begin();

			int interfaceIndex =
				node->phyData[phyIndex]->macInterfaceIndex;
			int beamIndex = Layer3FxGetBeamIndexofYHAtXG(node, interfaceIndex, yhRnti.nodeId);
			assert(beamIndex != -1);

			PhyFxBeamInfo* beamInfo = (PhyFxBeamInfo*)MESSAGE_ReturnInfo(msg, (unsigned short)INFO_TYPE_FxPhyBeamInfo);
			if (beamIndex != beamInfo->beamIndex) return FALSE;

			phyFx->raGrants->erase(phyFx->raGrants->begin());

			raresponse->BCID = yhRnti.nodeId;
			raresponse->sub_chl_num = phyFx->BLChannels.back();
			phyFx->BLChannels.pop_back();

			
			PhyFxTxInfo* txInfo = (PhyFxTxInfo*)MESSAGE_ReturnInfo(msg, INFO_TYPE_FxPhyTxInfo);
			txInfo->destRNTI = yhRnti;

			return TRUE;
		}
	}

	return FALSE;
}

BOOL PhySphySetRrcconnectionSetupCompleteInfoAtYH(Node* node, int phyIndex, Message* msg)
{

	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	if (phyFx->rrcSetupCompleteflag)
	{
		RRCConnectionSetupComplete* rrcComplete = 
			(RRCConnectionSetupComplete*)MESSAGE_AppendInfo(node, msg, sizeof(RRCConnectionSetupComplete), (unsigned short)INFO_TYPE_FxPhyRrcConnectionSetupComplete);
		

		if (rrcComplete != NULL)
		{
			rrcComplete->MacAddr = node->nodeId;

			phyFx->rrcSetupCompleteflag = FALSE;

			return TRUE;
		}
	}

	return FALSE;
}


BOOL PhySphySetRrcHandoverRequestInfoAtYH(Node* node, int phyIndex, Message* msg)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	if (phyFx->nextBeam->size() != 0)
	{
		UInt8 nextBeamIndex = *(phyFx->nextBeam->begin());

		RRCHandoverRequest* HoRequest = 
			(RRCHandoverRequest*)MESSAGE_AppendInfo(node, msg, sizeof(RRCHandoverRequest),INFO_TYPE_FxPhyRrcHandoverRequest);

		if (HoRequest != NULL)
		{
			HoRequest->targetBeamIndex = nextBeamIndex;
			phyFx->nextBeam->clear();
			
			cout << "node" << node->nodeId << " : request for beam handover at " << getSimTime(node) / (double)SECOND << " second." << endl;
			return TRUE;
		}
	}

	return FALSE;
}


BOOL PhyFxSetForwardLinkControlInfoAtXG(Node* node, int phyIndex, Message* packet)
{
	BOOL transmissonFlag = FALSE;

	// 是否设置广播信息
	transmissonFlag = transmissonFlag | PhySphySetMIBInfoAtXG(node, phyIndex, packet);

	// 是否有随机接入应答
	transmissonFlag = transmissonFlag | PhySphySetRARSPInfoAtXG(node, phyIndex, packet);

	// 是否有RRCConnectionSetup报文
	transmissonFlag = transmissonFlag | PhySphySetRrcconnectionSetupInfoAtXG(node, phyIndex, packet);

	// 是否有RRCConnectionReconfiguration报文
	transmissonFlag = transmissonFlag | PhySphySetRrcconnectionReconfigurationInfoAtXG(node, phyIndex, packet);

	

	return transmissonFlag;
}


BOOL PhyFxSetBackwardLinkControlInfoAtYH(Node* node, int phyIndex, Message* packet)
{
	BOOL transmissonFlag = FALSE;

	// 是否有随机接入报文
	transmissonFlag = transmissonFlag | PhySphySetRAREQInfoAtYH(node, phyIndex, packet);

	// 是否有RRCRequest报文
	transmissonFlag = transmissonFlag | PhySphySetRrcconnectionRequestInfoAtYH(node, phyIndex, packet);

	// 是否有RRCConnectionSetupComplete报文
	transmissonFlag = transmissonFlag | PhySphySetRrcconnectionSetupCompleteInfoAtYH(node, phyIndex, packet);

	// 是否有RRCHandoverRequest报文
	transmissonFlag = transmissonFlag | PhySphySetRrcHandoverRequestInfoAtYH(node, phyIndex, packet);

	// 是否有RRCConnectionReconfigurationComplete报文
	transmissonFlag = transmissonFlag | PhySphySetRrcConnectionReconfigurationCompleteAtYH(node, phyIndex, packet);
	
	return transmissonFlag;
}


void PhySphyHandleMessageControlInfo(Node* node,
	int phyIndex,
	PropRxInfo* propRxInfo)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyfx = (PhyDataFx*)thisPhy->phyVar;
	int interfaceIndex =
		node->phyData[phyIndex]->macInterfaceIndex;
	
	PhyFxEstablishmentData* establishmentData = phyfx->establishmentData;
	fxRnti myRnti = fxRnti(
		node->nodeId,
		node->phyData[phyIndex]->macInterfaceIndex);
	Message* rcvMsg = propRxInfo->txMsg;
	PhyFxTxInfo* txInfo = (PhyFxTxInfo*)MESSAGE_ReturnInfo(rcvMsg, (unsigned short)INFO_TYPE_FxPhyTxInfo);

	PhyFxBeamInfo* beamInfo = (PhyFxBeamInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg, (unsigned short)INFO_TYPE_FxPhyBeamInfo);

	// 前向
	if (phyfx->stationType == FX_STATION_TYPE_YH)
	{

		MIB2* mib2 = (MIB2*)MESSAGE_ReturnInfo(rcvMsg, INFO_TYPE_FxPhyMIB2); // MIB2里包含绝大部分控制信息
		MIB3* mib3 = (MIB3*)MESSAGE_ReturnInfo(rcvMsg, INFO_TYPE_FxPhyMIB3);
		
		if (!establishmentData->informed)
		{
			// 收到MIB2
			if (establishmentData->selectedRntiXG == FX_INVALID_RNTI && mib2 != NULL)
			{
				fxRnti xgRnti = fxRnti(mib2->xgId, 0);

				std::set<fxRnti>* xgRntiList = establishmentData->rxRntiList;
				std::set<fxRnti>::iterator it;
				it = xgRntiList->find(xgRnti);

				if (it != xgRntiList->end())
					xgRntiList->erase(it);

				xgRntiList->insert(xgRnti);

				establishmentData->selectedRntiXG = xgRnti;
				
				Layer3FxAddNodetoBeam(node, interfaceIndex, mib2->xgId, beamInfo->beamIndex);

				// 用户站此时进行反向链路的同步
				SmacFxSetNextTtiTimer(node, interfaceIndex);
			}

			// 收到MIB3
			if (establishmentData->selectedRntiXG != FX_INVALID_RNTI && mib3 != NULL)
			{
				// 用户站发起随机接入
				RrcFxNotifyCellSelectionResultsAtYH(node, interfaceIndex, TRUE, establishmentData->selectedRntiXG);
				establishmentData->informed = TRUE;
				
				cout << "node" << node->nodeId << " : received all 3 MIB message at "
					<< getSimTime(node) / (double)SECOND << " second." << " Start random access." << endl;
			}
		}
		
		// 收到随机接入回复消息
		RARSP* raResponse = (RARSP*)MESSAGE_ReturnInfo(propRxInfo->txMsg, (unsigned short)INFO_TYPE_FxPhyRandamAccessResponse);
		if (raResponse != NULL)
		{
			if (node->nodeId == raResponse->BCID)
			{				
				SmacFxNotifyReceivedRaGrantAtYH(node, node->phyData[phyIndex]->macInterfaceIndex);

				// 用户站收到RARSP后开始RRC连接建立
				phyfx->randomAccessCompleteFlag = TRUE;

				// 根据分配的反向信道号设置反向信道TB
				int BLchannelIndx = raResponse->sub_chl_num;
				if (BLchannelIndx == 42)
					PhyFxSetTransblockSize(node, phyIndex, SPHY_FX_TRANSBLOCK_SIZE_15MHZ_BYTE);
				if (BLchannelIndx == 38 || BLchannelIndx == 37)
					PhyFxSetTransblockSize(node, phyIndex, SPHY_FX_TRANSBLOCK_SIZE_5MHZ_BYTE);
				if (BLchannelIndx == 26)
					PhyFxSetTransblockSize(node, phyIndex, SPHY_FX_TRANSBLOCK_SIZE_2500KHZ_BYTE);
			}
		}

		// 收到RRCConnectionSetup
		RRCConnectionSetup* rrcSetup = (RRCConnectionSetup*)MESSAGE_ReturnInfo(rcvMsg, (unsigned short)INFO_TYPE_FxPhyRrcConnectionSetup);
		if (rrcSetup != NULL)
		{
			if (rrcSetup->MacAddr == node->nodeId)			
				SmacFxNotifyReceivedRrcSetupAtYH(node, node->phyData[phyIndex]->macInterfaceIndex);
		}

		// 收到RRCConnectionReconfiguration，可以切换到下一个波束
		RRCConnectionReconfiguration* reconf = (RRCConnectionReconfiguration*)MESSAGE_ReturnInfo(rcvMsg, INFO_TYPE_FxPhyRrcConnectionReconfiguration);
		if (reconf != NULL)
		{
			fxRnti xgRnti(txInfo->sourceRNTI.nodeId, 0);
			Layer3FxProcessRrcReconfigurationAtYH(node, interfaceIndex, xgRnti);
			Layer3FxChangeBeamIndexAtYH(node, interfaceIndex, reconf->targetBeamIndex);

			// 更改物理层上下行信道
			PHY_StopListeningToChannel(node, phyIndex, phyfx->DLchannel.channelIndex);

			phyfx->ULchannel.channelIndex = reconf->ulChannelIndex;
			phyfx->DLchannel.channelIndex = reconf->dlChannelIndex;
						
			PHY_StartListeningToChannel(node, phyIndex, reconf->dlChannelIndex);
			
			cout << "node" << node->nodeId << " : change the uplink and downlink channel at " << getSimTime(node) / (double)SECOND << " second." << endl;
		}
		
	}

	// 反向
	else if (phyfx->stationType == FX_STATION_TYPE_XG)
	{
		// 收到随机接入请求
		RAREQ* rarequest = (RAREQ*)MESSAGE_ReturnInfo(rcvMsg, INFO_TYPE_FxPhyRandamAccessRequest);
		if (rarequest)
		{
			fxRnti yh(rarequest->MacAddr, 0);
			SmacFxNotifyReceivedRaPreambleAtXG(node, interfaceIndex, yh);
			Layer3FxAddNodetoBeam(node, interfaceIndex, rarequest->MacAddr, beamInfo->beamIndex);
		}

		// 收到RRCConnectionRequest
		RRCConnectionRequest* rrcRequest = (RRCConnectionRequest*)MESSAGE_ReturnInfo(rcvMsg, INFO_TYPE_FxPhyRrcConnectionRequest);
		if (rrcRequest)
		{
			phyfx->RRCsetups->insert(fxRnti(rrcRequest->MacAddr, 0));
		}

		// 收到RRCConnectionSetupComplete
		RRCConnectionSetupComplete* rrcComplete = (RRCConnectionSetupComplete*)MESSAGE_ReturnInfo(rcvMsg, INFO_TYPE_FxPhyRrcConnectionSetupComplete);
		if (rrcComplete)
		{
			fxRnti yhRnti(rrcComplete->MacAddr, 0);
			Layer3FxProcessRrcConnected(node, interfaceIndex, yhRnti);					
		}

		// 收到RRCHandoverRequest
		RRCHandoverRequest* HoRequest = (RRCHandoverRequest*)MESSAGE_ReturnInfo(rcvMsg, INFO_TYPE_FxPhyRrcHandoverRequest);
		if (HoRequest)
		{
			UInt8 currentBeam = beamInfo->beamIndex;
			UInt8 targetBeam = HoRequest->targetBeamIndex;
			fxRnti yhRnti(txInfo->sourceRNTI.nodeId, 0);

			Layer3FxProcessHandoverRequestAtXG(node, interfaceIndex, yhRnti);
			PhyFxRrcConnectionReconfIndicationAtXG(node, phyIndex, yhRnti.nodeId, targetBeam);			

			cout << "node" << node->nodeId << " : " << "node " << txInfo->sourceRNTI.nodeId << " is trying to make handover at "<<getSimTime(node) / (double)SECOND << " second."<<endl;
		}

		// 收到RRCReconfigurationComplete
		RRCConnectionReconfigurationComplete* reconfComplete = (RRCConnectionReconfigurationComplete*)MESSAGE_ReturnInfo(rcvMsg, INFO_TYPE_FxPhyRrcConnectionReconfigurationComplete);
		if (reconfComplete)
		{
			UInt8 yhNodeId = reconfComplete->MacAddr;
			fxRnti yhRnti(yhNodeId, 0);

			int currentBeam = Layer3FxGetBeamIndexofYHAtXG(node, interfaceIndex, yhNodeId);
			assert(currentBeam != -1);

			Layer3FxProcessSwitchBeamInfoAtXG(node, interfaceIndex, yhRnti, currentBeam, beamInfo->beamIndex);
			Layer3FxProcessHandoverCompleteAtXG(node, interfaceIndex, yhRnti);
			cout << "node"<<node->nodeId<<" : " <<"node " << (int)yhNodeId << " complete the handover at "<<getSimTime(node) / (double)SECOND << " second." << endl;
		}
	}
}




void PhySphyInit(
	Node* node,
	const int phyIndex,
	const NodeInput *nodeInput)
{
	BOOL   wasFound;

	PhyDataFx* phy_sphy;
	phy_sphy = (PhyDataFx*)MEM_malloc(sizeof(PhyDataFx));
	memset(phy_sphy, 0, sizeof(PhyDataFx));

	// set pointer in upper structure
	node->phyData[phyIndex]->phyVar = (void*)phy_sphy;

	// get a reference pointer to upper structure
	phy_sphy->thisPhy = node->phyData[phyIndex];

	// Antenna model init
	ANTENNA_Init(node, phyIndex, nodeInput);



	#pragma region construct phyData

	char lbuf[MAX_STRING_LENGTH];
	//set station stype
	IO_ReadString(
		node->nodeId,
		phy_sphy->thisPhy->networkAddress,
		nodeInput,
		"PHY-FX-STATION-TYPE",
		&wasFound,
		lbuf);

	if (wasFound) {
		if (strcmp(lbuf, "XG") == 0)
			phy_sphy->stationType = FX_STATION_TYPE_XG;

	}
	if (wasFound) {
		if (strcmp(lbuf, "YH") == 0)
		{
			phy_sphy->stationType = FX_STATION_TYPE_YH;
			phy_sphy->latest_rxSinr_db = -100; //initialize the CQI as unacceptable
		}
	}
	if (wasFound) {
		if (strcmp(lbuf, "SAT") == 0)
			phy_sphy->stationType = FX_STATION_TYPE_SAT;
	}




	// set channel config
	int dlChannelNo;
	int ulChannelNo;


	IO_ReadInt(
		node->nodeId,
		phy_sphy->thisPhy->networkAddress,
		nodeInput,
		"PHY-FX-DL-CHANNEL",
		&wasFound,
		&dlChannelNo);
	if (wasFound) {
		phy_sphy->DLchannel.channelIndex = dlChannelNo;
		phy_sphy->DLchannel.rxMsgError = FALSE;
	}

	IO_ReadInt(
		node->nodeId,
		phy_sphy->thisPhy->networkAddress,
		nodeInput,
		"PHY-FX-UL-CHANNEL",
		&wasFound,
		&ulChannelNo);
	if (wasFound) {
		phy_sphy->ULchannel.channelIndex = ulChannelNo;
		phy_sphy->ULchannel.currentMode = PHY_IDLE;
	}

	if (phy_sphy->stationType == FX_STATION_TYPE_SAT)
	{
		PHY_StartListeningToChannel(node, phyIndex, ulChannelNo);
	}
	else
		PHY_StartListeningToChannel(node, phyIndex, dlChannelNo);

	//set txpower
	double txPower_dBW;
	IO_ReadDouble(
		node->nodeId,
		phy_sphy->thisPhy->networkAddress,
		nodeInput,
		"PHY-FX-TX-POWER",
		&wasFound,
		&txPower_dBW);
	if (wasFound) {
		phy_sphy->txPower_dBm = txPower_dBW;
	}

	double rxThreshold_dBm;
	IO_ReadDouble(
		node->nodeId,
		phy_sphy->thisPhy->networkAddress,
		nodeInput,
		"PHY-FX-RX-THRESHOLD",
		&wasFound,
		&rxThreshold_dBm);
	if (wasFound) {
		phy_sphy->rxThreshold_dBm = rxThreshold_dBm;
	}

#pragma endregion

	phy_sphy->channelBandwidth = 30000000;

	phy_sphy->establishmentData = new PhyFxEstablishmentData;

	phy_sphy->establishmentData->rxRrcConfigList
		= new std::map < fxRnti, FxRrcConfig >;

	phy_sphy->establishmentData->rxRntiList = new std::set<fxRnti>;
	phy_sphy->establishmentData->selectedRntiXG = FX_INVALID_RNTI;
	phy_sphy->establishmentData->informed = FALSE;

	phy_sphy->rrcSetupCompleteflag = FALSE;
	phy_sphy->randomAccessCompleteFlag = FALSE;
	phy_sphy->raGrants = new std::set<fxRnti>;
	phy_sphy->RRCsetups = new std::set<fxRnti>;
	phy_sphy->beamState = new std::map<int, pair<Message*, BOOL>>;
	phy_sphy->nextBeam = new std::set<UInt8>;
	phy_sphy->rrcReconfs = new std::set<pair<UInt8, UInt8>>;
	phy_sphy->beamChannel = new BeamChannelMap;

	phy_sphy->RAREQcount = -1;

	phy_sphy->rrcReconfCompleteFlag = FALSE;

	phy_sphy->lastSignalArrival = 0;

	// xyt : temp for backward link channel assignment.
	if (phy_sphy->stationType == FX_STATION_TYPE_XG)
	{
		phy_sphy->BLChannels.push_back(26);
		phy_sphy->BLChannels.push_back(37);		
		phy_sphy->BLChannels.push_back(38);
		phy_sphy->BLChannels.push_back(42);

		phy_sphy->TBsize = SPHY_FX_TRANSBLOCK_SIZE_30MHZ_BYTE;
	}
}



// 被MAC层调用，开始发送信号
void PhySphyStartTransmittingSignal(
	Node* node,
	int phyIndex,
	Message* packet,
	BOOL useMacLayerSpecifiedDelay,
	clocktype initDelayUntilAirborne)
{

	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phy_sphy = (PhyDataFx*)thisPhy->phyVar;
	clocktype duration;
	
	if (phy_sphy->stationType == FX_STATION_TYPE_SAT)
	{
		PROP_ReleaseSignal(
			node,
			packet,
			phyIndex,
			phy_sphy->DLchannel.channelIndex,
			phy_sphy->txPower_dBm,
			100,
			initDelayUntilAirborne);

		return;
	}
	
	// 检查节点类型，一般卫星不能自己发送信号，只能在接收时转发
	assert(phy_sphy->stationType != FX_STATION_TYPE_SAT);
	
	// 获取上行信道号
	int channelIndex = phy_sphy->ULchannel.channelIndex;	

	// 如果有控制包或数据包的话该变量为true
	BOOL transmissonFlag = TRUE;

	// 给每个包设置 PhyTxinfo
	// PhyTxinfo包含发送时间、发送节点类型、发送源RNTI、发送目的RNTI，供卫星判断转发下行信道
	PhyFxTxInfo* txInfo = (PhyFxTxInfo*)MESSAGE_AppendInfo(node, packet, sizeof(PhyFxTxInfo), (unsigned short)INFO_TYPE_FxPhyTxInfo);

	txInfo->Txtime = getSimTime(node);
	txInfo->txStationType = phy_sphy->stationType;
	txInfo->sourceRNTI = fxRnti(node->nodeId,
		node->phyData[phyIndex]->macInterfaceIndex);


	if (MESSAGE_ReturnInfo(packet, (unsigned short)INFO_TYPE_FxSmacNoTransportBlock))
	{
		// 不含数据包，当前时刻只可能控制包
		transmissonFlag = FALSE;
		
		duration = SMAC_FX_DEFAULT_SLOT_DURATION - 1000;
	}
	else
	{
		// 含数据包
		// 计算信号传输时间
		int packetsize = 0;
		if (!packet->isPacked)
		{
			packetsize = MESSAGE_ReturnPacketSize(packet);
		}
		else
		{
			packetsize = MESSAGE_ReturnActualPacketSize(packet);
		}
		duration = packetsize / (double)PhyFxGetTransblockSize(node, phyIndex) * SMAC_FX_DEFAULT_SLOT_DURATION;
	}

	//duration = SMAC_FX_DEFAULT_SLOT_DURATION-1000;

	// 设置前向链路（信关->用户）的控制信息
	if (phy_sphy->stationType == FX_STATION_TYPE_XG)
		transmissonFlag = transmissonFlag | PhyFxSetForwardLinkControlInfoAtXG(node, phyIndex, packet);

	
	// 设置反向链路（用户->信关）的控制信息
	if (phy_sphy->stationType == FX_STATION_TYPE_YH)
	{
		transmissonFlag = transmissonFlag | PhyFxSetBackwardLinkControlInfoAtYH(node, phyIndex, packet);
				
		if (transmissonFlag)
		{
			txInfo->destRNTI = phy_sphy->establishmentData->selectedRntiXG;
		}
	}

	
	if (!transmissonFlag)
	{
		MESSAGE_Free(node, packet);
		return;
	}
	
	fxRnti* info = (fxRnti*)MESSAGE_ReturnInfo(packet, INFO_TYPE_FxSmacTxInfo);

	if (info != NULL)
	{
		txInfo->destRNTI = *info;
		MESSAGE_RemoveInfo(node, packet, INFO_TYPE_FxSmacTxInfo);
	}


	// release the signal
	PROP_ReleaseSignal(
		node,
		packet,
		phyIndex,
		channelIndex,
		phy_sphy->txPower_dBm,
		duration,
		initDelayUntilAirborne);

	// send the TransmissionEnd message
	Message *endMsg;
	endMsg = MESSAGE_Alloc(node,
		PHY_LAYER,
		PHY_FX,
		MSG_PHY_TransmissionEnd);

	MESSAGE_SetInstanceId(endMsg, (short)phyIndex);
	MESSAGE_Send(node, endMsg, initDelayUntilAirborne + duration + 1);
}


void PhySphyTransmissionEnd(Node *node, int phyIndex)
{
	// 只有上行的传输会进这个函数

	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phy_sphy = (PhyDataFx*)thisPhy->phyVar;
	
	// 暂无处理
}


void PhySphySignalArrivalFromChannel(
	Node* node,
	int phyIndex,
	int channelIndex,
	PropRxInfo *propRxInfo)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phy_sphy = (PhyDataFx*)thisPhy->phyVar;

	// 信关站：处理第一次卫星发过来的的波束信息以知道有多少个用户波束
	BeamChannelMap** info = (BeamChannelMap**)MESSAGE_ReturnInfo(propRxInfo->txMsg, INFO_TYPE_FxPhyVirtualBeamChannelsInfo);
	if (info != NULL)
	{
		assert(phy_sphy->stationType == FX_STATION_TYPE_XG);
		BeamChannelMap* beamChannels = *info;
		
		auto beaminfo = beamChannels->begin();
		while (beaminfo != beamChannels->end())
		{
			phy_sphy->beamChannel->insert(make_pair((beaminfo->first), make_pair(beaminfo->second.first, beaminfo->second.second)));
			
			// 给信关站的layer3添加Beam信息
			int interfaceIndex =
				node->phyData[phyIndex]->macInterfaceIndex;
			Layer3FxAddBeam(node, interfaceIndex, beaminfo->first);

			beaminfo++;
		}
		return;
	}

	
	// 如果本节点是卫星，弯管转发过程
	if (phy_sphy->stationType == FX_STATION_TYPE_SAT)
	{		
		if (phyIndex == node->FLinterfaceIndex)
		{
			// 如果是馈电波束上行信号
			PhyFxBeamInfo* BPInfo = (PhyFxBeamInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg, (unsigned short)INFO_TYPE_FxPhyBeamInfo);
			assert(BPInfo != NULL);

			int forwardPhyIndex = BPInfo->beamIndex;

			PhyData* forwardPhy = node->phyData[forwardPhyIndex];
			PhyDataFx* forwardSphy = (PhyDataFx*)forwardPhy->phyVar;

			Message* newMsg = MESSAGE_Duplicate(node,
				propRxInfo->txMsg);

			PROP_ReleaseSignal(node,
				newMsg,
				forwardPhyIndex,
				forwardSphy->DLchannel.channelIndex,
				phy_sphy->txPower_dBm,
				propRxInfo->duration,
				0);
		}
		else
		{
			// 是用户波束上行信号
			PhyFxBeamInfo* BPInfo = (PhyFxBeamInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg, (unsigned short)INFO_TYPE_FxPhyBeamInfo);
			assert(BPInfo != NULL);
			assert(BPInfo->beamIndex == phyIndex);

			int forwardPhyIndex = node->FLinterfaceIndex;
			PhyData* forwardPhy = node->phyData[forwardPhyIndex];
			PhyDataFx* forwardSphy = (PhyDataFx*)forwardPhy->phyVar;

			Message* newMsg = MESSAGE_Duplicate(node,
				propRxInfo->txMsg);

			PROP_ReleaseSignal(node,
				newMsg,
				forwardPhyIndex,
				forwardSphy->DLchannel.channelIndex,
				phy_sphy->txPower_dBm,
				propRxInfo->duration,
				0);
		}

		// 卫星的处理结束
		return;
	}
	
	assert(phy_sphy->stationType != FX_STATION_TYPE_SAT);

	BOOL isData = TRUE;
	if (MESSAGE_ReturnInfo(propRxInfo->txMsg, (unsigned short)INFO_TYPE_FxSmacNoTransportBlock))
		isData = FALSE;

		PhyFxBeamInfo* BPInfo = (PhyFxBeamInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg, (unsigned short)INFO_TYPE_FxPhyBeamInfo);
		map<int, pair<Message*, BOOL>>::iterator beam = phy_sphy->beamState->find(BPInfo->beamIndex);

	if (!isData)
	{
		PhyFxTxInfo* txInfo = (PhyFxTxInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg, (unsigned short)INFO_TYPE_FxPhyTxInfo);

		// 如果该波束正在接收其他的信号，则发生碰撞，也不需要继续处理这个信号
		if (beam != phy_sphy->beamState->end() && beam->second.first != NULL)
		{
			cout << "on node " << node->nodeId << " collision happen in beam " << BPInfo->beamIndex << endl;
			beam->second.second = FALSE;
			return;
		}
	}

	// calculate the rx power
	double antennaGain = ANTENNA_GainForThisSignal(node,
		phyIndex,
		propRxInfo);
	double rxPowerInOmnimW
		= (double)NON_DB(antennaGain + propRxInfo->rxPower_dBm);

	//gss xd 记录FX的接收机功率 即RSS，该值跟YH与卫星的位置有关
	if (phy_sphy->stationType == FX_STATION_TYPE_YH) {
		//char clockStr[MAX_CLOCK_STRING_LENGTH];
		//ctoa((getSimTime(node) / SECOND), clockStr);
		/*double time = getSimTime(node) / SECOND;
			set<Fx_Rss>s;
			Fx_Rss a;
			a.time = time;
			a.rxPower = rxPowerInOmnimW;
			s.insert(a);
			ofstream outfile("FX_RSS.txt", ofstream::app);
			set<Fx_Rss>::iterator it;
			for (it = s.begin(); it != s.end(); it++) {
				outfile << (*it).time << "  " << (*it).rxPower << endl;
			}
			outfile.close();*/
		double time = getSimTime(node) /SECOND;
		ofstream outfile("FX_RSS.txt", ofstream::app);
		outfile << time << ',' << rxPowerInOmnimW << endl;
		outfile.close();
	}
		
	
	// calculate the interference power
	double rxPowermW(0);
	double interferencePowermW(0);
	PHY_SignalInterference(node,
		phyIndex,
		channelIndex,
		propRxInfo->txMsg,
		&rxPowermW,
		&interferencePowermW);

	// decide if the packet is error
	BOOL isPacketError = PhySphyCheckRxPacketError(node, phy_sphy, rxPowermW, interferencePowermW);

	if (isPacketError)
	{
		PHY_NotificationOfPacketDrop(
			node,
			phyIndex,
			channelIndex,
			propRxInfo->txMsg,
			"Header In Error",
			rxPowermW,
			interferencePowermW,
			propRxInfo->pathloss_dB);

	}

	if (!isData)
	{
		if (beam == phy_sphy->beamState->end())
		{
			phy_sphy->beamState->insert(make_pair(BPInfo->beamIndex, make_pair(propRxInfo->txMsg, !isPacketError)));
		}
		else
		{
			beam->second.first = propRxInfo->txMsg;
			beam->second.second = !isPacketError;
		}
	}
	
	if (phy_sphy->stationType == FX_STATION_TYPE_YH)
	{
		PhyFxTxInfo* txInfo = (PhyFxTxInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg, (unsigned short)INFO_TYPE_FxPhyTxInfo);
		phy_sphy->propagationDelay = getSimTime(node) - txInfo->Txtime;
		double temp = phy_sphy->propagationDelay / (double)SECOND;
		phy_sphy->lastSignalArrival = getSimTime(node);
	}
}


void PhySphySignalEndFromChannel(
	Node* node,
	int phyIndex,
	int channelIndex,
	PropRxInfo *propRxInfo)
{

	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phy_sphy = (PhyDataFx*)thisPhy->phyVar;
	
	if (phy_sphy->stationType == FX_STATION_TYPE_SAT)
	{
		return;		
	}

	// temp
	BeamChannelMap** info = (BeamChannelMap**)MESSAGE_ReturnInfo(propRxInfo->txMsg, INFO_TYPE_FxPhyVirtualBeamChannelsInfo);
	if (info != NULL)
	{
		return;
	}

	assert(phy_sphy->stationType != FX_STATION_TYPE_SAT);


	// 只有控制包的信号暂不上传给SMAC
	BOOL isData = TRUE;
	if (MESSAGE_ReturnInfo(propRxInfo->txMsg, (unsigned short)INFO_TYPE_FxSmacNoTransportBlock))
		isData = FALSE;

	
	PhyFxBeamInfo* BPInfo = (PhyFxBeamInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg, (unsigned short)INFO_TYPE_FxPhyBeamInfo);
	map<int, pair<Message*, BOOL>>::iterator beam = phy_sphy->beamState->find(BPInfo->beamIndex);

	if (!isData)
	{
		// 如果该信号不是正在接收的信号，返回
		if (propRxInfo->txMsg != beam->second.first)
		{
			return;
		}
		// 如果该信号发生错误，返回
		else if (!beam->second.second)
		{
			cout << "error happen to this signal" << endl;
			beam->second.first = NULL;
			return;
		}
	}

	PhyFxTxInfo* txInfo = (PhyFxTxInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg, (unsigned short)INFO_TYPE_FxPhyTxInfo);
	fxRnti myRnti = fxRnti(
		node->nodeId,
		node->phyData[phyIndex]->macInterfaceIndex);

	if (txInfo->destRNTI != FX_INVALID_RNTI && txInfo->destRNTI != myRnti)
	{
		if (!isData)beam->second.first = NULL;
		return;
	}		

	// 处理控制消息
	PhySphyHandleMessageControlInfo(node, phyIndex, propRxInfo);	



	if (phy_sphy->stationType != FX_STATION_TYPE_SAT && isData)
	{
		// send the packet to up layer
		Message* newMsg = MESSAGE_Duplicate(node, propRxInfo->txMsg);
		MESSAGE_SetInstanceId(newMsg, (short)phyIndex);

		MESSAGE_AppendInfo(node,
			newMsg,
			sizeof(PhyFxPhyToMacInfo),
			(unsigned short)INFO_TYPE_FxPhyFxPhyToMacInfo);

		PhyFxPhyToMacInfo* info =
			(PhyFxPhyToMacInfo*)MESSAGE_ReturnInfo(
				newMsg,
				(unsigned short)INFO_TYPE_FxPhyFxPhyToMacInfo);

		info->srcRnti = txInfo->sourceRNTI;

		MAC_ReceivePacketFromPhy(node,
			node->phyData[phyIndex]->macInterfaceIndex,
			newMsg);
	}

	beam->second.first = NULL;
}

void PhySphyFinalize(Node *node, const int phyIndex)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;
	
	delete phyFx->establishmentData;
	delete phyFx->raGrants;
	delete phyFx->RRCsetups;
	delete phyFx->beamState;
	delete phyFx->nextBeam;
	delete phyFx->rrcReconfs;
	delete phyFx->beamChannel;

	return;
}




void PhyFxNotifyPowerOn(Node* node, const int phyIndex)
{

	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	if (phyFx->stationType == FX_STATION_TYPE_YH)
	{
		phyFx->phyFxState = PHY_FX_IDLE_CELL_SELECTION;
	}

	return;
}

void FxPhySetBeamChannels(Node * node, const int phyIndex)
{
	PhyDataFx* phyFx = (PhyDataFx*)node->phyData[phyIndex]->phyVar;

	int i = 0;
	PhyData* itPhy = node->phyData[i];
	while (itPhy)
	{
		if (i == phyIndex)
		{
			itPhy = node->phyData[++i];
			continue;
		}

		PhyDataFx* ItphyFx = (PhyDataFx*)node->phyData[i]->phyVar;
		phyFx->beamChannel->insert(make_pair(i, make_pair(ItphyFx->ULchannel.channelIndex, ItphyFx->DLchannel.channelIndex)));
		
		itPhy = node->phyData[++i];			
	}

	assert(phyFx->stationType == FX_STATION_TYPE_SAT);

	Message* sendMsg = NULL;
	sendMsg = MESSAGE_Alloc(node, 0, 0, 0);
	BeamChannelMap** info = (BeamChannelMap**)MESSAGE_AddInfo(
		node, sendMsg, sizeof(BeamChannelMap*), INFO_TYPE_FxPhyVirtualBeamChannelsInfo);
	*info = phyFx->beamChannel;

	PhySphyStartTransmittingSignal(node, phyIndex, sendMsg, FALSE, 0);

}

clocktype PhyFxGetPropagationDelay(Node* node, const int phyIndex)
{
	PhyDataFx* phyFx = (PhyDataFx*)node->phyData[phyIndex]->phyVar;

	return phyFx->propagationDelay;
}

clocktype PhyFxGetLastSignalArrivalTime(Node* node, const int phyIndex)
{
	PhyDataFx* phyFx = (PhyDataFx*)node->phyData[phyIndex]->phyVar;
	return phyFx->lastSignalArrival;
}

void PhyFxSetRAREQcount(Node* node, const int phyIndex, UInt8 V)
{
	PhyDataFx* phyFx = (PhyDataFx*)node->phyData[phyIndex]->phyVar;

	phyFx->RAREQcount = V;
}



void PhyFxRaGrantTransmissionIndicationAtXG(Node * node, int phyIndex, fxRnti ueRnti)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	phyFx->raGrants->insert(ueRnti);

}

void PhyFxRrcConnectedNotification(Node * node, int phyIndex)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	phyFx->rrcSetupCompleteflag = TRUE;
}

void PhyFxRrcHandoverRequestNotificationAtYH(Node * node, int phyIndex, UInt8 nextBeamIndex)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	phyFx->nextBeam->insert(nextBeamIndex);
}

void PhyFxRrcReconfCompleteNotificationAtYH(Node * node, int phyIndex)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	phyFx->rrcReconfCompleteFlag = TRUE;
}


void PhyFxRrcConnectionReconfIndicationAtXG(Node * node, int phyIndex, UInt8 yhNodeId, UInt8 beam)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	phyFx->rrcReconfs->insert(make_pair(yhNodeId, beam));
}


void PhyFxSetTransblockSize(Node * node, int phyIndex, UInt16 size)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	phyFx->TBsize = size;
}


UInt16 PhyFxGetTransblockSize(Node * node, int phyIndex)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	return phyFx->TBsize;
}



void PhyFxStartXGSearchAtHY(Node* node, const int phyIndex)
{
	
	return;
}
//Added by WLH
void PhyFxIndicateDisconnectToUe(
	Node* node,
	int phyIndex,
	const fxRnti& ueRnti)
{
	PhyData* thisPhy = node->phyData[phyIndex];
	PhyDataFx* phyFx = (PhyDataFx*)thisPhy->phyVar;

	ERROR_Assert(phyFx->stationType != FX_STATION_TYPE_YH,
		"PhyFxIndicateDisconnectToUe is called only eNB.");

	std::map < fxRnti, PhyFxConnectedUeInfo > ::iterator itr;
	itr = phyFx->yhInfoList->find(ueRnti);
	if (itr != phyFx->yhInfoList->end())
	{
		phyFx->yhInfoList->erase(itr);
	}
}

//Added by WLH
BOOL PhyFxGetMessageSrsInfo(
	Node* node, int phyIndex, Message* msg, PhyFxSrsInfo* srs)
{


	PhyFxSrsInfo* info =
		(PhyFxSrsInfo*)MESSAGE_ReturnInfo(msg,
		(unsigned short)INFO_TYPE_FxPhySrsInfo);
	if (info != NULL)
	{
		memcpy(srs, info, sizeof(PhyFxSrsInfo));
		return TRUE;
	}
	return FALSE;
}
