#ifndef PHY_FX_H
#define PHY_FX_H


#include "fx_rrc_config.h"
#include <set>
#include <vector>

// xyt : temp solution
#define SPHY_FX_TRANSBLOCK_SIZE_30MHZ_BYTE (11648)
#define SPHY_FX_TRANSBLOCK_SIZE_15MHZ_BYTE (7168)
#define SPHY_FX_TRANSBLOCK_SIZE_5MHZ_BYTE (1792)
#define SPHY_FX_TRANSBLOCK_SIZE_2500KHZ_BYTE (896)


typedef enum {
	PHY_FX_POWER_OFF,
	// Cell selection
	PHY_FX_IDLE_CELL_SELECTION,
	PHY_FX_CELL_SELECTION_MONITORING,

	// Random access
	PHY_FX_IDLE_RANDOM_ACCESS,
	PHY_FX_RA_PREAMBLE_TRANSMISSION_RESERVED,
	PHY_FX_RA_PREAMBLE_TRANSMITTING,
	PHY_FX_RA_GRANT_WAITING,

	PHY_FX_IDLE,
	// Stationary status
	PHY_FX_DL_IDLE,
	PHY_FX_UL_IDLE,

	// eNB status
	PHY_FX_DL_FRAME_RECEIVING,
	PHY_FX_UL_FRAME_TRANSMITTING,

	// UE status
	PHY_FX_UL_FRAME_RECEIVING,
	PHY_FX_DL_FRAME_TRANSMITTING,
	PHY_FX_STATUS_TYPE_LEN
} PhyFxState;

typedef struct phy_fx_tx_info {
	clocktype Txtime;
	FxStationType txStationType;
	fxRnti destRNTI;
	fxRnti sourceRNTI;
} PhyFxTxInfo;


typedef struct phy_fx_establishment_data_str {
	// Cell selection
	std::map < fxRnti, FxRrcConfig >* rxRrcConfigList;
	std::set<fxRnti>* rxRntiList;

	// Random Access
	fxRnti selectedRntiXG;
	FxRrcConfig selectedRrcConfig;
	BOOL informed;
} PhyFxEstablishmentData;


struct UlChannelinfo {
	int channelIndex;
	PhyStatusType currentMode;
	BOOL rxMsgError;
};
struct DlChannelinfo {
	int channelIndex;
	BOOL rxMsgError;
};

void PhyFxStartXGSearchAtHY(Node* node, const int phyIndex);
//Added by WLH
typedef struct phy_Fx_connected_ue_info_str {
	double maxTxPower_dBm;

	//FxDciFormat0 dciFormat0;

	//// CQI receiver
	//CqiReceiver cqiReceiver;

	// UL instant pathloss measured by SRS
	std::vector < double > ulInstantPathloss_dB;

	// UE lost detection
	BOOL isDetecting;
	clocktype connectedTime;
	BOOL isFirstChecking;
} PhyFxConnectedUeInfo;


typedef std::map<UInt8, pair<UInt8, UInt8>> BeamChannelMap;

typedef struct struct_phy_sphy_str {

	PhyData* thisPhy;

	FxStationType stationType;

	double channelBandwidth;
	double     txPower_dBm;
	double rxThreshold_dBm;

	double latest_rxSinr_db;

	PhyFxState phyFxState;

	// 用户/信关：该接口上下行信道记录
	UlChannelinfo ULchannel;
	DlChannelinfo DLchannel;
	
	// 用户：
	clocktype propagationDelay;
	clocktype lastSignalArrival;

	// 用户：接入记录
	PhyFxEstablishmentData* establishmentData;

	// 信关：接入/切换控制信令有关
	std::set<fxRnti>* raGrants;
	std::vector<int> BLChannels;
	std::set<fxRnti>* RRCsetups;
	std::set<pair<UInt8, UInt8>>* rrcReconfs; // pair<nodeId, targetBeamIndex>

	// 用户/信关：下行信号碰撞检测
	std::map<int, pair<Message*,BOOL>>* beamState; // map<beamIndex, receiving>
	
	// 信关：记录所有波束的上下行信道
	BeamChannelMap* beamChannel; // map<beam, <ul, dl>>

	// 用户：接入/切换控制信令有关
	int RAREQcount;
	BOOL randomAccessCompleteFlag;
	BOOL rrcSetupCompleteflag;
	std::set<UInt8>* nextBeam;
	BOOL rrcReconfCompleteFlag;

	// Added by WLH
	std::map < fxRnti, PhyFxConnectedUeInfo >* yhInfoList;

	UInt16 TBsize;
} PhyDataFx;


typedef struct phy_beam_info {
	int beamIndex; 
	UInt8 UlChannelIndex;
	UInt8 DlChannelIndex;
}PhyFxBeamInfo; 


typedef struct phy_sphy_cqi {

}PhySphyCqi; //信道质量



typedef struct phy_fx_control_phy_to_mac_info {
	BOOL isError;
	fxRnti srcRnti;
} PhyFxPhyToMacInfo;


void PhySphyInit(
	Node* node,
	const int phyIndex,
	const NodeInput *nodeInput);


void PhySphyFinalize(Node *node, const int phyIndex);


void PhySphyStartTransmittingSignal(
	Node* node,
	int phyIndex,
	Message* packet,
	BOOL useMacLayerSpecifiedDelay,
	clocktype initDelayUntilAirborne);


void PhySphyTransmissionEnd(Node *node, int phyIndex);



void PhySphySignalArrivalFromChannel(
	Node* node,
	int phyIndex,
	int channelIndex,
	PropRxInfo *propRxInfo);


void PhySphySignalEndFromChannel(
	Node* node,
	int phyIndex,
	int channelIndex,
	PropRxInfo *propRxInfo);


void PhyFxNotifyPowerOn(Node* node, const int phyIndex);

void FxPhySetBeamChannels(Node* node, const int phyIndex);


clocktype PhyFxGetPropagationDelay(Node* node, const int phyIndex);

clocktype PhyFxGetLastSignalArrivalTime(Node* node, const int phyIndex);

void PhyFxSetRAREQcount(Node* node, const int phyIndex, UInt8 V);


UInt16 PhyFxGetTransblockSize(Node * node, int phyIndex);

void PhyFxSetTransblockSize(Node * node, int phyIndex, UInt16 size);


void PhyFxRaGrantTransmissionIndicationAtXG(
	Node* node,
	int phyIndex,
	fxRnti ueRnti);


void PhyFxRrcConnectedNotification(Node* node, int phyIndex);

void PhyFxRrcHandoverRequestNotificationAtYH(Node* node, int phyIndex, UInt8 nextBeamIndex);

void PhyFxRrcConnectionReconfIndicationAtXG(Node * node, int phyIndex, UInt8 yhNodeId, UInt8 beam);

void PhyFxRrcReconfCompleteNotificationAtYH(Node * node, int phyIndex);

// /**
// STRUCT     :: PhyFxSrsInfo
// DESCRIPTION:: SRS Information
// **/
typedef struct phy_Fx_srs_info_str {
	fxRnti dstRnti;
} PhyFxSrsInfo;
void PhyFxIndicateDisconnectToUe(
	Node* node,
	int phyIndex,
	const fxRnti& ueRnti);
BOOL PhyFxGetMessageSrsInfo(
	Node* node, int phyIndex, Message* msg, PhyFxSrsInfo* srs);
#endif