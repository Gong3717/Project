#ifndef FX_RRC_CONFIG_H
#define FX_RRC_CONFIG_H

#include "fx_common.h"



struct FxSacConfig
{
	// no member at Phase 1
};

struct FxSlcConfig
{
	// RLC-FX-MAX-RETX-THRESHOLD
	UInt8 maxRetxThreshold;
	// RLC-FX-POLL-PDU
	int pollPdu;
	// RLC-FX-POLL-BYTE
	int pollByte;
	// RLC-FX-POLL-RETRANSMIT
	int tPollRetransmit;
	// RLC-FX-T-REORDERINNG
	int tReordering;
	// RLC-FX-T-STATUS-PROHIBIT
	int tStatusProhibit;

	FxSlcConfig()
	{
		maxRetxThreshold = 0;
		pollPdu = 0;
		pollByte = 0;
		tPollRetransmit = 0;
		tReordering = 0;
		tStatusProhibit = 0;
	}
};

struct FxSmacConfig
{
	//MAC-FX-RA-BACKOFF-TIME
	clocktype raBackoffTime;

	//MAC-FX-RA-PREAMBLE-INITIAL-RECEIVED-TARGET-POWER
	double raPreambleInitialReceivedTargetPower;

	//MAC-FX-RA-POWER-RAMPING-STEP
	double raPowerRampingStep;

	//MAC-FX-RA-PREAMBLE-TRANS-MAX
	int raPreambleTransMax;

	//MAC-FX-RA-RESPONSE-WINDOW-SIZE
	// ra-ResponseWindowSize
	int raResponseWindowSize;

	//MAC-FX-RA-PRACH-CONFIG-INDEX
	// prach-ConfigIndex
	int raPrachConfigIndex; // 0-15

							//MAC-FX-PERIODIC-BSR-TTI
							// in 3GPP specification, number of sending every X subframe
							// in FX Library, number of sending every X TTI
	int periodicBSRTimer;

	//MAC-FX-TRANSMISSION-MODE
	int transmissionMode;

	FxSmacConfig()
	{
		raBackoffTime = 0;
		raPreambleInitialReceivedTargetPower = 0;
		raPowerRampingStep = 0;
		raPreambleTransMax = 0;
		raResponseWindowSize = 0;
		raPrachConfigIndex = 0;
		periodicBSRTimer = 0;
		transmissionMode = 0;
	};

};

struct FxPhyConfig
{
	//PHY-FX-DL-CHANNEL
	int dlChannelNo;

	//PHY-FX-UL-CHANNEL
	int ulChannelNo;

	//PHY-FX-TX-POWER
	double maxTxPower_dBm;

	// xyt : cell 是不是要改成波束
	//PHY-FX-CELL-SELECTION-RX-LEVEL-MIN
	double cellSelectionRxLevelMin_dBm;

	//PHY-FX-CELL-SELECTION-RX-LEVEL-MIN-OFFSET
	double cellSelectionRxLevelMinOff_dBm;

	//PHY-FX-CELL-SELECTION-QUAL-LEVEL-MIN
	double cellSelectionQualLevelMin_dB;

	//PHY-FX-CELL-SELECTION-QUAL-LEVEL-MIN-OFFSET
	double cellSelectionQualLevelMinOffset_dB;


	//PHY-FX-NON-SERVING-CELL-MEASUREMENT-INTERVAL
	clocktype nonServingCellMeasurementInterval;

	//PHY-FX-NON-SERVING-CELL-MEASUREMENT-PREIOD
	clocktype nonServingCellMeasurementPeriod;

	//PHY-FX-CELL-SELECTION-MIN-SERVING-DURATION
	clocktype cellSelectionMinServingDuration;

	//PHY-FX-RX-SENSITIVITY
	double rxSensitivity_dBm;

	FxPhyConfig()
	{
		dlChannelNo = 0;
		ulChannelNo = 0;
		maxTxPower_dBm = FX_NEGATIVE_INFINITY_POWER_dBm;

		cellSelectionRxLevelMin_dBm = FX_NEGATIVE_INFINITY_POWER_dBm;
		cellSelectionRxLevelMinOff_dBm = 0;
		cellSelectionQualLevelMin_dB = 0;
		cellSelectionQualLevelMinOffset_dB = 0;

		nonServingCellMeasurementInterval = 0;
		nonServingCellMeasurementPeriod = 0;
		cellSelectionMinServingDuration = 0;
		rxSensitivity_dBm = 0;
	};
};

struct FxRrcConfig
{
	FxSacConfig fxSacConfig; // SAC Config
	FxSlcConfig  fxSlcConfig;  // RLC Config
	FxSmacConfig  fxSmacConfig;  // MAC Config
	FxPhyConfig  fxPhyConfig;  // PHY Config

	//double fifxrCoefficient; // RRC-FX-MEAS-FIFXR-COEFFICIENT

	//FxRrcConfig()
	//{
	//	fifxrCoefficient = 0.0;
	//};
};

//// Getter
// Config Obj pointer
FxRrcConfig* GetFxRrcConfig(Node* node, int interfaceIndex);
FxSacConfig* GetFxSacConfig(Node* node, int interfaceIndex);
FxSlcConfig* GetFxSlcConfig(Node* node, int interfaceIndex);
FxSmacConfig* GetFxSmacConfig(Node* node, int interfaceIndex);
FxPhyConfig* GetFxPhyConfig(Node* node, int interfaceIndex);

//// Setter
void
SetFxRrcConfig(Node* node, int interfaceIndex, FxRrcConfig& rrcConfig);




#endif 
