// Copyright (c) 2001-2009, Scalable Network Technologies, Inc.  All Rights Reserved.
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

/*
 * This file contains initialization function, message processing
 * function, and finalize function used by telnet application.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "transport_tcp.h"
#include "tcpapps.h"
#include "app_util.h"
#include "app_telnet.h"
#define noDEBUG

/*
 * NAME:        AppLayerTelnetClient.
 * PURPOSE:     Models the behaviour of TelnetClient Client on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  node - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
AppLayerTelnetClient(Node *node, Message *msg)
{
    char buf[MAX_STRING_LENGTH];
    AppDataTelnetClient *clientPtr;

    ctoa(getSimTime(node), buf);

    switch (msg->eventType)
    {
        case MSG_APP_FromTransOpenResult:
        {
            TransportToAppOpenResult *openResult;

            openResult = (TransportToAppOpenResult *)MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("%s: node %u got OpenResult\n", buf, node->nodeId);
#endif /* DEBUG */

            assert(openResult->type == TCP_CONN_ACTIVE_OPEN);

            if (openResult->connectionId < 0)
            {
                node->appData.numAppTcpFailure ++;
            }
            else
            {
                AppDataTelnetClient *clientPtr;
                clientPtr = AppTelnetClientUpdateTelnetClient(node,
                                                              openResult);
                assert(clientPtr != NULL);
                AppTelnetClientScheduleNextPkt(node, clientPtr);
            }

            break;
        }
        case MSG_APP_FromTransDataSent:
        {
            TransportToAppDataSent *dataSent;

            dataSent = (TransportToAppDataSent *) MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("%s: node %u sent data %ld\n",
                   buf, node->nodeId, dataSent->length);
#endif /* DEBUG */

            clientPtr = AppTelnetClientGetTelnetClient(node,
                                        dataSent->connectionId);
            assert(clientPtr != NULL);

            clientPtr->numBytesSent += (clocktype) dataSent->length;
            clientPtr->lastItemSent = getSimTime(node);

            if (clientPtr->sessionIsClosed == FALSE)
            {
                AppTelnetClientScheduleNextPkt(node, clientPtr);
            }
            else
            {
                APP_TcpCloseConnection(
                    node,
                    clientPtr->connectionId);
            }

            break;
        }
        case MSG_APP_FromTransDataReceived:
        {
            TransportToAppDataReceived *dataRecvd;

            dataRecvd = (TransportToAppDataReceived *)MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("%s: node %u received data %ld\n",
                   buf, node->nodeId, msg->packetSize);
#endif /* DEBUG */

            clientPtr = AppTelnetClientGetTelnetClient(node,
                                                 dataRecvd->connectionId);

            assert(clientPtr != NULL);

            clientPtr->numBytesRecvd += (clocktype) msg->packetSize;

            break;
        }
        case MSG_APP_TimerExpired:
        {
            AppTimer *timer;

            timer = (AppTimer *) MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("%s: node %u got timer %d\n",
                   buf, node->nodeId, timer->type);
#endif /* DEBUG */

            clientPtr = AppTelnetClientGetTelnetClient(node,
                                                       timer->connectionId);

            assert(clientPtr != NULL);

            switch (timer->type)
            {
                case APP_TIMER_SEND_PKT:
                {
                    if (clientPtr->sessionIsClosed == FALSE)
                    {
                        APP_TcpSendData(
                            node,
                            clientPtr->connectionId,
                            (char *)"d",
                            1,
                            TRACE_TELNET);
                    }

                    break;
                }
                case APP_TIMER_CLOSE_SESS:
                {
                    if (clientPtr->sessionIsClosed == FALSE)
                    {
                        APP_TcpSendData(
                            node,
                            clientPtr->connectionId,
                            (char *)"c",
                            1,
                            TRACE_TELNET);
                    }

                    clientPtr->sessionIsClosed = TRUE;

                    break;
                }
                default:
                    assert(FALSE);
            }

            break;
        }
        case MSG_APP_FromTransCloseResult:
        {
            TransportToAppCloseResult *closeResult;

            closeResult = (TransportToAppCloseResult *)
                          MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("%s: node %u got close result\n",
                   buf, node->nodeId);
#endif /* DEBUG */

            clientPtr = AppTelnetClientGetTelnetClient(node,
                                                 closeResult->connectionId);
            assert(clientPtr != NULL);

            if (clientPtr->sessionIsClosed == FALSE)
            {
                clientPtr->sessionIsClosed = TRUE;
                clientPtr->sessionFinish = getSimTime(node);
            }

            break;
        }
        default:
            ctoa(getSimTime(node), buf);
            printf("Time %s: Node %u received message of unknown type"
                   " %d.\n", buf, node->nodeId, msg->eventType);
#ifndef EXATA
            assert(FALSE);
#endif
    }

    MESSAGE_Free(node, msg);
}

/*
 * NAME:        AppTelnetClientInit.
 * PURPOSE:     Initialize a TelnetClient session.
 * PARAMETERS:  node - pointer to the node,
 *              serverAddr - address of the server,
 *              sessDuration - length of telnet session.
 *              waitTime - time until the session starts.
 * RETURN:      none.
 */
void
AppTelnetClientInit(
    Node *node,
    Address clientAddr,
    Address serverAddr,
    clocktype sessDuration,
    clocktype waitTime)
{
    AppDataTelnetClient *clientPtr;

    clientPtr = AppTelnetClientNewTelnetClient(
                    node,
                    clientAddr,
                    serverAddr,
                    sessDuration);

    if (clientPtr == NULL)
    {
        printf("TELNET Client: Node %d cannot allocate "
               "new telnet client\n", node->nodeId);

        assert(FALSE);
    }

    APP_TcpOpenConnectionWithPriority(
        node,
        APP_TELNET_CLIENT,
        clientAddr,
        node->appData.nextPortNum++,
        serverAddr,
        (short) APP_TELNET_SERVER,
        clientPtr->uniqueId,
        waitTime,
        APP_DEFAULT_TOS);
}

/*
 * NAME:        AppTelnetClientFinalize.
 * PURPOSE:     Collect statistics of a TelnetClient session.
 * PARAMETERS:  node - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
AppTelnetClientFinalize(Node *node, AppInfo* appInfo)
{
    AppDataTelnetClient* clientPtr =
        (AppDataTelnetClient*)appInfo->appDetail;

    if (node->appData.appStats == TRUE) {
        AppTelnetClientPrintStats(node, clientPtr);
    }
}

/*
 * NAME:        AppTelnetClientPrintStats.
 * PURPOSE:     Prints statistics of a TelnetClient session.
 * PARAMETERS:  node - pointer to the node.
 *              clientPtr - pointer to the telnet client data structure.
 * RETURN:      none.
 */
void //inline//
AppTelnetClientPrintStats(Node *node, AppDataTelnetClient *clientPtr)
{
    clocktype throughput;
    char addrStr[MAX_STRING_LENGTH];
    char startStr[MAX_STRING_LENGTH];
    char finishStr[MAX_STRING_LENGTH];
    char statusStr[MAX_STRING_LENGTH];
    char numBytesSentStr[MAX_STRING_LENGTH];
    char numBytesRecvdStr[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char throughputStr[MAX_STRING_LENGTH];

    TIME_PrintClockInSecond(clientPtr->sessionStart, startStr);
    TIME_PrintClockInSecond(clientPtr->lastItemSent, finishStr);

    if (clientPtr->connectionId < 0)
    {
        sprintf(statusStr, "Failed");
    }
    else if (clientPtr->sessionIsClosed == FALSE)
    {
        clientPtr->sessionFinish = getSimTime(node);
        sprintf(statusStr, "Not closed");
    }
    else
    {
        sprintf(statusStr, "Closed");
    }

    if (clientPtr->sessionFinish <= clientPtr->sessionStart)
    {
        throughput = 0;
    }
    else
    {
        throughput =
            (clocktype) ((clientPtr->numBytesSent * 8.0 * SECOND)
            / (clientPtr->sessionFinish - clientPtr->sessionStart));
    }

    ctoa(throughput, throughputStr);
    IO_ConvertIpAddressToString(&clientPtr->remoteAddr, addrStr);
    sprintf(buf, "Server address = %s", addrStr);
    IO_PrintStat(
        node,
        "Application",
        "Telnet Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    sprintf(buf, "First Packet Sent at (s) = %s", startStr);
    IO_PrintStat(
        node,
        "Application",
        "Telnet Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    sprintf(buf, "Last Packet Sent at (s) = %s", finishStr);
    IO_PrintStat(
        node,
        "Application",
        "Telnet Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    sprintf(buf, "Session Status = %s", statusStr);
    IO_PrintStat(
        node,
        "Application",
        "Telnet Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    ctoa(clientPtr->numBytesSent, numBytesSentStr);
    sprintf(buf, "Total Bytes Sent = %s", numBytesSentStr);
    IO_PrintStat(
        node,
        "Application",
        "Telnet Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    ctoa(clientPtr->numBytesRecvd, numBytesRecvdStr);
    sprintf(buf, "Total Bytes Received = %s", numBytesRecvdStr);
    IO_PrintStat(
        node,
        "Application",
        "Telnet Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);

    sprintf(buf, "Throughput (bits/s) = %s", throughputStr);
    IO_PrintStat(
        node,
        "Application",
        "Telnet Client",
        ANY_DEST,
        clientPtr->connectionId,
        buf);
}

/*
 * NAME:        AppTelnetClientGetTelnetClient.
 * PURPOSE:     search for a telnet client data structure.
 * PARAMETERS:  node - pointer to the node,
 *              connId - connection ID of the telnet client.
 * RETURN:      the pointer to the telnet client data structure,
 *              NULL if nothing found.
 */
AppDataTelnetClient * //inline//
AppTelnetClientGetTelnetClient(Node *node, int connId)
{
    AppInfo *appList = node->appData.appPtr;
    AppDataTelnetClient *telnetClient;

    for (; appList != NULL; appList = appList->appNext)
    {
        if (appList->appType == APP_TELNET_CLIENT)
        {
            telnetClient = (AppDataTelnetClient *) appList->appDetail;

            if (telnetClient->connectionId == connId)
            {
                return telnetClient;
            }
        }
    }

    return NULL;
}

/*
 * NAME:        AppTelnetClientUpdateTelnetClient.
 * PURPOSE:     create a new telnet client data structure, place it
 *              at the beginning of the application list.
 * PARAMETERS:  node - pointer to the node,
 *              openResult - result of the open request.
 * RETRUN:      the pointer to the created telnet client data structure,
 *              NULL if no data structure allocated.
 */
AppDataTelnetClient * //inline//
AppTelnetClientUpdateTelnetClient(Node *node,
                                  TransportToAppOpenResult *openResult)
{
    AppInfo *appList = node->appData.appPtr;
    AppDataTelnetClient *telnetClient = {0};
    AppDataTelnetClient *tmpTelnetClient;

    for (; appList != NULL; appList = appList->appNext)
    {
        if (appList->appType == APP_TELNET_CLIENT)
        {
            tmpTelnetClient = (AppDataTelnetClient *) appList->appDetail;

#ifdef DEBUG
            printf("TELNET Client: Node %ld comparing uniqueId "
                   "%ld with %ld\n", node->nodeId,
                   tmpTelnetClient->uniqueId, openResult->uniqueId);
#endif /* DEBUG */

            if (tmpTelnetClient->uniqueId == openResult->uniqueId)
            {
                telnetClient = tmpTelnetClient;
                break;
            }
        }
    }

    if (telnetClient == NULL)
    {
        assert(FALSE);
    }

    /*
     * fill in connection id, etc.
     */
    telnetClient->connectionId = openResult->connectionId;
    telnetClient->localAddr = openResult->localAddr;
    telnetClient->remoteAddr = openResult->remoteAddr;
    telnetClient->sessionStart = getSimTime(node);

    telnetClient->sessionFinish =
        getSimTime(node) + telnetClient->sessDuration;
    telnetClient->sessionIsClosed = FALSE;

    return telnetClient;
}

/*
 * NAME:        AppTelnetClientNewTelnetClient.
 * PURPOSE:     create a new telnet client data structure, place it
                at the beginning of the application list.
 * PARAMETERS:  node - pointer to the node,
 *              clientAddr - address of this client.
 *              serverAddr - server node of this telnet session.
 *              sessDuration - length of telnet session.
 * RETRUN:      the pointer to the created telnet client data structure,
 *              NULL if no data structure allocated.
 */
AppDataTelnetClient * //inline//
AppTelnetClientNewTelnetClient(
    Node *node,
    Address clientAddr,
    Address serverAddr,
    clocktype sessDuration)
{
    AppDataTelnetClient *telnetClient;

    telnetClient = (AppDataTelnetClient *)
                    MEM_malloc(sizeof(AppDataTelnetClient));

    memset(telnetClient, 0, sizeof(AppDataTelnetClient));

    /*
     * fill in connection id, etc.
     */
    telnetClient->connectionId = -1;
    telnetClient->uniqueId = node->appData.uniqueId++;

    telnetClient->localAddr = clientAddr;
    telnetClient->remoteAddr = serverAddr;

    RANDOM_SetSeed(telnetClient->durationSeed,
                   node->globalSeed,
                   node->nodeId,
                   APP_TELNET_CLIENT,
                   telnetClient->uniqueId);
    RANDOM_SetSeed(telnetClient->intervalSeed,
                   node->globalSeed,
                   node->nodeId,
                   APP_TELNET_CLIENT,
                   telnetClient->uniqueId + 1);

    if (sessDuration > 0)
    {
        telnetClient->sessDuration = sessDuration;
    }
    else
    {
        telnetClient->sessDuration = AppTelnetClientSessDuration(node, telnetClient);
    }

    telnetClient->numBytesSent = 0;
    telnetClient->numBytesRecvd = 0;
    telnetClient->lastItemSent = 0;

    APP_RegisterNewApp(node, APP_TELNET_CLIENT, telnetClient);

    return telnetClient;
}

/*
 * NAME:        AppTelnetClientScheduleNextPkt.
 * PURPOSE:     schedule the next packet the client will send.  If next packet
 *              won't arrive until the finish deadline, schedule a close.
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the telnet client data structure.
 * RETRUN:      none.
 */
void //inline//
AppTelnetClientScheduleNextPkt(Node *node,
                               AppDataTelnetClient *clientPtr)
{
    clocktype interval = AppTelnetClientPktInterval(node, clientPtr);
    Message *timerMsg;
    AppTimer *timer;

    timerMsg = MESSAGE_Alloc(node, APP_LAYER,
                              APP_TELNET_CLIENT, MSG_APP_TimerExpired);

    MESSAGE_InfoAlloc(node, timerMsg, sizeof(AppTimer));

    timer = (AppTimer *) MESSAGE_ReturnInfo(timerMsg);
    timer->connectionId = clientPtr->connectionId;

    if ((getSimTime(node) + interval) < clientPtr->sessionFinish)
    {
        timer->type = APP_TIMER_SEND_PKT;
    }
    else
    {
        timer->type = APP_TIMER_CLOSE_SESS;
        interval = clientPtr->sessionFinish - getSimTime(node);
    }

    MESSAGE_Send(node, timerMsg, interval);
}

/*
 * NAME:        AppTelnetClientSessDuration.
 * PURPOSE:     call tcplib function telnet_duration to get the duration
 *              of this session.
 * PARAMETERS:  node - pointer to the node.
 * RETRUN:      session duration in clocktype.
 */
clocktype //inline//
AppTelnetClientSessDuration(Node *node,
                            AppDataTelnetClient *clientPtr)
{
    float duration;
    duration = telnet_duration(clientPtr->durationSeed);

#ifdef DEBUG
    printf("TELNET duration = %f\n", duration);
#endif /* DEBUG */

    return (((clocktype)(duration + 0.5)) * MILLI_SECOND);
}

/*
 * NAME:        AppTelnetClientPktInterval.
 * PURPOSE:     call tcplib function telnet_interarrival to get the
 *              between the arrival of the next packet and the current one.
 * PARAMETERS:  node - pointer to the node.
 * RETRUN:      interarrival time in clocktype.
 */
clocktype //inline//
AppTelnetClientPktInterval(Node *node,
                           AppDataTelnetClient *clientPtr)
{
    float interval;
    interval = telnet_interarrival(clientPtr->intervalSeed);

#ifdef DEBUG
    printf("TELNET interarrival = %f\n", interval);
#endif /* DEBUG */

    return (((clocktype)(interval + 0.5)) * MILLI_SECOND);
}



/*
 * NAME:        AppLayerTelnetServer.
 * PURPOSE:     Models the behaviour of Telnet server on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  node - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
AppLayerTelnetServer(Node *node, Message *msg)
{
    char buf[MAX_STRING_LENGTH];
    AppDataTelnetServer *serverPtr;

    ctoa(getSimTime(node), buf);

    switch (msg->eventType)
    {
        case MSG_APP_FromTransListenResult:
        {
            TransportToAppListenResult *listenResult;

            listenResult = (TransportToAppListenResult *)
                           MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("%s: node %u got listenResult\n",
                   buf, node->nodeId);
#endif /* DEBUG */

            if (listenResult->connectionId == -1)
            {
                node->appData.numAppTcpFailure ++;
            }

            break;
        }
        case MSG_APP_FromTransOpenResult:
        {
            TransportToAppOpenResult *openResult;

            openResult = (TransportToAppOpenResult *)MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("%s: node %u got OpenResult\n", buf, node->nodeId);
#endif /* DEBUG */

            assert(openResult->type == TCP_CONN_PASSIVE_OPEN);

            if (openResult->connectionId < 0)
            {
                node->appData.numAppTcpFailure ++;
            }
            else
            {
                AppDataTelnetServer *serverPtr;
                serverPtr = AppTelnetServerNewTelnetServer(node, openResult);
                assert(serverPtr != NULL);
            }

            break;
        }
        case MSG_APP_FromTransDataSent:
        {
            TransportToAppDataSent *dataSent;

            dataSent = (TransportToAppDataSent *) MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("%s: node %u sent data %ld\n", buf, node->nodeId,
                   dataSent->length);
#endif /* DEBUG */

            serverPtr = AppTelnetServerGetTelnetServer(node,
                                        dataSent->connectionId);
            assert(serverPtr != NULL);

            serverPtr->numBytesSent += (clocktype) dataSent->length;
            serverPtr->lastItemSent = getSimTime(node);

            break;
        }
        case MSG_APP_FromTransDataReceived:
        {
            TransportToAppDataReceived *dataRecvd;

            dataRecvd = (TransportToAppDataReceived *)
                        MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("%s: node %u received data %ld\n",
                   buf, node->nodeId, msg->packetSize);
#endif /* DEBUG */

            serverPtr = AppTelnetServerGetTelnetServer(
                            node,
                            dataRecvd->connectionId);

            assert(serverPtr != NULL);

            serverPtr->numBytesRecvd += (clocktype) msg->packetSize;

            if (msg->packet[msg->packetSize - 1] == 'c')
            {
                /*
                * Client wants to close the session, so server also
                * initiates a close.
                */
                APP_TcpCloseConnection(
                    node,
                    serverPtr->connectionId);

                serverPtr->sessionFinish = getSimTime(node);
                serverPtr->sessionIsClosed = TRUE;
            }
            else if (msg->packet[msg->packetSize - 1] == 'd')
            {
                /* Send a response packet back. */
                if (serverPtr->sessionIsClosed == FALSE)
                {
                    AppTelnetServerSendResponse(node, serverPtr);
                }
            }
            else
            {
            assert(FALSE);
            }
            break;
        }
        case MSG_APP_FromTransCloseResult:
        {
            TransportToAppCloseResult *closeResult;

            closeResult = (TransportToAppCloseResult *)
                          MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("%s: node %u got close result\n",
                   buf, node->nodeId);
#endif /* DEBUG */

            serverPtr = AppTelnetServerGetTelnetServer(node,
                                                 closeResult->connectionId);

            assert(serverPtr != NULL);

            if (serverPtr->sessionIsClosed == FALSE)
            {
                serverPtr->sessionIsClosed = TRUE;
                serverPtr->sessionFinish = getSimTime(node);
            }

            break;
        }
        default:
            ctoa(getSimTime(node), buf);
            printf("Time %s: Node %u received message of unknown type"
                   " %d.\n", buf, node->nodeId, msg->eventType);
#ifndef EXATA
            assert(FALSE);
#endif
    }

    MESSAGE_Free(node, msg);
}

/*
 * NAME:        AppTelnetServerInit.
 * PURPOSE:     listen on Telnet server port.
 * PARAMETERS:  node - pointer to the node.
 * RETURN:      none.
 */
void
AppTelnetServerInit(Node *node, Address serverAddr)
{
    APP_TcpServerListen(
        node,
        APP_TELNET_SERVER,
        serverAddr,
        (short) APP_TELNET_SERVER);
}

/*
 * NAME:        AppTelnetServerFinalize.
 * PURPOSE:     Collect statistics of a TelnetServer session.
 * PARAMETERS:  node - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
AppTelnetServerFinalize(Node *node, AppInfo* appInfo)
{
    AppDataTelnetServer *serverPtr =
        (AppDataTelnetServer*)appInfo->appDetail;

    if (node->appData.appStats == TRUE)
    {
        AppTelnetServerPrintStats(node, serverPtr);
    }
}

/*
 * NAME:        AppTelnetServerPrintStats.
 * PURPOSE:     Prints statistics of a TelnetServer session.
 * PARAMETERS:  node - pointer to the node.
 *              serverPtr - pointer to the ftp server data structure.
 * RETURN:      none.
 */
void //inline//
AppTelnetServerPrintStats(Node *node, AppDataTelnetServer *serverPtr)
{
    clocktype throughput;
    char addrStr[MAX_STRING_LENGTH];
    char startStr[MAX_STRING_LENGTH];
    char finishStr[MAX_STRING_LENGTH];
    char statusStr[MAX_STRING_LENGTH];
    char numBytesSentStr[MAX_STRING_LENGTH];
    char numBytesRecvdStr[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char throughputStr[MAX_STRING_LENGTH];

    TIME_PrintClockInSecond(serverPtr->sessionStart, startStr);
    TIME_PrintClockInSecond(serverPtr->lastItemSent, finishStr);

    if (serverPtr->connectionId < 0)
    {
        sprintf(statusStr, "Failed");
    }
    else if (serverPtr->sessionIsClosed == FALSE)
    {
        serverPtr->sessionFinish = getSimTime(node);
        sprintf(statusStr, "Not closed");
    }
    else
    {
        sprintf(statusStr, "Closed");
    }

    if (serverPtr->sessionFinish <= serverPtr->sessionStart)
    {
        throughput = 0;
    }
    else
    {
        throughput =
            (clocktype) ((serverPtr->numBytesRecvd * 8.0 * SECOND)
            / (serverPtr->sessionFinish - serverPtr->sessionStart));
    }

    ctoa(throughput, throughputStr);
    IO_ConvertIpAddressToString(&serverPtr->remoteAddr, addrStr);
    sprintf(buf, "Client address = %s", addrStr);
    IO_PrintStat(
        node,
        "Application",
        "Telnet Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    sprintf(buf, "First Packet Sent at (s) = %s", startStr);
    IO_PrintStat(
        node,
        "Application",
        "Telnet Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    sprintf(buf, "Last Packet Sent at (s) = %s", finishStr);
    IO_PrintStat(
        node,
        "Application",
        "Telnet Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    sprintf(buf, "Session Status = %s", statusStr);
    IO_PrintStat(
        node,
        "Application",
        "Telnet Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    ctoa(serverPtr->numBytesSent, numBytesSentStr);
    sprintf(buf, "Total Bytes Sent = %s", numBytesSentStr);
    IO_PrintStat(
        node,
        "Application",
        "Telnet Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    ctoa(serverPtr->numBytesRecvd, numBytesRecvdStr);
    sprintf(buf, "Total Bytes Received = %s", numBytesRecvdStr);
    IO_PrintStat(
        node,
        "Application",
        "Telnet Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);

    sprintf(buf, "Throughput (bits/s) = %s", throughputStr);
    IO_PrintStat(
        node,
        "Application",
        "Telnet Server",
        ANY_DEST,
        serverPtr->connectionId,
        buf);
}

/*
 * NAME:        AppTelnetServerGetTelnetServer.
 * PURPOSE:     search for a telnet server data structure.
 * PARAMETERS:  node - pointer to the node,
 *              connId - connection ID of the telnet server.
 * RETURN:      the pointer to the telnet server data structure,
 *              NULL if nothing found.
 */
AppDataTelnetServer * //inline//
AppTelnetServerGetTelnetServer(Node *node, int connId)
{
    AppInfo *appList = node->appData.appPtr;
    AppDataTelnetServer *telnetServer;

    for (; appList != NULL; appList = appList->appNext)
    {
        if (appList->appType == APP_TELNET_SERVER)
        {
            telnetServer = (AppDataTelnetServer *) appList->appDetail;

            if (telnetServer->connectionId == connId)
            {
                return telnetServer;
            }
        }
    }

    return NULL;
}

/*
 * NAME:        AppTelnetServerNewTelnetServer.
 * PURPOSE:     create a new telnet server data structure, place it
                at the beginning of the application list.
 * PARAMETERS:  node - pointer to the node,
 *              openResult - result of the open request.
 * RETRUN:      the pointer to the created telnet server data structure,
 *              NULL if no data structure allocated.
 */
AppDataTelnetServer * //inline//
AppTelnetServerNewTelnetServer(Node *node,
                               TransportToAppOpenResult *openResult)
{
    AppDataTelnetServer *telnetServer;

    telnetServer = (AppDataTelnetServer *)
                    MEM_malloc(sizeof(AppDataTelnetServer));

    /*
     * fill in connection id, etc.
     */
    telnetServer->connectionId = openResult->connectionId;
    telnetServer->localAddr = openResult->localAddr;
    telnetServer->remoteAddr = openResult->remoteAddr;
    telnetServer->sessionStart = getSimTime(node);
    telnetServer->sessionFinish = 0;
    telnetServer->sessionIsClosed = FALSE;
    telnetServer->numBytesSent = 0;
    telnetServer->numBytesRecvd = 0;
    telnetServer->lastItemSent = 0;
//DERIUS for Stand alone Telnet server
    telnetServer->state = STATE_NORMAL;
    telnetServer->bufPtr = 0;
    
    RANDOM_SetSeed(telnetServer->packetSizeSeed,
                   node->globalSeed,
                   node->nodeId,
                   APP_TELNET_SERVER,
                   openResult->connectionId); // not invariant

    APP_RegisterNewApp(node, APP_TELNET_SERVER, telnetServer);

    return telnetServer;
}


/*
 * NAME:        AppTelnetServerSendResponse.
 * PURPOSE:     call AppTelnetServerRespPktSize() to get the
 *              response packet size,
                and send the packet.
 * PARAMETERS:  node - pointer to the node,
 *              serverPtr - pointer to the server data structure.
 * RETRUN:      none.
 */
void //inline//
AppTelnetServerSendResponse(Node *node, AppDataTelnetServer *serverPtr)
{
    int pktSize;
    char *payload;

    pktSize = AppTelnetServerRespPktSize(node, serverPtr);

    payload = (char *)MEM_malloc(pktSize);
    memset(payload, 0, pktSize);

    if (serverPtr->sessionIsClosed == FALSE)
    {
        APP_TcpSendData(
            node,
            serverPtr->connectionId,
            payload,
            pktSize,
            TRACE_TELNET);
    }

    MEM_free(payload);
}

/*
 * NAME:        AppTelnetServerRespPktSize.
 * PURPOSE:     call tcplib function telnet_pktsize().
 * PARAMETERS:  node - pointer to the node.
 * RETRUN:      telnet control packet size.
 */
int
AppTelnetServerRespPktSize(Node *node, AppDataTelnetServer *serverPtr)
{
    int ctrlPktSize;
    ctrlPktSize = telnet_pktsize(serverPtr->packetSizeSeed);

#ifdef DEBUG
    printf("TELNET control pktsize = %d\n", ctrlPktSize);
#endif /* DEBUG */

    return (ctrlPktSize);
}
