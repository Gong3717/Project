#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <list>

#include "api.h"
#include "layer2_fx_sch.h"
#include "layer3_fx_filtering.h"


#define ENABLE_SCH_IF_CHECKING_LOG 0
#define FX_UMTS_RLC_DEF_PDUSIZE 3

FxScheduler::FxScheduler()
{
	_node = NULL;
	_interfaceIndex = -1;
	_phyIndex = -1;
	_nodeInput = NULL;
	_ttiNumber = 0;

	// Error if default constructor called.
	char errorStr[MAX_STRING_LENGTH];
	sprintf(errorStr,
		"SCH-FX: Invalid FxScheduler initializing");
	ERROR_Assert(FALSE, errorStr);
}

FxScheduler::FxScheduler(
	Node* node, int interfaceIndex, const NodeInput* nodeInput)
{
	_node = node;
	_interfaceIndex = interfaceIndex;
	_phyIndex =
		FxGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);
	_nodeInput = nodeInput;
	_ttiNumber = 0;

	initConfigurableParameters();
}

void FxScheduler::init()
{
	_phyIndex = FxGetPhyIndexFromMacInterfaceIndex(_node, _interfaceIndex);
}

void FxScheduler::initConfigurableParameters()
{
}

void FxScheduler::prepareForScheduleTti(UInt64 ttiNumber)
{
	_ttiNumber = ttiNumber;
}

void FxScheduler::notifyPowerOn()
{
}

void FxScheduler::notifyPowerOff()
{
}

void FxScheduler::notifyAttach(const fxRnti& rnti) {

}

void FxScheduler::notifyDetach(const fxRnti& rnti) {

}

int FxScheduler::determineDequeueSize(int transportBlockSize)
{
	return transportBlockSize / 8;
}

FxScheduler::~FxScheduler()
{
}

FxSchedulerXG::FxSchedulerXG(
	Node* node, int interfaceIndex, const NodeInput* nodeInput)
	: FxScheduler(node, interfaceIndex, nodeInput)
{
	initConfigurableParameters();
}

void FxSchedulerXG::initConfigurableParameters()
{
	// Load parameters for LteSchedulerENB

	BOOL wasFound;
	double retDouble;
	char errBuf[MAX_STRING_LENGTH] = { 0 };

	NodeAddress interfaceAddress =
		MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
			_node, _node->nodeId, _interfaceIndex);
	IO_ReadDouble(_node->nodeId,
		interfaceAddress,
		_nodeInput,
		"MAC-LTE-TARGET-BLER",
		&wasFound,
		&retDouble);

	if (wasFound == TRUE)
	{
		if (retDouble < 0.0 || retDouble > 1.0)
		{
			sprintf(errBuf,
				"Invalid Target BLER %e. "
				"Should be in the range of [0.0, 1.0].", retDouble);

			ERROR_Assert(FALSE, errBuf);
		}

		_targetBler = retDouble;
	}
	else
	{
		_targetBler = SMAC_FX_DEFAULT_TARGET_BLER;
	}
}

FxSchedulerXG::~FxSchedulerXG()
{
}

void FxSchedulerXG::purgeInvalidSchedulingResults(
	std::vector < FxDlSchedulingResultInfo > &schedulingResult)
{
	// Purge scheduling results which dequeueSize equals to 0

	std::vector < FxDlSchedulingResultInfo > ::iterator it;
	for (it = schedulingResult.begin(); it != schedulingResult.end();)
	{
		bool invalid = true;
		for (int tbIndex = 0; tbIndex < it->numTransportBlock; ++tbIndex)
		{
			if (it->dequeueInfo[tbIndex].dequeueSizeByte > 0)
			{
				invalid = false;
			}
		}

		if (invalid)
		{
			it = schedulingResult.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void FxSchedulerXG::purgeInvalidSchedulingResults(
	std::vector < FxUlSchedulingResultInfo > &schedulingResult)
{
	// Do nothing for UL scheduling at XG scheduler
}

void FxSchedulerXG::createDequeueInformation(
	std::vector < FxDlSchedulingResultInfo > &schedulingResult)
{
	for (size_t ueIndex = 0; ueIndex < schedulingResult.size(); ++ueIndex)
	{
		for (int tbIndex = 0;
			tbIndex < schedulingResult[ueIndex].numTransportBlock;
			++tbIndex)
		{
			schedulingResult[ueIndex].dequeueInfo[tbIndex].bearerId =
				FX_DEFAULT_BEARER_ID;

			int transportBlockSize = PhyFxGetDlTxBlockSize(
				_node,
				_phyIndex,
				schedulingResult[ueIndex].mcsIndex[tbIndex],
				schedulingResult[ueIndex].numResourceBlocks);

			schedulingResult[ueIndex].dequeueInfo[tbIndex].dequeueSizeByte =
				determineDequeueSize(transportBlockSize);
		}
	}
}

void FxSchedulerYH::purgeInvalidSchedulingResults(
	std::vector < FxUlSchedulingResultInfo > &schedulingResult)
{
	// Purge scheduling results which dequeueSize equals to 0

	std::vector < FxUlSchedulingResultInfo > ::iterator it;
	for (it = schedulingResult.begin(); it != schedulingResult.end();)
	{
		bool invalid = true;

		if (it->dequeueInfo.dequeueSizeByte > 0)
		{
			invalid = false;
		}

		if (invalid)
		{
			it = schedulingResult.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void FxSchedulerXG::calculateEstimatedSinrUl(
	const fxRnti& rnti,
	int numResourceBlocks,
	int startResourceBlock,
	double& estimatedSinr_dB)
{
}

bool FxSchedulerXG::dlIsTargetUe(const fxRnti& rnti)
{
	
	return true;
}

bool FxSchedulerXG::ulIsTargetUe(const fxRnti& rnti)
{
	return false;
}
int FxSchedulerXG::dlSelectMcs(
	int numResourceBlocks,
	double estimatedSinr_dB,
	double targetBler)
{
	// Select the maximum mcs which estimated BLER is
	// greater than or equal to target BLER.
	int mcsIndex;

	for (mcsIndex = PHY_FX_REGULAR_MCS_INDEX_LEN - 1;
		mcsIndex >= 0;
		--mcsIndex) {

		// Sinr for determining MCS
		double estimatedSinrForMcsSelection_dB =
			estimatedSinr_dB - SMAC_FX_SNR_OFFSET_FOR_DL_MCS_SELECTION;

		// Calculate bit error ratio
		double ber = PHY_BER(
			_node->phyData[_phyIndex],
			mcsIndex,
			NON_DB(estimatedSinrForMcsSelection_dB));

		int transportBlockSize = PhyFxGetDlTxBlockSize(
			_node,
			_phyIndex,
			mcsIndex,
			numResourceBlocks);


		// Calculate estimated block error rate
		double bler = 1.0 - pow(1.0 - ber, (double)transportBlockSize);

		if (bler <= targetBler)
		{
			break;
		}
	}

	return mcsIndex;
}

int FxSchedulerXG::ulSelectMcs(
	int numResourceBlocks,
	double estimatedSinr_dB,
	double targetBler)
{

	// Select the maximum mcs which estimated BLER is
	// greater than or equal to target BLER.
	int mcsIndex;

	for (mcsIndex = PHY_FX_REGULAR_MCS_INDEX_LEN - 1;
		mcsIndex >= 0;
		--mcsIndex) {

		// Sinr for determining MCS
		double estimatedSinrForMcsSelection_dB =
			estimatedSinr_dB - SMAC_FX_SNR_OFFSET_FOR_UL_MCS_SELECTION;

		// Calculate bit error ratio
		double ber = PHY_BER(
			_node->phyData[_phyIndex],
			mcsIndex + PHY_FX_MCS_INDEX_LEN,
			NON_DB(estimatedSinrForMcsSelection_dB));

		int transportBlockSize;

		transportBlockSize = PhyFxGetUlTxBlockSize(
			_node,
			_phyIndex,
			mcsIndex,
			numResourceBlocks);

		// Calculate estimated block error rate
		double bler = 1.0 - pow(1.0 - ber, (double)transportBlockSize);

		if (bler <= targetBler)
		{
			break;
		}
	}

	return mcsIndex;
}

void FxSchedulerYH::purgeInvalidSchedulingResults(
	std::vector < FxDlSchedulingResultInfo > &schedulingResult)
{
	// Error if default constructor called.
	char errorStr[MAX_STRING_LENGTH];
	sprintf(errorStr,
		"SCH-FX: FxSchedulerYH::purgeInvalidSchedulingResults()"
		"is not supported.");
	ERROR_Assert(FALSE, errorStr);
}
FxSchedulerYH::FxSchedulerYH(
	Node* node, int interfaceIndex, const NodeInput* nodeInput)
	: FxScheduler(node, interfaceIndex, nodeInput)
{
}

FxSchedulerYH::~FxSchedulerYH()
{
}

void FxSchedulerYH::scheduleDlTti(
	std::vector < FxDlSchedulingResultInfo > &schedulingResult)
{
	// Error if default constructor called.
	char errorStr[MAX_STRING_LENGTH];
	sprintf(errorStr,
		"SCH-FX: FxSchedulerYH::schedulerDlTti() is not supported.");
	ERROR_Assert(FALSE, errorStr);
}