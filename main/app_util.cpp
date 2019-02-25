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
 * utilities used by application to send packet to transport layer
 * and open a connection.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <set>

#include "api.h"
#include "network_ip.h"
#include "ipv6.h"
#include "partition.h"
#include "app_util.h"
#include "app_cbr.h"

#include "tcpapps.h"
#include "transport_tcp.h"

#ifdef ADDON_DB
#include "db.h"
#include "dbapi.h"
#endif

// for voice over ip in superapplication
#include "app_superapplication.h"

#include "app_mdp.h"
#include "multicast_igmp.h"

#define QOS_DEBUG 0
#define DEBUG_QOS_ENABLED 0


/*
 * FUNCTION:   APP_RegisterNewApp
 * PURPOSE:    Insert a new application into the list of apps on this node.
 * PARAMETERS: node - node that is issuing the Timer.
 *             appType - application type
 *             dataPtr - pointer to the data space for this app
 * RETURN:     pointer to the new AppInfo data structure for this app
 */
AppInfo *
APP_RegisterNewApp(Node *node, AppType appType, void *dataPtr)
{
    AppInfo *newApp;

    newApp = (AppInfo *)MEM_malloc(sizeof(AppInfo));

    newApp->appType = appType;
    newApp->appDetail = dataPtr;
    newApp->appNext = node->appData.appPtr;
    node->appData.appPtr = newApp;

    return newApp;
}


/*
 * FUNCTION:   APP_UnregisterApp
 * PURPOSE:    Remove an application from the list of apps on this node.
 * PARAMETERS: node - node that is issuing the Timer.
 *             dataPtr - pointer to the data space for this app
 * RETURN:     pointer to the new AppInfo data structure for this app
 */
void
APP_UnregisterApp(Node *node, void *dataPtr, bool freeData)
{
    AppInfo* appList = node->appData.appPtr;

    AppInfo* prev = NULL;

    for (; appList != NULL; prev=appList, appList = appList->appNext)
    {
        if (appList->appDetail == dataPtr) {
            break;
        }
    }

    ERROR_Assert(appList != NULL, "Session not found in the list");

    // now prev is the prev in list and appList points to the one to be unregistered.

    // If first app in the list is to be unregistered, then simply move the node->appData.appPtr to the next one
    if (prev == NULL) {
        node->appData.appPtr = appList->appNext;
    } else {
        prev->appNext = appList->appNext;
    }

    if (freeData) {
        MEM_free(dataPtr);
    }
    MEM_free(appList);
}



/*
 * FUNCTION:   APP_SetTimer
 * PURPOSE:    Set a new App Layer Timer and send to self after delay.
 * PARAMETERS: node - node that is issuing the Timer.
 *             appType - application type
 *             connId - if applicable, the TCP connectionId for this timer
 *             sourcePort - the source port of the application setting
 *                          this timer
 *             timerType  - an integer value that can be used to distinguish
 *                          between types of timers
 *             delay - send the timer to self after this delay.
 * RETURN:     none.
 */
void
APP_SetTimer(
    Node *node,
    AppType appType,
    int connId,
    unsigned short sourcePort,
    int timerType,
    clocktype delay)
{
    Message *msg;
    AppTimer *info;

    msg = MESSAGE_Alloc (node, APP_LAYER,
                         appType, MSG_APP_TimerExpired);

    MESSAGE_InfoAlloc (node, msg, sizeof(AppTimer));
    info = (AppTimer *) MESSAGE_ReturnInfo(msg);
    info->connectionId = connId;
    info->sourcePort = sourcePort;
    info->type = timerType;

    MESSAGE_Send(node, msg, delay);
}

/*
 * FUNCTION:   APP_UdpSendNewData
 * PURPOSE:    Allocate data and send to UDP.
 * PARAMETERS: node - node that is sending the data.
 *             appType - application type, to be used as destination port.
 *             sourceAddr, the source sending the data.
 *             sourcePort, the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             payload  - pointer to the data.
 *             payloadSize - size of the data in bytes.
 *             delay - send the data after this delay.
 *             traceProtocol - specify the type of application used for
 *                             packet tracing.
 * RETURN:     Message *.
 */
Message *
APP_UdpSendNewData(
    Node *node,
    AppType appType,
    NodeAddress sourceAddr,
    short sourcePort,
    NodeAddress destAddr,
    char *payload,
    int payloadSize,
    clocktype delay,
    TraceProtocolType traceProtocol)
{
    Message *msg;
    AppToUdpSend *info;
    ActionData acnData;

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_UDP,
              MSG_TRANSPORT_FromAppSend);

    MESSAGE_PacketAlloc(node, msg, payloadSize, traceProtocol);
    memcpy(MESSAGE_ReturnPacket(msg), payload, payloadSize);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToUdpSend));
    info = (AppToUdpSend *) MESSAGE_ReturnInfo(msg);

    SetIPv4AddressInfo(&info->sourceAddr, sourceAddr);
    info->sourcePort = sourcePort;

    SetIPv4AddressInfo(&info->destAddr, destAddr);
    info->destPort = (short) appType;
    info->priority = APP_DEFAULT_TOS;
    info->outgoingInterface = ANY_INTERFACE;
    info->ttl = IPDEFTTL;

    //Trace Information
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
        PACKET_OUT, &acnData);

    MESSAGE_Send(node, msg, delay);
    return msg;
}

#ifdef EXATA
/*
 * FUNCTION:   APP_UdpSendNewData (overloaded for EmulationPacket)
 * PURPOSE:    Allocate data and send to UDP.
 * PARAMETERS: node - node that is sending the data.
 *             appType - application type, to be used as destination port.
 *             sourceAddr, the source sending the data.
 *             sourcePort, the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             payload  - pointer to the data.
 *             payloadSize - size of the data in bytes.
 *             delay - send the data after this delay.
 *             traceProtocol - specify the type of application used for
 *                             packet tracing.
 *             isEmulationPacket - is Emulation packet?
 * RETURN:     none.
 */
void
APP_UdpSendNewData(
    Node *node,
    AppType appType,
    NodeAddress sourceAddr,
    short sourcePort,
    NodeAddress destAddr,
    char *payload,
    int payloadSize,
    clocktype delay,
    TraceProtocolType traceProtocol,
    BOOL isEmulationPacket)
{
    Message *msg;
    AppToUdpSend *info;
    ActionData acnData;

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_UDP,
              MSG_TRANSPORT_FromAppSend);

    MESSAGE_PacketAlloc(node, msg, payloadSize, traceProtocol);
    memcpy(MESSAGE_ReturnPacket(msg), payload, payloadSize);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToUdpSend));
    info = (AppToUdpSend *) MESSAGE_ReturnInfo(msg);

    msg->isEmulationPacket = isEmulationPacket;

    SetIPv4AddressInfo(&info->sourceAddr, sourceAddr);
    info->sourcePort = sourcePort;

    SetIPv4AddressInfo(&info->destAddr, destAddr);
    info->destPort = (short) appType;
    info->priority = APP_DEFAULT_TOS;
    info->outgoingInterface = ANY_INTERFACE;
    info->ttl = IPDEFTTL;

    //Trace Information
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
    PACKET_OUT, &acnData);

    MESSAGE_Send(node, msg, delay);
}
#endif


/*
 * FUNCTION:   APP_UdpSendNewDataWithPriority
 * PURPOSE:    Allocate data with specified priority and send to UDP.
 * PARAMETERS: node - node that is sending the data.
 *             appType - application type, to be used as destination port.
 *             sourceAddr, the source sending the data.
 *             sourcePort, the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             outgoingInterface, interface used to send data.
 *             payload  - pointer to the data.
 *             payloadSize - size of the data in bytes.
 *             priority - priority of data.
 *             delay - send the data after this delay.
 *             traceProtocol - specify the type of application used for
 *                             packet tracing.
 *             isMdpEnabled - specify whether MDP is enabled.
 *             uniqueId - specify uniqueId related to MDP.
 *             mdpDataObjectInfo - specify the mdp data object info if any.
 *             mdpDataInfosize - specify the mdp data info size.
 * RETURN:     Message *.
 */
Message *
APP_UdpSendNewDataWithPriority(
    Node *node,
    AppType appType,
    NodeAddress sourceAddr,
    short sourcePort,
    NodeAddress destAddr,
    int outgoingInterface,
    char *payload,
    int payloadSize,
    TosType priority,
    clocktype delay,
    TraceProtocolType traceProtocol,
    BOOL isMdpEnabled,
    Int32 uniqueId,
    int regionId,
    const char* mdpDataObjectInfo,
    unsigned short mdpDataInfosize
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam
#endif
    )
{
    Message *msg;
    AppToUdpSend *info;
    ActionData acnData;

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_UDP,
              MSG_TRANSPORT_FromAppSend);

    MESSAGE_PacketAlloc(node, msg, payloadSize, traceProtocol);

    memcpy(MESSAGE_ReturnPacket(msg), payload, payloadSize);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToUdpSend));
    info = (AppToUdpSend *) MESSAGE_ReturnInfo(msg);

    SetIPv4AddressInfo(&info->sourceAddr, sourceAddr);
    info->sourcePort = sourcePort;

    SetIPv4AddressInfo(&info->destAddr, destAddr);
    info->destPort = (short) appType;

    info->priority = priority;
    info->outgoingInterface = outgoingInterface;

    info->ttl = IPDEFTTL;

#ifdef ADDON_BOEINGFCS
    if (regionId >= -1)
    {
        int* regionInfo = (int*)MESSAGE_AddInfo(node, msg, sizeof(int),
                                                INFO_TYPE_HSLSRegionId);
        *regionInfo = regionId;
    }
#endif

#ifdef ADDON_DB
    if (appParam != NULL)
    {
        // record one event for each fragment
        HandleStatsDBAppSendEventsInsertion(
            node,
            msg,
            payloadSize,
            appParam);
    }
#endif

    //Trace Information
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
        PACKET_OUT, &acnData);

    if (isMdpEnabled)
    {
        Address clientAddr, remoteAddr;

        memset(&clientAddr,0,sizeof(Address));
        memset(&remoteAddr,0,sizeof(Address));

        clientAddr.networkType = NETWORK_IPV4;
        clientAddr.interfaceAddr.ipv4 = sourceAddr;

        remoteAddr.networkType = NETWORK_IPV4;
        remoteAddr.interfaceAddr.ipv4 = destAddr;

        MdpQueueDataObject(node,
                          clientAddr,
                          remoteAddr,
                          uniqueId,
                          payloadSize,
                          payload,
                          0,               // for virtual size
                          msg,
                          FALSE,
                          (UInt16)sourcePort,
                          mdpDataObjectInfo,
                          mdpDataInfosize);
    }
    else
    {
        //MEM_free(payload);

        MESSAGE_Send(node, msg, delay);
    }
    return msg;
}

/*
 * FUNCTION:   APP_UdpSendNewDataWithPriority
 * PURPOSE:    Allocate data with specified priority and send to UDP(For IPv6).
 * PARAMETERS: node - node that is sending the data.
 *             appType - application type, to be used as destination port.
 *             sourceAddr, the source sending the data.
 *             sourcePort, the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             outgoingInterface, interface used to send data.
 *             payload  - pointer to the data.
 *             payloadSize - size of the data in bytes.
 *             priority - priority of data.
 *             delay - send the data after this delay.
 *             traceProtocol - specify the type of application used for
 *                             packet tracing.
 *             isMdpEnabled - specify whether MDP is enabled.
 *             uniqueId - specify uniqueId related to MDP.
 * RETURN:     Message*
 */

Message*
APP_UdpSendNewDataWithPriority(
    Node *node,
    AppType appType,
    Address sourceAddr,
    short sourcePort,
    Address destAddr,
    int outgoingInterface,
    char *payload,
    int payloadSize,
    TosType priority,
    clocktype delay,
    TraceProtocolType traceProtocol,
    BOOL isMdpEnabled,
    Int32 uniqueId
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam
#endif
    )
{
    Message *msg;
    AppToUdpSend *info;
    ActionData acnData;

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_UDP,
              MSG_TRANSPORT_FromAppSend);

    MESSAGE_PacketAlloc(node, msg, payloadSize, traceProtocol);

    memcpy(MESSAGE_ReturnPacket(msg), payload, payloadSize);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToUdpSend));
    info = (AppToUdpSend *) MESSAGE_ReturnInfo(msg);

    info->sourceAddr = sourceAddr;
    info->sourcePort = sourcePort;

    info->destAddr = destAddr;
    info->destPort = (short) appType;

    info->priority = priority;
    info->outgoingInterface = outgoingInterface;
    info->ttl = IPDEFTTL;

    //Trace Information
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
                   PACKET_OUT, &acnData);
#ifdef ADDON_DB
    if (appParam != NULL)
    {
        // record one event for each fragment
        HandleStatsDBAppSendEventsInsertion(
            node,
            msg,
            payloadSize,
            appParam);
    }
#endif

    if (isMdpEnabled)
    {
        MdpQueueDataObject(node,
                          sourceAddr,
                          destAddr,
                          uniqueId,
                          payloadSize,
                          payload,
                          0,               // for virtual size
                          msg,
                          FALSE,
                          (UInt16)sourcePort);
    }
    else
    {
        //MEM_free(payload); HDuong
        MESSAGE_Send(node, msg, delay);
    }

    return msg;
}


/*
 * FUNCTION:   APP_UdpSendNewHeaderData
 * PURPOSE:    Allocate header + data and send to UDP.
 * PARAMETERS: node - node that is sending the data.
 *             appType - application type, to be used as destination port.
 *             sourceAddr, the source sending the data.
 *             sourcePort, the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             header - header of the payload.
 *             headerSize - size of the header.q
 *             payload  - pointer to the data.
 *             payloadSize - size of the data in bytes.
 *             delay - send the data after this delay.
 *             traceProtocol - specify the type of application used for
 *                             packet tracing.
 * RETURN:     none.
 */
void
APP_UdpSendNewHeaderData(
    Node *node,
    AppType appType,
    NodeAddress sourceAddr,
    short sourcePort,
    NodeAddress destAddr,
    char *header,
    int headerSize,
    char *payload,
    int payloadSize,
    clocktype delay,
    TraceProtocolType traceProtocol)
{
    Message *msg;
    AppToUdpSend *info;

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_UDP,
              MSG_TRANSPORT_FromAppSend);

    MESSAGE_PacketAlloc(node, msg, payloadSize, traceProtocol);
    memcpy(MESSAGE_ReturnPacket(msg), payload, payloadSize);

    MESSAGE_AddHeader(node, msg, headerSize, traceProtocol);

    memcpy(MESSAGE_ReturnPacket(msg), header, headerSize);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToUdpSend));
    info = (AppToUdpSend *) MESSAGE_ReturnInfo(msg);

    SetIPv4AddressInfo(&info->sourceAddr, sourceAddr);
    info->sourcePort = sourcePort;

    SetIPv4AddressInfo(&info->destAddr, destAddr);
    info->destPort = (short) appType;
    info->priority = APP_DEFAULT_TOS;
    info->outgoingInterface = ANY_INTERFACE;
    info->ttl = IPDEFTTL;

    MESSAGE_Send(node, msg, delay);
}

/*
 * FUNCTION:   APP_UdpSendNewHeaderDataWithPriority
 * PURPOSE:    Allocate header + data with specified priority and send to UDP.
 * PARAMETERS: node - node that is sending the data.
 *             appType - application type, to be used as destination port.
 *             sourceAddr, the source sending the data.
 *             sourcePort, the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             outgoingInterface, interface used to send data.
 *             header - header of the payload.
 *             headerSize - size of the header.
 *             payload  - pointer to the data.
 *             payloadSize - size of the data in bytes.
 *             priority - priority of data.
 *             delay - send the data after this delay.
 *             traceProtocol - specify the type of application used for
 *                             packet tracing.
 * RETURN:     none.
 */
void
APP_UdpSendNewHeaderDataWithPriority(
    Node *node,
    AppType appType,
    NodeAddress sourceAddr,
    short sourcePort,
    NodeAddress destAddr,
    int outgoingInterface,
    char *header,
    int headerSize,
    char *payload,
    int payloadSize,
    TosType priority,
    clocktype delay,
    TraceProtocolType traceProtocol
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam)
#else
    )
#endif
{
    Message *msg;
    AppToUdpSend *info;
    ActionData acnData;

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_UDP,
              MSG_TRANSPORT_FromAppSend);

    MESSAGE_PacketAlloc(node, msg, payloadSize, traceProtocol);
    memcpy(MESSAGE_ReturnPacket(msg), payload, payloadSize);

    MESSAGE_AddHeader(node, msg, headerSize, traceProtocol);

    memcpy(MESSAGE_ReturnPacket(msg), header, headerSize);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToUdpSend));
    info = (AppToUdpSend *) MESSAGE_ReturnInfo(msg);

    SetIPv4AddressInfo(&info->sourceAddr, sourceAddr);
    info->sourcePort = sourcePort;

    SetIPv4AddressInfo(&info->destAddr, destAddr);
    info->destPort = (short) appType;

    info->priority = priority;
    info->outgoingInterface = outgoingInterface;
    info->ttl = IPDEFTTL;

    //Trace Information
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
                   PACKET_OUT, &acnData);
    MESSAGE_Send(node, msg, delay);
}

/*
 * FUNCTION:   APP_UdpCreateNewHeaderVirtualDataWithPriority
 * PURPOSE:    Allocate header + virtual data with specified priority
 * PARAMETERS: node - node that is sending the data.
 *             appType - application type, to be used as destination port.
 *             sourceAddr, the source sending the data.
 *             sourcePort, the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             header - header of the payload.
 *             headerSize - size of the header.
 *             payloadSize - size of the data in bytes.
 *             priority - priority of data.
 *             traceProtocol - specify the type of application used for
 *                 packet tracing.
 * RETURN:     none.
 */
 Message* APP_UdpCreateNewHeaderVirtualDataWithPriority(
     Node *node,
     AppType appType,
     NodeAddress sourceAddr,
     short sourcePort,
     NodeAddress destAddr,
     char *header,
     int headerSize,
     int payloadSize,
     TosType priority,
     TraceProtocolType traceProtocol)
 {
     Message *msg = NULL;
     AppToUdpSend *info = NULL;

     msg = MESSAGE_Alloc(
               node,
               TRANSPORT_LAYER,
               TransportProtocol_UDP,
               MSG_TRANSPORT_FromAppSend);

     MESSAGE_PacketAlloc(node, msg, headerSize, traceProtocol);

     if (headerSize > 0)
     {
         memcpy(MESSAGE_ReturnPacket(msg), header, headerSize);
     }

     MESSAGE_AddVirtualPayload(node, msg, payloadSize);

     MESSAGE_InfoAlloc(node, msg, sizeof(AppToUdpSend));
     info = (AppToUdpSend *) MESSAGE_ReturnInfo(msg);

     SetIPv4AddressInfo(&info->sourceAddr, sourceAddr);
     info->sourcePort = sourcePort;

     SetIPv4AddressInfo(&info->destAddr, destAddr);
     info->destPort = (short) appType;

     info->priority = priority;
     info->outgoingInterface = ANY_INTERFACE;
     info->ttl = IPDEFTTL;

     return msg;
}

/*
 * FUNCTION:   APP_UdpCreateNewHeaderVirtualDataWithPriority
 * PURPOSE:    Allocate header + virtual data with specified priority
 * PARAMETERS: node - node that is sending the data.
 *             appType - application type, to be used as destination port.
 *             sourceAddr, the source sending the data.
 *             sourcePort, the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             header - header of the payload.
 *             headerSize - size of the header.
 *             payloadSize - size of the data in bytes.
 *             priority - priority of data.
 *             traceProtocol - specify the type of application used for
 *                 packet tracing.
 * RETURN:     none.
 */
 Message* APP_UdpCreateNewHeaderVirtualDataWithPriority(
     Node *node,
     AppType appType,
     const Address& sourceAddr,
     short sourcePort,
     const Address& destAddr,
     const char *header,
     int headerSize,
     int payloadSize,
     TosType priority,
     TraceProtocolType traceProtocol)
 {
     Message *msg = NULL;
     AppToUdpSend *info = NULL;

     msg = MESSAGE_Alloc(
               node,
               TRANSPORT_LAYER,
               TransportProtocol_UDP,
               MSG_TRANSPORT_FromAppSend);

     MESSAGE_PacketAlloc(node, msg, headerSize, traceProtocol);

     if (headerSize > 0)
     {
         memcpy(MESSAGE_ReturnPacket(msg), header, headerSize);
     }

     MESSAGE_AddVirtualPayload(node, msg, payloadSize);

     MESSAGE_InfoAlloc(node, msg, sizeof(AppToUdpSend));
     info = (AppToUdpSend *) MESSAGE_ReturnInfo(msg);
     info->sourceAddr = sourceAddr;
     info->sourcePort = sourcePort;
     info->destAddr = destAddr;
     info->destPort = (short) appType;
     info->priority = priority;
     info->outgoingInterface = ANY_INTERFACE;
     info->ttl = IPDEFTTL;

     return msg;
}

/*
 * FUNCTION:   APP_UdpSendNewHeaderVirtualDataWithPriority
 * PURPOSE:    Allocate header + virtual data with specified priority
 *             and send to UDP.
 * PARAMETERS: node - node that is sending the data.
 *             appType - application type, to be used as destination port.
 *             sourceAddr, the source sending the data.
 *             sourcePort, the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             header - header of the payload.
 *             headerSize - size of the header.
 *             payloadSize - size of the data in bytes.
 *             priority - priority of data.
 *             delay - send the data after this delay.
 *             traceProtocol - specify the type of application used for
 *                 packet tracing.
 *             isMdpEnabled - gives the status of MDP layer.
 *             mdpUniqueId - unique id for MPD session.
 * RETURN:     none.
 */

Message *
APP_UdpSendNewHeaderVirtualDataWithPriority(
    Node *node,
    AppType appType,
    NodeAddress sourceAddr,
    unsigned short sourcePort,
    NodeAddress destAddr,
    char *header,
    int headerSize,
    int payloadSize,
    TosType priority,
    clocktype delay,
    TraceProtocolType traceProtocol,
    BOOL isMdpEnabled,
    Int32  mdpUniqueId
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam)
#else
    )
#endif
{
    Message *msg = NULL;
    ActionData acnData;
    msg = APP_UdpCreateNewHeaderVirtualDataWithPriority(node,
                                                        appType,
                                                        sourceAddr,
                                                        sourcePort,
                                                        destAddr,
                                                        header,
                                                        headerSize,
                                                        payloadSize,
                                                        priority,
                                                        traceProtocol);

    //Trace Information
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
                    PACKET_OUT, &acnData);

    if (isMdpEnabled)
    {
        Address localAddr, remoteAddr;

        memset(&localAddr,0,sizeof(Address));
        memset(&remoteAddr,0,sizeof(Address));

        localAddr.networkType = NETWORK_IPV4;
        localAddr.interfaceAddr.ipv4 = sourceAddr;

        remoteAddr.networkType = NETWORK_IPV4;
        remoteAddr.interfaceAddr.ipv4 = destAddr;

        MdpQueueDataObject(node,
                          localAddr,
                          remoteAddr,
                          mdpUniqueId,
                          headerSize,
                          header,
                          payloadSize,       // for virtual size
                          msg,
                          FALSE,
                          (UInt16)sourcePort);
    }
    else
    {
    MESSAGE_Send(node, msg, delay);
    }
    return msg;
}

// /**
// API         :: APP_UdpCreateNewHeaderVirtualDataWithPriority.
// PURPOSE     :: Create the message.
// PARAMETERS  ::
// + node           : Node *            : node that is sending the data.
// + sourceAddr     : Address       : the source sending the data.
// + sourcePort     : short             : the application source port.
// + destAddr       : Address       : the destination node Id data
//                                       is sent to.
// + destinationPort: short             : the destination port
// + header         : char*             : header of the payload.
// + headerSize     : int               : size of the header.
// + payloadSize    : int               : size of the data in bytes.
// + priority       : TosType           : priority of data.
// + traceProtocol  : TraceProtocolType : specify the type of application
//                                       used for packet tracing.
// RETURN         :: Message * :message created in the function
Message *
APP_UdpCreateNewHeaderVirtualDataWithPriority(Node *node,
               NodeAddress sourceAddr,
               short sourcePort,
               NodeAddress destAddr,
               short destinationPort,
               char *header,
               int headerSize,
               int payloadSize,
               TosType priority,
               TraceProtocolType traceProtocol
               )
{
    Message *msg;
    AppToUdpSend *info;
    ActionData acnData;

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_UDP,
              MSG_TRANSPORT_FromAppSend);

   //MESSAGE_PacketAlloc(node, msg, headerSize, traceProtocol);
    if (headerSize > 0)
    {
        MESSAGE_PacketAlloc(node, msg, headerSize, traceProtocol);
        memcpy(MESSAGE_ReturnPacket(msg), header, headerSize);
    }
    else
    {
        MESSAGE_PacketAlloc(node, msg,0, traceProtocol);
    }

    MESSAGE_AddVirtualPayload(node, msg, payloadSize);
    MESSAGE_InfoAlloc(node, msg, sizeof(AppToUdpSend));
    info = (AppToUdpSend *) MESSAGE_ReturnInfo(msg);

    SetIPv4AddressInfo(&info->sourceAddr, sourceAddr);
    info->sourcePort = sourcePort;
    SetIPv4AddressInfo(&info->destAddr, destAddr);
    info->destPort = destinationPort;
    info->priority = priority;
    info->outgoingInterface = ANY_INTERFACE;
    info->ttl = IPDEFTTL;
    //Trace Information
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
                    PACKET_OUT, &acnData);
     return msg;

}


/*
 * FUNCTION:   APP_UdpSendNewHeaderVirtualDataWithPriority
 * PURPOSE:    Allocate header + virtual data with specified priority and
 *             send to UDP. Delivers data to a particular destination port
 *             at the destination node. The destination port may not be the
 *             same as the AppType value.
 * PARAMETERS: node - node that is sending the data.
 *             sourceAddr - the source sending the data.
 *             sourcePort - the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             destinationPort - the destination port
 *             infoData - UDP header to be added in message info.
 *             infoSize - size of the UDP header.
 *             infoType - info type of the UDP header.
 *             payloadSize - size of the data in bytes.
 *             priority - priority of data.
 *             delay - send the data after this delay.
 *             traceProtocol - specify the type of application used for
 *                 packet tracing.
 *             isMdpEnabled - gives the status of MDP layer.
 *             mdpUniqueId - unique id for MPD session.
 * RETURN:     none.
 */
void
APP_UdpSendNewHeaderVirtualDataWithPriority(
    Node *node,
    NodeAddress sourceAddr,
    unsigned short sourcePort,
    NodeAddress destAddr,
    unsigned short destinationPort,
    char *infoData,
    int infoSize,
    unsigned short infoType,
    int payloadSize,
    TosType priority,
    clocktype delay,
    TraceProtocolType traceProtocol,
    BOOL isMdpEnabled,
    Int32 mdpUniqueId
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam)
#else
    )
#endif
{
    Message *msg;
    msg = APP_UdpCreateNewHeaderVirtualDataWithPriority(node,
                              sourceAddr,
                              sourcePort,
                              destAddr,
                              destinationPort,
                              NULL,
                              0,
                              payloadSize,
                              priority,
                              traceProtocol);

    // Fill in fragmentation info so packet can be reassembled
    if (infoData!= NULL)
    {
       char* udpHeaderInfo;
       udpHeaderInfo = (char*) MESSAGE_AddInfo(
                       node,
                       msg,
                       infoSize,
                       infoType);
       ERROR_Assert(udpHeaderInfo != NULL, "Unable to add an info field!");
       memcpy(udpHeaderInfo, infoData, infoSize);
    }
    char *dataSize = NULL;
    dataSize = MESSAGE_AddInfo(node, msg, sizeof(int), INFO_TYPE_DataSize);
    *((int*) dataSize) = payloadSize;

#ifdef ADDON_DB
    // STATS DB CODE.
    if (appParam != NULL)
    {
        HandleStatsDBAppSendEventsInsertion(
            node,
            msg,
            payloadSize,
            appParam);
    }
#endif

    if (isMdpEnabled)
    {
        Address localAddr, remoteAddr;

        memset(&localAddr,0,sizeof(Address));
        memset(&remoteAddr,0,sizeof(Address));

        localAddr.networkType = NETWORK_IPV4;
        localAddr.interfaceAddr.ipv4 = sourceAddr;

        remoteAddr.networkType = NETWORK_IPV4;
        remoteAddr.interfaceAddr.ipv4 = destAddr;

        MdpQueueDataObject(node,
                          localAddr,
                          remoteAddr,
                          mdpUniqueId,
                          0,
                          NULL,
                          payloadSize,       // for virtual size
                          msg,
                          FALSE,
                          (UInt16)sourcePort);
    }
    else
    {
        MESSAGE_Send(node, msg, delay);
    }
}



/*
 * FUNCTION:   APP_UdpSendNewHeaderVirtualDataWithPriority
 * PURPOSE:    Allocate header + virtual data with specified priority and
 *             send to UDP. Delivers data to a particular destination port
 *             at the destination node. The destination port may not be the
 *             same as the AppType value.
 * PARAMETERS: node - node that is sending the data.
 *             sourceAddr - the source sending the data.
 *             sourcePort - the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             destinationPort - the destination port
 *             infoData - UDP header to be added in message info.
 *             infoSize - size of the UDP header.
 *             infoType - info type of UDP header
 *             payloadSize - size of the data in bytes.
 *             priority - priority of data.
 *             delay - send the data after this delay.
 *             traceProtocol - specify the type of application used for
 *                 packet tracing.
 *             appname - Application name.
 *             isMdpEnabled - gives the status of MDP layer.
 *             mdpUniqueId - unique id for MPD session.
 * RETURN:     none.
 */

void
APP_UdpSendNewHeaderVirtualDataWithPriority(
    Node *node,
    NodeAddress sourceAddr,
    unsigned short sourcePort,
    NodeAddress destAddr,
    unsigned short destinationPort,
    char *infoData,
    int infoSize,
    unsigned short infoType,
    int payloadSize,
    TosType priority,
    clocktype delay,
    TraceProtocolType traceProtocol,
    char *appName,
    BOOL isMdpEnabled,
    Int32 mdpUniqueId
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam)
#else
    )
#endif
{
    Message *msg;
    msg = APP_UdpCreateNewHeaderVirtualDataWithPriority(node,
                               sourceAddr,
                               sourcePort,
                               destAddr,
                               destinationPort,
                               NULL,
                               0,
                               payloadSize,
                               priority,
                               traceProtocol);

    // Fill in fragmentation info so packet can be reassembled
     if (infoData!= NULL)
     {
        char* udpHeaderInfo;
        udpHeaderInfo = (char*) MESSAGE_AddInfo(
                         node,
                         msg,
                         infoSize,
                         infoType);
        ERROR_Assert(udpHeaderInfo != NULL, "Unable to add an info field!");
        memcpy(udpHeaderInfo, infoData, infoSize);
     }

    char *srcAdd = NULL;
    srcAdd = MESSAGE_AddInfo(node, msg, sizeof(NodeAddress), INFO_TYPE_SourceAddr);
    *((NodeAddress*) srcAdd) = sourceAddr;

    char *dstAdd = NULL;
    dstAdd = MESSAGE_AddInfo(node, msg, sizeof(NodeAddress), INFO_TYPE_DestAddr);
    *((NodeAddress*) dstAdd) = destAddr;

    char *srcPort = NULL;
    srcPort = MESSAGE_AddInfo(node, msg, sizeof(short), INFO_TYPE_SourcePort);
    *((short*) srcPort) = sourcePort;

    char *dstPort = NULL;
    dstPort = MESSAGE_AddInfo(node, msg, sizeof(short), INFO_TYPE_DestPort);
    *((short*) dstPort) = destinationPort;

    char *dataSize = NULL;
    dataSize = MESSAGE_AddInfo(node, msg, sizeof(int), INFO_TYPE_DataSize);
    *((int*) dataSize) = payloadSize;

    // STATS DB CODE.
#ifdef ADDON_DB
    if (appParam != NULL)
    {
        HandleStatsDBAppSendEventsInsertion(node,
            msg,
            payloadSize,
            appParam);
    }
#endif

    if (isMdpEnabled)
    {
        Address localAddr, remoteAddr;

        memset(&localAddr,0,sizeof(Address));
        memset(&remoteAddr,0,sizeof(Address));

        localAddr.networkType = NETWORK_IPV4;
        localAddr.interfaceAddr.ipv4 = sourceAddr;

        remoteAddr.networkType = NETWORK_IPV4;
        remoteAddr.interfaceAddr.ipv4 = destAddr;

        MdpQueueDataObject(node,
                          localAddr,
                          remoteAddr,
                          mdpUniqueId,
                          0,
                          NULL,
                          payloadSize,       // for virtual size
                          msg,
                          FALSE,
                          (UInt16)sourcePort);
    }
    else
    {
        MESSAGE_Send(node, msg, delay);
    }
}

/*
 * FUNCTION:   APP_UdpSendNewHeaderVirtualDataWithPriority
 * PURPOSE:    Allocate header + virtual data with specified priority and
 *             send to UDP. Delivers data to a particular destination port
 *             at the destination node. The destination port may not be the
 *             same as the AppType value.
 * PARAMETERS: node - node that is sending the data.
 *             sourceAddr - the source sending the data.
 *             sourcePort - the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             destinationPort - the destination port
 *             header - UDP header to be added in message info.
 *             headerSize - size of the UDP header.
 *             infoType - info type of UDP header
 *             payloadSize - size of the data in bytes.
 *             priority - priority of data.
 *             delay - send the data after this delay.
 *             traceProtocol - specify the type of application used for
 *                 packet tracing.
 *             appname - Application name.
 *             infoData - UDP header to be added in message info.
 *             infoSize - size of UDP header
 *             infoType - infot ype of UDP header.
 *             isMdpEnabled - gives the status of MDP layer.
 *             mdpUniqueId - unique id for MPD session.
 * RETURN:     none.
 */

void
APP_UdpSendNewHeaderVirtualDataWithPriority(
    Node *node,
    NodeAddress sourceAddr,
    unsigned short sourcePort,
    NodeAddress destAddr,
    unsigned short destinationPort,
    char *header,
    int headerSize,
    int payloadSize,
    TosType priority,
    clocktype delay,
    TraceProtocolType traceProtocol,
    char *appName,
    char *infoData,
    int infoSize,
    unsigned short infoType,
    BOOL isMdpEnabled,
    Int32 mdpUniqueId
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam)
#else
    )
#endif
{
    Message *msg;

     msg = APP_UdpCreateNewHeaderVirtualDataWithPriority(node,
                               sourceAddr,
                               sourcePort,
                               destAddr,
                               destinationPort,
                               header,
                               headerSize,
                               payloadSize,
                               priority,
                               traceProtocol);

    char *srcAdd = NULL;
    srcAdd = MESSAGE_AddInfo(node, msg, sizeof(NodeAddress), INFO_TYPE_SourceAddr);
    *((NodeAddress*) srcAdd) = sourceAddr;

    char *dstAdd = NULL;
    dstAdd = MESSAGE_AddInfo(node, msg, sizeof(NodeAddress), INFO_TYPE_DestAddr);
    *((NodeAddress*) dstAdd) = destAddr;

    char *srcPort = NULL;
    srcPort = MESSAGE_AddInfo(node, msg, sizeof(short), INFO_TYPE_SourcePort);
    *((short*) srcPort) = sourcePort;

    char *dstPort = NULL;
    dstPort = MESSAGE_AddInfo(node, msg, sizeof(short), INFO_TYPE_DestPort);
    *((short*) dstPort) = destinationPort;

    char *dataSize = NULL;
    dataSize = MESSAGE_AddInfo(node, msg, sizeof(int), INFO_TYPE_DataSize);
    *((int*) dataSize) = payloadSize + headerSize;

     // Fill in fragmentation info so packet can be reassembled
    if (infoData!= NULL)
    {
       char* udpHeaderInfo;
       udpHeaderInfo = (char *)MESSAGE_AddInfo(node,
                        msg,
                        infoSize,
                        infoType);
       ERROR_Assert(udpHeaderInfo != NULL, "Unable to add an info field!");
       memcpy(udpHeaderInfo, infoData, infoSize);
    }

    // STATS DB CODE.
#ifdef ADDON_DB
    if (appParam != NULL)
    {
        HandleStatsDBAppSendEventsInsertion(node,
            msg,
            payloadSize + headerSize,
            appParam);

        if (strcmp(appParam->m_ApplicationType, "SUPER-APPLICATION") == 0)
        {
            int * sessionIdInfo =  (int *)
                MESSAGE_ReturnInfo( msg, INFO_TYPE_StatsDbAppSessionId);
            if (sessionIdInfo == NULL)
            {
                sessionIdInfo = (int*) MESSAGE_AddInfo(
                    node, msg, sizeof(int), INFO_TYPE_StatsDbAppSessionId);
                *sessionIdInfo = appParam->m_SessionId;
            }
        }
    }
#endif

    if (isMdpEnabled)
    {
        Address localAddr, remoteAddr;

        memset(&localAddr,0,sizeof(Address));
        memset(&remoteAddr,0,sizeof(Address));

        localAddr.networkType = NETWORK_IPV4;
        localAddr.interfaceAddr.ipv4 = sourceAddr;

        remoteAddr.networkType = NETWORK_IPV4;
        remoteAddr.interfaceAddr.ipv4 = destAddr;

        MdpQueueDataObject(node,
                          localAddr,
                          remoteAddr,
                          mdpUniqueId,
                          headerSize,
                          header,
                          payloadSize,       // for virtual size
                          msg,
                          FALSE,
                          (UInt16)sourcePort);
    }
    else
    {
        MESSAGE_Send(node, msg, delay);
    }
}



/*
 * FUNCTION:   APP_UdpSendNewHeaderVirtualDataWithPriority
 * PURPOSE:    Allocate header + virtual data with specified priority and
 *             send to UDP. Delivers data to a particular destination port
 *             at the destination node. The destination port may not be the
 *             same as the AppType value.
 * PARAMETERS: node - node that is sending the data.
 *             sourceAddr - the source sending the data.
 *             sourcePort - the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             destinationPort - the destination port
 *             header - header of the payload.
 *             headerSize - size of the header.
 *             payloadSize - size of the data in bytes.
 *             priority - priority of data.
 *             delay - send the data after this delay.
 *             traceProtocol - specify the type of application used for
 *                 packet tracing.
 * RETURN:     none.
 */
void
APP_UdpSendNewHeaderVirtualDataWithPriority(
    Node *node,
    NodeAddress sourceAddr,
    unsigned short sourcePort,
    NodeAddress destAddr,
    unsigned short destinationPort,
    char *header,
    int headerSize,
    int payloadSize,
    TosType priority,
    clocktype delay,
    TraceProtocolType traceProtocol
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam)
#else
    )
#endif
{
    Message *msg;

     msg = APP_UdpCreateNewHeaderVirtualDataWithPriority(node,
                               sourceAddr,
                               sourcePort,
                               destAddr,
                               destinationPort,
                               header,
                               headerSize,
                               payloadSize,
                               priority,
                               traceProtocol);

    char *dataSize = NULL;
    dataSize = MESSAGE_AddInfo(node, msg, sizeof(int), INFO_TYPE_DataSize);
    *((int*) dataSize) = payloadSize + headerSize;

#ifdef ADDON_DB
    // STATS DB CODE.
    if (appParam != NULL)
    {
        HandleStatsDBAppSendEventsInsertion(
            node,
            msg,
            payloadSize + headerSize,
            appParam);

        if (strcmp(appParam->m_ApplicationType, "SUPER-APPLICATION") == 0)
        {
            int * sessionIdInfo =  (int *)
                MESSAGE_ReturnInfo( msg, INFO_TYPE_StatsDbAppSessionId);
            if (sessionIdInfo == NULL)
            {
                sessionIdInfo = (int*) MESSAGE_AddInfo(
                    node, msg, sizeof(int), INFO_TYPE_StatsDbAppSessionId);
                *sessionIdInfo = appParam->m_SessionId;
            }
        }
    }
#endif

    MESSAGE_Send(node, msg, delay);
}

/*
 * FUNCTION:   APP_UdpSendNewHeaderVirtualDataWithPriority
 * PURPOSE:    Allocate header + virtual data with specified priority and
 *             send to UDP. Delivers data to a particular destination port
 *             at the destination node. The destination port may not be the
 *             same as the AppType value.
 * PARAMETERS: node - node that is sending the data.
 *             sourceAddr - the source sending the data.
 *             sourcePort - the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             destinationPort - the destination port
 *             header - header of the payload.
 *             headerSize - size of the header.
 *             payloadSize - size of the data in bytes.
 *             priority - priority of data.
 *             delay - send the data after this delay.
 *             traceProtocol - specify the type of application used for
 *                 packet tracing.
  *             appname - Application name.
 * RETURN:     none.
 */

void
APP_UdpSendNewHeaderVirtualDataWithPriority(
    Node *node,
    NodeAddress sourceAddr,
    unsigned short sourcePort,
    NodeAddress destAddr,
    unsigned short destinationPort,
    char *header,
    int headerSize,
    int payloadSize,
    TosType priority,
    clocktype delay,
    TraceProtocolType traceProtocol,
    char *appName
#ifdef ADDON_DB
    , StatsDBAppEventParam* appParam
#endif
    )
{
    Message *msg;
    msg = APP_UdpCreateNewHeaderVirtualDataWithPriority(node,
                               sourceAddr,
                               sourcePort,
                               destAddr,
                               destinationPort,
                               header,
                               headerSize,
                               payloadSize,
                               priority,
                               traceProtocol);

    char *srcAdd = NULL;
    srcAdd = MESSAGE_AddInfo(node, msg, sizeof(NodeAddress), INFO_TYPE_SourceAddr);
    *((NodeAddress*) srcAdd) = sourceAddr;

    char *dstAdd = NULL;
    dstAdd = MESSAGE_AddInfo(node, msg, sizeof(NodeAddress), INFO_TYPE_DestAddr);
    *((NodeAddress*) dstAdd) = destAddr;

    char *srcPort = NULL;
    srcPort = MESSAGE_AddInfo(node, msg, sizeof(short), INFO_TYPE_SourcePort);
    *((short*) srcPort) = sourcePort;

    char *dstPort = NULL;
    dstPort = MESSAGE_AddInfo(node, msg, sizeof(short), INFO_TYPE_DestPort);
    *((short*) dstPort) = destinationPort;

    char *dataSize = NULL;
    dataSize = MESSAGE_AddInfo(node, msg, sizeof(int), INFO_TYPE_DataSize);
    *((int*) dataSize) = payloadSize + headerSize;

    // STATS DB CODE.
#ifdef ADDON_DB
    if (appParam != NULL)
    {
        HandleStatsDBAppSendEventsInsertion(node,
            msg,
            payloadSize + headerSize,
            appParam);

        if (strcmp(appParam->m_ApplicationType, "SUPER-APPLICATION") == 0)
        {
            int * sessionIdInfo =  (int *)
                MESSAGE_ReturnInfo( msg, INFO_TYPE_StatsDbAppSessionId);
            if (sessionIdInfo == NULL)
            {
                sessionIdInfo = (int*) MESSAGE_AddInfo(
                    node, msg, sizeof(int), INFO_TYPE_StatsDbAppSessionId);
                *sessionIdInfo = appParam->m_SessionId;
            }
        }

    }
#endif


    MESSAGE_Send(node, msg, delay);

}


/*
 * FUNCTION:   APP_UdpSendNewHeaderVirtualDataWithPriority
 * PURPOSE:    Allocate header + virtual data with specified priority and
 *             send to UDP. Delivers data to a particular destination port
 *             at the destination node. The destination port may not be the
 *             same as the AppType value.
 * PARAMETERS: node - node that is sending the data.
 *             sourceAddr - the source sending the data.
 *             sourcePort - the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             destinationPort - the destination port
 *             header - header of the payload.
 *             headerSize - size of the header.
 *             payloadSize - size of the data in bytes.
 *             priority - priority of data.
 *             delay - send the data after this delay.
 *             traceProtocol - specify the type of application used for
 *                 packet tracing.
 *             appname - Application name.
 *.............data - super application UDP data packet
 * RETURN:     Message *.
 */


Message *
APP_UdpSendNewHeaderVirtualDataWithPriority(
    Node *node,
    NodeAddress sourceAddr,
    unsigned short sourcePort,
    NodeAddress destAddr,
    unsigned short destinationPort,
    char *header,
    int headerSize,
    int payloadSize,
    TosType priority,
    clocktype delay,
    TraceProtocolType traceProtocol,
    char *appName,
    SuperApplicationUDPDataPacket *data,
    BOOL isMdpEnabled,
    Int32 mdpUniqueId
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam)
#else
    )
#endif
{
#ifdef ADDON_DB
    UInt64 fragSentForThisPacket = 0;
    UInt64 bytesSentForThisPacket = 0;
    UInt64 msgSent = 0;
#endif
    Message *msg;
       msg = APP_UdpCreateNewHeaderVirtualDataWithPriority(node,
                               sourceAddr,
                               sourcePort,
                               destAddr,
                               destinationPort,
                               header,
                               headerSize,
                               payloadSize,
                               priority,
                               traceProtocol);

    char *srcAdd = NULL;
    srcAdd = MESSAGE_AddInfo(node, msg, sizeof(NodeAddress), INFO_TYPE_SourceAddr);
    *((NodeAddress*) srcAdd) = sourceAddr;

    char *dstAdd = NULL;
    dstAdd = MESSAGE_AddInfo(node, msg, sizeof(NodeAddress), INFO_TYPE_DestAddr);
    *((NodeAddress*) dstAdd) = destAddr;

    char *srcPort = NULL;
    srcPort = MESSAGE_AddInfo(node, msg, sizeof(short), INFO_TYPE_SourcePort);
    *((short*) srcPort) = sourcePort;

    char *dstPort = NULL;
    dstPort = MESSAGE_AddInfo(node, msg, sizeof(short), INFO_TYPE_DestPort);
    *((short*) dstPort) = destinationPort;

    char *dataSize = NULL;
    dataSize = MESSAGE_AddInfo(node, msg, sizeof(int), INFO_TYPE_DataSize);
    *((int*) dataSize) = payloadSize + headerSize;

    char *superAppUDPData = NULL;
    superAppUDPData = MESSAGE_AddInfo(node, msg, sizeof(SuperApplicationUDPDataPacket), INFO_TYPE_SuperAppUDPData);
    memset(superAppUDPData, 0, sizeof(SuperApplicationUDPDataPacket));
    memcpy(superAppUDPData, data, sizeof(SuperApplicationUDPDataPacket));

    // STATS DB CODE.
#ifdef ADDON_DB
    if (appParam != NULL)
    {
        HandleStatsDBAppSendEventsInsertion(node,
            msg,
            payloadSize + headerSize,
            appParam);

        if (strcmp(appParam->m_ApplicationType, "SUPER-APPLICATION") == 0)
        {
            int * sessionIdInfo =  (int *)
                MESSAGE_ReturnInfo( msg, INFO_TYPE_StatsDbAppSessionId);
            if (sessionIdInfo == NULL)
            {
                sessionIdInfo = (int*) MESSAGE_AddInfo(
                    node, msg, sizeof(int), INFO_TYPE_StatsDbAppSessionId);
                *sessionIdInfo = appParam->m_SessionId;
            }
        }
    }
#endif

    if (isMdpEnabled)
    {
        Address sourceNodeAddr;
        Address destNodeAddr;

        sourceNodeAddr.networkType = NETWORK_IPV4;
        sourceNodeAddr.interfaceAddr.ipv4 = sourceAddr;

        destNodeAddr.networkType = NETWORK_IPV4;
        destNodeAddr.interfaceAddr.ipv4 = destAddr;

        MdpQueueDataObject(node,
                          sourceNodeAddr,
                          destNodeAddr,
                          mdpUniqueId,
                          headerSize,
                          header,
                          payloadSize,       // for virtual size
                          msg,
                          FALSE,
                          (UInt16)sourcePort);
    }
    else
    {
        MESSAGE_Send(node, msg, delay);
    }
    return msg;

}

#ifdef ADDON_BOEINGFCS
Message*
APP_UdpSendNewHeaderVirtualDataWithPriority(
    Node *node,
    NodeAddress sourceAddr,
    unsigned short sourcePort,
    NodeAddress destAddr,
    unsigned short destinationPort,
    char *header,
    int headerSize,
    int payloadSize,
    TosType priority,
    clocktype delay,
    TraceProtocolType traceProtocol,
    char *appName,
    SuperApplicationUDPDataPacket *data,
    int sessionId,
    int requiredTput,
    clocktype requiredDelay,
    float dataRate,
    BOOL isMdpEnabled,
    Int32 mdpUniqueId
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam)
#else
    )
#endif
{
    Message *msg;
#ifdef ADDON_DB
    UInt64 fragSentForThisPacket = 0;
    UInt64 bytesSentForThisPacket = 0;
    UInt64 msgSent = 0;
#endif
       msg = APP_UdpCreateNewHeaderVirtualDataWithPriority(node,
                               sourceAddr,
                               sourcePort,
                               destAddr,
                               destinationPort,
                               header,
                               headerSize,
                               payloadSize,
                               priority,
                               traceProtocol);

    char *appNameInfo;
    int infoSize = 0;

    char *srcAdd = NULL;
    srcAdd = MESSAGE_AddInfo(node, msg, sizeof(NodeAddress), INFO_TYPE_SourceAddr);
    *((NodeAddress*) srcAdd) = sourceAddr;

    char *dstAdd = NULL;
    dstAdd = MESSAGE_AddInfo(node, msg, sizeof(NodeAddress), INFO_TYPE_DestAddr);
    *((NodeAddress*) dstAdd) = destAddr;

    char *srcPort = NULL;
    srcPort = MESSAGE_AddInfo(node, msg, sizeof(short), INFO_TYPE_SourcePort);
    *((short*) srcPort) = sourcePort;

    char *dstPort = NULL;
    dstPort = MESSAGE_AddInfo(node, msg, sizeof(short), INFO_TYPE_DestPort);
    *((short*) dstPort) = destinationPort;

    char *dataSize = NULL;
    dataSize = MESSAGE_AddInfo(node, msg, sizeof(int), INFO_TYPE_DataSize);
    *((int*) dataSize) = payloadSize + headerSize;

    char *superAppUDPData = NULL;
    superAppUDPData = MESSAGE_AddInfo(node, msg, sizeof(SuperApplicationUDPDataPacket), INFO_TYPE_SuperAppUDPData);
    memset(superAppUDPData, 0, sizeof(SuperApplicationUDPDataPacket));
    memcpy(superAppUDPData, data, sizeof(SuperApplicationUDPDataPacket));

    // STATS DB CODE.
#ifdef ADDON_DB
    if (appParam != NULL)
    {
        HandleStatsDBAppSendEventsInsertion(node,
            msg,
            payloadSize + headerSize,
            appParam);
        if (strcmp(appParam->m_ApplicationType, "SUPER-APPLICATION") == 0)
        {
            int * sessionIdInfo =  (int *)
                MESSAGE_ReturnInfo( msg, INFO_TYPE_StatsDbAppSessionId);
            if (sessionIdInfo == NULL)
            {
                sessionIdInfo = (int*) MESSAGE_AddInfo(
                    node, msg, sizeof(int), INFO_TYPE_StatsDbAppSessionId);
                *sessionIdInfo = appParam->m_SessionId;
            }
        }
    }
#endif

    if (isMdpEnabled)
    {
        Address sourceNodeAddr;
        Address destNodeAddr;

        sourceNodeAddr.networkType = NETWORK_IPV4;
        sourceNodeAddr.interfaceAddr.ipv4 = sourceAddr;

        destNodeAddr.networkType = NETWORK_IPV4;
        destNodeAddr.interfaceAddr.ipv4 = destAddr;

        MdpQueueDataObject(node,
                          sourceNodeAddr,
                          destNodeAddr,
                          mdpUniqueId,
                          headerSize,
                          header,
                          payloadSize,       //for virtual size
                          msg,
                          FALSE,
                          (UInt16)sourcePort);
    }
    else
    {
        MESSAGE_Send(node, msg, delay);
    }
    return msg;
}
#endif

/*
.* FUNCTION        :: APP_UdpSendNewHeaderVirtualDataWithPriority
.* PURPOSE         :: (Overloaded for MDP) Allocate actual + virtual
.*                    data with specified priority and send to UDP.
.*                    Data is sent to a non-default destination port
.*                    (port number may not have same value as the AppType).
.* PARAMETERS: node - node that is sending the data.
.*             appType - specify the application type.
.*             sourceAddr - the source sending the data.
.*             sourcePort - the application source port.
.*             destAddr - the destination node on which data is sent to.
.*             destPort - the destination port
.*             header - pointer to the payload.
.*             headerSize - size of the payload.
.*             payloadSize - size of the virtual data in bytes.
.*             priority - priority of data.
.*             delay - send the data after this delay.
.*             mdpInfo - persistent info for Mdp.
.*             traceProtocol - specify the application type for packet tracing.
.*             theMsgPtr - pointer the original message to copy the
.*                         persistent info. Default value is NULL.
 * RETURN      none
 */
void
APP_UdpSendNewHeaderVirtualDataWithPriority(
    Node *node,
    AppType appType,
    Address sourceAddr,
    unsigned short sourcePort,
    Address destAddr,
    unsigned short destPort,
    char *header,
    int headerSize,
    int payloadSize,
    TosType priority,
    clocktype delay,
    char* mdpInfo,
    TraceProtocolType traceProtocol,
    Message* theMsgPtr
#ifdef ADDON_DB
    ,
    BOOL fragIdSpecified,
    Int32 fragId
#endif
    )
{
    ActionData acnData;
    Message* msg;
    AppToUdpSend* info;
    AppToUdpSend* infoMsgPtr;
    MdpPersistentInfo* mdpInfoPtr = (MdpPersistentInfo*)mdpInfo;

    // populating dest port value from the original message to MDP info
    if (theMsgPtr)
    {
        infoMsgPtr = (AppToUdpSend*) MESSAGE_ReturnInfo(theMsgPtr);
        mdpInfoPtr->destPort = infoMsgPtr->destPort;
    }
    else
    {
        mdpInfoPtr->destPort = destPort;
    }

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_UDP,
              MSG_TRANSPORT_FromAppSend);

        // Copy info field of the original message.
        if (theMsgPtr != NULL)
        {
            if (theMsgPtr->infoArray.size() > 0)
            {
                // Copy the info fields.
                MESSAGE_CopyInfo(node, msg, theMsgPtr);
            }
        }

    if (headerSize > 0)
    {
        MESSAGE_PacketAlloc(node, msg, headerSize, traceProtocol);
        memcpy(MESSAGE_ReturnPacket(msg), header, headerSize);
    }
    // Add virtual data if present
    if (payloadSize > 0)
    {
        MESSAGE_AddVirtualPayload(node, msg, payloadSize);
    }

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToUdpSend));
    info = (AppToUdpSend *) MESSAGE_ReturnInfo(msg);
    info->sourceAddr = sourceAddr;
    info->sourcePort = sourcePort;
    info->destAddr = destAddr;

    // first check for external MDP packet
    if (destAddr.networkType == NETWORK_IPV4)
    {
        if (MdpIsExataIncomingPort(node,(unsigned short)destPort)
            || MdpIsExataPortInOutPortList(node,
                                           (unsigned short)destPort,
                                           destAddr.interfaceAddr.ipv4))
        {
            info->destPort = destPort;
        }
        else
        {
            info->destPort = (unsigned short) appType;
        }
    }
    else
    {
        if (MdpIsExataIncomingPort(node,(unsigned short)destPort))
        {
            info->destPort = destPort;
        }
        else
        {
            info->destPort = (unsigned short) appType;
        }
    }

    info->priority = priority;
    info->outgoingInterface = ANY_INTERFACE;
    info->ttl = IPDEFTTL;

    //Trace Information
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
                    PACKET_OUT, &acnData);

    char *mdpInfoData = NULL;
    mdpInfoData = MESSAGE_AddInfo(node,
                                  msg,
                                  sizeof(MdpPersistentInfo),
                                  INFO_TYPE_MdpInfoData);

    memcpy(mdpInfoData, mdpInfoPtr, sizeof(MdpPersistentInfo));

#ifdef ADDON_DB
    // STATS DB code to generate the new message ID if the app message
    // is fragmented by MDP
    if (fragIdSpecified)
    {
        StatsDBAppendMessageMsgId(node, msg, fragId, 'T');
    }
#endif

    MESSAGE_Send(node, msg, delay);
}


/*
 * FUNCTION:   APP_UdpSendNewHeaderVirtualDataWithPriority
 * PURPOSE:    Allocate header + virtual data with specified priority and
 *             send to UDP. Delivers data to a particular destination port
 *             at the destination node. The destination port may not be the
 *             same as the AppType value.
 * PARAMETERS: node - node that is sending the data.
 *             appType - application type
 *             sourceAddr - the source sending the data.
 *             sourcePort - the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             destinationPort - the destination port
 *             header - header of the payload.
 *             headerSize - size of the header.
 *             payloadSize - size of the data in bytes.
 *             priority - priority of data.
 *             delay - send the data after this delay.
 *             traceProtocol - specify the type of application used for
 *                 packet tracing.
 *             isMdpEnabled - gives the status of MDP layer.
 *             mdpUniqueId - unique id for MPD session.
 * RETURN:     Message *.
 */

Message *
APP_UdpSendNewHeaderVirtualDataWithPriority(
    Node *node,
    AppType appType,
    Address sourceAddr,
    unsigned short sourcePort,
    Address destAddr,
    char *header,
    int headerSize,
    int payloadSize,
    TosType priority,
    clocktype delay,
    TraceProtocolType traceProtocol,
    BOOL isMdpEnabled,
    Int32 mdpUniqueId
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam)
#else
    )
#endif
{
    ActionData acnData;
    Message *msg = NULL;
    AppToUdpSend *info = NULL;

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_UDP,
              MSG_TRANSPORT_FromAppSend);

    MESSAGE_PacketAlloc(node, msg, headerSize, traceProtocol);

    if (headerSize > 0)
    {
        memcpy(MESSAGE_ReturnPacket(msg), header, headerSize);
    }

    MESSAGE_AddVirtualPayload(node, msg, payloadSize);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToUdpSend));
    info = (AppToUdpSend *) MESSAGE_ReturnInfo(msg);
    info->sourceAddr = sourceAddr;
    info->sourcePort = sourcePort;
    info->destAddr = destAddr;
    info->destPort = (unsigned short) appType;
    info->priority = priority;
    info->outgoingInterface = ANY_INTERFACE;
    info->ttl = IPDEFTTL;

    //Trace Information
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
                    PACKET_OUT, &acnData);

    char *dataSize = NULL;
    dataSize = MESSAGE_AddInfo(node, msg, sizeof(int), INFO_TYPE_DataSize);
    *((int*) dataSize) = payloadSize;

    // STATS DB CODE.
#ifdef ADDON_DB
    if (appParam != NULL)
    {
        HandleStatsDBAppSendEventsInsertion(node,
            msg,
            payloadSize + headerSize,
            appParam);
    }
#endif
#ifdef ADDON_DB
    if (appType == APP_CBR_SERVER)
    {
        AppDataCbrClient *clientPtr =
            AppCbrClientGetCbrClient(node, sourcePort);

        int * sessionIdInfo =  (int *)
            MESSAGE_ReturnInfo( msg, INFO_TYPE_StatsDbAppSessionId);
        if (sessionIdInfo == NULL)
        {
            sessionIdInfo = (int*) MESSAGE_AddInfo(
                node, msg, sizeof(int), INFO_TYPE_StatsDbAppSessionId);
            *sessionIdInfo = clientPtr->uniqueId;
        }
    }
#endif

    if (isMdpEnabled)
    {
        MdpQueueDataObject(node,
                          sourceAddr,
                          destAddr,
                          mdpUniqueId,
                          headerSize,
                          header,
                          payloadSize,       //for virtual size
                          msg,
                          FALSE,
                          (UInt16)sourcePort);
    }
    else
    {
        MESSAGE_Send(node, msg, delay);
    }
    return msg;
}

/*
 * FUNCTION:   APP_UdpSendNewHeaderVirtualDataWithPriority
 * PURPOSE:    Allocate header + virtual data with specified priority and
 *             send to UDP. Delivers data to a particular destination port
 *             at the destination node. The destination port may not be the
 *             same as the AppType value.
 * PARAMETERS: node - node that is sending the data.
 *             sourceAddr - the source sending the data.
 *             sourcePort - the application source port.
 *             destAddr - the destination node Id data is sent to.
 *             destinationPort - the destination port
 *             header - header of the payload.
 *             headerSize - size of the header.
 *             payloadSize - size of the data in bytes.
 *             priority - priority of data.
 *             endTime - zigbeeApp end time.
 *             itemSize - zigbeeApp item size.
 *             interval - zigbeeApp interval
 *             delay - send the data after this delay.
 *             traceProtocol - specify the type of application used for
 *                 packet tracing.
 *             isMdpEnabled - gives the status of MDP layer.
 *             mdpUniqueId - unique id for MPD session.
 * RETURN:     none.
 */

void
APP_UdpSendNewHeaderVirtualDataWithPriority(
    Node* node,
    AppType appType,
    Address sourceAddr,
    unsigned short sourcePort,
    Address destAddr,
    char* header,
    int headerSize,
    int payloadSize,
    TosType priority,
    clocktype endTime,
    UInt32 itemSize,
    D_Clocktype interval,
    clocktype delay,
    TraceProtocolType traceProtocol)
{
    ActionData acnData;
    Message* msg = NULL;
    AppToUdpSend* info = NULL;

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_UDP,
              MSG_TRANSPORT_FromAppSend);

    MESSAGE_PacketAlloc(node, msg, headerSize, traceProtocol);

    if (headerSize > 0)
    {
        memcpy(MESSAGE_ReturnPacket(msg), header, headerSize);
    }

    MESSAGE_AddVirtualPayload(node, msg, payloadSize);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToUdpSend));
    info = (AppToUdpSend *) MESSAGE_ReturnInfo(msg);
    info->sourceAddr = sourceAddr;
    info->sourcePort = sourcePort;
    info->destAddr = destAddr;
    info->destPort = (unsigned short) appType;
    info->priority = priority;
    info->outgoingInterface = ANY_INTERFACE;
    info->ttl = IPDEFTTL;

    // Add the zigbeeapp information as an info field in the packet
    ZigbeeAppInfo* zigbeeAppInfo = NULL;
    zigbeeAppInfo = (ZigbeeAppInfo*)MESSAGE_AddInfo(node,
                                                    msg,
                                                    sizeof(ZigbeeAppInfo),
                                                    INFO_TYPE_ZigbeeApp_Info);
    zigbeeAppInfo->priority = priority;
    zigbeeAppInfo->endTime = endTime;
    zigbeeAppInfo->zigbeeAppInterval = interval;
    zigbeeAppInfo->zigbeeAppPktSize = itemSize;
    zigbeeAppInfo->srcAddress = GetIPv4Address(sourceAddr);
    zigbeeAppInfo->srcPort = sourcePort;

    // Trace Information
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
                    PACKET_OUT, &acnData);

    char* dataSize = NULL;
    dataSize = MESSAGE_AddInfo(node, msg, sizeof(int), INFO_TYPE_DataSize);
    *((int*) dataSize) = payloadSize;
    MESSAGE_Send(node, msg, delay);
}



/*
 * NAME:        APP_TcpServerListen.
 * PURPOSE:     Listen on a server port.
 * PARAMETERS:  node - pointer to the node.
 *              appType - which application initiates this request.
 *              serverAddr - server address.
 *              serverPort - server port number.
 * RETURN:      none.
 */
void
APP_TcpServerListen(
    Node *node,
    AppType appType,
    NodeAddress serverAddr,
    short serverPort)
{
    AppToTcpListen *listenRequest;
    Message *msg;

#ifdef DEBUG
    printf("Node %ld (TCP Server) starting to listen.\n",
            node->nodeId);
#endif /* DEBUG */

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_TCP,
              MSG_TRANSPORT_FromAppListen);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToTcpListen));
    listenRequest = (AppToTcpListen *) MESSAGE_ReturnInfo(msg);
    listenRequest->appType = appType;
    SetIPv4AddressInfo(&listenRequest->localAddr, serverAddr);
    listenRequest->localPort = serverPort;
    listenRequest->priority = APP_DEFAULT_TOS;
    MESSAGE_Send(node, msg, 0);
}


/*
 * NAME:        APP_TcpServerListen.
 * PURPOSE:     (Overloaded for IPv6) Listen on a server port.
 * PARAMETERS:  node - pointer to the node.
 *              appType - which application initiates this request.
 *              serverAddr - server address.
 *              serverPort - server port number.
 * RETURN:      none.
 */
void
APP_TcpServerListen(
    Node *node,
    AppType appType,
    Address serverAddr,
    short serverPort)
{
    AppToTcpListen *listenRequest;
    Message *msg;

#ifdef DEBUG
    printf("Node %ld (TCP Server) starting to listen.\n",
            node->nodeId);
#endif /* DEBUG */

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_TCP,
              MSG_TRANSPORT_FromAppListen);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToTcpListen));
    listenRequest = (AppToTcpListen *) MESSAGE_ReturnInfo(msg);
    listenRequest->appType = appType;
    listenRequest->localAddr = serverAddr;
    listenRequest->localPort = serverPort;
    listenRequest->priority = APP_DEFAULT_TOS;
    MESSAGE_Send(node, msg, 0);
}

/*
 * NAME:        APP_TcpServerListenWithPriority.
 * PURPOSE:     Listen on a server port with specified priority.
 * PARAMETERS:  node - pointer to the node.
 *              appType - which application initiates this request.
 *              serverAddr - server address.
 *              serverPort - server port number.
 *              priority - priority of this data for this session.
 * RETURN:      none.
 */
void
APP_TcpServerListenWithPriority(
    Node *node,
    AppType appType,
    NodeAddress serverAddr,
    short serverPort,
    TosType priority)
{
    AppToTcpListen *listenRequest;
    Message *msg;

#ifdef DEBUG
    printf("Node %ld (TCP Server) starting to listen.\n",
            node->nodeId);
#endif /* DEBUG */

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_TCP,
              MSG_TRANSPORT_FromAppListen);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToTcpListen));
    listenRequest = (AppToTcpListen *) MESSAGE_ReturnInfo(msg);
    listenRequest->appType = appType;
    // listenRequest->localAddr = serverAddr;
    SetIPv4AddressInfo(&listenRequest->localAddr, serverAddr);
    listenRequest->localPort = serverPort;
    listenRequest->priority = priority;
    MESSAGE_Send(node, msg, 0);
}
/*
 * NAME:        APP_TcpServerListenWithPriority.
 * PURPOSE:     (Overloaded for IPv6) Listen on a server port with specified priority.
 * PARAMETERS:  node - pointer to the node.
 *              appType - which application initiates this request.
 *              serverAddr - server address.
 *              serverPort - server port number.
 *              priority - priority of this data for this session.
 * RETURN:      none.
 */
void
APP_TcpServerListenWithPriority(
    Node *node,
    AppType appType,
    Address serverAddr,
    short serverPort,
    TosType priority)
{
    AppToTcpListen *listenRequest;
    Message *msg;

#ifdef DEBUG
    printf("Node %ld (TCP Server) starting to listen.\n",
            node->nodeId);
#endif /* DEBUG */

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_TCP,
              MSG_TRANSPORT_FromAppListen);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToTcpListen));
    listenRequest = (AppToTcpListen *) MESSAGE_ReturnInfo(msg);
    listenRequest->appType = appType;
    listenRequest->localAddr = serverAddr;
    listenRequest->localPort = serverPort;
    listenRequest->priority = priority;
    MESSAGE_Send(node, msg, 0);
}

/*
 * NAME:        APP_TcpOpenConnection.
 * PURPOSE:     Open a connection.
 * PARAMETERS:  node - pointer to the node.
 *              appType - which application initiates this request.
 *              localAddr - address of the source node.
 *              localPort - port number on the source node.
 *              remoteAddr - address of the remote node.
 *              remotePort - port number on the remote node (server port).
 *              uniqueId - used to determine which client is requesting
 *                         connection.
 *              waitTime - time until the session starts.
 * RETURN:      none.
 */
void
APP_TcpOpenConnection(
    Node *node,
    AppType appType,
    NodeAddress localAddr,
    short localPort,
    NodeAddress remoteAddr,
    short remotePort,
    int uniqueId,
    clocktype waitTime)
{
    AppToTcpOpen *openRequest;
    Message *msg;

#ifdef DEBUG
    printf("Node %ld (TCP Client) starting to open connection.\n",
           node->nodeId);
#endif /* DEBUG */

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_TCP,
              MSG_TRANSPORT_FromAppOpen);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToTcpOpen));
    openRequest = (AppToTcpOpen *) MESSAGE_ReturnInfo(msg);
    openRequest->appType = appType;
    SetIPv4AddressInfo(&openRequest->localAddr, localAddr);
    // openRequest->localAddr = localAddr;
    openRequest->localPort = localPort;
    // openRequest->remoteAddr = remoteAddr;
    SetIPv4AddressInfo(&openRequest->remoteAddr, remoteAddr);
    openRequest->remotePort = remotePort;

    openRequest->uniqueId = uniqueId;
    openRequest->priority = APP_DEFAULT_TOS;
    openRequest->outgoingInterface = ANY_INTERFACE;

    MESSAGE_Send(node, msg, waitTime);
}
/*
 * NAME:        APP_TcpOpenConnection.
 * PURPOSE:     (Overloaded for IPv6) Open a connection.
 * PARAMETERS:  node - pointer to the node.
 *              appType - which application initiates this request.
 *              localAddr - address of the source node.
 *              localPort - port number on the source node.
 *              remoteAddr - address of the remote node.
 *              remotePort - port number on the remote node (server port).
 *              uniqueId - used to determine which client is requesting
 *                         connection.
 *              waitTime - time until the session starts.
 * RETURN:      none.
 */
void
APP_TcpOpenConnection(
    Node *node,
    AppType appType,
    Address localAddr,
    short localPort,
    Address remoteAddr,
    short remotePort,
    int uniqueId,
    clocktype waitTime)
{
    AppToTcpOpen *openRequest;
    Message *msg;

#ifdef DEBUG
    printf("Node %ld (TCP Client) starting to open connection.\n",
           node->nodeId);
#endif /* DEBUG */

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_TCP,
              MSG_TRANSPORT_FromAppOpen);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToTcpOpen));
    openRequest = (AppToTcpOpen *) MESSAGE_ReturnInfo(msg);
    openRequest->appType = appType;
    openRequest->localAddr = localAddr;
    openRequest->localPort = localPort;
    openRequest->remoteAddr = remoteAddr;
    openRequest->remotePort = remotePort;

    openRequest->uniqueId = uniqueId;
    openRequest->priority = APP_DEFAULT_TOS;
    openRequest->outgoingInterface = ANY_INTERFACE;

    MESSAGE_Send(node, msg, waitTime);
}


// /**
// API        :: APP_TcpOpenConnection.
// PURPOSE    :: Open a connection.
// PARAMETERS ::
// + outgoingInterface : int : User specific outgoing Interface.
void
APP_TcpOpenConnection(
    Node *node,
    AppType appType,
    NodeAddress localAddr,
    short localPort,
    NodeAddress remoteAddr,
    short remotePort,
    int uniqueId,
    clocktype waitTime,
    int outgoingInterface)
{
    AppToTcpOpen *openRequest;
    Message *msg;

#ifdef DEBUG
    printf("Node %ld (TCP Client) starting to open connection.\n",
           node->nodeId);
#endif /* DEBUG */

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_TCP,
              MSG_TRANSPORT_FromAppOpen);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToTcpOpen));
    openRequest = (AppToTcpOpen *) MESSAGE_ReturnInfo(msg);
    openRequest->appType = appType;
    // openRequest->localAddr = localAddr;
    SetIPv4AddressInfo(&openRequest->localAddr, localAddr);
    openRequest->localPort = localPort;
    // openRequest->remoteAddr = remoteAddr;
    SetIPv4AddressInfo(&openRequest->remoteAddr, remoteAddr);
    openRequest->remotePort = remotePort;

    openRequest->uniqueId = uniqueId;
    openRequest->priority = APP_DEFAULT_TOS;

    if (node->numberInterfaces < outgoingInterface ||
        outgoingInterface < ANY_INTERFACE)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Node: %u\n"
            "Value of outgoingInterface is out of range\n", node->nodeId);
        ERROR_ReportWarning(errString);
        openRequest->outgoingInterface = ANY_INTERFACE;
    }
    openRequest->outgoingInterface = outgoingInterface;

    MESSAGE_Send(node, msg, waitTime);
}


// /**
// API        :: APP_TcpOpenConnection.
// PURPOSE    :: (Overloaded for IPv6) Open a connection.
// PARAMETERS ::
// + outgoingInterface : int : User specific outgoing Interface.
void
APP_TcpOpenConnection(
    Node *node,
    AppType appType,
    Address localAddr,
    short localPort,
    Address remoteAddr,
    short remotePort,
    int uniqueId,
    clocktype waitTime,
    int outgoingInterface)
{
    AppToTcpOpen *openRequest;
    Message *msg;

#ifdef DEBUG
    printf("Node %ld (TCP Client) starting to open connection.\n",
           node->nodeId);
#endif /* DEBUG */

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_TCP,
              MSG_TRANSPORT_FromAppOpen);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToTcpOpen));
    openRequest = (AppToTcpOpen *) MESSAGE_ReturnInfo(msg);
    openRequest->appType = appType;
    openRequest->localAddr = localAddr;
    openRequest->localPort = localPort;
    openRequest->remoteAddr = remoteAddr;
    openRequest->remotePort = remotePort;

    openRequest->uniqueId = uniqueId;
    openRequest->priority = APP_DEFAULT_TOS;

    if (node->numberInterfaces < outgoingInterface ||
        outgoingInterface < ANY_INTERFACE)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Node: %u\n"
            "Value of outgoingInterface is out of range\n", node->nodeId);
        ERROR_ReportWarning(errString);
        openRequest->outgoingInterface = ANY_INTERFACE;
    }
    openRequest->outgoingInterface = outgoingInterface;

    MESSAGE_Send(node, msg, waitTime);
}


/*
 * NAME:        APP_TcpOpenConnectionWithPriority.
 * PURPOSE:     Open a connection with specified priority.
 * PARAMETERS:  node - pointer to the node.
 *              appType - which application initiates this request.
 *              localAddr - address of the source node.
 *              localPort - port number on the source node.
 *              remoteAddr - address of the remote node.
 *              remotePort - port number on the remote node (server port).
 *              uniqueId - used to determine which client is requesting
 *                         connection.
 *              waitTime - time until the session starts.
 *              priority - priority of the data.
 * RETURN:      none.
 */
void
APP_TcpOpenConnectionWithPriority(
    Node *node,
    AppType appType,
    NodeAddress localAddr,
    short localPort,
    NodeAddress remoteAddr,
    short remotePort,
    int uniqueId,
    clocktype waitTime,
    TosType priority)
{
    AppToTcpOpen *openRequest;
    Message *msg;

#ifdef DEBUG
    printf("Node %ld (TCP Client) starting to open connection.\n",
           node->nodeId);
#endif /* DEBUG */

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_TCP,
              MSG_TRANSPORT_FromAppOpen);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToTcpOpen));
    openRequest = (AppToTcpOpen *) MESSAGE_ReturnInfo(msg);
    openRequest->appType = appType;
    // openRequest->localAddr = localAddr;
    SetIPv4AddressInfo(&openRequest->localAddr, localAddr);
    openRequest->localPort = localPort;
    // openRequest->remoteAddr = remoteAddr;
    SetIPv4AddressInfo(&openRequest->remoteAddr, remoteAddr);
    openRequest->remotePort = remotePort;

    openRequest->uniqueId = uniqueId;
    openRequest->priority = priority;
    openRequest->outgoingInterface = ANY_INTERFACE;

    MESSAGE_Send(node, msg, waitTime);
}


/*
 * NAME:        APP_TcpOpenConnectionWithPriority.
 * PURPOSE:     (Overloaded for IPv6) Open a connection with specified priority.
 * PARAMETERS:  node - pointer to the node.
 *              appType - which application initiates this request.
 *              localAddr - address of the source node.
 *              localPort - port number on the source node.
 *              remoteAddr - address of the remote node.
 *              remotePort - port number on the remote node (server port).
 *              uniqueId - used to determine which client is requesting
 *                         connection.
 *              waitTime - time until the session starts.
 *              priority - priority of the data.
 * RETURN:      none.
 */
void
APP_TcpOpenConnectionWithPriority(
    Node *node,
    AppType appType,
    Address localAddr,
    short localPort,
    Address remoteAddr,
    short remotePort,
    int uniqueId,
    clocktype waitTime,
    TosType priority)
{
    AppToTcpOpen *openRequest;
    Message *msg;

#ifdef DEBUG
    printf("Node %ld (TCP Client) starting to open connection.\n",
           node->nodeId);
#endif /* DEBUG */

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_TCP,
              MSG_TRANSPORT_FromAppOpen);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToTcpOpen));
    openRequest = (AppToTcpOpen *) MESSAGE_ReturnInfo(msg);
    openRequest->appType = appType;
    openRequest->localAddr = localAddr;
    openRequest->localPort = localPort;
    openRequest->remoteAddr = remoteAddr;
    openRequest->remotePort = remotePort;

    openRequest->uniqueId = uniqueId;
    openRequest->priority = priority;
    openRequest->outgoingInterface = ANY_INTERFACE;

    MESSAGE_Send(node, msg, waitTime);
}


// API        :: APP_TcpOpenConnectionWithPriority..
// PURPOSE    :: Open a connection with specified priority.
// PARAMETERS ::
// + outgoingInterface : int : User specific outgoing Interface.
void
APP_TcpOpenConnectionWithPriority(
    Node *node,
    AppType appType,
    NodeAddress localAddr,
    short localPort,
    NodeAddress remoteAddr,
    short remotePort,
    int uniqueId,
    clocktype waitTime,
    TosType priority,
    int outgoingInterface)
{
    AppToTcpOpen *openRequest;
    Message *msg;

#ifdef DEBUG
    printf("Node %ld (TCP Client) starting to open connection.\n",
           node->nodeId);
#endif /* DEBUG */

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_TCP,
              MSG_TRANSPORT_FromAppOpen);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToTcpOpen));
    openRequest = (AppToTcpOpen *) MESSAGE_ReturnInfo(msg);
    openRequest->appType = appType;
    // openRequest->localAddr = localAddr;
    SetIPv4AddressInfo(&openRequest->localAddr, localAddr);
    openRequest->localPort = localPort;
    // openRequest->remoteAddr = remoteAddr;
    SetIPv4AddressInfo(&openRequest->remoteAddr, remoteAddr);
    openRequest->remotePort = remotePort;

    openRequest->uniqueId = uniqueId;
    openRequest->priority = priority;

    if (node->numberInterfaces < outgoingInterface ||
        outgoingInterface < ANY_INTERFACE)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Node: %u\n"
            "Value of outgoingInterface is out of range\n", node->nodeId);
        ERROR_ReportWarning(errString);
        openRequest->outgoingInterface = ANY_INTERFACE;
    }
    openRequest->outgoingInterface = outgoingInterface;

    //Trace sending packet
    ActionData acnData;
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
              PACKET_OUT, &acnData);

    MESSAGE_Send(node, msg, waitTime);
}


// API        :: APP_TcpOpenConnectionWithPriority..
// PURPOSE    :: (Overloaded for IPv6) Open a connection with specified priority.
// PARAMETERS ::
// + outgoingInterface : int : User specific outgoing Interface.
void
APP_TcpOpenConnectionWithPriority(
    Node *node,
    AppType appType,
    Address localAddr,
    short localPort,
    Address remoteAddr,
    short remotePort,
    int uniqueId,
    clocktype waitTime,
    TosType priority,
    int outgoingInterface)
{
    AppToTcpOpen *openRequest;
    Message *msg;

#ifdef DEBUG
    printf("Node %ld (TCP Client) starting to open connection.\n",
           node->nodeId);
#endif /* DEBUG */

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_TCP,
              MSG_TRANSPORT_FromAppOpen);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToTcpOpen));
    openRequest = (AppToTcpOpen *) MESSAGE_ReturnInfo(msg);
    openRequest->appType = appType;
    openRequest->localAddr = localAddr;
    openRequest->localPort = localPort;
    openRequest->remoteAddr = remoteAddr;
    openRequest->remotePort = remotePort;

    openRequest->uniqueId = uniqueId;
    openRequest->priority = priority;

    if (node->numberInterfaces < outgoingInterface ||
        outgoingInterface < ANY_INTERFACE)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Node: %u\n"
            "Value of outgoingInterface is out of range\n", node->nodeId);
        ERROR_ReportWarning(errString);
        openRequest->outgoingInterface = ANY_INTERFACE;
    }
    openRequest->outgoingInterface = outgoingInterface;

    MESSAGE_Send(node, msg, waitTime);
}


// /**
// API         :: App_TcpCreateMessage.
// PURPOSE     :: Create the message.
// PARAMETERS  ::
// + node          : Node *           : Node pointer that the protocol is
//                                      being instantiated in
// + connId        : int              : connection id.
// + payload       : char *           : data to send.
// + length        : int              : length of the data to send.
// + traceProtocol : traceProtocolType : specify the type of application
//                                      used for packet tracing.
// RETURN         :: Message * :message created in the function
Message *
App_TcpCreateMessage(Node *node,
                     int connId,
                     char *payload,
                     int length,
                     TraceProtocolType traceProtocol,
                     UInt8 ttl)
{
    Message *msg;
    AppToTcpSend *sendRequest;
    ActionData acnData;

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_TCP,
              MSG_TRANSPORT_FromAppSend);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToTcpSend));
    sendRequest = (AppToTcpSend *) MESSAGE_ReturnInfo(msg);
    sendRequest->connectionId = connId;
    sendRequest->ttl = ttl;
    MESSAGE_PacketAlloc(node, msg, length, traceProtocol);
    memcpy(MESSAGE_ReturnPacket(msg), payload, length);

    //Trace Information
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
        PACKET_OUT, &acnData);

    return msg;
}

/*
 * NAME:        APP_TcpSendData.
 * PURPOSE:     send an application data unit.
 * PARAMETERS:  node - pointer to the node.
 *              connId - connection id.
 *              payload - data to send.
 *              length - length of the data to send.
 *              traceProtocol - specify the type of application used for
 *                              packet tracing.
 * RETRUN:      none.
 */
Message *
APP_TcpSendData(
    Node *node,
    int connId,
    char *payload,
    int length,
    TraceProtocolType traceProtocol
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam,
    UInt8 ttl)
#else
    ,UInt8 ttl)
#endif
{
    Message *msg;
    msg = App_TcpCreateMessage(node,
                              connId,
                              payload,
                              length,
                              traceProtocol,
                              ttl);

#ifdef ADDON_DB
    if (appParam != NULL)
    {
        if (!appParam->m_PktCreationTimeSpecified)
        {
            appParam->SetPacketCreateTime(msg->packetCreationTime);
        }
        HandleStatsDBAppSendEventsInsertion(node, msg, length, appParam);
    }

#endif
    MESSAGE_Send(node, msg, 0);
    return msg;
}

#ifdef EXATA
/*
* NAME:        APP_TcpSendData.
* PURPOSE:     send an application data unit.
* PARAMETERS:  node - pointer to the node.
*              connId - connection id.
*              payload - data to send.
*              length - length of the data to send.
*              traceProtocol - specify the type of application used for
*                              packet tracing.
*              isEmulationPacket - emulation packet?
* RETRUN:      none.
*/

void
APP_TcpSendData(
    Node *node,
    int connId,
    char *payload,
    int length,
    TraceProtocolType traceProtocol,
    BOOL isEmulationPacket
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam,
    UInt8 ttl)
#else
    ,UInt8 ttl)
#endif
{
    Message *msg;
    msg = App_TcpCreateMessage(node,
                              connId,
                              payload,
                              length,
                              traceProtocol,
                              ttl);

    msg->isEmulationPacket = isEmulationPacket;

#ifdef ADDON_DB
    if (appParam != NULL)
    {
        if (!appParam->m_PktCreationTimeSpecified)
        {
            appParam->SetPacketCreateTime(msg->packetCreationTime);
        }
        HandleStatsDBAppSendEventsInsertion(node, msg, length, appParam);
    }

#endif
    MESSAGE_Send(node, msg, 0);
}
#endif


/*
 * NAME:        APP_TcpSendData.
 * PURPOSE:     send an application data unit.
 * PARAMETERS:  node - pointer to the node.
 *              connId - connection id.
 *              payload - data to send.
 *              length - length of data to send.
 *              traceProtocol - specify the type of application used for
 *                              packet tracing.
 *              appName - Application name.
 * RETRUN:      none.
 */

void
APP_TcpSendData(
    Node *node,
    int connId,
    char *payload,
    int length,
    TraceProtocolType traceProtocol,
    char* appName
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam,
    UInt8 ttl)
#else
    ,UInt8 ttl)
#endif
{
    Message *msg;
    msg = App_TcpCreateMessage(node,
                              connId,
                              payload,
                              length,
                              traceProtocol,
                              ttl);

#ifdef ADDON_DB
    if (appParam != NULL)
    {
        appParam->SetPacketCreateTime(msg->packetCreationTime);
        HandleStatsDBAppSendEventsInsertion(node, msg, length, appParam);
    }

#endif
    MESSAGE_Send(node, msg, 0);
}

/*
 * NAME:        APP_TcpSendData.
 * PURPOSE:     send an application data unit.
 * PARAMETERS:  node - pointer to the node.
 *              connId - connection id.
 *              payload - data to send.
 *              length - length of data to send.
 *              InfoData - super application TCP header data.
 *              infoSize - super application TCP header size
 *              traceProtocol - specify the type of application used for
 *                              packet tracing.
 * RETRUN:      Message *
 */
#ifdef ADDON_BOEINGFCS
Message *
APP_TcpSendData(
    Node *node,
    int connId,
    char *payload,
    int length,
    char *infoData,
    int infoSize,
    unsigned short infoType,
    char *qosData,
    int qosSize,
    unsigned short qosType,
    TraceProtocolType traceProtocol,
#ifdef ADDON_DB
    StatsDBAppEventParam* appParam,
    UInt8 ttl)
#else
    UInt8 ttl)
#endif
{
    Message *msg;
    msg = App_TcpCreateMessage(node,
                              connId,
                              payload,
                              length,
                              traceProtocol,
                              ttl);

    // Add the TCP Header Info Field
    // Fill in fragmentation info so packet can be reassembled
    if (infoData != NULL)
    {
       char* tcpHeaderInfo = NULL;
       tcpHeaderInfo = (char *)MESSAGE_AddInfo(
                        node,
                        msg,
                        infoSize,
                        infoType);
       ERROR_Assert(tcpHeaderInfo != NULL, "Unable to add an info field!");
       memcpy(tcpHeaderInfo,infoData,infoSize);
    }

#ifdef ADDON_BOEINGFCS
    if (qosData != NULL)
    {
       char* qosInfo = NULL;
       qosInfo = (char *)MESSAGE_AddInfo(
                        node,
                        msg,
                        qosSize,
                        qosType);
       ERROR_Assert(qosInfo != NULL, "Unable to add an info field for QoS!");
       memcpy(qosInfo,qosData,qosSize);
    }

    TransportToAppDataReceived *appData;

    MESSAGE_AddInfo(node,
        msg,
        sizeof(TransportToAppDataReceived),
        INFO_TYPE_TransportToAppData);

    appData = (TransportToAppDataReceived *)
        MESSAGE_ReturnInfo(msg, INFO_TYPE_TransportToAppData);

    appData->connectionId = connId;
    appData->priority = qosType;
#endif

 #ifdef ADDON_DB
    if (appParam != NULL)
    {
        if (!appParam->m_PktCreationTimeSpecified)
        {
            appParam->SetPacketCreateTime(msg->packetCreationTime);
        }
        HandleStatsDBAppSendEventsInsertion(node, msg, length, appParam);
    }

#endif
    MESSAGE_Send(node, msg, 0);
    return msg;
}
#endif

Message *
APP_TcpSendData(
    Node *node,
    int connId,
    char *payload,
    int length,
    char *infoData,
    int infoSize,
    unsigned short infoType,
    TraceProtocolType traceProtocol,
#ifdef ADDON_DB
    StatsDBAppEventParam* appParam,
    UInt8 ttl)
#else
    UInt8 ttl)
#endif
{
    Message *msg;
    msg = App_TcpCreateMessage(node,
                              connId,
                              payload,
                              length,
                              traceProtocol,
                              ttl);

    // Add the TCP Header Info Field
    // Fill in fragmentation info so packet can be reassembled
    if (infoData != NULL)
    {
       char* tcpHeaderInfo = NULL;
       tcpHeaderInfo = (char *)MESSAGE_AddInfo(
                        node,
                        msg,
                        infoSize,
                        infoType);
       ERROR_Assert(tcpHeaderInfo != NULL, "Unable to add an info field!");
       memcpy(tcpHeaderInfo,infoData,infoSize);
    }


 #ifdef ADDON_DB
    if (appParam != NULL)
    {
        if (!appParam->m_PktCreationTimeSpecified)
        {
            appParam->SetPacketCreateTime(msg->packetCreationTime);
        }
        HandleStatsDBAppSendEventsInsertion(node, msg, length, appParam);
    }

#endif
    MESSAGE_Send(node, msg, 0);
    return msg;
}


Message *
APP_TcpSendData(
    Node *node,
    int connId,
    char *payload,
    int length,
    char *infoData,
    int infoSize,
    unsigned short infoType,
    TraceProtocolType traceProtocol,
    unsigned short tos,
#ifdef ADDON_DB
    StatsDBAppEventParam* appParam,
    UInt8 ttl)
#else
    UInt8 ttl)
#endif
{
    Message *msg;
    msg = App_TcpCreateMessage(node,
                              connId,
                              payload,
                              length,
                              traceProtocol,
                              ttl);

    // Add the TCP Header Info Field
    // Fill in fragmentation info so packet can be reassembled
    if (infoData != NULL)
    {
       char* tcpHeaderInfo = NULL;
       tcpHeaderInfo = (char *)MESSAGE_AddInfo(
                        node,
                        msg,
                        infoSize,
                        infoType);
       ERROR_Assert(tcpHeaderInfo != NULL, "Unable to add an info field!");
       memcpy(tcpHeaderInfo,infoData,infoSize);
    }

#ifdef ADDON_BOEINGFCS
    TransportToAppDataReceived *appData;

    MESSAGE_AddInfo(node,
        msg,
        sizeof(TransportToAppDataReceived),
        INFO_TYPE_TransportToAppData);

    appData = (TransportToAppDataReceived *)
        MESSAGE_ReturnInfo(msg, INFO_TYPE_TransportToAppData);

    appData->connectionId = connId;
    appData->priority = tos;
#endif

 #ifdef ADDON_DB
    if (appParam != NULL)
    {
        if (!appParam->m_PktCreationTimeSpecified)
        {
            appParam->SetPacketCreateTime(msg->packetCreationTime);
        }
        HandleStatsDBAppSendEventsInsertion(node, msg, length, appParam);
    }

#endif
    MESSAGE_Send(node, msg, 0);
    return msg;
}


Message *
APP_TcpSendData(
    Node *node,
    int connId,
    char *payload,
    int length,
    char *infoData,
    int infoSize,
    unsigned short infoType,
    TraceProtocolType traceProtocol,
    unsigned short tos,
    NodeAddress clientAddr,
    int sourcePort,
    NodeAddress serverAddr,
    int destinationPort,
    int sessionId,
    int reqSize,
    int requiredTput,
    clocktype requiredDelay,
    float dataRate,
#ifdef ADDON_DB
    StatsDBAppEventParam* appParam,
    UInt8 ttl)
#else
    UInt8 ttl)
#endif
{
    Message *msg;
    msg = App_TcpCreateMessage(node,
                              connId,
                              payload,
                              length,
                              traceProtocol,
                              ttl);

    // Add the TCP Header Info Field
    // Fill in fragmentation info so packet can be reassembled
    if (infoData != NULL)
    {
       char* tcpHeaderInfo = NULL;
       tcpHeaderInfo = (char *)MESSAGE_AddInfo(
                        node,
                        msg,
                        infoSize,
                        infoType);
       ERROR_Assert(tcpHeaderInfo != NULL, "Unable to add an info field!");
       memcpy(tcpHeaderInfo,infoData,infoSize);
    }

#ifdef ADDON_BOEINGFCS
    TransportToAppDataReceived *appData;

    MESSAGE_AddInfo(node,
        msg,
        sizeof(TransportToAppDataReceived),
        INFO_TYPE_TransportToAppData);

    appData = (TransportToAppDataReceived *)
        MESSAGE_ReturnInfo(msg, INFO_TYPE_TransportToAppData);

    appData->connectionId = connId;
    appData->priority = tos;
#endif

 #ifdef ADDON_DB
    if (appParam != NULL)
    {
        if (!appParam->m_PktCreationTimeSpecified)
        {
            appParam->SetPacketCreateTime(msg->packetCreationTime);
        }
        HandleStatsDBAppSendEventsInsertion(node, msg, length, appParam);
    }

#endif
    MESSAGE_Send(node, msg, 0);
    return msg;
}


/*
 * NAME:        APP_TcpSendNewHeaderVirtualData.
 * PURPOSE:     Send header + virtual data using TCP.
 * PARAMETERS:  node - pointer to the node.
 *              connId - connection id.
 *              header - header to send.
 *              headerLength - length of the header to send.
 *              payloadSize - size of data to send along with header.
 *              traceProtocol - specify the type of application used for
 *                              packet tracing.
 * RETRUN:      Message* - The sent message
 */
Message*
APP_TcpSendNewHeaderVirtualData(
    Node *node,
    int connId,
    char *header,
    int headerLength,
    int payloadSize,
    TraceProtocolType traceProtocol,
#ifdef ADDON_DB
    StatsDBAppEventParam* appParam,
#endif
    UInt8 ttl)
{
    Message *msg;
    AppToTcpSend *sendRequest;

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_TCP,
              MSG_TRANSPORT_FromAppSend);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToTcpSend));
    sendRequest = (AppToTcpSend *) MESSAGE_ReturnInfo(msg);
    sendRequest->connectionId = connId;
    sendRequest->ttl = ttl;

    MESSAGE_PacketAlloc(node, msg, headerLength, traceProtocol);

    if (headerLength > 0)
    {
        memcpy(MESSAGE_ReturnPacket(msg), header, headerLength);
    }

    MESSAGE_AddVirtualPayload(node, msg, payloadSize);

    // Trace for sending packet
    ActionData acnData;
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
              PACKET_OUT, &acnData);

#ifdef ADDON_DB
    if (appParam != NULL)
    {
        appParam->SetPacketCreateTime(msg->packetCreationTime);
        HandleStatsDBAppSendEventsInsertion(
            node,
            msg,
            headerLength + payloadSize ,
            appParam);
    }

#endif
    MESSAGE_Send(node, msg, 0);

    return msg;
}


/*
 * NAME:        APP_TcpCloseConnection.
 * PURPOSE:     Close the connection.
 * PARAMETERS:  node - pointer to the node.
 *              connId - connection id.
 * RETRUN:      none.
 */
void
APP_TcpCloseConnection(
    Node *node,
    int connId)
{
    Message *msg;
    AppToTcpClose *closeRequest;

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_TCP,
              MSG_TRANSPORT_FromAppClose);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToTcpClose));
    closeRequest = (AppToTcpClose *) MESSAGE_ReturnInfo(msg);
    closeRequest->connectionId = connId;
    MESSAGE_Send(node, msg, 0);
}



/*
 * NAME:        APP_CheckMulticastByParsingSourceAndDestString
 * PURPOSE:     Application input parsing function that parses source and
 *              destination strings. At the same time validates those
 *              strings for multicast address.
 * PARAMETERS:  node - node parsing the addresses
 *              inputString - input string.
 *              sourceString - source address string.
 *              sourceNodeId - pointer to source node id.
 *              sourceAddr - pointer to source node address.
 *              destString - destination address string.
 *              destNodeId - pointer to destination node id.
 *              destAddr - pointer to destination node address.
 *              isDestMulticast - flag indicates dest is multicast addr.
 * RETURN: NONE
 */
void APP_CheckMulticastByParsingSourceAndDestString(
    Node *node,
    const char *inputString,
    const char *sourceString,
    NodeAddress *sourceNodeId,
    NodeAddress *sourceAddr,
    const char *destString,
    NodeAddress *destNodeId,
    NodeAddress *destAddr,
    BOOL *isDestMulticast)
{
    BOOL isNodeId;
    char errString[MAX_STRING_LENGTH];
    Address address;

    *isDestMulticast = FALSE;

    // Parse and check given source address.
    IO_ParseNodeIdOrHostAddress(
        sourceString, sourceAddr, &isNodeId);

    if (isNodeId)
    {
        *sourceNodeId = *sourceAddr;

        *sourceAddr = MAPPING_GetDefaultInterfaceAddressFromNodeId(
                        node, *sourceNodeId);

        if (*sourceAddr == INVALID_MAPPING)
        {
            sprintf(errString,
                    "Source node %u does not exist:\n  %s\n",
                    *sourceNodeId, inputString);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        // Check if source address is a multicast address.
        if (*sourceAddr >= IP_MIN_MULTICAST_ADDRESS &&
            *sourceAddr <= IP_MAX_MULTICAST_ADDRESS)
        {
            sprintf(errString,
                "Given source address %s is not valid.\n"
                "Source of an application is either a node"
                " or a unicast address.\n  %s\n",
                sourceString, inputString);
            ERROR_ReportError(errString);
        }

        *sourceNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                          node, *sourceAddr);

        if (*sourceNodeId == INVALID_MAPPING)
        {
            sprintf(errString,
                    "Source address %s is not bound to a node:\n  %s\n",
                    sourceString, inputString);
            ERROR_ReportError(errString);
        }
    }


    // Parse and check given destination address.
    IO_ParseNodeIdOrHostAddress(
        destString, destAddr, &isNodeId);

    if (isNodeId)
    {
        *destNodeId = *destAddr;

        *destAddr = MAPPING_GetDefaultInterfaceAddressFromNodeId(
                        node, *destNodeId);

        if (*destAddr == INVALID_MAPPING)
        {
            sprintf(errString,
                    "Destination node %u does not exist:\n  %s\n",
                    *destNodeId, inputString);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        // Check if given destination address is a multicast address.
        if (!(*destAddr >= IP_MIN_MULTICAST_ADDRESS &&
              *destAddr <= IP_MAX_MULTICAST_ADDRESS))
        {
            if (NetworkIpIsLoopbackInterfaceAddress(*destAddr))
            {
                *destNodeId = *sourceNodeId;
            }
            else
            {
                *destNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                              node, *destAddr);
            }

            if (*destNodeId == INVALID_MAPPING)
            {
                sprintf(errString,
                        "Destination address %s is not bound to a node:\n"
                        "  %s\n",
                        destString, inputString);
                ERROR_ReportError(errString);
            }
            return;
        }

        // Destination is a multicast address.
        *isDestMulticast = TRUE;

        // Check if it is a reserved multicast address.

        if (*destAddr >= IP_MIN_RESERVED_MULTICAST_ADDRESS &&
            *destAddr <= IP_MAX_RESERVED_MULTICAST_ADDRESS)
        {
            char minUsableMcastAddr[MAX_STRING_LENGTH];
            char maxUsableMcastAddr[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(
                IP_MAX_RESERVED_MULTICAST_ADDRESS + 1, minUsableMcastAddr);
            IO_ConvertIpAddressToString(
                IP_MAX_MULTICAST_ADDRESS, maxUsableMcastAddr);

            sprintf(errString,
                "Given destination multicast address %s is "
                "reserved one.\n"
                "Usable address range is from %s to %s\n  %s\n",
                destString,
                minUsableMcastAddr, maxUsableMcastAddr,
                inputString);

            ERROR_ReportError(errString);
        }
    }

    // PATCH: to avoid any other application than CBR.
    // In this version all the other applications are not
    // allowed in IPV6 network. This will be removed when
    // other applications will be active for IPv6
    address = MAPPING_GetDefaultInterfaceAddressInfoFromNodeId(
        node, *sourceNodeId);

    if (address.networkType == NETWORK_IPV6)
    {
        ERROR_ReportError("Wrong application type in IPv6 network");
    }
}

// /**
// API        :: APP_ParsingSourceAndDestString
// PURPOSE    :: API to parse the input source and destination strings read
//               from the *.app file.At the same time checks and fills the
//               destination type parameter.
// PARAMETERS ::
// + node            : Node *           : A pointer to Node.
// + inputString     : const char *     : The input string.
// + sourceString    : const char *     : The source string.
// + sourceNodeId    : NodeAddress *    : A pointer to NodeAddress.
// + sourceAddr      : NodeAddress *    : A pointer to NodeAddress.
// + destString      : const char *     : The destination string.
// + destNodeId      : NodeAddress *    : A pointer to NodeAddress.
// + destAddr        : NodeAddress *    : A pointer to NodeAddress.
// + destType        : DestinationType *: A pointer to Destinationtype.
// RETURN     :: void
// **/
void APP_ParsingSourceAndDestString(
    Node* node,
    const char* inputString,
    const char* sourceString,
    NodeAddress* sourceNodeId,
    NodeAddress* sourceAddr,
    const char* destString,
    NodeAddress* destNodeId,
    NodeAddress* destAddr,
    DestinationType* destType)
{
    BOOL isNodeId;
    char errString[MAX_STRING_LENGTH];
    Address address;

    *destType = DEST_TYPE_UNICAST;

    // Parse and check given source address.
    if (strcmp(sourceString, "255.255.255.255") == 0)
    {
        sprintf(errString,
            "Given source address %s is not valid.\n"
            "Source of an application is either a node"
            " or a unicast address.\n  %s\n", sourceString, inputString);
        ERROR_ReportError(errString);
    }

    IO_ParseNodeIdOrHostAddress(
        sourceString, sourceAddr, &isNodeId);

    if (isNodeId)
    {
        *sourceNodeId = *sourceAddr;

        *sourceAddr = MAPPING_GetDefaultInterfaceAddressFromNodeId(
                        node, *sourceNodeId);

        if (*sourceAddr == INVALID_MAPPING)
        {
            sprintf(errString,
                    "Source node %u does not exist:\n  %s\n",
                    *sourceNodeId, inputString);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        // Check if source address is a multicast address.
        if (*sourceAddr >= IP_MIN_MULTICAST_ADDRESS &&
            *sourceAddr <= IP_MAX_MULTICAST_ADDRESS)
        {
            sprintf(errString,
                "Given source address %s is not valid.\n"
                "Source of an application is either a node"
                " or a unicast address.\n  %s\n",
                sourceString, inputString);
            ERROR_ReportError(errString);
        }

        *sourceNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                          node, *sourceAddr);

        if (*sourceNodeId == INVALID_MAPPING)
        {
            sprintf(errString,
                    "Source address %s is not bound to a node:\n  %s\n",
                    sourceString, inputString);
            ERROR_ReportError(errString);
        }
    }


    // Parse and check given destination address.
    if (strcmp(destString, "255.255.255.255") == 0)
    {
        *destAddr = ANY_DEST;
        *destType = DEST_TYPE_BROADCAST;
        *destNodeId = ANY_NODEID;
        return;
    }

    IO_ParseNodeIdOrHostAddress(
        destString, destAddr, &isNodeId);

    if (isNodeId)
    {
        *destNodeId = *destAddr;

        *destAddr = MAPPING_GetDefaultInterfaceAddressFromNodeId(
                        node, *destNodeId);

        if (*destAddr == INVALID_MAPPING)
        {
            sprintf(errString,
                    "Destination node %u does not exist:\n  %s\n",
                    *destNodeId, inputString);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        // Check if given destination address is NOT a multicast address.
        if (!(*destAddr >= IP_MIN_MULTICAST_ADDRESS &&
              *destAddr <= IP_MAX_MULTICAST_ADDRESS))
        {
            if (NetworkIpIsLoopbackInterfaceAddress(*destAddr))
            {
                *destNodeId = *sourceNodeId;
            }
            else
            {
                *destNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                              node, *destAddr);
            }

            if (*destNodeId == INVALID_MAPPING)
            {
                sprintf(errString,
                        "Destination address %s is not bound to a node:\n"
                        "  %s\n",
                        destString, inputString);
                ERROR_ReportError(errString);
            }
            return;
        }

        // Destination is a multicast address.
        *destType = DEST_TYPE_MULTICAST;

        // Check if it is a reserved multicast address.
        if (*destAddr >= IP_MIN_RESERVED_MULTICAST_ADDRESS &&
            *destAddr <= IP_MAX_RESERVED_MULTICAST_ADDRESS)
        {
            char minUsableMcastAddr[MAX_STRING_LENGTH];
            char maxUsableMcastAddr[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(
                IP_MAX_RESERVED_MULTICAST_ADDRESS + 1, minUsableMcastAddr);
            IO_ConvertIpAddressToString(
                IP_MAX_MULTICAST_ADDRESS, maxUsableMcastAddr);

            sprintf(errString,
                "Given destination multicast address %s is "
                "reserved one.\n"
                "Usable address range is from %s to %s\n  %s\n",
                destString,
                minUsableMcastAddr, maxUsableMcastAddr,
                inputString);

            ERROR_ReportError(errString);
        }
    }

    // PATCH: to avoid any other application than CBR.
    // In this version all the other applications are not
    // allowed in IPV6 network. This will be removed when
    // other applications will be active for IPv6
    address = MAPPING_GetDefaultInterfaceAddressInfoFromNodeId(
        node, *sourceNodeId);

    if (address.networkType == NETWORK_IPV6)
    {
        ERROR_ReportError("Wrong application type in IPv6 network");
    }
}

// /**
// API        :: APP_ParsingSourceAndDestString
// PURPOSE    :: API to parse the input source and destination strings read
//               from the *.app file.At the same time checks and fills the
//               destination type parameter.
// PARAMETERS ::
// + node            : Node *           : A pointer to Node.
// + inputString     : const char *     : The input string.
// + sourceString    : const char *     : The source string.
// + sourceNodeId    : NodeAddress *    : A pointer to NodeAddress.
// + sourceAddr      : NodeAddress *    : A pointer to NodeAddress.
// + destString      : const char *     : The destination string.
// + destNodeId      : NodeAddress *    : A pointer to NodeAddress.
// + destAddr        : NodeAddress *    : A pointer to NodeAddress.
// + destType        : DestinationType *: A pointer to Destinationtype.
// RETURN     :: void
// **/
void APP_ParsingSourceAndDestString(
    Node* node,
    const char* inputString,
    const char* sourceString,
    NodeAddress* sourceNodeId,
    Address* sourceAddr,
    const char* destString,
    NodeAddress* destNodeId,
    Address* destAddr,
    DestinationType* destType)
{
    BOOL isNodeId = TRUE;
        *destNodeId = 0;
    char errString[MAX_STRING_LENGTH];

    *destType = DEST_TYPE_UNICAST;

    // Parse and check given source address.
    if (strcmp(sourceString, "255.255.255.255") == 0)
    {
        sprintf(errString,
            "Given source address %s is not valid.\n"
            "Source of an application is either a node"
            " or a unicast address.\n  %s\n", sourceString, inputString);
        ERROR_ReportError(errString);
    }

    IO_AppParseSourceString(
        node,
        inputString,
        sourceString,
        sourceNodeId,
        sourceAddr);

    if (sourceAddr->networkType == NETWORK_IPV4)
    {
        if (GetIPv4Address(*sourceAddr) == ANY_DEST ||
            (GetIPv4Address(*sourceAddr) >= IP_MIN_MULTICAST_ADDRESS &&
            GetIPv4Address(*sourceAddr) <= IP_MAX_MULTICAST_ADDRESS))
        {
            sprintf(errString,
                "Given source address %s is not valid.\n"
                "Source of an application is either a node"
                " or a unicast address.\n  %s\n",
                sourceString, inputString);
            ERROR_ReportError(errString);
        }
    }
    else if (sourceAddr->networkType == NETWORK_IPV6)
    {
        if (IS_ANYADDR6(GetIPv6Address(*sourceAddr)) ||
            IS_MULTIADDR6(GetIPv6Address(*sourceAddr)))
        {
            sprintf(errString,
                "Given source address %s is not valid.\n"
                "Source of an application is either a node"
                " or a unicast address.\n  %s\n",
                sourceString, inputString);
            ERROR_ReportError(errString);
        }
    }

    // Parse and check given destination address.
    if (strcmp(destString, "255.255.255.255") == 0)
    {
        SetIPv4AddressInfo(destAddr, ANY_DEST);
        *destType = DEST_TYPE_BROADCAST;
        *destNodeId = ANY_NODEID;
        return;
    }

    if (strchr(destString, ':'))
    {
        IO_ParseNodeIdOrHostAddress(
            destString, &((*destAddr).interfaceAddr.ipv6), destNodeId);

        if (!*destNodeId)
        {
            isNodeId = FALSE;
            (*destAddr).networkType = NETWORK_IPV6;

            if (IS_ANYADDR6(GetIPv6Address(*destAddr)))
            {
                *destType = DEST_TYPE_BROADCAST;
            }
            else if (IS_MULTIADDR6(GetIPv6Address(*destAddr)))
            {
                *destType = DEST_TYPE_MULTICAST;
            }
            else
            {
                *destType = DEST_TYPE_UNICAST;
                *destNodeId = MAPPING_GetNodeIdFromGlobalAddr(
                          node, (*destAddr).interfaceAddr.ipv6);
                if (*destNodeId == INVALID_MAPPING)
                {
                    sprintf(errString, "Destination address %s is "
                            "not bound to a node:\n  %s\n",
                            destString, inputString);
                    ERROR_ReportError(errString);
                }
            }
        }
    }
    else
    {
        IO_ParseNodeIdOrHostAddress(
                destString, destNodeId, &isNodeId);

        if (!isNodeId)
        {
            // Here it is the address.
            (*destAddr).networkType = NETWORK_IPV4;
            (*destAddr).interfaceAddr.ipv4 = *destNodeId;
            if (GetIPv4Address(*destAddr) == ANY_DEST)
            {
                *destType = DEST_TYPE_BROADCAST;
            }
            else if (
                GetIPv4Address(*destAddr) >= IP_MIN_MULTICAST_ADDRESS &&
                GetIPv4Address(*destAddr) <= IP_MAX_MULTICAST_ADDRESS)
            {
                *destType = DEST_TYPE_MULTICAST;

                if (GetIPv4Address(*destAddr) >= IP_MIN_RESERVED_MULTICAST_ADDRESS &&
                        GetIPv4Address(*destAddr) <= IP_MAX_RESERVED_MULTICAST_ADDRESS)
                {
                    char minUsableMcastAddr[MAX_STRING_LENGTH];
                    char maxUsableMcastAddr[MAX_STRING_LENGTH];

                    IO_ConvertIpAddressToString(
                        IP_MAX_RESERVED_MULTICAST_ADDRESS + 1, minUsableMcastAddr);
                    IO_ConvertIpAddressToString(
                        IP_MAX_MULTICAST_ADDRESS, maxUsableMcastAddr);

                    sprintf(errString,
                        "Given destination multicast address %s is "
                        "reserved one.\n"
                        "Usable address range is from %s to %s\n  %s\n",
                        destString,
                        minUsableMcastAddr, maxUsableMcastAddr,
                        inputString);

                    ERROR_ReportError(errString);
                }
            }
            else
            {
                *destType = DEST_TYPE_UNICAST;
                *destNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                             node, (*destAddr).interfaceAddr.ipv4);

                if (*destNodeId == INVALID_MAPPING)
                {
                    if (!NetworkIpIsLoopbackInterfaceAddress(
                             (*destAddr).interfaceAddr.ipv4))
                    {
                        sprintf(errString, "Destination address %s is "
                            "not bound to a node:\n  %s\n",
                            destString, inputString);
                        ERROR_ReportError(errString);
                    }
                }
            }
        }
    }

    if ((*destNodeId) >= 0 && isNodeId)
    {
        (*destAddr).networkType = NETWORK_INVALID;

        *destAddr = MAPPING_GetDefaultInterfaceAddressInfoFromNodeId(
                        node, *destNodeId);

        if ((*destAddr).networkType == NETWORK_INVALID)
        {
            sprintf(errString,
                    "Destination node %u does not exist:\n  %s\n",
                    *destNodeId, inputString);
            ERROR_ReportError(errString);
        }
    }
}

/*
 * NAME:        APP_InitMulticastGroupMembershipIfAny
 * PURPOSE:     Start process of joining multicast group if need to do so.
 * PARAMETERS:  node - node that is joining a group.
 *              nodeInput - used to access configuration file.
 * RETRUN:      none.
 */
void APP_InitMulticastGroupMembershipIfAny(Node* node,
                                           const NodeInput *nodeInput)
{
    NodeInput memberInput;
    BOOL retVal;
    int i;
    int numValues;

    char joinTimeStr[MAX_STRING_LENGTH];
    char leaveTimeStr[MAX_STRING_LENGTH];
    clocktype joinTime, leaveTime;
    Address srcAddr;
    NodeAddress mcastAddr,nodeId;
    in6_addr ipv6_mcastAddr;

    char mcastString[MAX_STRING_LENGTH];
    char sourceString[MAX_STRING_LENGTH];
    BOOL isNodeId;
    BOOL isSrcNodeId;
    BOOL isIpv6Addr;
    int numSubnetBits;
    Int32 intfId;
    NetworkDataIp* ip = (NetworkDataIp*)node->networkData.networkVar;
    IgmpData* igmp = NULL;

    IO_ReadCachedFile(ANY_NODEID, ANY_ADDRESS, nodeInput,
                         "MULTICAST-GROUP-FILE", &retVal,
                         &memberInput);

    if (retVal == FALSE)
    {
        return;
    }

    for (i = 0; i < memberInput.numLines; i++)
    {
        numValues = sscanf(memberInput.inputStrings[i],
                                           "%s %s %s %s",
                                            &sourceString,
                                            mcastString,
                                            joinTimeStr,
                                            leaveTimeStr);


        if (numValues != 4)
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr, "Application: Wrong configuration format!\n");
            ERROR_Assert(FALSE, errStr);
        }

        IO_ParseNodeIdHostOrNetworkAddress(sourceString,
                                           &srcAddr,
                                           &isSrcNodeId);

        joinTime = TIME_ConvertToClock(joinTimeStr);
        leaveTime = TIME_ConvertToClock(leaveTimeStr);

        joinTime -= getSimStartTime(node);
        leaveTime -= getSimStartTime(node);
        if (!isSrcNodeId)
        {
            nodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                           srcAddr);
        }
        else
        {
            if (srcAddr.networkType ==  NETWORK_IPV4)
            {
                nodeId = srcAddr.interfaceAddr.ipv4;
            }
            else
            {
                if (srcAddr.networkType == NETWORK_IPV6)
                {
                   IO_ParseNodeIdHostOrNetworkAddress(sourceString,
                                                &srcAddr.interfaceAddr.ipv6,
                                                &isIpv6Addr,
                                                &nodeId);
                }
            }
        }

        // validity check
        // Check if the node is present in the topology or not
        if (NetworkIpGetNetworkProtocolType(node,
                                       nodeId) == INVALID_NETWORK_TYPE)
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr, "Node%u in MULTICAST-GROUP-FILE is not a "
                    "valid node in this scenario\n", srcAddr);
            ERROR_Assert(FALSE, errStr);
        }

        // validity of join & leave Time
        if ((joinTime >= leaveTime) && (leaveTime != 0))
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr, "Join time should be less than leave time\n");
            ERROR_Assert(FALSE, errStr);
        }

        if ((node->nodeId == nodeId) && (ip->isIgmpEnable))
        {
            igmp = IgmpGetDataPtr(node);

            if (MAPPING_GetNetworkIPVersion(mcastString) == NETWORK_IPV4)
            {
                IO_ParseNodeIdHostOrNetworkAddress(mcastString,
                                                   &mcastAddr,
                                                   &numSubnetBits,
                                                   &isNodeId);

                // group joined on interface level
                if (!isSrcNodeId)
                {
                    intfId = 
                          MAPPING_GetInterfaceIndexFromInterfaceAddress(node,
                                                                    srcAddr);
                    if (igmp->igmpInterfaceInfo[intfId])
                    {
                        NetworkIpJoinMulticastGroup(node,
                                                    intfId,
                                                    mcastAddr,
                                                    joinTime);
                    }
                    if (leaveTime > 0)
                    {
                        if (igmp->igmpInterfaceInfo[intfId])
                        {
                            NetworkIpLeaveMulticastGroup(node,
                                                         intfId,
                                                         mcastAddr,
                                                         leaveTime);
                        }
                    }
                }
                else
                {
                    NetworkIpJoinMulticastGroup(node,
                                                mcastAddr,
                                                joinTime);

                    if (leaveTime > 0)
                    {
                            NetworkIpLeaveMulticastGroup(node,
                                                         mcastAddr,
                                                         leaveTime);
                    }
                }
            }
            else
            {
                BOOL isIpv6Addr;
                NodeId isIpv6NodeId;

                IO_ParseNodeIdHostOrNetworkAddress(mcastString,
                                                   &ipv6_mcastAddr,
                                                   &isIpv6Addr,
                                                   &isIpv6NodeId);
                Ipv6JoinMulticastGroup(node,
                                      ipv6_mcastAddr,
                                      joinTime);

                if (leaveTime > 0)
                {
                    Ipv6LeaveMulticastGroup(node,
                                            ipv6_mcastAddr,
                                            leaveTime);
                }
                
            }
        }
    }
}


/*
 * FUNCTION:   APP_InserInPortTable
 * PURPOSE:    Insert an entry in the port table.
 * PARAMETERS: node - node that needs to insert in port table.
 *             appType - Application type running at that port
 *             myPort - port number to insert
 * RETURN:     none
 */
void APP_InserInPortTable(Node* node, AppType appType,unsigned short myPort)
{
    unsigned short index;
    PortInfo* portInfo;
    PortInfo* portTable;
    PortInfo* newPortInfo;
    portTable = node->appData.portTable;
    index = myPort % PORT_TABLE_HASH_SIZE;
    portInfo = &(portTable[index]);

    // Find the last entry at this hashed location in table
    while (portInfo->next != NULL)
    {
        if (portInfo->portNumber == myPort) return; // Already present
        portInfo = portInfo->next;
    }

    // Check if index location is being used
    if (portInfo->portNumber != 0)
    {
        // Entry not free. Create a new one.
        newPortInfo = (PortInfo*)MEM_malloc(sizeof(PortInfo));
        memset(newPortInfo,0,sizeof(PortInfo));

        // Form a link
        portInfo->next = newPortInfo;
        portInfo = portInfo->next;
    }

    // Insert new entry
    portInfo->portNumber = myPort;
    portInfo->appType = appType;
    portInfo->next = NULL;
}

/*
 * FUNCTION:   APP_RemoveFromPortTable
 * PURPOSE:    Remove an entry from the port table.
 * PARAMETERS: node - node that needs to remove in port table.
 *             myPort - port number to insert
 * RETURN:     none
 */
void APP_RemoveFromPortTable(Node* node, unsigned short myPort)
{
    unsigned short index;
    PortInfo* portInfo = NULL;
    PortInfo* portTable = NULL;
    PortInfo* foundPortInfo = NULL;
    PortInfo* prevPortInfo = NULL;
    PortInfo* nextPortInfo = NULL;
    portTable = node->appData.portTable;
    index = myPort % PORT_TABLE_HASH_SIZE;
    portInfo = &(portTable[index]);

    prevPortInfo = portInfo;
    nextPortInfo = portInfo->next;

    // special case when head of the list equals to myPort
    if (portInfo->portNumber == myPort)
    {
        // special case if it is the only entry
        if (portInfo->next == NULL)
        {
            portInfo->portNumber = 0;
            return;
        }
        else
        {
            portInfo->portNumber = portInfo->next->portNumber;
            portInfo->appType = portInfo->next->appType;
            foundPortInfo = portInfo->next;
            portInfo->next = portInfo->next->next;
            MEM_free(foundPortInfo);
            return;
        }
    }
    while (nextPortInfo != NULL)
    {
        if (nextPortInfo->portNumber == myPort)
        {
            prevPortInfo->next = nextPortInfo->next;
            MEM_free(nextPortInfo);
            return;
        }
        nextPortInfo = nextPortInfo->next;
        prevPortInfo = prevPortInfo->next;
    }

    char errorStr[MAX_STRING_LENGTH] = "";
    sprintf(errorStr,
            "Port number %d not found in node Id %d port table\n",
            myPort,
            node->nodeId);
    ERROR_ReportError(errorStr);
}

/*
 * FUNCTION:   APP_RegisterNewApp
 * PURPOSE:    Insert a new application into the list of apps on this node.
 *             Also inserts the port number being used for this app in the
 *             port table.
 * PARAMETERS: node - node that is issuing the Timer.
 *             appType - application type
 *             dataPtr - pointer to the data space for this app
 * RETURN:     pointer to the new AppInfo data structure for this app
 */
AppInfo *
APP_RegisterNewApp(Node *node, AppType appType, void *dataPtr, unsigned short myPort)
{
    AppInfo *newApp;

    newApp = (AppInfo *)MEM_malloc(sizeof(AppInfo));

    newApp->appType = appType;
    newApp->appDetail = dataPtr;
    newApp->appNext = node->appData.appPtr;
    node->appData.appPtr = newApp;

    // Make an entry in the port table
    APP_InserInPortTable(node,appType,myPort);

    return newApp;
}

//#ifdef ADDON_SDF_PDEF

/*
 * FUNCTION:   APP_UnregisterApp
 * PURPOSE:    Remove an application from the list of apps on this node.
 * PARAMETERS: node - node that is issuing the Timer.
 *             dataPtr - pointer to the data space for this app
 * RETURN:     pointer to the new AppInfo data structure for this app
 */
void
APP_UnregisterApp(Node *node, void *dataPtr, unsigned short myPort)
{
    AppInfo* appList = node->appData.appPtr;

    AppInfo* prev = NULL;

    for (; appList != NULL; prev=appList, appList = appList->appNext)
    {
        if (appList->appDetail == dataPtr) {
            break;
        }
    }

    ERROR_Assert(appList != NULL, "Session not found in the list");

    // now prev is the prev in list and appList points to the one to be unregistered.

    // If first app in the list is to be unregistered, then simply move the node->appData.appPtr to the next one
    if (prev == NULL)
    {
        node->appData.appPtr = appList->appNext;
    }
    else
    {
        prev->appNext = appList->appNext;
    }

    MEM_free(dataPtr);
    MEM_free(appList);

    // remove port number from port table
    APP_RemoveFromPortTable(node, myPort);
}

/*
 * FUNCTION:   APP_IsFreePort
 * PURPOSE:    Check if the port number is free or in use. Also check if
 *             there is an application running at the node that uses
 *             an AppType that has been assigned the same value as this
 *             port number. This is done since applications such as CBR
 *             use AppType as destination port.
 * PARAMETERS: node - node that is checking it's port table.
 *             portNumber - port number to check
 * RETURN:     TRUE indicates the port is free
 *             FALSE indicates the port is already in use
 */
BOOL APP_IsFreePort(Node* node, unsigned short portNumber)
{

    AppInfo *appList = node->appData.appPtr;
    unsigned short index;
    PortInfo *portInfo;
    PortInfo *portTable;
    // Assume it is a free port
    BOOL portIsAppType = FALSE;
    BOOL portIsFree = TRUE;

    index = (unsigned short) (portNumber % PORT_TABLE_HASH_SIZE);
    portTable = node->appData.portTable;
    portInfo = &(portTable[index]);
    // Check if it is a free port
    while (portInfo != NULL)
    {
        if (portInfo->portNumber == portNumber)
        {
            portIsFree = FALSE;
            break;
        }
        portInfo = portInfo->next;
    }

    // Ensure port is not being used as an AppType by existing
    // applications running at the node. This is done because
    // some old applications such as CBR use the APP_TYPE
    // value as their assigned port.

    if (portIsFree == TRUE)
    {
        for (; appList != NULL; appList = appList->appNext)
        {
            if (portNumber == appList->appType )
            {
                portIsAppType = TRUE;
                break;
            }
        }
    }

    if ((portIsFree == TRUE) && (portIsAppType == FALSE) )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}

/*
 * FUNCTION:   APP_IsFreePort
 * PURPOSE:    Check if the port number is free or in use. Also check if
 *             there is an application running at the node that uses
 *             an AppType that has been assigned the same value as this
 *             port number. This is done since applications such as CBR
 *             use AppType as destination port.
 * PARAMETERS: node - node that is checking it's port table.
 *             portNumber - port number to check
 * RETURN:     TRUE indicates the port is free
 *             FALSE indicates the port is already in use
 */
BOOL APP_IsFreePort(Node* node, unsigned short portNumber, AppType appType)
{

    AppInfo *appList = node->appData.appPtr;
    short index;
    PortInfo *portInfo;
    PortInfo *portTable;
    // Assume it is a free port
    BOOL portIsAppType = FALSE;
    BOOL portIsFree = TRUE;

    index = (short) (portNumber % PORT_TABLE_HASH_SIZE);
    portTable = node->appData.portTable;
    portInfo = &(portTable[index]);
    // Check if it is a free port
    while (portInfo != NULL)
    {
        if (portInfo->portNumber == portNumber)
        {
            if (portInfo->appType == appType) {
                portIsFree = TRUE;
                break;
            } else {
                portIsFree = FALSE;
                break;
            }
        }
        portInfo = portInfo->next;
    }

    // Ensure port is not being used as an AppType by existing
    // applications running at the node. This is done because
    // some old applications such as CBR use the APP_TYPE
    // value as their assigned port.

    if (portIsFree == TRUE)
    {
        for (; appList != NULL; appList = appList->appNext)
        {
            if (portNumber == appList->appType )
            {
                portIsAppType = TRUE;
                break;
            }
        }
    }

    if ((portIsFree == TRUE) && (portIsAppType == FALSE) )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*
 * FUNCTION:   APP_GetFreePort
 * PURPOSE:    Returns a free port
 * PARAMETERS: node - node that needs a port.
 * RETURN:     Free port number
 */
unsigned short APP_GetFreePort(Node *node)
{
    // Get a free port that is not being used as an APP_TYPE
    unsigned short freePortNumber;
    BOOL isFreePort = FALSE;
    unsigned short maxFreePort = 0;
    do
    {
        // if node->appData.nextPortNum < 0 meaning we are out of port number
        if (node->appData.nextPortNum > 0)
        {
            // Get a possible free port
            freePortNumber = node->appData.nextPortNum++;

            // Check that if it is free
            isFreePort = APP_IsFreePort(node, freePortNumber);
        }
        else
        {
            maxFreePort = node->appData.nextPortNum - 1;
            // loop thorugh the old port number to see if there is any free port.
            for (freePortNumber = 1024; freePortNumber < maxFreePort; freePortNumber++)
            {
                isFreePort = APP_IsFreePort(node, freePortNumber);
                if (isFreePort == TRUE)
                {
                    //printf("Free Port Number is %d\n", freePortNumber);
                    break;
                }
            }
            if (isFreePort == FALSE)
            {
                char errorStr[MAX_STRING_LENGTH] = "";
                sprintf(errorStr, "Out of Source Port");
                ERROR_ReportError(errorStr);
            }
        }
    } while (isFreePort!= TRUE);
    return freePortNumber;
}

/*
 * FUNCTION:   APP_GetFreePort
 * PURPOSE:    Returns a free port
 * PARAMETERS: node - node that needs a port.
 *             serverAddr - address of server
 * RETURN:     Free port number
 */
short APP_GetFreePortForEachServer(Node *node, NodeAddress serverAddr)
{
    // Get a free port that is not being used as an APP_TYPE
    short freePortNumber;
    BOOL isFreePort = FALSE;

    do
    {
        // if node->appData.nextPortNum < 0 meaning we are out of port number
        if (node->appData.nextPortNum > 0)
        {
            // Get a possible free port
            freePortNumber = node->appData.nextPortNum++;
#ifdef ADDON_BOEINGFCS
        if (freePortNumber == APP_ROUTING_HSLS)
        {
                freePortNumber = node->appData.nextPortNum++;
        }
#endif

            // Check that if it is free
            //isFreePort = APP_IsFreePort(node, freePortNumber);
        isFreePort = TRUE;
        }
        else
        {
             AppInfo* appList = node->appData.appPtr;
             SuperApplicationData* dataPtr = NULL;
             for (; appList != NULL; appList = appList->appNext)
             {
                 if (appList->appType == APP_SUPERAPPLICATION_CLIENT)
                 {
                     dataPtr = (SuperApplicationData*) appList->appDetail;
                     if (dataPtr->serverAddr != serverAddr)
                     {
                         freePortNumber = dataPtr->sourcePort;
                         isFreePort = TRUE;
                         //break;
                     }
                 }
             }

        }
    } while (isFreePort!= TRUE);
    return freePortNumber;
}



/*
 * FUNCTION:   APP_GetProtocolType
 * PURPOSE:    Returns the protocol for which the message is destined
 * PARAMETERS: node - node that received the message.
 *             msg - pointer to the message received
 * RETURN:     Protocol which will receive the message.
 */
unsigned short APP_GetProtocolType(Node* node, Message* msg)
{
    if (msg->eventType == MSG_APP_FromTransport)
    {
        // UDP is being used.
        // For UDP, protocol type may be a reference to destination port.
        // If so, look up the port table to get the protocol at that port.
        unsigned short index;
        unsigned short portNumber;
        PortInfo* portInfo;
        PortInfo* portTable;
        portTable = node->appData.portTable;
        // Assume protocol type is port number
        portNumber = (unsigned short) MESSAGE_GetProtocol(msg);
        index = portNumber % PORT_TABLE_HASH_SIZE;
        portInfo = &(portTable[index]);
        // Check for an entry for this port at this hashed location in table.
        while (portInfo != NULL)
        {
            if (portInfo->portNumber == portNumber)
            {
                // Found an entry for port number in the port table
                return portInfo->appType;
            }

            portInfo = portInfo->next;
        }

        // Port number already refers to AppType since no entry was found
        // in the port table.
        return (unsigned short) MESSAGE_GetProtocol(msg);
    }

    return (unsigned short) MESSAGE_GetProtocol(msg);
}

/*
 * NAME:        GetFreePort
 * PURPOSE:     Gets the next free port for use. Returns
 *              a consecutive number of nodes for the caller.
 * PARAMETERS:  node - pointer to the node,
 *                       first_port - whether the first port to be reserved
 *                       is even/odd or no restriction
 *                       num_of_nodes - no. of nodes to be returned
 *
 * RETRUN:      The first port no..
 */
short GetFreePort(Node *node,PortType first_port,int num_of_nodes)
{
    unsigned short sourcePort = 0;
    switch (first_port)
    {
        case START_EVEN:
        {
            do {
                sourcePort = APP_GetFreePort(node);
            }while ((sourcePort %2 != 0) &&
               (FALSE == IsNextPortFree(node,sourcePort,num_of_nodes-1)));
        }
        break;
        case START_ODD:
        {
            do {
                sourcePort = APP_GetFreePort(node);
           }while ((sourcePort %2 == 0) &&
              (FALSE == IsNextPortFree(node,sourcePort,num_of_nodes-1)));

        }
        break;
        case START_NONE:
        {
            do {
                sourcePort = APP_GetFreePort(node);
           }while (FALSE == IsNextPortFree(node,sourcePort,num_of_nodes-1));
        }
        break;
    }

    return sourcePort;
}

/*
 * NAME:        IsNextPortFree
 * PURPOSE:     Checks if the next n nodes as indicated in the parameter
 *              is free or not.
 *              Returns true if all are free or return FALSE
                if even 1 is blocked.
 * PARAMETERS:  node - pointer to the node,
 *                       sourcePort - starting from which node
 *                       num_of_nodes - no. of nodes to be returned
 *
 * RETRUN:      The first port no..
 */
BOOL IsNextPortFree(Node *node,unsigned short sourcePort,int num_of_nodes)
{
   for (int i=1;i<=num_of_nodes;i++)
   {
       if (FALSE == APP_IsFreePort(node, (unsigned short)(sourcePort + i)))
       {
           return FALSE;
       }
   }
   return TRUE;
}


//
// Implementation for the SequenceNumber utility class
//

// private initialization function primarily for supporting multiple
// constructors. We cannot call another constructor from one constructor.
void SequenceNumber::InitializeValues(int sizeLimit, Int64 lowestSeqNotRcvd)
{
    // size limit is either 0 or larger than 1. For invalid values, we
    // use the default value which is 0 indicating unlimited cache size
    if (sizeLimit <= 1)
    {
        m_SizeLimit = 0; // unlimited cache size
    }
    else
    {
        m_SizeLimit = sizeLimit;
    }

    m_LowestSeqNotRcvd = lowestSeqNotRcvd;

    // when cache is empty, m_HighestSeqRcvd is same as m_LowestSeqNotRcvd
    m_HighestSeqRcvd = m_LowestSeqNotRcvd;
}

// Constructor with size limit and lowest sequence number passed in
SequenceNumber::SequenceNumber(int sizeLimit, Int64 lowestSeqNotRcvd)
{
    // call internal initialization function
    InitializeValues(sizeLimit, lowestSeqNotRcvd);
}

// Constructor with size limit specified. Initial value of
// m_LowestSeqNotRcvd will be 0
SequenceNumber::SequenceNumber(int sizeLimit)
{
    // call internal initialization function
    InitializeValues(sizeLimit, 0);
}

// Constructor without parameter. m_SizeLimitwill be 0 (unlimited). Initial
// value of m_LowestSeqNotRcvd will be 0.
SequenceNumber::SequenceNumber()
{
    // call internal initialization function
    InitializeValues(0, 0);
}

// Major interface function. For the specified seqNumber, return
// whether it is new or duplicate or out-of-order. In addition, it
// will be inserted into the cache.
SequenceNumber::Status SequenceNumber::Insert(Int64 seqNumber)
{
    SequenceNumber::Status returnStatus;

    if (seqNumber < m_LowestSeqNotRcvd)
    {
        // smaller than the lowest one we cached, so treat it as duplicate
        returnStatus = SequenceNumber::SEQ_DUPLICATE;
    }
    else if (m_SeqCache.size() == 0)
    { // empty cache. In such a condition, m_LowestSeqNotRcvd is same as
      // m_HighestSeqRcvd. So if seqNumber is not smaller than
      // m_LowestSeqNotRcvd and cache is empty, this seqNumber is a new one
        returnStatus = SequenceNumber::SEQ_NEW;

        // If equal to m_LowestSeqNotRcvd, this is exactly the next one that
        // we are expecting. So no need to cache it. Just update
        // the m_LowestSeqNotRcvd and m_HighestSeqRcvd. Cache is still empty
        // So they are still same
        if (seqNumber == m_LowestSeqNotRcvd)
        {
            // in sequence delivery
            m_LowestSeqNotRcvd++;
            m_HighestSeqRcvd = m_LowestSeqNotRcvd;
        }
        else
        {
            // This is a new sequence number, however, not the next one
            // expected by m_LowestSeqNotRcvd. Thus, cache it and move
            // m_HighestSeqRcvd forward.
            m_SeqCache.insert(seqNumber);
            m_HighestSeqRcvd = seqNumber;
        }
    }
    else if (seqNumber > m_HighestSeqRcvd)
    {
        // larger than the highest one we have seen so far, so must be new
        returnStatus = SequenceNumber::SEQ_NEW;

        // insert this sequence # into the cache and update
        // the m_HighestSeqRcvd to be this one
        m_SeqCache.insert(seqNumber);
        m_HighestSeqRcvd = seqNumber;

    }
    else
    {
        // If come to here, it could be duplicate or out of order depending
        // on whether it is in the cache or not

        // check if it is already in the cache
        if (m_SeqCache.find(seqNumber) != m_SeqCache.end())
        {
            // Already in cache, this is a duplicate
            returnStatus = SequenceNumber::SEQ_DUPLICATE;
        }
        else
        {
            // Not in cache, this is an out of order
            returnStatus = SequenceNumber::SEQ_OUT_OF_ORDER;

            // check if we need to shift the lowest bound
            if (m_LowestSeqNotRcvd == seqNumber)
            {
                // it is lowest one that we are waiting for
                m_LowestSeqNotRcvd ++;

                // move lowest bound to next missing one
                while (m_SeqCache.erase(m_LowestSeqNotRcvd) != 0)
                {
                    m_LowestSeqNotRcvd++;
                }

                // If cache becomes empty, all in sequence, set
                // m_HighestSeqRcvd to be m_LowestSeqNotRcvd
                if (m_SeqCache.size() == 0)
                {
                    m_HighestSeqRcvd = m_LowestSeqNotRcvd;
                }
            }
            else
            {
                // There is still a smaller sequence number missing,
                // just insert it into the cache
                m_SeqCache.insert(seqNumber);
            }
        }
    }

    // check if we have reached the size limit of the cache
    // m_SizeLimit == 0 means unlimited cache size
    if ((unsigned)m_SizeLimit > 0 && m_SeqCache.size() >
        (unsigned int)m_SizeLimit)
    {
        // find the lowest sequence number
        m_LowestSeqNotRcvd = *m_SeqCache.begin();

        // move lowest bound to next missing one
        while (m_SeqCache.erase(m_LowestSeqNotRcvd) != 0)
        {
            m_LowestSeqNotRcvd++;
        }

        // if cache becomes empty, all in sequence. Need to set
        // m_HighestSeqRcvd to be same as m_LowestSeqNotRcvd
        if (m_SeqCache.size() == 0)
        {
            m_HighestSeqRcvd = m_LowestSeqNotRcvd;
        }
    }

    return returnStatus;
}

// Major interface function. For the specified seqNumber, return
// whether it is new or duplicate or out-of-order. In addition, it
// will be inserted into the cache.
SequenceNumber::Status SequenceNumber::Insert(Int64 seqNumber,
                                              AppMsgStatus msgStatus)
{
    SequenceNumber::Status returnStatus;

    ERROR_Assert(msgStatus != APP_MSG_UNKNOWN,
                 "Wrong message status for sequence number cache!");

    if (msgStatus == APP_MSG_NEW)
    {
        // This is a new valid message
        returnStatus = SequenceNumber::SEQ_NEW;
        m_SeqCache.insert(seqNumber);
        if (seqNumber > m_HighestSeqRcvd)
        {
            m_HighestSeqRcvd = seqNumber;
        }
    }
    else
    {
        // this is not a new message
        if (seqNumber < m_LowestSeqNotRcvd)
        {
            returnStatus = SequenceNumber::SEQ_DUPLICATE;
        }
        else if (m_SeqCache.find(seqNumber) != m_SeqCache.end())
        {
            // Already in cache, this is a duplicate
            returnStatus = SequenceNumber::SEQ_DUPLICATE;
        }
        else
        {
            // Not in cache, this is an out of order
            returnStatus = SequenceNumber::SEQ_OUT_OF_ORDER;
            m_SeqCache.insert(seqNumber);

            if (seqNumber > m_HighestSeqRcvd)
            {
                m_HighestSeqRcvd = seqNumber;
            }
        }
    }

    // check if we need to shift the lowest bound
    if (m_LowestSeqNotRcvd == seqNumber)
    {
        // move lowest bound to next missing one
        while (m_SeqCache.erase(m_LowestSeqNotRcvd) != 0)
        {
            m_LowestSeqNotRcvd++;
        }

        // If cache becomes empty, all in sequence, set
        // m_HighestSeqRcvd to be m_LowestSeqNotRcvd
        if (m_SeqCache.size() == 0)
        {
            m_HighestSeqRcvd = m_LowestSeqNotRcvd;
        }
    }

    // check if we have reached the size limit of the cache
    // m_SizeLimit == 0 means unlimited cache size
    if ((unsigned)m_SizeLimit > 0 && m_SeqCache.size() >
        (unsigned int)m_SizeLimit)
    {
        // find the lowest sequence number
        m_LowestSeqNotRcvd = *m_SeqCache.begin();

        // move lowest bound to next missing one
        while (m_SeqCache.erase(m_LowestSeqNotRcvd) != 0)
        {
            m_LowestSeqNotRcvd++;
        }

        // if cache becomes empty, all in sequence. Need to set
        // m_HighestSeqRcvd to be same as m_LowestSeqNotRcvd
        if (m_SeqCache.size() == 0)
        {
            m_HighestSeqRcvd = m_LowestSeqNotRcvd;
        }
    }

    return returnStatus;
}

// Print out the content in cache as well as
// the internal member variables
void SequenceNumber::Print(FILE* out)
{
    std::set<Int64>::iterator it;

    // print out member variables
    fprintf(out, "Total number of elements in cache: %" TYPES_SIZEOFMFT "d\n", m_SeqCache.size());
    fprintf(out, "Lowest Seq Not Received:  %" TYPES_64BITFMT "d\n",
            m_LowestSeqNotRcvd);
    fprintf(out, "Highest Seq Received:     %" TYPES_64BITFMT "d\n",
            m_HighestSeqRcvd);
    fprintf(out, "Size limit:    %d\n", m_SizeLimit);
    fprintf(out, "Elements:");

    // print out sequence numbers in the cache
    for (it = m_SeqCache.begin(); it != m_SeqCache.end(); it++)
    {
        fprintf(out, " %" TYPES_64BITFMT "d", *it);
    }

    fprintf(out, "\n\n");
}

#ifdef ADDON_DB
// /**
// FUNCTION     :: APP_ReportStatsDbReceiveEvent
// LAYER        :: Application
// PURPOSE      :: Report receive event to StatsDB app event table
//                 This function will check duplicate and out of order msgs
// PARAMETERS   ::
// + node        : Node*    : Pointer to a node who recieves the msg
// + msg         : Message* : The received message or fragment
// + seqCache    : SequenceNumber** : Pointer to the sequence number cache
//                                    which is used to detect duplicate
// + seqNo       : Int64    : Sequence number of the message or fragment
// + delay       : clocktype: Delay of the message/fragment
// + jitter      : clocktype: Smoothed jitter of the received message
// + size        : int      : Size of msg/fragment to be report to db
// + numRcvd     : int      : # of msgs/frags received so far
// + msgStatus   : AppMsgStatus : This is for performance optimization. If
//                            the app already know this is a new msg, it is
//                            helpful when dup record and out of order
//                            record are both disabled.
// Note: The last four parameters are required by the stats DB API
// RETURN       :: SequenceNumber::Status : Indicate whether the msg is dup
//                                          or out of order or new
// **/
SequenceNumber::Status APP_ReportStatsDbReceiveEvent(
                           Node* node,
                           Message* msg,
                           SequenceNumber **seqCache,
                           Int64 seqNo,
                           clocktype delay,
                           clocktype jitter,
                           int size,
                           int numRcvd,
                           AppMsgStatus msgStatus)
{
    SequenceNumber::Status seqStatus = SequenceNumber::SEQ_NEW;

    if (node->partitionData->statsDb == NULL ||
        node->partitionData->statsDb->statsEventsTable == NULL ||
        !node->partitionData->statsDb->statsEventsTable->createAppEventsTable)
    {
        return seqStatus;
    }

    // This is for performance optimization as the sequence cache
    // might use a lot of memory if there are a lot of traffic and
    // packet losses.
    //
    // if recording duplicate and record out of order are both disabled
    // and if this is a new msg, just report it. When both the toggle
    // are disabled, there is no need to use the cache as the cache is
    // mainly to distinguish between duplicate and out or order
    if (!node->partitionData->statsDb->statsAppEvents->recordDuplicate &&
        !node->partitionData->statsDb->statsAppEvents->recordOutOfOrder)
    {
        if (msgStatus == APP_MSG_NEW)
        {
            // caller already know this is a new one, then report it
            HandleStatsDBAppReceiveEventsInsertion(
                    node,
                    msg,
                    "AppReceiveFromLower",
                    delay,
                    jitter,
                    size,
                    numRcvd);
            return seqStatus;
        }
        else if (msgStatus == APP_MSG_OLD)
        {
            // caller already know this is an old one, then no need to report
            // just return as duplicate.
            return SequenceNumber::SEQ_DUPLICATE;
        }
        else
        {
            // caller doesn't know. Then use the seq cache
        }
    }

    // if the caller pass NULL for seCache, it means there is
    // no cache needed. This indicates that just report it as
    // a regular message receive. No need to worry about duplicate
    // or out of order. This is current used for TCP based mode
    if (seqCache == NULL)
    {
        HandleStatsDBAppReceiveEventsInsertion(
                node,
                msg,
                "AppReceiveFromLower",
                delay,
                jitter,
                size,
                numRcvd);
        return seqStatus;
    }

    // if cache is not created yet, create it
    if (*seqCache == NULL)
    {
        *seqCache = new SequenceNumber(
            node->partitionData->statsDb->statsAppEvents->seqCacheSize);
    }

    if (msgStatus == APP_MSG_NEW)
    {
        // This is a new one. Insert to cache and make sure status is new
        seqStatus = (*seqCache)->Insert(seqNo, msgStatus);
        ERROR_Assert(seqStatus == SequenceNumber::SEQ_NEW,
                     "Wronge sequence number status!");
    }
    else if (msgStatus == APP_MSG_OLD)
    {
        // This is not a new one, insert to cache and get its status
        seqStatus = (*seqCache)->Insert(seqNo, msgStatus);
        ERROR_Assert(seqStatus != SequenceNumber::SEQ_NEW,
                     "Wronge sequence number status!");
    }
    else
    {
        // insert to cache and get its status
        seqStatus = (*seqCache)->Insert(seqNo);
    }

    if (seqStatus == SequenceNumber::SEQ_NEW)
    {
        // This is a new message or fragment
        HandleStatsDBAppReceiveEventsInsertion(
                node,
                msg,
                "AppReceiveFromLower",
                delay,
                jitter,
                size,
                numRcvd);
    }
    else if (node->partitionData->statsDb->statsAppEvents->recordDuplicate &&
             seqStatus == SequenceNumber::SEQ_DUPLICATE)
    {
        // This is a duplicated message or fragment. Only
        // report if recording duplicate is enabled
        HandleStatsDBAppReceiveEventsInsertion(
                node,
                msg,
                "AppReceiveDuplicateFromLower",
                delay,
                jitter,
                size,
                numRcvd);
        if (msgStatus != APP_MSG_UNKNOWN)
        {
             HandleStatsDBAppDropEventsInsertion(node,msg,
                "AppDrop",delay, size, const_cast<char*>("Duplicate Message"));
        }
    }
    else if (node->partitionData->statsDb->statsAppEvents->recordOutOfOrder &&
             seqStatus == SequenceNumber::SEQ_OUT_OF_ORDER)
    {
        // This is a out of order message or fragment. Only
        // report if recording out of order is enabled
        HandleStatsDBAppReceiveEventsInsertion(
                node,
                msg,
                "AppReceiveOutOfOrderFromLower",
                delay,
                jitter,
                size,
                numRcvd);
        if (msgStatus != APP_MSG_UNKNOWN)
        {
             HandleStatsDBAppDropEventsInsertion(node,msg,
                "AppDrop",delay, size, const_cast<char*>("Out of order Message"));
        }
    }
    else
    {
        // nothing to do
    }

    return seqStatus;
}
#endif // ADDON_DB

 /* FUNCTION:   App_HandleIcmpMessage
 * PURPOSE:    Used to handle ICMP message received from transport layer
 * PARAMETERS: node - node that received the message.
 *             sourcePort - Source port of original message which generated
 *                          ICMP Error Message
 *             destinationPort - Destination port of original message which
 *                               generated ICMP Error Message
 *             icmpType - ICMP Meassage Type
 *             icmpCode - ICMP Message Code
 * RETURN:     void.
 */
void App_HandleIcmpMessage(Node *node,
                           unsigned short sourcePort,
                           unsigned short destinationPort,
                           unsigned short icmpType,
                           unsigned short icmpCode)
{
    return;
}

