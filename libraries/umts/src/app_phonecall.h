// Copyright (c) 2001-2006, Scalable Network Technologies, Inc.  All Rights Reserved.
//                          6100 Center Drive
//                          Suite 1250
//                          Los Angeles, CA 90045
//                          sales@scalable-networks.com
//
// This source code is licensed, not sold, and is subject to a written
// license agreement.  Among other things, no portion of this source
// code may be copied, transmitted, disclosed, displayed, distributed,
// translated, used as the basis for a derivative work, or used, in
// whole or in part, for any program or purpose other than its intended
// use in compliance with the license agreement as part of the QualNet
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

// /**
// PACKAGE     :: APP_PHONECALL
// DESCRIPTION :: Defines the data structures used 
//                in the PHONE CALL APPLICAITON
//                Most of the time the data structures and functions start
//                with PhoneCallApp** 
// **/
#ifndef _APP_PHONE_CALL_H_
#define _APP_PHONE_CALL_H_
// /**
// CONSTANT    :: APP_PHONE_CALL_MAXIMUM_RETRY  :   5
// DESCRIPTION :: APPLICAITON MAXIMUM RETRY IF REJCTED OR FAILED
// **/
#define APP_PHONE_CALL_MAXIMUM_RETRY 5

const clocktype  APP_PHONE_CALL_DEF_PKTITVL = 20*MILLI_SECOND;
const int APP_PHONE_CALL_DEF_DATARATE = 12400;
const clocktype APP_PHONE_CALL_MIN_TALK_TIME  = 1*SECOND;
const clocktype APP_PHONE_CALL_TALK_TURNAROUND_TIME = 100*MILLI_SECOND;

// /**
// ENUM        :: App_Phone_Call_Status
// DESCRIPTION :: Status of applicaiton
// **/
typedef enum
{
    APP_PHONE_CALL_STATUS_SUCCESSED = 0,
    APP_PHONE_CALL_STATUS_ONGOING = 1,
    APP_PHONE_CALL_STATUS_REJECTED = 2,
    APP_PHONE_CALL_STATUS_FAILED = 3,
    APP_PHONE_CALL_STATUS_INITIATING = 4,
    APP_PHONE_CALL_STATUS_RETRYING = 5,
    APP_PHONE_CALL_STATUS_NEEDRETRY = 6,
    APP_PHONE_CALL_STATUS_LISTENING,
    APP_PHONE_CALL_STATUS_TALKING,
    APP_PHONE_CALL_STATUS_PENDING = -1
} App_Phone_Call_Status;

// /**
// ENUM        :: App_Phone_Call_Timer_Type
// DESCRIPTION :: Type of Timer
// **/
typedef enum
{
    APP_PHONE_CALL_START_TIMER = 0,
    APP_PHONE_CALL_END_TIMER = 1,
    APP_PHONE_CALL_RETRY_TIMER = 2,
    APP_PHONE_CALL_TALK_START_TIMER
}App_Phone_Call_Timer_Type;

// /**
// ENUM        :: AppPhoneCallRejectCauseType
// DESCRIPTION :: Define the casue of call rejection
// **/
typedef enum
{
    APP_PHONE_CALL_REJECT_CAUSE_SYSTEM_BUSY =1,
    APP_PHONE_CALL_REJECT_CAUSE_USER_BUSY,
    APP_PHONE_CALL_REJECT_CAUSE_TOO_MANY_ACTIVE_APP,
    APP_PHONE_CALL_REJECT_CAUSE_USER_UNREACHABLE,
    APP_PHONE_CALL_REJECT_CAUSE_USER_POWEROFF,
    APP_PHONE_CALL_REJECT_CAUSE_NETWORK_NOT_FOUND,
    APP_PHONE_CALL_REJECT_CAUSE_UNSUPPORTED_SERVICE,
    APP_PHONE_CALL_REJECT_CAUSE_UNKNOWN_USER,
    APP_PHONE_CALL_REJECT_CAUSE_UNKNOWN
}AppPhoneCallRejectCauseType;

// /**
// ENUM        :: AppPhoneCallDropCauseType
// DESCRIPTION :: Define the casue of call rejection
// **/
typedef enum
{
    APP_PHONE_CALL_DROP_CAUSE_SYSTEM_CALL_ADMISSION_CONTROL =1,
    APP_PHONE_CALL_DROP_CAUSE_SYSTEM_CONGESTION_CONTROL,
    APP_PHONE_CALL_DROP_CAUSE_HANDOVER_FAILURE,
    APP_PHONE_CALL_DROP_CAUSE_SELF_POWEROFF,
    APP_PHONE_CALL_DROP_CAUSE_REMOTEUSER_POWEROFF,
    APP_PHONE_CALL_DROP_CAUSE_UNKNOWN
}AppPhoneCallDropCauseType;

// /**
// STRUCT      :: AppPhoneCallInfo
// DESCRIPTION :: Definition of the data structure 
//                for the phone call App
// **/
typedef struct struct_app_phone_call_info_str
{
    int appId;  
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    clocktype appStartTime;
    clocktype appDuration;
    clocktype avgTalkTime;

    clocktype appSetupTime;
    clocktype appEndTime;
    
    clocktype pktItvl;      //Packetization interval
    int       dataRate;     //Speech data rate 
    int       pktSize; 
    clocktype curTalkEndTime;

    BOOL appRetryAllowed;
    int appNumRetry;
    int appMaxRetry;
    App_Phone_Call_Status  appStatus;

    Message *startCallTimer;
    Message *endCallTimer;
    Message *retryCallTimer;
    Message *talkStartTimer;
    clocktype nextTalkStartTime;

    struct_app_phone_call_info_str *next;
    // in the future implementation, more applications could
    // be possible including FTP,CBR,VBR and so on

    RandomDistribution<clocktype>* avgTalkDurDis;
}AppPhoneCallInfo;

// /**
// STRUCT      :: AppPhoneCallTimerInfo
// DESCRIPTION :: Info struncture for app timer
// **/
typedef struct app_phone_call_timer_info_str
{
    NodeId appSrcNodeId;
    int appId;
    App_Phone_Call_Timer_Type timerType;
}AppPhoneCallTimerInfo;

//info structure for the message between APP and Network layers
// /**
// STRUCT      :: AppPhoneCallStartMessageInfo
// DESCRIPTION :: Definition of Info field structure for 
//                the MSG_NETWORK_CELLULAR_FromAppStartCall
// **/
typedef struct 
{
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    clocktype callEndTime;
    clocktype avgTalkTime;
}AppPhoneCallStartMessageInfo;

// /**
// STRUCT      :: AppPhoneCallEndMessageInfo
// DESCRIPTION :: Definition of Info field structure 
//                for the MSG_NETWORK_CELLULAR_FromAppEndCall
// **/
typedef AppPhoneCallStartMessageInfo  
            AppPhoneCallEndMessageInfo;

// /**
// STRUCT      :: AppPhoneCallRejectedInfo
// DESCRIPTION :: Definition of Info field structure
//                for the MSG_APP_CELLULAR_FromNetworkCallRejected
// **/
typedef AppPhoneCallStartMessageInfo 
            AppPhoneCallRejectedInfo;

// /**
// STRUCT      :: AppPhoneCallArriveMessageInfo
// DESCRIPTION :: Definition of Info field structure 
//                for the MSG_APP_CELLULAR_FromNetworkCallArrive
// **/
typedef struct 
{
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    clocktype callEndTime;
    clocktype avgTalkTime;
    int transactionId;
}AppPhoneCallArriveMessageInfo;

// /**
// STRUCT      :: AppPhoneCallAcceptMessageInfo;
// DESCRIPTION :: Definition of Info field structure 
//                for the MSG_APP_CELLULAR_FromNetworkCallAccepted
// **/
typedef AppPhoneCallArriveMessageInfo  
            AppPhoneCallAcceptMessageInfo;

// /**
// STRUCT      :: AppPhoneCallAnsweredMessageInfo
// DESCRIPTION :: Definition of Info field structure 
//                for the MSG_NETWORK_CELLULAR_FromAppCallAnswered
// **/
typedef AppPhoneCallArriveMessageInfo 
            AppPhoneCallAnsweredMessageInfo;

// /**
// STRUCT      :: AppPhoneCallEndByRemoteMessageInfo
// DESCRIPTION :: Definition of Info field structure 
//                for the MSG_APP_CELLULAR_FromNetworkCallEndByRemote
// **/
typedef AppPhoneCallArriveMessageInfo 
            AppPhoneCallEndByRemoteMessageInfo;

// /**
// STRUCT      :: AppPhoneCallRejectMessageInfo
// DESCRIPTION :: Definition of Info field structure 
//                for the MSG_APP_CELLULAR_FromNetworkCallRejected
// **/
typedef struct 
{
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    clocktype avgTalkTime;
    int transactionId;
    AppPhoneCallRejectCauseType rejectCause;
}AppPhoneCallRejectMessageInfo;

// /**
// STRUCT      :: AppPhoneCallDropMessageInfo
// DESCRIPTION :: Definition of Info field structure 
//                for the MSG_APP_CELLULAR_FromNetworkCallDropped
// **/
typedef struct 
{
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    clocktype avgTalkTime;
    int transactionId;
    AppPhoneCallDropCauseType dropCause;
}AppPhoneCallDropMessageInfo;

// /**
// STRUCT      :: AppPhoneCallPktArrivalInfo
// DESCRIPTION :: Definition of Info field structure 
//                for the MSG_APP_CELLULAR_FromNetworkPktArrive
// **/
typedef struct 
{
    int appId;
    NodeAddress appSrcNodeId;
} AppPhoneCallPktArrivalInfo;

// /**
// STRUCT      :: AppPhoneCallStats
// DESCRIPTION :: stats for app
// **/
typedef struct app_phone_call_stat_str

{
    int numAppRequestSent; //only for src
    int numAppRequestRcvd;//only for dest

    //only for src MS
    int numAppAcceptedByNetwork;
    int numAppRejectedByNetwork;
    int numAppRejectedCauseSystemBusy;
    int numAppRejectedCauseUserBusy;
    int numAppRejectedCauseTooManyActiveApp;
    int numAppRejectedCauseUserUnreachable;
    int numAppRejectedCauseUserPowerOff;
    int numAppRejectedCauseNetworkNotFound;
    int numAppRejectedCauseUnsupportService;
    int numAppRejectedCauseUnknowUser;
    int numAppRetry;
    
    //for both
    int numAppSuccessEnd;
    int numOriginAppSuccessEnd;
    int numTerminateAppSuccessEnd;

    //for both
    int numAppDropped;
    int numOriginAppDropped;
    int numOriginAppDroppedCauseHandoverFailure;
    int numOriginAppDroppedCauseSelfPowerOff;
    int numOriginAppDroppedCauseRemotePowerOff;
    int numTerminateAppDropped;
    int numTerminateAppDroppedCauseHandoverFailure;
    int numTerminateAppDroppedCauseSelfPowerOff;
    int numTerminateAppDroppedCauseRemotePowerOff;

    int numPktsSent;
    int numPktsRcvd;
    clocktype totEndToEndDelay;
}AppPhoneCallStats;

// /**
// STRUCT      :: AppPhoneCalllayerData
// DESCRIPTION :: data structure for phone call app layer
// **/
typedef struct 
{
    int numOriginApp;
    int numTerminateApp;

    //application list

    //app originating from this node
    AppPhoneCallInfo* appInfoOriginList;

    //app terminating to this node
    AppPhoneCallInfo* appInfoTerminateList;

    //ms active status
    
    //statistics
    AppPhoneCallStats    stats;
    BOOL collectStatistics;    // whether collect satistics
}AppPhoneCallLayerData;

// /**
// STRUCT      :: AppPhoneCallData
// DESCRIPTION :: APP PHONE CALL 
// **/
typedef struct app_phone_call_data
{
    int numPkts;
    clocktype TalkDurLeft;
    clocktype ts_send;
} AppPhoneCallData;

//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: AppPhoneCallInit
// LAYER      :: APP
// PURPOSE    :: Init an application.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void AppPhoneCallInit(Node *node,
                      const NodeInput *nodeInput);

// /**
// FUNCTION   :: AppPhoneCallFinalize
// LAYER      :: APP
// PURPOSE    :: Print stats and clear protocol variables
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// RETURN     :: void : NULL
// **/ 
void AppPhoneCallFinalize(Node *node);

// /**
// FUNCTION   :: AppPhoneCallLayer
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/ 
void AppPhoneCallLayer(Node *node, Message *msg);

// /**
// FUNCTION   :: AppPhoneCallInsertList
// LAYER      :: APP
// PURPOSE    :: Insert the application info to the specified app List.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + appList : AppPhoneCallInfo ** : 
//                 Originating app List or terminating app List
// + appId : int : id of the application.
// + appSrcNodeId : NodeAddress : Source address of the app
// + appDestNodeId : NodeAddress : Destination address of the app
// + avgTalkTime   : clockType : average talk timeo f the phone call
// + appStartTime : clocktype : Start time of the app
// + appDuration : clocktype : Duration of the app
// + isAppSrc : BOOL : Originaitng or terminating application 
// RETURN     :: void : NULL 
// **/ 
//************************************************************************
void 
AppPhoneCallInsertList(Node *node, 
                       AppPhoneCallInfo** appList, 
                       int appId, 
                       NodeAddress appSrcNodeId,
                       NodeAddress appDestNodeId,
                       clocktype avgTalkTime,
                       clocktype appStartTime,
                       clocktype appDuration,
                       BOOL isAppSrc);
#endif /* _APP_PHONE_CALL_H_ */
