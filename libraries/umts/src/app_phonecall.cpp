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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network.h"
#include "network_ip.h"

#include "app_phonecall.h"
#include "cellular.h"
#include "user.h"
#include "random.h"

#define DEBUG_APP   0
//***********************************************************************
// /**
// FUNCTION   :: AppPhoneCallGetApp
// LAYER      :: APP
// PURPOSE    :: Retrive the application info from the app List.
// PARAMETERS ::
// + node       : Node* : Pointer to node.
// + appId      : int : id of the application.
// + appSrcNodeId  : NodeAddress : src node id, who generate this app
// + appList    : AppPhoneCallInfo** :
//                  Originating app List or terminating app List
// + appInfo    : AppPhoneCallInfo** :
//                  Pointer to hold the retrived infomation
// RETURN     :: BOOL : success or fail when looking for the app in the list
// **/
//************************************************************************
static BOOL
AppPhoneCallGetApp(Node *node,
                   int appId,
                   NodeAddress appSrcNodeId,
                   AppPhoneCallInfo** appList,
                   AppPhoneCallInfo** appInfo)

{
    BOOL found=FALSE;
    AppPhoneCallInfo *currentApp;

    currentApp = *appList;
    if (DEBUG_APP && 0)
    {
        printf(
            "node %d: in Get APP Info\n",
            node->nodeId);
    }
    while (currentApp != NULL)
    {
        if (DEBUG_APP && 0)
        {
            printf(
                "node %d: Get APP Info, appId %d,"
                "src %d, dest %d \n",
                node->nodeId, currentApp->appId,
                currentApp->appSrcNodeId, currentApp->appDestNodeId);
        }
        if (currentApp->appId == appId &&
            currentApp->appSrcNodeId == appSrcNodeId)
        {
            found = TRUE;
            *appInfo = currentApp;
            break;
        }
            currentApp = currentApp->next;
    }
    if (DEBUG_APP && 0)
    {
        printf(
            "node %d: Get APP Info, success/fail= %d\n",
            node->nodeId,found);
    }
    return found;
}
//************************************************************************
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
                       BOOL isAppSrc)
{
    AppPhoneCallInfo *newApp;
    AppPhoneCallInfo *currentApp;
    AppPhoneCallLayerData *appPhoneCallLayerData;
    Message *callStartTimerMsg;
    AppPhoneCallTimerInfo *info;

    appPhoneCallLayerData = (AppPhoneCallLayerData *)
        node->appData.phonecallVar;
    if (isAppSrc == TRUE)
    {
        (appPhoneCallLayerData->numOriginApp) ++;
    }
    else
    {
        (appPhoneCallLayerData->numTerminateApp) ++;
    }

    newApp = (AppPhoneCallInfo *)
        MEM_malloc(sizeof(AppPhoneCallInfo));
    memset(newApp,
           0,
           sizeof(AppPhoneCallInfo));

    if (isAppSrc ==TRUE)
    {
        newApp->appId = appId + 1;//src app use local one
    }
    else
    {
        newApp->appId = appId;//dest app, use app src's
    }

    newApp->appSrcNodeId = appSrcNodeId;
    newApp->appDestNodeId = appDestNodeId;
    newApp->appStartTime = appStartTime;
    newApp->appDuration = appDuration;
    newApp->avgTalkTime = avgTalkTime;
    assert(appDuration >= 0);
    newApp->appEndTime = newApp->appStartTime + newApp->appDuration;
    newApp->appMaxRetry = APP_PHONE_CALL_MAXIMUM_RETRY;
    newApp->appStatus = APP_PHONE_CALL_STATUS_PENDING;
    newApp->next = NULL;

    newApp->startCallTimer = NULL;
    newApp->endCallTimer = NULL;
    newApp->retryCallTimer = NULL;

    newApp->pktItvl = APP_PHONE_CALL_DEF_PKTITVL;
    newApp->dataRate = APP_PHONE_CALL_DEF_DATARATE;
    newApp->pktSize = (int)((newApp->dataRate*newApp->pktItvl)/(8*SECOND));

    //Setup distribution for avgTalk time generation at the SRC and dest
    newApp->avgTalkDurDis = new RandomDistribution<clocktype>();
    if (isAppSrc == TRUE)
    {
        newApp->avgTalkDurDis->setSeed(node->globalSeed,
                                       newApp->appSrcNodeId,
                                       newApp->appDestNodeId,
                                       (UInt32)newApp->appStartTime);
    }
    else
    {
        newApp->avgTalkDurDis->setSeed(node->globalSeed,
                                       newApp->appDestNodeId,
                                       newApp->appSrcNodeId,
                                       (UInt32)newApp->appEndTime);
    }
    newApp->avgTalkDurDis->setDistributionExponential(
                                    newApp->avgTalkTime);

    if (*appList == NULL)
    {
       *appList = newApp;
    }
    else
    {   currentApp = *appList;
        while (currentApp->next != NULL)
            currentApp = currentApp->next;
        currentApp->next = newApp;
    }

    //schedule a timer for start app event
    //Setup Call Start Timer with info on dest & call end time
    if (isAppSrc == TRUE)
    {
        callStartTimerMsg = MESSAGE_Alloc(
                                          node,
                                          APP_LAYER,
                                          APP_PHONE_CALL,
                                          MSG_APP_TimerExpired);
        MESSAGE_InfoAlloc(
                          node,
                          callStartTimerMsg,
                          sizeof(AppPhoneCallTimerInfo));

        info = (AppPhoneCallTimerInfo*)
            MESSAGE_ReturnInfo(callStartTimerMsg);

        info->appSrcNodeId = newApp->appSrcNodeId;
        info->appId = newApp->appId;
        info->timerType = APP_PHONE_CALL_START_TIMER;

        MESSAGE_Send(node, callStartTimerMsg, newApp->appStartTime);

        if (DEBUG_APP)
        {
            char buf1[MAX_STRING_LENGTH];
            char buf2[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(appStartTime, buf1);
            TIME_PrintClockInSecond(appDuration, buf2);
            printf(
                "node %d: PHONE CALL APP Add one new %d th app"
                "src %d dest %d app start %s duration %s\n",
                node->nodeId, appPhoneCallLayerData->numOriginApp,
                appSrcNodeId, appDestNodeId, buf1, buf2);
        }
    }
}

//**************************************************************
// /**
// FUNCTION   :: AppPhoneCallSchedEndTimer
// LAYER      :: APP
// PURPOSE    :: Schedule a call end timer
// PARAMETERS ::
// + node    : Node* : Pointer to node.
// + callApp : AppPhoneCallInfo* : Pointer to the phone call app
// + delay   : clocktype : delay for the timer
// RETURN     :: void : NULL
// **/
//*************************************************************
static
void AppPhoneCallSchedEndTimer(
        Node *node,
        AppPhoneCallInfo* callApp,
        clocktype delay)
{
    Message* callEndTimerMsg = MESSAGE_Alloc(
                                    node,
                                    APP_LAYER,
                                    APP_PHONE_CALL,
                                    MSG_APP_TimerExpired);

    MESSAGE_InfoAlloc(
        node,
        callEndTimerMsg,
        sizeof(AppPhoneCallTimerInfo));

    AppPhoneCallTimerInfo* info = (AppPhoneCallTimerInfo*)
                MESSAGE_ReturnInfo(callEndTimerMsg);
    info->appSrcNodeId = callApp->appSrcNodeId;
    info->appId = callApp->appId;
    info->timerType = APP_PHONE_CALL_END_TIMER;

    MESSAGE_Send(node, callEndTimerMsg, delay);
    if (callApp->endCallTimer)
    {
        MESSAGE_CancelSelfMsg(node, callApp->endCallTimer);
    }
    callApp->endCallTimer = callEndTimerMsg;

    if (DEBUG_APP)
    {
        printf("node %d: scchedule a call end timer \n",node->nodeId);
    }
}

//**************************************************************
// /**
// FUNCTION   :: AppPhoneCallSchedTalkStartTimer
// LAYER      :: APP
// PURPOSE    :: Schedule a call end timer
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + callApp : AppPhoneCallInfo* : Pointer to the phone call app
// + delay   : clocktype : delay for the timer
// RETURN     :: void : NULL
// **/
//*************************************************************
static
void AppPhoneCallSchedTalkStartTimer(
        Node *node,
        AppPhoneCallInfo* callApp,
        clocktype delay)
{
    callApp->talkStartTimer = MESSAGE_Alloc(
                                   node,
                                   APP_LAYER,
                                   APP_PHONE_CALL,
                                   MSG_APP_TimerExpired);
    MESSAGE_InfoAlloc(node,
                      callApp->talkStartTimer,
                      sizeof(AppPhoneCallTimerInfo));

    AppPhoneCallTimerInfo* timerInfo = (AppPhoneCallTimerInfo*)
                        MESSAGE_ReturnInfo(callApp->talkStartTimer);

    timerInfo->appSrcNodeId = callApp->appSrcNodeId;
    timerInfo->appId = callApp->appId;
    timerInfo->timerType = APP_PHONE_CALL_TALK_START_TIMER;

    MESSAGE_Send(node, callApp->talkStartTimer, delay);
}

//**************************************************************
// /**
// FUNCTION   :: AppPhoneCallStartTalking
// LAYER      :: APP
// PURPOSE    :: Start to generate voice packet and send them to lower layer
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + appPhoneCallLayerData : AppPhoneCallLayerData *:Pointer to the app data
// + callApp : AppPhoneCallInfo* : Pointer to the phone call app
// RETURN     :: void : NULL
// **/
//*************************************************************
static
void AppPhoneCallStartTalking(
        Node *node,
        AppPhoneCallLayerData *appPhoneCallLayerData,
        AppPhoneCallInfo* callApp)
{
    BOOL endSession;
    clocktype ts_now = getSimTime(node);
    if (ts_now + APP_PHONE_CALL_MIN_TALK_TIME > callApp->appEndTime)
    {
        endSession = TRUE;
    }
    else
    {
        int  numPkts;
        clocktype talkDur = callApp->avgTalkDurDis->getRandomNumber();
        if (talkDur < APP_PHONE_CALL_MIN_TALK_TIME)
        {
            talkDur = APP_PHONE_CALL_MIN_TALK_TIME;
        }

        if (ts_now + talkDur + APP_PHONE_CALL_MIN_TALK_TIME
                    <= callApp->appEndTime)
        {
            endSession = FALSE;
            numPkts = (int) (talkDur / callApp->pktItvl);
        }
        else if (ts_now + talkDur >= callApp->appEndTime)
        {
            numPkts = (int) ((callApp->appEndTime - ts_now)
                            / callApp->pktItvl);
            endSession = TRUE;
        }
        else
        {
            endSession = TRUE;
            numPkts = (int) (talkDur / callApp->pktItvl);
        }

        //Generating packets and send them to the network layer
        for (int i = 0; i < numPkts; i++)
        {
            Message* voicePkt = MESSAGE_Alloc(
                                    node,
                                    NETWORK_LAYER,
                                    NETWORK_PROTOCOL_CELLULAR,
                                    MSG_NETWORK_CELLULAR_FromAppPktArrival);
            MESSAGE_PacketAlloc(node, voicePkt, 0, TRACE_UMTS_LAYER3);
            MESSAGE_AddVirtualPayload(node, voicePkt, callApp->pktSize);

            MESSAGE_AddInfo(node,
                            voicePkt,
                            sizeof(AppPhoneCallData),
                            INFO_TYPE_AppPhoneCallData);
            AppPhoneCallData* voicePktData =
                (AppPhoneCallData*) MESSAGE_ReturnInfo(
                                        voicePkt,
                                        INFO_TYPE_AppPhoneCallData);

            voicePktData->numPkts = numPkts;
            voicePktData->TalkDurLeft = talkDur - i*callApp->pktItvl;
            voicePktData->ts_send = ts_now + (i+1) * callApp->pktItvl;

            MESSAGE_InfoAlloc(node,
                              voicePkt,
                              sizeof(AppPhoneCallPktArrivalInfo));
            AppPhoneCallPktArrivalInfo* callInfo =
                (AppPhoneCallPktArrivalInfo*) MESSAGE_ReturnInfo(voicePkt);
            callInfo->appId = callApp->appId;
            callInfo->appSrcNodeId = callApp->appSrcNodeId;

            MESSAGE_Send(node, voicePkt, (i+1)*callApp->pktItvl);
        }
        if (numPkts > 0)
        {
            callApp->curTalkEndTime = numPkts * callApp->pktItvl;
            callApp->appStatus = APP_PHONE_CALL_STATUS_TALKING;
            appPhoneCallLayerData->stats.numPktsSent += numPkts;
        }
    }
    if (endSession)
    {
        clocktype delayToEnd = (callApp->appEndTime > ts_now) ?
                        callApp->appEndTime - ts_now : 0;
        AppPhoneCallSchedEndTimer(
                node,
                callApp,
                delayToEnd);
    }
}

//**************************************************************
// /**
// FUNCTION   :: AppPhoneCallHandleCallStartTimer
// LAYER      :: APP
// PURPOSE    :: Handle call start timer expiration event
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the Callstart Msg
// RETURN     :: void : NULL
// **/
//*************************************************************
static
void AppPhoneCallHandleCallStartTimer(Node *node, Message *msg)
{
    Message *callStartMsgToNetwork;
    AppPhoneCallTimerInfo *infoIn;
    int appId;
    App_Phone_Call_Timer_Type timerType;
    AppPhoneCallStartMessageInfo *infoOut;
    AppPhoneCallLayerData *appPhoneCallLayerData;
    AppPhoneCallInfo *appList;
    AppPhoneCallInfo *retAppInfo;


    //get the appId associated with this timer event
    infoIn = (AppPhoneCallTimerInfo*) MESSAGE_ReturnInfo(msg);
    appId = infoIn->appId;
    timerType = infoIn->timerType;

    if (DEBUG_APP)
    {
        printf(
            "node %d: handle appStartTimer start for the appId is %d\n",
            node->nodeId,appId);
    }

    appPhoneCallLayerData = (AppPhoneCallLayerData *)
                                node->appData.phonecallVar;
    appList = appPhoneCallLayerData->appInfoOriginList;

    if (DEBUG_APP)
    {
        printf(
            "node %d: ready to Get APP Info, HandleCallStartTimer\n",
            node->nodeId);
    }

    if (AppPhoneCallGetApp(
        node, appId, node->nodeId, &appList, &retAppInfo) == TRUE)
    {
        retAppInfo->startCallTimer = NULL;
        callStartMsgToNetwork  = MESSAGE_Alloc(
                                    node,
                                    NETWORK_LAYER,
                                    NETWORK_PROTOCOL_CELLULAR,
                                    MSG_NETWORK_CELLULAR_FromAppStartCall);

        MESSAGE_InfoAlloc(
            node,
            callStartMsgToNetwork,
            sizeof(AppPhoneCallStartMessageInfo));

        infoOut = (AppPhoneCallStartMessageInfo *)
            MESSAGE_ReturnInfo(callStartMsgToNetwork);

        infoOut->appId = retAppInfo->appId;
        infoOut->appSrcNodeId = retAppInfo->appSrcNodeId;
        infoOut->appDestNodeId = retAppInfo->appDestNodeId;
        infoOut->callEndTime = retAppInfo->appEndTime;
        infoOut->avgTalkTime = retAppInfo->avgTalkTime;

        callStartMsgToNetwork->protocolType = NETWORK_PROTOCOL_CELLULAR;

        MESSAGE_Send(node, callStartMsgToNetwork, 0);

        //update the stats
        appPhoneCallLayerData->stats.numAppRequestSent ++;
        if (DEBUG_APP)
        {
            printf("node %d: Get app info appid %d"
                "src %d dest %d \n",
                node->nodeId,infoOut->appId,
                infoOut->appSrcNodeId, infoOut->appDestNodeId);
        }
        if (DEBUG_APP)
        {
            printf("node %d: appStartTimerProc end for the appId is %d\n",
                node->nodeId, appId);
        }
    }
    else
    {
        char errorBuf[MAX_STRING_LENGTH];
        sprintf(
            errorBuf,
            "node %d: cannot resovle the appId %d"
            "provide by the app layer\n",
            node->nodeId, appId);
        ERROR_Assert(FALSE, errorBuf);
    }
}

//*****************************************************************
// /**
// FUNCTION   :: AppPhoneCallHandleCallEndTimer
// LAYER      :: APP
// PURPOSE    :: Handle Call end timer expiration event
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the CallEnd Msg
// RETURN     :: void : NULL
// **/
//****************************************************************
static
void AppPhoneCallHandleCallEndTimer(Node *node, Message *msg)
{
    Message *callEndMsgToNetwork;
    AppPhoneCallTimerInfo* infoIn;
    int appId;
    App_Phone_Call_Timer_Type timerType;
    AppPhoneCallEndMessageInfo *infoOut;
    AppPhoneCallLayerData *appPhoneCallLayerData;
    AppPhoneCallInfo *appList;
    AppPhoneCallInfo *retAppInfo;


    //get the appId associated with this timer event
    infoIn = (AppPhoneCallTimerInfo*) MESSAGE_ReturnInfo(msg);

    appId = infoIn->appId;
    timerType = infoIn->timerType;

    if (DEBUG_APP)
    {
        printf("node %d: handle appEndTimer start for the appId is %d\n",
            node->nodeId, appId);
    }

    appPhoneCallLayerData = (AppPhoneCallLayerData *)
                                node->appData.phonecallVar;
    if (node->nodeId == infoIn->appSrcNodeId)
    {
        appList = appPhoneCallLayerData->appInfoOriginList;
    }
    else
    {
        appList = appPhoneCallLayerData->appInfoTerminateList;
    }

    if (DEBUG_APP)
    {
        printf(
            "node %d: ready to Get APP Info, HandleCallEndTimer\n",
            node->nodeId);
    }

    if (AppPhoneCallGetApp(
        node, appId, infoIn->appSrcNodeId, &appList, &retAppInfo) == TRUE)
    {

        callEndMsgToNetwork = MESSAGE_Alloc(
                                node,
                                NETWORK_LAYER,
                                NETWORK_PROTOCOL_CELLULAR,
                                MSG_NETWORK_CELLULAR_FromAppEndCall);

        MESSAGE_InfoAlloc(
            node,
            callEndMsgToNetwork,
            sizeof(AppPhoneCallEndMessageInfo));

        infoOut = (AppPhoneCallEndMessageInfo *)
                        MESSAGE_ReturnInfo(callEndMsgToNetwork);

        infoOut->appId = retAppInfo->appId;
        infoOut->appSrcNodeId = retAppInfo->appSrcNodeId;
        infoOut->appDestNodeId = retAppInfo->appDestNodeId;
        assert(retAppInfo->appDuration >= 0);

        callEndMsgToNetwork->protocolType = NETWORK_PROTOCOL_CELLULAR;
        MESSAGE_Send(node, callEndMsgToNetwork, 0);

        // cancel timer
        if (retAppInfo->retryCallTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(
                node,
                retAppInfo->retryCallTimer);

            retAppInfo->retryCallTimer = NULL;
        }
        if (retAppInfo->startCallTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(
                node,
                retAppInfo->startCallTimer);

            retAppInfo->startCallTimer = NULL;
        }
        if (retAppInfo->talkStartTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(
                node,
                retAppInfo->talkStartTimer);

            retAppInfo->talkStartTimer = NULL;
        }

        retAppInfo->endCallTimer = NULL;

        //update the stats
        appPhoneCallLayerData->stats.numAppSuccessEnd ++;

        if (node->nodeId == retAppInfo->appSrcNodeId)
        {
            appPhoneCallLayerData->stats.numOriginAppSuccessEnd ++;
        }
        else
        {
            appPhoneCallLayerData->stats.numTerminateAppSuccessEnd ++;
        }

        retAppInfo->appStatus = APP_PHONE_CALL_STATUS_SUCCESSED;
        if (DEBUG_APP)
        {
            printf(
                "node %d: Get app info appid %d"
                " src %d dest %d \n",
                node->nodeId,infoOut->appId,
                infoOut->appSrcNodeId, infoOut->appDestNodeId);
        }
        if (DEBUG_APP)
        {
            printf(
                "node %d: appEndTimerProc end for the appId is %d\n",
                node->nodeId, appId);
        }

    }
    else
    {
        char errorBuf[MAX_STRING_LENGTH];
        sprintf(
            errorBuf,
            "node %d: cannot resovle the appId %d"
            " provide by the app layer\n",
            node->nodeId,appId);
        ERROR_Assert(FALSE, errorBuf);
    }

}

//**************************************************************
// /**
// FUNCTION   :: AppPhoneCallHandleTalkStartTimer
// LAYER      :: APP
// PURPOSE    :: Handle talk start timer expiration event
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the Callstart Msg
// RETURN     :: void : NULL
// **/
//*************************************************************
static
void AppPhoneCallHandleTalkStartTimer(Node *node, Message *msg)
{
    AppPhoneCallLayerData *appPhoneCallLayerData;
    AppPhoneCallInfo *phoneCallApp;
    AppPhoneCallInfo *appList;

    AppPhoneCallTimerInfo *timerInfo =
        (AppPhoneCallTimerInfo*) MESSAGE_ReturnInfo(msg);
    appPhoneCallLayerData = (AppPhoneCallLayerData *)
                                node->appData.phonecallVar;

    if (node->nodeId == timerInfo->appSrcNodeId)
    {
        appList = appPhoneCallLayerData->appInfoOriginList;
    }
    else
    {
        appList = appPhoneCallLayerData->appInfoTerminateList;
    }
    if (AppPhoneCallGetApp(
            node,
            timerInfo->appId,
            timerInfo->appSrcNodeId,
            &(appList),
            &phoneCallApp) == TRUE)
    {
        phoneCallApp->talkStartTimer = NULL;
        clocktype ts_now = TIME_getSimTime(node);
        if (ts_now < phoneCallApp->nextTalkStartTime)
        {
            AppPhoneCallSchedTalkStartTimer(
                node,
                phoneCallApp,
                phoneCallApp->nextTalkStartTime - ts_now);
        }
        else
        {
            AppPhoneCallStartTalking(node,
                                     appPhoneCallLayerData,
                                     phoneCallApp);
        }
    }
}

//**************************************************************
// /**
// FUNCTION   :: AppPhoneCallHandleCallAcceptMsg
// LAYER      :: APP
// PURPOSE    :: Handle Call Accept msg from netowrk
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the call accept
// RETURN     :: void : NULL
// **/
//***************************************************************
static void
AppPhoneCallHandleCallAcceptMsg(Node *node, Message *msg)
{
    int appId;
    AppPhoneCallLayerData *appPhoneCallLayerData;
    AppPhoneCallInfo *appList;
    AppPhoneCallInfo *retAppInfo;

    AppPhoneCallAcceptMessageInfo* callAcceptInfo;

    callAcceptInfo =
        (AppPhoneCallAcceptMessageInfo *) MESSAGE_ReturnInfo(msg);

    appId = callAcceptInfo->appId;
    appPhoneCallLayerData = (AppPhoneCallLayerData *)
                                node->appData.phonecallVar;
    appList = appPhoneCallLayerData->appInfoOriginList;

    if (DEBUG_APP)
    {
        printf(
            "node %d: ready to Get APP Info, HandleCallAcceptMsg\n",
            node->nodeId);
    }
    if (AppPhoneCallGetApp(
        node, appId, node->nodeId, &appList, &retAppInfo) == TRUE)
    {
        AppPhoneCallStartTalking(node, appPhoneCallLayerData, retAppInfo);
    }
    appPhoneCallLayerData->stats.numAppAcceptedByNetwork ++;
}

//**********************************************************************
// /**
// FUNCTION   :: AppPhoneCallHandleCallDroppedMsg
// LAYER      :: APP
// PURPOSE    :: Handle Call Dropped msg from netowrk
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the call accept
// RETURN     :: void : NULL
// **/
//*************************************************************************
static
void AppPhoneCallHandleCallDroppedMsg(Node *node, Message *msg)
{
    AppPhoneCallDropMessageInfo *msgInfo;
    int appId;
    AppPhoneCallDropCauseType callDropCause;
    AppPhoneCallInfo *appList;
    AppPhoneCallInfo *retAppInfo;

    AppPhoneCallLayerData *appPhoneCallLayerData;

    appPhoneCallLayerData = (AppPhoneCallLayerData *)
                                node->appData.phonecallVar;

    msgInfo =
        (AppPhoneCallDropMessageInfo *)MESSAGE_ReturnInfo(msg);

    appId = msgInfo->appId;

    callDropCause = msgInfo->dropCause;
    
    //update the application status to dropped
    if (node->nodeId == msgInfo->appSrcNodeId)
    {
        appList = appPhoneCallLayerData->appInfoOriginList;
        if (AppPhoneCallGetApp(
            node, appId, msgInfo->appSrcNodeId,
            &appList, &retAppInfo) == TRUE)
        {
            if (retAppInfo->appStatus ==
                APP_PHONE_CALL_STATUS_SUCCESSED)
            {
                //if call end msg is  sent before rcv this call drop msg
                //do nothing
            }
            else
            {
                retAppInfo->appStatus = APP_PHONE_CALL_STATUS_FAILED;

                // cancel timer
                if (retAppInfo->retryCallTimer != NULL)
                {
                    MESSAGE_CancelSelfMsg(
                        node,
                        retAppInfo->retryCallTimer);

                    retAppInfo->retryCallTimer = NULL;
                }
                if (retAppInfo->startCallTimer != NULL)
                {
                    MESSAGE_CancelSelfMsg(
                        node,
                        retAppInfo->startCallTimer);

                    retAppInfo->startCallTimer = NULL;
                }
                if (retAppInfo->talkStartTimer != NULL)
                {
                    MESSAGE_CancelSelfMsg(
                        node,
                        retAppInfo->talkStartTimer);

                    retAppInfo->talkStartTimer = NULL;
                }
                if (retAppInfo->endCallTimer != NULL)
                {
                    MESSAGE_CancelSelfMsg(
                        node,
                        retAppInfo->endCallTimer);

                    retAppInfo->endCallTimer = NULL;
                }
                if (callDropCause ==
                    APP_PHONE_CALL_DROP_CAUSE_HANDOVER_FAILURE)
                {
                    appPhoneCallLayerData->
                        stats.numOriginAppDroppedCauseHandoverFailure ++;
                }
                else if (callDropCause ==
                    APP_PHONE_CALL_DROP_CAUSE_SELF_POWEROFF)
                {
                    appPhoneCallLayerData->
                        stats.numOriginAppDroppedCauseSelfPowerOff ++;
                }
                else if (callDropCause ==
                    APP_PHONE_CALL_DROP_CAUSE_REMOTEUSER_POWEROFF)
                {
                    appPhoneCallLayerData->
                        stats.numOriginAppDroppedCauseRemotePowerOff ++;
                }

                //update stats
                appPhoneCallLayerData->stats.numAppDropped ++;
                appPhoneCallLayerData->stats.numOriginAppDropped ++;

                //TODO:schedule a retry timer if required
                //if the user layer is enabled, let it know of this event
                //if the user layer is enabled, let it know of this event
                if (node->userData->enabled)
                {
                    USER_HandleCallUpdate(node,
                                          appId,
                                          USER_CALL_DROPPED);
                }
            } // if (retAppInfo->appStatus)
        } // if (AppPhoneCallGetApp()), app exist
    } // if (node->nodeId == msgInfo->appSrcNodeId);src of phone call

    else if (node->nodeId == msgInfo->appDestNodeId)
    {
        appList = appPhoneCallLayerData->appInfoTerminateList;

        if (AppPhoneCallGetApp(
            node, appId, msgInfo->appSrcNodeId,
            &appList, &retAppInfo) == TRUE)
        {
           if (retAppInfo->appStatus == APP_PHONE_CALL_STATUS_SUCCESSED)
           {
               //do nothing
           }
            else
            {
               retAppInfo->appStatus = APP_PHONE_CALL_STATUS_FAILED;

               if (callDropCause ==
                APP_PHONE_CALL_DROP_CAUSE_HANDOVER_FAILURE)
                {
                    appPhoneCallLayerData->
                        stats.numTerminateAppDroppedCauseHandoverFailure ++;
                }
                else if (callDropCause ==
                    APP_PHONE_CALL_DROP_CAUSE_SELF_POWEROFF)
                {
                    appPhoneCallLayerData->
                        stats.numTerminateAppDroppedCauseSelfPowerOff ++;
                }
                else if (callDropCause ==
                    APP_PHONE_CALL_DROP_CAUSE_REMOTEUSER_POWEROFF)
                {
                    appPhoneCallLayerData->
                        stats.numTerminateAppDroppedCauseRemotePowerOff ++;
                }

                //update stats
                appPhoneCallLayerData->stats.numAppDropped ++;
                appPhoneCallLayerData->stats.numTerminateAppDropped ++;
            } // if (retAppInfo->appStatus)
        } // if (AppPhoneCallGetApp()); app exist
    } // if (node->nodeId == msgInfo->appDestNodeId), dest of phone call

    if (DEBUG_APP)
    {
        printf(
            "node %d: ready to Get APP Info, HandleCallDroppedMsg\n",
            node->nodeId);
    }
}
//**********************************************************************
// /**
// FUNCTION   :: AppPhoneCallHandleCallRejectedMsg
// LAYER      :: APP
// PURPOSE    :: Handle Call Reject msg from netowrk
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the call accept
// RETURN     :: void : NULL
// **/
//*************************************************************************
static
void AppPhoneCallHandleCallRejectedMsg(Node *node, Message *msg)
{
    AppPhoneCallRejectMessageInfo *msgInfo;

    int appId;
    AppPhoneCallInfo *appList;
    AppPhoneCallInfo *retAppInfo;

    AppPhoneCallLayerData *appPhoneCallLayerData;

    appPhoneCallLayerData = (AppPhoneCallLayerData *)
                                node->appData.phonecallVar;

    msgInfo =
        (AppPhoneCallRejectMessageInfo *)MESSAGE_ReturnInfo(msg);

    //update the application status to rejected
    appList = appPhoneCallLayerData->appInfoOriginList;

    if (DEBUG_APP)
    {
        printf(
            "node %d: ready to Get APP Info, HandleCallRejectedMsg\n",
            node->nodeId);
    }

    appId = msgInfo->appId;

    if (AppPhoneCallGetApp(
        node, appId, node->nodeId, &appList, &retAppInfo) == TRUE)
    {
        // cancel timer
        if (retAppInfo->retryCallTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(
                node,
                retAppInfo->retryCallTimer);

            retAppInfo->retryCallTimer = NULL;
        }
        if (retAppInfo->startCallTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(
                node,
                retAppInfo->startCallTimer);

            retAppInfo->startCallTimer = NULL;
        }
        if (retAppInfo->talkStartTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(
                node,
                retAppInfo->talkStartTimer);

            retAppInfo->talkStartTimer = NULL;
        }
        if (retAppInfo->endCallTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(
                node,
                retAppInfo->endCallTimer);

            retAppInfo->endCallTimer = NULL;
        }
        retAppInfo->appStatus = APP_PHONE_CALL_STATUS_REJECTED;
    }

    //TODO:schedule a retry timer if required

    appPhoneCallLayerData->stats.numAppRejectedByNetwork ++;

    if (msgInfo->rejectCause ==
        APP_PHONE_CALL_REJECT_CAUSE_SYSTEM_BUSY)
    {
        appPhoneCallLayerData->stats.numAppRejectedCauseSystemBusy ++;
        if (DEBUG_APP)
        {
            printf("node %d: SYSTEM BUSY\n", node->nodeId);
        }
    }
    else if (msgInfo->rejectCause ==
        APP_PHONE_CALL_REJECT_CAUSE_USER_BUSY)
    {
        appPhoneCallLayerData->stats.numAppRejectedCauseUserBusy ++;
        if (DEBUG_APP)
        {
            printf("node %d: USER BUSY\n", node->nodeId);
        }
    }
    else if (msgInfo->rejectCause ==
        APP_PHONE_CALL_REJECT_CAUSE_TOO_MANY_ACTIVE_APP)
    {
        appPhoneCallLayerData->stats.numAppRejectedCauseTooManyActiveApp ++;
    }
    else if (msgInfo->rejectCause ==
        APP_PHONE_CALL_REJECT_CAUSE_USER_UNREACHABLE)
    {
        appPhoneCallLayerData->stats.numAppRejectedCauseUserUnreachable ++;
    }
    else if (msgInfo->rejectCause ==
        APP_PHONE_CALL_REJECT_CAUSE_USER_POWEROFF)
    {
        appPhoneCallLayerData->stats.numAppRejectedCauseUserPowerOff ++;
    }
    else if (msgInfo->rejectCause ==
        APP_PHONE_CALL_REJECT_CAUSE_NETWORK_NOT_FOUND)
    {
        appPhoneCallLayerData->stats.numAppRejectedCauseNetworkNotFound ++;
        if (DEBUG_APP)
        {
            printf("node %d: NETWORK NOT FOUND\n", node->nodeId);
        }
    }
    else if (msgInfo->rejectCause ==
        APP_PHONE_CALL_REJECT_CAUSE_UNSUPPORTED_SERVICE)
    {
        appPhoneCallLayerData->stats.numAppRejectedCauseUnsupportService ++;
    }
    else if (msgInfo->rejectCause ==
        APP_PHONE_CALL_REJECT_CAUSE_UNKNOWN_USER)
    {
        appPhoneCallLayerData->stats.numAppRejectedCauseUnknowUser ++;
        if (DEBUG_APP)
        {
            printf("node %d: UNKNOWN USER\n",node->nodeId);
        }
    }
}

//**********************************************************************
// /**
// FUNCTION   :: AppPhoneCallHandlePktArrival
// LAYER      :: APP
// PURPOSE    :: Handle Packet arrival from the lower layer
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the call accept
// RETURN     :: void : NULL
// **/
//*************************************************************************
static
void AppPhoneCallHandlePktArrival(Node *node, Message *msg)
{
    AppPhoneCallPktArrivalInfo* callInfo =
        (AppPhoneCallPktArrivalInfo*)MESSAGE_ReturnInfo(msg);
    AppPhoneCallInfo *phoneCallApp;

    AppPhoneCallLayerData *appPhoneCallLayerData
                 = (AppPhoneCallLayerData *)node->appData.phonecallVar;

    AppPhoneCallInfo* appList;
    if (node->nodeId == callInfo->appSrcNodeId)
    {
        appList = appPhoneCallLayerData->appInfoOriginList;
    }
    else
    {
        appList = appPhoneCallLayerData->appInfoTerminateList;
    }
    if (AppPhoneCallGetApp(
            node,
            callInfo->appId,
            callInfo->appSrcNodeId,
            &appList,
            &phoneCallApp))
    {
        clocktype ts_now = TIME_getSimTime(node);
        AppPhoneCallData* voicePktData =
            (AppPhoneCallData*) MESSAGE_ReturnInfo(
                                    msg,
                                    INFO_TYPE_AppPhoneCallData);

        appPhoneCallLayerData->stats.numPktsRcvd++;
        appPhoneCallLayerData->stats.totEndToEndDelay +=
                    (ts_now - voicePktData->ts_send);
        if (phoneCallApp->appStatus == APP_PHONE_CALL_STATUS_ONGOING ||
            (phoneCallApp->appStatus == APP_PHONE_CALL_STATUS_TALKING &&
             phoneCallApp->curTalkEndTime < ts_now))
        {
            phoneCallApp->appStatus = APP_PHONE_CALL_STATUS_LISTENING;

            //Start timer to start talking
            AppPhoneCallSchedTalkStartTimer(
                    node,
                    phoneCallApp,
                    voicePktData->TalkDurLeft);
            phoneCallApp->nextTalkStartTime =
                ts_now + voicePktData->TalkDurLeft
                       + APP_PHONE_CALL_TALK_TURNAROUND_TIME;
        }
        else if (phoneCallApp->appStatus == APP_PHONE_CALL_STATUS_LISTENING)
        {
            if (!phoneCallApp->talkStartTimer)
            {
                AppPhoneCallSchedTalkStartTimer(
                        node,
                        phoneCallApp,
                        voicePktData->TalkDurLeft);
                phoneCallApp->nextTalkStartTime =
                    ts_now + voicePktData->TalkDurLeft
                           + APP_PHONE_CALL_TALK_TURNAROUND_TIME;
            }
            else
            {
                if (ts_now + voicePktData->TalkDurLeft
                           + APP_PHONE_CALL_TALK_TURNAROUND_TIME
                           > phoneCallApp->nextTalkStartTime)
                {
                    phoneCallApp->nextTalkStartTime =
                        ts_now + voicePktData->TalkDurLeft
                               + APP_PHONE_CALL_TALK_TURNAROUND_TIME;
                }
            } // if (phoneCallApp->talkStartTimer); timer is active
        } // if (phoneCallApp->appStatus)
    } // if (AppPhoneCallGetApp()), app exists
}

//***********************************************************************
// /**
// FUNCTION   :: AppPhoneCallInitStats
// LAYER      :: APP
// PURPOSE    :: Init the statistic of app layer
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + nodeInput  : NodeInput : Point to the input
// RETURN     :: void : NULL
// **/
//***********************************************************************
static
void AppPhoneCallInitStats(Node *node, const NodeInput *nodeInput)
{
    if (DEBUG_APP)
    {
        printf("node %d: PHONE-CALL APP Init Stat\n", node->nodeId);
    }
}

//***********************************************************************
// /**
// FUNCTION   :: AppPhoneCallInit
// LAYER      :: APP
// PURPOSE    :: Init an application.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
//**********************************************************************
void AppPhoneCallInit(Node *node, const NodeInput *nodeInput)
{
    AppPhoneCallLayerData *appPhoneCallLayerData;
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;

    if (DEBUG_APP)
    {
        printf("node %d: PHONE-CALL APP Init\n",node->nodeId);
    }

    appPhoneCallLayerData =
        (AppPhoneCallLayerData *)
            MEM_malloc(sizeof(AppPhoneCallLayerData));

    memset(appPhoneCallLayerData,
           0,
           sizeof(AppPhoneCallLayerData));
    appPhoneCallLayerData->appInfoOriginList = NULL;
    appPhoneCallLayerData->appInfoTerminateList = NULL;
    appPhoneCallLayerData->numOriginApp = 0;
    appPhoneCallLayerData->numTerminateApp = 0;


    node->appData.phonecallVar = (void *) appPhoneCallLayerData;

    //init stats
    AppPhoneCallInitStats(node, nodeInput);

    IO_ReadString(
           node->nodeId,
           ANY_DEST,
           nodeInput,
           "CELLULAR-STATISTICS",
           &retVal,
           buf);

    if ((retVal == FALSE) || (strcmp(buf, "YES") == 0))
    {
        appPhoneCallLayerData->collectStatistics = TRUE;
    }
    else if (retVal && strcmp(buf, "NO") == 0)
    {
        appPhoneCallLayerData->collectStatistics = FALSE;
    }
    else
    {
        ERROR_ReportWarning(
            "Value of CELLULAR-STATISTICS should be YES or NO! "
            "Default value YES is used.");
        appPhoneCallLayerData->collectStatistics = TRUE;
    }
}

//***********************************************************************
// /**
// FUNCTION   :: AppPhoneCallFinalize
// LAYER      :: APP
// PURPOSE    :: Print stats and clear protocol variables
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// RETURN     :: void : NULL
// **/
//**********************************************************************
void AppPhoneCallFinalize(Node *node)
{
    if (DEBUG_APP)
    {
        printf("node %d: APP Layer finalize the node \n", node->nodeId);
    }

    AppPhoneCallLayerData *appPhoneCallLayerData;
    char buf[MAX_STRING_LENGTH];

    appPhoneCallLayerData =
        (AppPhoneCallLayerData *)
            node->appData.phonecallVar;

    if (!appPhoneCallLayerData->collectStatistics)
    {
        return;
    }

    if (appPhoneCallLayerData->numOriginApp == 0 &&
        appPhoneCallLayerData->numTerminateApp == 0)
    {
        return;
    }
    sprintf(
        buf,
        "Number of app request sent to layer 3 = %d",
        appPhoneCallLayerData->stats.numAppRequestSent);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request rcvd = %d",
        appPhoneCallLayerData->stats.numAppRequestRcvd);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request accepted = %d",
        appPhoneCallLayerData->stats.numAppAcceptedByNetwork);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request rejected = %d",
        appPhoneCallLayerData->stats.numAppRejectedByNetwork);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (casue: System Busy) = %d",
        appPhoneCallLayerData->stats.numAppRejectedCauseSystemBusy);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (cause: Network Not Found) = %d",
        appPhoneCallLayerData->stats.numAppRejectedCauseNetworkNotFound);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (cause: Too Many Active App) = %d",
        appPhoneCallLayerData->stats.numAppRejectedCauseTooManyActiveApp);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (cause: Unknown User) = %d",
        appPhoneCallLayerData->stats.numAppRejectedCauseUnknowUser);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (cause: User Power Off) = %d",
        appPhoneCallLayerData->stats.numAppRejectedCauseUserPowerOff);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (cause: User Busy) = %d",
        appPhoneCallLayerData->stats.numAppRejectedCauseUserBusy);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (cause: Unsupported Service) = %d",
        appPhoneCallLayerData->stats.numAppRejectedCauseUnsupportService);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (cause: User Unreachable) = %d",
        appPhoneCallLayerData->stats.numAppRejectedCauseUserUnreachable);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app successfully end = %d",
        appPhoneCallLayerData->stats.numAppSuccessEnd);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);
    sprintf(
        buf,
        "Number of originating app successfully end = %d",
        appPhoneCallLayerData->stats.numOriginAppSuccessEnd);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);
    sprintf(
        buf,
        "Number of terminating app successfully end = %d",
        appPhoneCallLayerData->stats.numTerminateAppSuccessEnd);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app dropped  = %d",
        appPhoneCallLayerData->stats.numAppDropped);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of originating app dropped  = %d",
        appPhoneCallLayerData->stats.numOriginAppDropped);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of originating app dropped (Cause: Handover Failure)  = %d",
        appPhoneCallLayerData->
            stats.numOriginAppDroppedCauseHandoverFailure);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of originating app dropped (Cause: Self PowerOff)  = %d",
        appPhoneCallLayerData->
            stats.numOriginAppDroppedCauseSelfPowerOff);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);
    sprintf(
        buf,
        "Number of originating app dropped (Cause: Remote PowerOff)  = %d",
        appPhoneCallLayerData->
            stats.numOriginAppDroppedCauseRemotePowerOff);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);
     sprintf(
        buf,
        "Number of terminating app dropped  = %d",
        appPhoneCallLayerData->stats.numTerminateAppDropped);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of terminating app dropped (Cause: Handover Faliure)  = %d",
        appPhoneCallLayerData->
            stats.numTerminateAppDroppedCauseHandoverFailure);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of terminating app dropped (Cause: Self PowerOff)  = %d",
        appPhoneCallLayerData->
            stats.numTerminateAppDroppedCauseSelfPowerOff);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of terminating app dropped (Cause: Remote PowerOff)  = %d",
        appPhoneCallLayerData->
            stats.numTerminateAppDroppedCauseRemotePowerOff);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of voice packets sent  = %d",
        appPhoneCallLayerData->stats.numPktsSent);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of voice packets received  = %d",
        appPhoneCallLayerData->stats.numPktsRcvd);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    char delayBuf[MAX_STRING_LENGTH];
    clocktype avgEndToEndDelay = 0;
    if (appPhoneCallLayerData->stats.numPktsRcvd > 0)
    {
        avgEndToEndDelay =
            appPhoneCallLayerData->stats.totEndToEndDelay /
            appPhoneCallLayerData->stats.numPktsRcvd;
    }
    TIME_PrintClockInSecond(
        avgEndToEndDelay,
        delayBuf);
    sprintf(
        buf,
        "Average end to end delay (seconds) = %s",
        delayBuf);
    IO_PrintStat(
                node,
                "Application",
                "PHONE-CALL",
                ANY_DEST,
                node->nodeId,
                buf);
}

//*********************************************************************
// /**
// FUNCTION   :: AppPhoneCallLayer
// LAYER      :: APPLICATION
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
//***********************************************************************
void AppPhoneCallLayer(Node *node, Message *msg)
{
    switch (msg->eventType)
    {
        //message from application layer itself,e.g., timer
        case MSG_APP_TimerExpired:
        {
            AppPhoneCallTimerInfo *info;
            info = (AppPhoneCallTimerInfo*) MESSAGE_ReturnInfo(msg);
            switch(info->timerType)
            {
                case APP_PHONE_CALL_START_TIMER:
                {
                    if (DEBUG_APP)
                    {
                        printf(
                            "node %d: PHONE-CALL APP handle appStartTimer\n",
                            node->nodeId);
                    }
                    AppPhoneCallHandleCallStartTimer(node, msg);
                    break;
                }
                case APP_PHONE_CALL_END_TIMER:
                {
                    if (DEBUG_APP)
                    {
                        printf(
                            "node %d: PHONE-CALL APP handle appEndTimer\n",
                            node->nodeId);
                    }
                    //if the user layer is enabled,let it know of this event
                    if (node->userData->enabled)
                    {
                        USER_HandleCallUpdate(node,
                                              info->appId,
                                              USER_CALL_COMPLETED);
                    }
                    AppPhoneCallHandleCallEndTimer(node, msg);
                    break;
                }
                case APP_PHONE_CALL_RETRY_TIMER:
                {
                    if (DEBUG_APP)
                    {
                        printf(
                            "node %d: PHONE-CALL APP handle appRetryTimer\n",
                            node->nodeId);
                    }

                    //update the application status to success
                    break;
                }
                case APP_PHONE_CALL_TALK_START_TIMER:
                {
                    AppPhoneCallHandleTalkStartTimer(node, msg);
                }
            }
            break;
        }

        //message from network layer
        case MSG_APP_CELLULAR_FromNetworkCallAccepted:
        {
            if (DEBUG_APP)
            {
                printf(
                    "node %d: PHONE-CALL APP handle"
                    "FromNetworkCallAccepted\n",
                    node->nodeId);
            }
            AppPhoneCallHandleCallAcceptMsg(node, msg);

            break;
        }

        case MSG_APP_CELLULAR_FromNetworkCallRejected:
        {
            if (DEBUG_APP)
            {
                printf(
                    "node %d: PHONE-CALL APP handle"
                    "APP_FROM_NETWORK_REJECTED\n",
                    node->nodeId);
            }
            //if the user layer is enabled, let it know of this event
            if (node->userData->enabled)
            {
                AppPhoneCallRejectMessageInfo *rejectedInfo;
                rejectedInfo = (AppPhoneCallRejectMessageInfo*)
                                    MESSAGE_ReturnInfo(msg);
                USER_HandleCallUpdate(node,
                                      rejectedInfo->appId,
                                      USER_CALL_REJECTED);
            }

            AppPhoneCallHandleCallRejectedMsg(node, msg);
            break;
        }

        case MSG_APP_CELLULAR_FromNetworkCallArrive:
        {
            if (DEBUG_APP)
            {
                printf(
                    "node %d: PHONE-CALL APP handle"
                    "APP_FROM_NETWORK_REMOTE_CALL_ARRIVE\n",
                    node->nodeId);
            }

             //add the app to terminating app list
            AppPhoneCallLayerData *appPhoneCallLayerData;
            AppPhoneCallInfo *retAppInfo;

            appPhoneCallLayerData =
                (AppPhoneCallLayerData *)
                    node->appData.phonecallVar;
            AppPhoneCallArriveMessageInfo *callArriveInfo =
                                (AppPhoneCallArriveMessageInfo *)
                                MESSAGE_ReturnInfo(msg);

            clocktype ts_now = TIME_getSimTime(node);
            clocktype duration = callArriveInfo->callEndTime > ts_now ?
                                callArriveInfo->callEndTime - ts_now : 0;

            AppPhoneCallInsertList(
                node,
                &(appPhoneCallLayerData->appInfoTerminateList),
                callArriveInfo->appId,
                callArriveInfo->appSrcNodeId,
                callArriveInfo->appDestNodeId,
                callArriveInfo->avgTalkTime,
                TIME_getSimTime(node),
                duration,
                FALSE);

            //TODO update app status to ongoing

            if (AppPhoneCallGetApp(
                node,
                callArriveInfo->appId,
                callArriveInfo->appSrcNodeId,
                &(appPhoneCallLayerData->appInfoTerminateList),
                &retAppInfo) == TRUE)
            {
                retAppInfo->appStatus = APP_PHONE_CALL_STATUS_ONGOING;
            }

            //accept the incoming call
            appPhoneCallLayerData->stats.numAppRequestRcvd ++;

            Message *answerMsg;
            AppPhoneCallAnsweredMessageInfo *callAnswerInfo;

            answerMsg = MESSAGE_Alloc(
                            node,
                            NETWORK_LAYER,
                            NETWORK_PROTOCOL_CELLULAR,
                            MSG_NETWORK_CELLULAR_FromAppCallAnswered);

            MESSAGE_InfoAlloc(
                node,
                answerMsg,
                sizeof(AppPhoneCallAnsweredMessageInfo));

            callAnswerInfo =
                (AppPhoneCallAnsweredMessageInfo*)
                    MESSAGE_ReturnInfo(answerMsg);

            callAnswerInfo->appId = callArriveInfo->appId;
            callAnswerInfo->transactionId = callArriveInfo->transactionId;
            callAnswerInfo->appSrcNodeId = callArriveInfo->appSrcNodeId;
            callAnswerInfo->appDestNodeId = callArriveInfo->appDestNodeId;

            //Usually it takes a user serveral seconds to answer a phone 
            MESSAGE_Send(node, answerMsg, 3*SECOND);

            break;
        }

        case MSG_APP_CELLULAR_FromNetworkCallEndByRemote:
        {

            AppPhoneCallEndByRemoteMessageInfo *msgInfo;
            int appId;
            NodeAddress srcNodeId;
            NodeAddress destNodeId;
            AppPhoneCallLayerData *appPhoneCallLayerData;
            AppPhoneCallInfo *retAppInfo;

            msgInfo =
                (AppPhoneCallEndByRemoteMessageInfo *)
                    MESSAGE_ReturnInfo(msg);
            appPhoneCallLayerData =
                (AppPhoneCallLayerData *)
                    node->appData.phonecallVar;

            appId = msgInfo->appId;
            srcNodeId = msgInfo->appSrcNodeId;
            destNodeId = msgInfo->appDestNodeId;

            if (node->nodeId == srcNodeId)
            {
                //update the application status to success
                if (AppPhoneCallGetApp(
                    node,
                    appId,
                    srcNodeId,
                    &(appPhoneCallLayerData->appInfoOriginList),
                    &retAppInfo) == TRUE)
                {
                    // cancel timer
                    if (retAppInfo->retryCallTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->retryCallTimer);

                        retAppInfo->retryCallTimer = NULL;
                    }
                    if (retAppInfo->startCallTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->startCallTimer);

                        retAppInfo->startCallTimer = NULL;
                    }
                    if (retAppInfo->talkStartTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->talkStartTimer);

                        retAppInfo->talkStartTimer = NULL;
                    }
                    if (retAppInfo->endCallTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->endCallTimer);

                        retAppInfo->endCallTimer = NULL;
                    }
                    retAppInfo->appStatus = APP_PHONE_CALL_STATUS_SUCCESSED;
                }

                appPhoneCallLayerData->stats.numOriginAppSuccessEnd ++;
                if (DEBUG_APP)
                {
                    printf(
                        "node %d: MO/FO call ended by "
                        "remote user--APP DEST\n",
                        node->nodeId);
                }
            }
            else if (node->nodeId == destNodeId)
            {
                //update the application status to success
                if (AppPhoneCallGetApp(
                    node,
                    appId,
                    srcNodeId,
                    &(appPhoneCallLayerData->appInfoTerminateList),
                    &retAppInfo) == TRUE)
                {
                    // cancel timer
                    if (retAppInfo->retryCallTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->retryCallTimer);

                        retAppInfo->retryCallTimer = NULL;
                    }
                    if (retAppInfo->startCallTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->startCallTimer);

                        retAppInfo->startCallTimer = NULL;
                    }
                    if (retAppInfo->talkStartTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->talkStartTimer);

                        retAppInfo->talkStartTimer = NULL;
                    }
                    if (retAppInfo->endCallTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->endCallTimer);

                        retAppInfo->endCallTimer = NULL;
                    }
                    retAppInfo->appStatus = APP_PHONE_CALL_STATUS_SUCCESSED;
                }

                if (node->nodeId == retAppInfo->appSrcNodeId)
                {
                    appPhoneCallLayerData->stats.numOriginAppSuccessEnd ++;
                }
                else
                {
                    appPhoneCallLayerData->stats.numTerminateAppSuccessEnd ++;
                }
                if (DEBUG_APP)
                {
                    printf(
                        "node %d: MT/FT call ended by "
                        "remote user--APP SRC\n",
                        node->nodeId);
                }
            }

            //update stats
            appPhoneCallLayerData->stats.numAppSuccessEnd ++;
            if (DEBUG_APP)
            {
                printf(
                    "node %d: PHONE APP handle "
                    "CELLULAR_APP_FROM_NETWORK_REMOTE_CALL_END\n",
                    node->nodeId);
            }
            break;
        }

        case MSG_APP_CELLULAR_FromNetworkCallDropped:
        {

            if (DEBUG_APP)
            {
                printf(
                    "node %d: PHONE-CALL APP handle"
                    "CELLULAR_APP_FROM_NETWORK_DROPPED\n",
                    node->nodeId);

            }

            AppPhoneCallHandleCallDroppedMsg(node, msg);
            break;
        }

        case MSG_APP_CELLULAR_FromNetworkPktArrive:
        {
            AppPhoneCallHandlePktArrival(node, msg);
            break;
        }
        default:
        {
            printf(
                "PHONE-CALL APP: Protocol = %d\n",
                MESSAGE_GetProtocol(msg));
            assert(FALSE);
            abort();
            break;
        }
    }

    MESSAGE_Free(node, msg);
}
