#ifndef MAC_FX_SCH_H
#define MAC_FX_SCH_H

#include "phy_fx.h"
#include "layer2_fx_smac.h"
#include "layer3_fx.h"

#define SMAC_FX_DEFAULT_TARGET_BLER (1.0E-2)
#define SMAC_FX_SNR_OFFSET_FOR_UL_MCS_SELECTION  (7.0) // dB
#define SMAC_FX_SNR_OFFSET_FOR_DL_MCS_SELECTION  (0.0) // dB
typedef struct {
	UInt32 bearerId;
	int dequeueSizeByte;
} FxSchedulingResultDequeueInfo;

typedef struct {
	fxRnti rnti;
//	FxTxScheme txScheme; // Transmission scheme
	UInt8 allocatedRb[FX_MAX_NUM_RB]; // List of allocated RB groups
	int numTransportBlock; // Number of Transport Blocks 1 or 2
	UInt8 mcsIndex[2]; // MCS for Transport block 1 or 2
	FxSchedulingResultDequeueInfo dequeueInfo[2];
	int numResourceBlocks; // Number of allocated resource blocks.

} FxDlSchedulingResultInfo;

typedef struct {
	fxRnti rnti;
	UInt8 startResourceBlock;
	int numResourceBlocks;
	UInt8 mcsIndex;
	FxSchedulingResultDequeueInfo dequeueInfo;

} FxUlSchedulingResultInfo;



class FxScheduler {
public:
	FxScheduler();
	FxScheduler(Node* node, int interfaceIndex, const NodeInput* nodeInput);
	virtual ~FxScheduler();

	virtual void scheduleDlTti(
		std::vector < FxDlSchedulingResultInfo > &schedulingResult) = 0;

	virtual void scheduleUlTti(
		std::vector < FxUlSchedulingResultInfo > &schedulingResult) = 0;

	virtual void initConfigurableParameters();

	virtual void prepareForScheduleTti(UInt64 ttiNumber);

	virtual void purgeInvalidSchedulingResults(
		std::vector < FxDlSchedulingResultInfo > &schedulingResult) = 0;

	virtual void purgeInvalidSchedulingResults(
		std::vector < FxUlSchedulingResultInfo > &schedulingResult) = 0;

	virtual int determineDequeueSize(int transportBlockSize);

	// Notification I/Fs

	virtual void notifyPowerOn();
	virtual void notifyPowerOff();
	virtual void notifyAttach(const fxRnti& rnti);
	virtual void notifyDetach(const fxRnti& rnti);

	void init();

protected:
	Node* _node;
	int _interfaceIndex;
	int _phyIndex;
	const NodeInput* _nodeInput;

	UInt64 _ttiNumber;

private:
};
class FxSchedulerXG : public FxScheduler {
public:
	FxSchedulerXG(
		Node* node, int interfaceIndex, const NodeInput* nodeInput);
	virtual ~FxSchedulerXG();

	virtual void scheduleDlTti(
		std::vector < FxDlSchedulingResultInfo > &schedulingResult) = 0;

	virtual void scheduleUlTti(
		std::vector < FxUlSchedulingResultInfo > &schedulingResult) = 0;

	virtual void initConfigurableParameters();

	virtual void purgeInvalidSchedulingResults(
		std::vector < FxDlSchedulingResultInfo > &schedulingResult);

	virtual void purgeInvalidSchedulingResults(
		std::vector < FxUlSchedulingResultInfo > &schedulingResult);

	virtual void createDequeueInformation(
		std::vector < FxDlSchedulingResultInfo > &schedulingResult);

	virtual void calculateEstimatedSinrUl(
		const fxRnti& rnti,
		int numResourceBlocks,
		int startResourceBlock,
		double& estimatedSinr_dB);

	virtual bool dlIsTargetUe(const fxRnti& rnti);
	virtual bool ulIsTargetUe(const fxRnti& rnti);

	int dlSelectMcs(
		int numResourceBlocks,
		double estimatedSinr_dB,
		double targetBler);

	int ulSelectMcs(
		int numResourceBlocks,
		double estimatedSinr_dB,
		double targetBler);

protected:
	double _targetBler;

private:
};
class FxSchedulerYH : public FxScheduler {
public:
	FxSchedulerYH(
		Node* node, int interfaceIndex, const NodeInput* nodeInput);
	virtual ~FxSchedulerYH();

	virtual void scheduleDlTti(
		std::vector < FxDlSchedulingResultInfo > &schedulingResult);
	virtual void scheduleUlTti(
		std::vector < FxUlSchedulingResultInfo > &schedulingResult) = 0;

	virtual void purgeInvalidSchedulingResults(
		std::vector < FxDlSchedulingResultInfo > &schedulingResult);

	virtual void purgeInvalidSchedulingResults(
		std::vector < FxUlSchedulingResultInfo > &schedulingResult);

private:
};
#endif 