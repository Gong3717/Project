#ifndef FX_COMMON
#define FX_COMMON

#include <map>
#include "types.h"
#include "lte_common.h" //gss xd



#define FX_INFINITY_POWER_dBm            (1000.0)
#define FX_NEGATIVE_INFINITY_POWER_dBm   (-1000.0)
#define FX_INFINITY_SINR_dB              (1000.0)
#define FX_NEGATIVE_INFINITY_SINR_dB     (-1000.0)
#define FX_INFINITY_PATHLOSS_dB          (1000.0)
#define FX_NEGATIVE_INFINITY_PATHLOSS_dB (-1000.0)
//Added by WLH
#define FX_STRING_STATION_TYPE_XG "xg"
#define FX_STRING_STATION_TYPE_YH "yh"

// ���ǽڵ�����
typedef enum {
	FX_STATION_TYPE_XG,
	FX_STATION_TYPE_YH,
	FX_STATION_TYPE_SAT,
}FxStationType;


// RNTI  Radio Network Temporary Identifier
// ����������ʱ��ʶ������Ϊ��ͬUE�ı�ʶ
// �ڵ�ID�ͽӿں�
typedef struct fx_rnti
{
	NodeAddress nodeId;
	int interfaceIndex;

	fx_rnti() : nodeId(ANY_ADDRESS), interfaceIndex(ANY_INTERFACE) {}

	fx_rnti(NodeAddress node_id, int if_id)
	{
		nodeId = node_id;
		interfaceIndex = if_id;
	}

	bool operator < (const fx_rnti& a) const
	{
		if (nodeId < a.nodeId ||
			(nodeId == a.nodeId && interfaceIndex < a.interfaceIndex))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool operator == (const fx_rnti& a) const
	{
		return nodeId == a.nodeId && interfaceIndex == a.interfaceIndex
			? true : false;
	}

	bool operator != (const fx_rnti& a) const
	{
		return nodeId != a.nodeId || interfaceIndex != a.interfaceIndex
			? true : false;
	}
} fxRnti;

typedef std::list< fxRnti> ListFxRnti;
typedef std::vector< fxRnti> vecFxRnti;

#define FX_INVALID_RNTI (fxRnti(ANY_ADDRESS, ANY_INTERFACE))


// ż��֡10��11��ʱ϶���ͱ���������Ϣ
typedef struct  
{
	
}MIB1;


// ÿ֡0�ż�16��ʱ϶�ظ�����
typedef struct
{
	UInt8 xgId;
	UInt8 frame_num;
	UInt8 slot_num;
	UInt8 SBCP_num;
	UInt8 RA_B;
	UInt8 RA_D;
	UInt8 RA_Timer;
}MIB2;


// ÿ֡24��25��26��ʱ϶���ͱ��룬��1�룬��2�������λ����Ϣ
typedef struct
{

}MIB3;


// �����������
typedef struct
{
	NodeAddress MacAddr;
}RAREQ;


// �������Ӧ��
typedef struct 
{
	NodeAddress MacAddr; // �û�վ��mac��ַ
	UInt16 BCID; // �Ź�վ�����û�վ�����CID��Դ

	UInt8 sub_chl_num; // ���û�վ��Ҫ�������ĸ����������ŵ���
	UInt8 frame_num; // ���û�վ�����֡��
	UInt8 slot_num; // ���û�վ�����ʱ϶�� 

}RARSP;


// RRCConnectionRequest
typedef struct  
{
	NodeAddress MacAddr; // �û�վ��mac��ַ
}RRCConnectionRequest;


// RRCConnectionSetup 
typedef struct
{
	NodeAddress MacAddr; // �û�վ��mac��ַ
}RRCConnectionSetup;


// RRCConnectionSetupComplete
typedef struct
{
	NodeAddress MacAddr; // �û�վ��mac��ַ
}RRCConnectionSetupComplete;



// RRCHandoverRequest
typedef struct
{
	UInt8 targetBeamIndex; // Ŀ�Ĳ�����
}RRCHandoverRequest;


// RRCConnectionReconfiguration
typedef struct
{
	UInt8 targetBeamIndex; // Ŀ�Ĳ�����
	UInt8 ulChannelIndex;
	UInt8 dlChannelIndex;
}RRCConnectionReconfiguration;

// RRCConnectionReconfigurationComplete
typedef struct
{
	NodeAddress MacAddr; // �û�վ��mac��ַ
}RRCConnectionReconfigurationComplete;

template<typename T>
void FxAddMsgInfo(
	Node* node,
	int interfaceIndex,
	Message* msg,
	unsigned short infoType,
	T& info)
{
	ERROR_Assert(msg, "msg is NULL");
	const int infoLength = (int) sizeof(T);
	T* infoHead = (T*)MESSAGE_AddInfo(node, msg, infoLength, infoType);
	ERROR_Assert(infoHead, "MESSAGE_AddInfo is failed.");
	memcpy(infoHead, &info, infoLength);
}
//Added by WLH
typedef enum
{
	TX_SCHEME_SINGLE_ANTENNA_FX,
	TX_SCHEME_DIVERSITY_FX,
	TX_SCHEME_OL_SPATIAL_MULTI_FX,
	TX_SCHEME_INVALID_FX,
} FxTxScheme;
//Added by WLH
typedef struct struct_handover_participator_fx
{
	fxRnti yhRnti;
	fxRnti srcXgRnti;
	fxRnti tgtXgRnti;
	struct_handover_participator_fx()
		: yhRnti(FX_INVALID_RNTI),
		srcXgRnti(FX_INVALID_RNTI),
		tgtXgRnti(FX_INVALID_RNTI)
	{}
	struct_handover_participator_fx(
		fxRnti yhRnti,
		fxRnti srcXgRnti,
		fxRnti tgtXgRnti)
		: yhRnti(yhRnti),
		srcXgRnti(srcXgRnti),
		tgtXgRnti(tgtXgRnti)
	{}
} FXHandoverParticipator;

//gss xd
typedef struct struct_handover_participator_xd
{
	LteRnti ueRnti;
	LteRnti srcEnbRnti;
	LteRnti sgwmmeRnti;
	fxRnti tgtxgRnti;
	struct_handover_participator_xd()
		: ueRnti(LTE_INVALID_RNTI),
		srcEnbRnti(LTE_INVALID_RNTI),
		sgwmmeRnti(LTE_INVALID_RNTI),
		tgtxgRnti(FX_INVALID_RNTI)
	{}
	struct_handover_participator_xd(
		LteRnti ueRnti,
		LteRnti srcEnbRnti,
		LteRnti sgwmmeRnti,
		fxRnti tgtxgRnti)
		: ueRnti(ueRnti),
		srcEnbRnti(srcEnbRnti),
		sgwmmeRnti(sgwmmeRnti),
		tgtxgRnti(tgtxgRnti)
	{}
} XdHandoverParticipator;

#endif
