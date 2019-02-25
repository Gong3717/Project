#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <list>

#include "api.h"
#include "network_ip.h"

#include "phy_fx.h"
#include "layer2_fx.h"

//Gss
static void SmacFxDoMultiplexing(
	Node* node, int interfaceIndex, const fxRnti& oppositeRnti,
	int bearerId, int tbSize,
	std::list < Message* > &sduList, Message** macPdu,
	UInt32* withoutPaddingByte);

static void SmacFxDoDemultiplexing(
	Node* node, int interfaceIndex,
	Message* macPdu, std::list < Message* > &sduList);
//

void SmacFxSetState(Node* node, int interfaceIndex, SmacFxState state)
{
	FxSmacData* smacData = FxLayer2GetFxSmacData(node, interfaceIndex);
	smacData->smacState = state;
}


void SmacFxSetTimerForMac(Node* node, int interfaceIndex, int eventType, clocktype delay)
{
	FxSmacData* smacData = FxLayer2GetFxSmacData(node, interfaceIndex);

	Message* timerMsg = MESSAGE_Alloc(node, MAC_LAYER, MAC_PROTOCOL_FX, eventType);
	
	MESSAGE_Send(node, timerMsg, delay);
}




void SmacFxAddDestinationInfo(Node* node,
	Int32 interfaceIndex,
	Message* msg,
	const fxRnti& oppositeRnti)
{
	SmacFxMsgDestinationInfo* info =
		(SmacFxMsgDestinationInfo*)MESSAGE_AddInfo(
			node,
			msg,
			sizeof(SmacFxMsgDestinationInfo),
			(UInt16)INFO_TYPE_FxSmacDestinationInfo);
	ERROR_Assert(info != NULL, "");
	info->dstRnti = oppositeRnti;
}

int FxGetPhyIndexFromMacInterfaceIndex(Node* node, int interfaceIndex)
{
	return node->macData[interfaceIndex]->phyNumber;
}

UInt64 SmacFxGetSlotNum(Node * node, int interfaceIndex)
{
	return FxLayer2GetFxSmacData(node, interfaceIndex)->currentSlotNum;
}

UInt64 SmacFxGetFrameNum(Node * node, int interfaceIndex)
{
	return FxLayer2GetFxSmacData(node, interfaceIndex)->currentFrameNum;
}

void SmacFxStartRandomAccessAtYH(Node * node, int interfaceIndex)
{
	int phyIndex =
		FxGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

	FxSmacData* smacData = FxLayer2GetFxSmacData(node, interfaceIndex);

	SmacFxSetState(node, interfaceIndex, SMAC_FX_RA_RSP_WAITING);

	// 根据SMAC文档7.2，一开始就退避
	// V = B + D * N
	int nRand = RANDOM_nrand(smacData->seed);
	int V = 4 + 4*smacData->RA_N; 
	
	smacData->RA_N=2;

	// V个时隙后发送随机接入报文
	PhyFxSetRAREQcount(node, phyIndex, nRand%V);

	SmacFxSetTimerForMac(node, interfaceIndex, MSG_MAC_FX_RaResponseWaitingTimerExpired, SMAC_FX_DEFAULT_RA_BACKOFF_TIME);

}


void SmacFxNotifyReceivedRaPreambleAtXG(Node * node, Int32 interfaceIndex, const fxRnti & hyRnti)
{

	int phyIndex =
		FxGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

	FxConnectionInfo* connectionInfo = Layer3FxGetConnectionInfo(node, interfaceIndex, hyRnti);

	if (connectionInfo) return;

	PhyFxRaGrantTransmissionIndicationAtXG(node, phyIndex, hyRnti);

	RrcFxNotifyNetworkEntryFromYH(node, interfaceIndex, hyRnti);
}



void SmacFxSetNextTtiTimer(Node* node, Int32 interfaceIndex)
{

	FxSmacData* smacData = FxLayer2GetFxSmacData(node, interfaceIndex);


	FxStationType stationType = FxLayer2GetStationType(node, interfaceIndex);
	clocktype ttiLength = 0;
	int phyIndex =
		FxGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

	switch (stationType)
	{
	case FX_STATION_TYPE_XG:
	{
		ttiLength = SMAC_FX_DEFAULT_SLOT_DURATION;
		break;
	}
	case FX_STATION_TYPE_YH:
	{
		// 由用户站调整Tx时间去同步信关站的时序，这样才能保证多个用户站的信号能够同时到达信关站
		clocktype propDelay = PhyFxGetPropagationDelay(node, phyIndex);

		if (//(smacData->smacState == SMAC_FX_RA_RSP_WAITING ||
			//smacData->smacState == SMAC_FX_RA_BACKOFF_WAITING) &&
			smacData->lastPropDelay != propDelay)
		{
			clocktype lastSignalArrival = PhyFxGetLastSignalArrivalTime(node, phyIndex);
			clocktype backwardArrivalLimit = getSimTime(node) + propDelay;

			UInt8 arrivalFrame = backwardArrivalLimit % SECOND  / SMAC_FX_DEFAULT_FRAME_DURATION;
			UInt8 arrivalSlot = backwardArrivalLimit % SECOND % SMAC_FX_DEFAULT_FRAME_DURATION / SMAC_FX_DEFAULT_SLOT_DURATION;

			smacData->currentSlotNum = arrivalSlot;
			smacData->currentFrameNum = arrivalFrame;

			ttiLength = SMAC_FX_DEFAULT_SLOT_DURATION - (2 * propDelay) % SMAC_FX_DEFAULT_SLOT_DURATION;
			ttiLength -= getSimTime(node) - lastSignalArrival;
			if (ttiLength < 0) ttiLength += SMAC_FX_DEFAULT_SLOT_DURATION;
			smacData->lastPropDelay = propDelay;
		}
		else
		{
			ttiLength = SMAC_FX_DEFAULT_SLOT_DURATION;
		}
		break;
	}

	default:
	{
		char errBuf[MAX_STRING_LENGTH] = { 0 };
		sprintf(errBuf,
			"FX do not support FxStationType(%d)\n"
			"Please check FxStationType.\n"
			, stationType);
		ERROR_ReportError(errBuf);
		break;
	}			}
	

	// 维护帧号和时隙号
	smacData->currentSlotNum++;
	if (smacData->currentSlotNum == 32)
	{
		smacData->currentSlotNum = 0;
		smacData->currentFrameNum++;
		ttiLength += SMAC_FX_DEFAULT_INTER_FRAME_DURATION;

		if (smacData->currentFrameNum == 10)
		{
			smacData->currentFrameNum = 0;
			smacData->currentSuperFrameNum++;
		}
	}


	// 发送下一个时隙的定时器
	Message* timerMsg =
		MESSAGE_Alloc(
			node, MAC_LAYER, MAC_PROTOCOL_FX, MSG_MAC_FX_TtiTimerExpired);
	MESSAGE_SetInstanceId(timerMsg, interfaceIndex);
	MESSAGE_Send(node, timerMsg, ttiLength);



}



void SmacFxNotifyReceivedRaGrantAtYH(Node * node, Int32 interfaceIndex)
{
	FxSmacData* smacData = FxLayer2GetFxSmacData(node, interfaceIndex);

	fxRnti xgRnti = Layer3FxGetRandomAccessingXG(node, interfaceIndex);
	if (xgRnti != FX_INVALID_RNTI)
	{
		// 用户站收到RARSP，随机接入完成
		RrcFxNotifyRandomAccessResultsAtYH(node, interfaceIndex, TRUE, xgRnti);
		SmacFxSetState(node, interfaceIndex, SMAC_FX_DEFAULT_STATUS);					
	}

	smacData->RA_N = 0;
}


void SmacFxNotifyReceivedRrcSetupAtYH(Node * node, Int32 interfaceIndex)
{

	fxRnti xgRnti = Layer3FxGetRandomAccessingXG(node, interfaceIndex);
	if (xgRnti != FX_INVALID_RNTI)
	{
		// temp
		Layer3FxProcessRrcConnected(node, interfaceIndex, xgRnti);
	}
}


void SmacFxReceiveTransportBlockFromPhy(Node * node, int interfaceIndex, fxRnti srcRnti, Message * msg)
{
	FxSmacData* smacData = FxLayer2GetFxSmacData(node, interfaceIndex);

	if (node->guiOption)
	{
		GUI_Receive(
			srcRnti.nodeId,
			node->nodeId,
			GUI_PHY_LAYER,
			GUI_DEFAULT_DATA_TYPE,
			srcRnti.interfaceIndex,
			interfaceIndex,
			getSimTime(node));
	}

	//int tbs = MESSAGE_ReturnPacketSize(transportBlockMsg);
	//ERROR_Assert(tbs > 0, "Size of transport block is 0.");
	std::list<Message*> sduList;
	SmacFxDoDemultiplexing(node, interfaceIndex, msg, sduList);
	
	//smacData->statData.numberOfSduToUpperLayer += sduList.size();
	//MESSAGE_RemoveInfo(node, msg, INFO_TYPE_FxPhyFxPhyToMacInfo);
	/*Message* newMsg = NULL;
	SmacFxListToMsg(node, sduList, newMsg);*/

	sduList.reverse();
	auto sdu = sduList.begin();
	while (sdu != sduList.end())
	{
		// slc和smac传递的应该总是一个msg的list，而不是单个msg
		SlcFxReceivePduFromSmac(node, interfaceIndex, srcRnti, FX_DEFAULT_BEARER_ID, *sdu);
		sdu++;
	}

}


void SmacFxInit(Node* node,
	UInt32 interfaceIndex,
	const NodeInput* nodeInput)
{
	FxSmacData* smacData = FxLayer2GetFxSmacData(node, interfaceIndex);

	// random seed
	RANDOM_SetSeed(smacData->seed,
		node->globalSeed,
		node->nodeId,
		MAC_PROTOCOL_FX,
		interfaceIndex);

	// init state variables
	smacData->currentSuperFrameNum = 0;
	smacData->currentFrameNum = 0;
	smacData->currentSlotNum = 0;

	smacData->preambleTransmissionCounter = 1;
	smacData->lastPropDelay = 0;
	
	smacData->smacState = SMAC_FX_POWER_OFF;

	smacData->RA_N = 0;

	FxStationType stationType = FxLayer2GetStationType(node, interfaceIndex);

	// 初始化时只给信关站set timer
	if (stationType == FX_STATION_TYPE_XG)
	{
		Message* timerMsg =
			MESSAGE_Alloc(
				node, MAC_LAYER, MAC_PROTOCOL_FX, MSG_MAC_FX_TtiTimerExpired);
		MESSAGE_SetInstanceId(timerMsg, interfaceIndex);
		MESSAGE_Send(node, timerMsg, SMAC_FX_DEFAULT_INTER_FRAME_DURATION);
	}
}

void SmacFxFinalize(Node* node, UInt32 interfaceIndex)
{
	FxSmacData* smacData = FxLayer2GetFxSmacData(node, interfaceIndex);
	delete smacData;
}


void SmacFxStartSchedulingAtXG(
	Node* node, UInt32 interfaceIndex, Message* msg)
{
	FxSmacData* smacData = FxLayer2GetFxSmacData(node, interfaceIndex);
	FxStationType stationType = FxLayer2GetStationType(node, interfaceIndex);
	int phyIndex =
		FxGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

	assert(stationType == FX_STATION_TYPE_XG);


	if (smacData->smacState != SMAC_FX_POWER_OFF)
	{
		// 判断slc有没有包要发
		BOOL dataflag = FALSE;

		// 获取所有波束，beams的成员是波束号
		vector<int> beams;
		Layer3FxGetMultiBeamIndexVec(node, interfaceIndex, beams);
				
		auto beam = beams.begin();

		int tbSize = PhyFxGetTransblockSize(node, phyIndex);
		
		while (beam != beams.end())
		{
			// 获得所有用户
			vecFxRnti destList;
			Layer3FxGetSchedulableListSortedByConnectedTime(node, interfaceIndex,
				&destList, *beam);

			// 如果存在任何用户
			if (destList.size() > 0)
			{
				// 如果该波束下有用户
				//gss
				Message* listMsg = NULL; // send to PHY
				Message* curMsg = NULL;

				int yhNums = destList.size();				
				int roundRobin = smacData->currentSlotNum % yhNums;
				
				std::list < Message* > sduList;
				
				// 目前一个时隙只发一个用户(desRnti)的包
				const fxRnti destRnti = *(destList.begin() + roundRobin);
				
				SlcFxDeliverPduToSmac(node, interfaceIndex, destRnti, &sduList, tbSize);
				
				// 如果当前有可发的包
				if (sduList.size() > 0)
				{
					// Do multiplexing from MAC SDUs to MAC PDU
					Message* macPdu = NULL;
					UInt32 withoutPaddingByte = 0;
					SmacFxDoMultiplexing(
						node, interfaceIndex, destRnti,
						*beam,
						tbSize, sduList, &macPdu,
						&withoutPaddingByte);

					// Add Destination Info
					SmacFxAddDestinationInfo(
						node, interfaceIndex, macPdu, destRnti);

					 //Add send message list	
					if (listMsg == NULL)
					{
						listMsg = macPdu;
						curMsg = macPdu;
					}
					else
					{
						curMsg->next = macPdu;
						curMsg = macPdu;
					}

					Message* sendMsg = listMsg;

					fxRnti* info = (fxRnti*)MESSAGE_AddInfo(node, sendMsg, sizeof(fxRnti), INFO_TYPE_FxSmacTxInfo);
					*info = destRnti;

					PhyFxBeamInfo* BPInfo = (PhyFxBeamInfo*)MESSAGE_AppendInfo(node, sendMsg, 
						sizeof(PhyFxBeamInfo), (unsigned short)INFO_TYPE_FxPhyBeamInfo);
					BPInfo->beamIndex = *beam;

					PHY_StartTransmittingSignal(
						node, phyIndex, sendMsg,
						FALSE, 0);

					dataflag = TRUE;
				}				
			}

			if (!dataflag)
			{
				Message* sendMsg = NULL;
				sendMsg = MESSAGE_Alloc(node, 0, 0, 0);
				MESSAGE_AddInfo(
					node, sendMsg, 1, INFO_TYPE_FxSmacNoTransportBlock);

				PhyFxBeamInfo* BeamInfo = (PhyFxBeamInfo*)MESSAGE_AppendInfo(node, sendMsg,
					sizeof(PhyFxBeamInfo), (unsigned short)INFO_TYPE_FxPhyBeamInfo);
				BeamInfo->beamIndex = *beam;

				PHY_StartTransmittingSignal(node, phyIndex, sendMsg, FALSE, 0);
			}
			beam++;
		}		
	}

	SmacFxSetNextTtiTimer(node, interfaceIndex);

}

void SmacFxStartSchedulingAtYH(
	Node* node, UInt32 interfaceIndex, Message* msg)
{
	FxSmacData* smacData = FxLayer2GetFxSmacData(node, interfaceIndex);
	FxStationType stationType = FxLayer2GetStationType(node, interfaceIndex);
	int phyIndex =
		FxGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

	assert(stationType == FX_STATION_TYPE_YH);

	int tbSize = PhyFxGetTransblockSize(node, phyIndex);

	if (smacData->smacState != SMAC_FX_POWER_OFF)
	{
		// 判断slc有没有包要发

		// 获取所有波束
		vector<int> beams;
		Layer3FxGetMultiBeamIndexVec(node, interfaceIndex, beams);

		// 用户上应该只有一个波束的信息
		assert(beams.size() == 1);

		vector<int>::iterator beam = beams.begin();
		
		BOOL dataflag = FALSE;

		vecFxRnti destList;
		Layer3FxGetSchedulableListSortedByConnectedTime(node, interfaceIndex,
			&destList, *beam);

		if (destList.size() > 0)
		{
			// 这个波束下应该只有一个节点，就是信关站
			assert(destList.size() == 1);

			//gss
			Message* listMsg = NULL; // send to PHY
			Message* curMsg = NULL;

			std::list < Message* > sduList;

			const fxRnti xgRnti = *destList.begin();

			SlcFxDeliverPduToSmac(node, interfaceIndex, xgRnti, &sduList, tbSize);


			if (sduList.size() > 0)
			{

				// Do multiplexing from MAC SDUs to MAC PDU
				Message* macPdu = NULL;
				UInt32 withoutPaddingByte = 0;
				SmacFxDoMultiplexing(
					node, interfaceIndex, xgRnti,
					*beam,
					tbSize, sduList, &macPdu,
					&withoutPaddingByte);

				// Add Destination Info
				SmacFxAddDestinationInfo(
					node, interfaceIndex, macPdu, xgRnti);

				//Add send message list	
				if (listMsg == NULL)
				{
					listMsg = macPdu;
					curMsg = macPdu;
				}
				else
				{
					curMsg->next = macPdu;
					curMsg = macPdu;
				}

				Message* sendMsg = listMsg;

				fxRnti* info = (fxRnti*)MESSAGE_AddInfo(node, sendMsg, sizeof(fxRnti), INFO_TYPE_FxSmacTxInfo);
				*info = xgRnti;

				PhyFxBeamInfo* BPInfo = (PhyFxBeamInfo*)MESSAGE_AppendInfo(node, sendMsg,
					sizeof(PhyFxBeamInfo), (unsigned short)INFO_TYPE_FxPhyBeamInfo);
				BPInfo->beamIndex = *beam;

				PHY_StartTransmittingSignal(
					node, phyIndex, sendMsg,
					FALSE, 0);

				dataflag = TRUE;
			}
		}
		
		// 如果slc没有数据包下来
		if (!dataflag)
		{
			Message* sendMsg = NULL;
			sendMsg = MESSAGE_Alloc(node, 0, 0, 0);

			PhyFxBeamInfo* BeamInfo = (PhyFxBeamInfo*)MESSAGE_AppendInfo(node, sendMsg,
				sizeof(PhyFxBeamInfo), (unsigned short)INFO_TYPE_FxPhyBeamInfo);
			BeamInfo->beamIndex = *beam;

			MESSAGE_AddInfo(
				node, sendMsg, 1, INFO_TYPE_FxSmacNoTransportBlock);
			PHY_StartTransmittingSignal(node, phyIndex, sendMsg, FALSE, 0);
		}
	}

	SmacFxSetNextTtiTimer(node, interfaceIndex);
}



void SmacFxNotifyPowerOn(Node* node, int interfaceIndex)
{
	
	FxStationType stationType = FxLayer2GetStationType(node, interfaceIndex);

	switch (stationType)
	{
	case FX_STATION_TYPE_XG:
	{
		SmacFxSetState(node, interfaceIndex, SMAC_FX_DEFAULT_STATUS);
		break;
	}
	case FX_STATION_TYPE_YH:
	{
		SmacFxSetState(node, interfaceIndex, SMAC_FX_IDEL);
		break;
	}
	default:
		break;
	}
}

void SmacFxNotifyPowerOff(Node* node, int interfaceIndex)
{
	FxSmacData* smacData = FxLayer2GetFxSmacData(node, interfaceIndex);
}



void SmacFxProcessEvent(Node* node, UInt32 interfaceIndex, Message* msg)
{
	FxSmacData* smacData = FxLayer2GetFxSmacData(node, interfaceIndex);

	switch (msg->eventType)
	{
	case MSG_MAC_FX_TtiTimerExpired:
	{
		FxStationType stationType = FxLayer2GetStationType(node, interfaceIndex);
		

		switch (stationType)
		{
		case FX_STATION_TYPE_XG:
		{
			SmacFxStartSchedulingAtXG(node, interfaceIndex, msg);
			break;
		}
		case FX_STATION_TYPE_YH:
		{
			SmacFxStartSchedulingAtYH(node, interfaceIndex, msg);
			break;
		}
		default:
		{
			char errBuf[MAX_STRING_LENGTH] = { 0 };
			sprintf(errBuf,
				"FX do not support FxStationType(%d)\n"
				"Please check FxStationType.\n"
				, stationType);
			ERROR_ReportError(errBuf);
			break;
		}
		}
		break;
	}
	case MSG_MAC_FX_RaResponseWaitingTimerExpired:
	{
		FxStationType stationType = FxLayer2GetStationType(node, interfaceIndex);

		assert(stationType == FX_STATION_TYPE_YH);

		if (smacData->smacState == SMAC_FX_RA_RSP_WAITING)
		{
			int phyIndex =
				FxGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

			int nRand = RANDOM_nrand(smacData->seed);
			int V = 4 + 4 * smacData->RA_N;
			smacData->RA_N *= 2;

			PhyFxSetRAREQcount(node, phyIndex, nRand%V);

			MESSAGE_Send(node, msg, SMAC_FX_DEFAULT_RA_BACKOFF_TIME);
			break;
		}

		MESSAGE_Free(node, msg);

		break;
	}
	default:
		break;
	}
}

//Gss
// /**
// FUNCTION   :: MacLteDoMultiplexing
// LAYER      :: MAC
// PURPOSE    :: Multiplexing from MAC SDU to MAC PDU
// PARAMETERS ::
// + node           : IN  : Node*                : Pointer to node.
// + interfaceIndex : IN  : int                  : Interface index
// + tbSize         : IN  : int                  : Transport Block Size
// + sduList        : IN  : std::list<Message*>& : List of MAC SDUs
// + msg            : OUT : Message*             : MAC PDU
// RETURN     :: void : NULL
// **/
static void SmacFxDoMultiplexing(
	Node* node, int interfaceIndex, const fxRnti& oppositeRnti,
	int bearerId, int tbSize,
	std::list < Message* > &sduList, Message** macPdu,
	UInt32* withoutPaddingByte)
{
	Message* msgList = NULL;
	if (sduList.size() > 0)
	{
		// for each SDUs
		std::list < Message* > ::iterator itr;
		for (itr = sduList.begin(); 
			itr != sduList.end(); 
			++itr)
		{
			Message* curSdu = *itr;
			UInt16 curSduSize = 0;
			curSduSize = (UInt16)MESSAGE_ReturnPacketSize(curSdu);

			if (msgList == NULL)
			{
				msgList = curSdu;
			}

			//// Add MAC Subheader to MAC SDU
			FxSmacRrelcidflWith15bitSubHeader* subHeader =
				(FxSmacRrelcidflWith15bitSubHeader*)MESSAGE_AddInfo(
					node,
					curSdu,
					sizeof(FxSmacRrelcidflWith15bitSubHeader),
					INFO_TYPE_FxSmacRRELCIDFLWith15bitSubHeader);
			subHeader->e = SMAC_FX_DEFAULT_E_FIELD;
			subHeader->lcid = SMAC_FX_DEFAULT_LCID_FIELD;
			subHeader->f = SMAC_FX_DEFAULT_F_FIELD;
			subHeader->l = curSduSize;

			// add virtualpayloadsize to mac pdu (mac subheader + mac sdu)
			MESSAGE_AddVirtualPayload(
				node, curSdu, SMAC_FX_RRELCIDFL_WITH_15BIT_SUBHEADER_SIZE);
			std::list<Message*>::iterator tmpItr = itr;
			if (++tmpItr != sduList.end())
			{
				curSdu->next = *tmpItr;
			}
			else
			{
				curSdu->next = NULL;
			}
			*withoutPaddingByte +=
				curSduSize + SMAC_FX_RRELCIDFL_WITH_15BIT_SUBHEADER_SIZE;
		}

		// xyt
		ERROR_Assert(*withoutPaddingByte <= tbSize - 4,
			"SMAC : packets' size larger than transportblock size.");

		// serialize & padding
		std::string buf;
		int numMsg = MESSAGE_SerializeMsgList(node, msgList, buf);
		msgList = NULL;
		*macPdu = MESSAGE_Alloc(node, 0, 0, 0);

		MESSAGE_AddVirtualPayload(node, (*macPdu), (*withoutPaddingByte)+4);

		char* multiplexingInfo =
			MESSAGE_AddInfo(node, *macPdu,
				buf.size(), INFO_TYPE_FxSmacMultiplexingMsg);
		memcpy(multiplexingInfo, buf.data(), buf.size());

		// number of sdu
		int* numSdu = (int*)MESSAGE_AddInfo(
			                    node,
			                    (*macPdu),
		                      	sizeof(int),
			                    INFO_TYPE_FxSmacNumMultiplexingMsg);
		*numSdu = numMsg;
	}
}

// /**
// FUNCTION   :: MacLteDoDemultiplexing
// LAYER      :: MAC
// PURPOSE    :: Multiplexing from MAC SDU to MAC PDU
// PARAMETERS ::
// + node           : IN  : Node*                : Pointer to node.
// + interfaceIndex : IN  : int                  : Interface index
// + msg            : IN  : Message*             : MAC PDU
// + sduList        : OUT : std::list<Message*>& : List of MAC SDUs
// RETURN     :: void : NULL
// **/
static void SmacFxDoDemultiplexing(
	Node* node, int interfaceIndex,
	Message* macPdu, std::list < Message* > &sduList)
{
	sduList.clear();
	char* retInfo = NULL;

	// num msg
	retInfo =
		MESSAGE_ReturnInfo(macPdu, INFO_TYPE_FxSmacNumMultiplexingMsg);
	ERROR_Assert(retInfo != NULL,
		"INFO_TYPE_FxSmacNumMultiplexingMsg is not found.");
	int numMsg = 0;
	memcpy(&numMsg, retInfo, sizeof(int));

	// serialized msg
	retInfo = MESSAGE_ReturnInfo(macPdu, INFO_TYPE_FxSmacMultiplexingMsg);
	ERROR_Assert(retInfo != NULL,
		"INFO_TYPE_FxSmacMultiplexingMsg is not found.");

	// unserialize
	int bufIndex = 0;
	Message* msgList =
		MESSAGE_UnserializeMsgList(node->partitionData, retInfo, bufIndex, numMsg);

	Message* curMsg = msgList;
	while (curMsg != NULL)
	{
		MESSAGE_RemoveInfo(node, curMsg,
			INFO_TYPE_FxSmacRRELCIDFLWith15bitSubHeader);
		MESSAGE_RemoveVirtualPayload(node, curMsg,
			SMAC_FX_RRELCIDFL_WITH_15BIT_SUBHEADER_SIZE);
		sduList.push_back(curMsg);
		curMsg = curMsg->next;
	}
	MESSAGE_Free(node, macPdu);
}