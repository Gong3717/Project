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

// PURPOSE: This is an implementation of Internet Group Management Protocol,
//          Version 2 as described in the RFC2236.
//          It resides at the network at the network layer just above IP.

// ASSUMPTION: 1. All nodes are only IGMPv2 enabled. Compatibility with
//                IGMPv1 was not done.

// REFERENCES: RFC2236, RFC4605

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network_ip.h"
#include "multicast_igmp.h"

#include "network_dualip.h"

#ifdef ADDON_DB
#include "dbapi.h"
#include "db_developer.h"
#endif

#ifdef ENTERPRISE_LIB
#include "multicast_dvmrp.h"
#include "multicast_mospf.h"
#endif // ENTERPRISE_LIB

#ifdef CYBER_CORE
#include "network_iahep.h"
#include "multicast_pim.h"
#endif // CYBER_CORE

#define DEBUG 0
#define IAHEP_DEBUG 0
static
void IgmpSendMembershipReport(
        Node* node,
        NodeAddress grpAddr,
        Int32 intfId);

static
void IgmpHostHandleQuery(
    Node* node,
    IgmpMessage* igmpPkt,
    IgmpInterfaceInfoType* thisInterface,
    Int32 intfId);


static
void IgmpRouterHandleReport(
    Node* node,
    IgmpMessage* igmpPkt,
    IgmpRouter* igmpRouter,
    int srcAddr,
    Int32 intfId,
    Int32 robustnessVariable
#ifdef CYBER_CORE
    ,
    NodeAddress* blackMcstSrcAddress = NULL
#endif
                           );


static
void IgmpRouterHandleLeave(
    Node* node,
    IgmpMessage* igmpPkt,
    IgmpRouter* igmpRouter,
    int srcAddr,
    Int32 intfId);


static void IgmpSetInterfaceType(Node* node,
                                 IgmpData* igmpData,
                                 Int32 interfaceIndex,
                                 IgmpInterfaceType igmpInterfaceType);


static
IgmpInterfaceType IgmpGetInterfaceType(
                                 Node* node,
                                 IgmpData* igmpData,
                                 Int32 interfaceIndex);


static BOOL IgmpValidateProxyParameters(Node* node,
                                        IgmpData* igmpData);


static BOOL IgmpCheckSubscriptionInAllOtherDownstreamInterface(
                                   Node* node,
                                   IgmpData* igmpData,
                                   NodeAddress groupId,
                                   Int32 interfaceIndex);


static
BOOL IgmpGetSubscription(Node* node,
                         Int32 interfaceIndex,
                         NodeAddress grpAddr);

static
void IgmpHandlePacketForProxy(
    Node* node,
    Message* msg,
    NodeAddress srcAddr,
    NodeAddress grpAddr,
    Int32 intfId);

void IgmpProxyRoutingFunction(Node* node,
                              Message* msg,
                              NodeAddress grpAddr,
                              Int32 interfaceIndex,
                              BOOL* packetWasRouted,
                              NodeAddress prevHop);

static
void IgmpProxyForwardPacketOnInterface(Node* node,
                                       Message* msg,
                                       Int32 incomingIntfId,
                                       Int32 outgoingIntfId);



//---------------------------------------------------------------------------
// FUNCTION     : IgmpGetDataPtr()
// PURPOSE      : Get IGMP data pointer.
// RETURN VALUE : igmpDataPtr if found, NULL otherwise.
// ASSUMPTION   : None.
//---------------------------------------------------------------------------
IgmpData* IgmpGetDataPtr(Node* node)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;

    if (!ipLayer->igmpDataPtr)
    {
        char errStr[MAX_STRING_LENGTH];

        sprintf(errStr, "Couldn't found igmpDataPtr for node %u\n",
            node->nodeId);
        ERROR_Assert(ipLayer->igmpDataPtr, errStr);

        return NULL;
    }

    return ipLayer->igmpDataPtr;
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpIsSrcInAttachedSubnet()
// PURPOSE      : Check whether the source is in any of the attached subnet.
//                This is to prevent accepting multicast packet that comes
//                from a remote subnet. This kind of situation can occur in
//                wireless network if two subnet overlaps.
// RETURN VALUE : TRUE if source is in attached subnet, FALSE otherwise.
// ASSUMPTION   : None.
//---------------------------------------------------------------------------
static
BOOL IgmpIsSrcInAttachedSubnet(Node* node, NodeAddress srcAddr)
{
    int i;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        NodeAddress netAddr = NetworkIpGetInterfaceNetworkAddress(node, i);
        NodeAddress netMask = NetworkIpGetInterfaceSubnetMask(node, i);

        if (netAddr == (srcAddr & netMask))
        {
            return TRUE;
        }
    }

    return FALSE;
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpIsMyPacket()
// PURPOSE      : Check whether the packet is meant for this node
// RETURN VALUE : TRUE if packet is meant for this node, FALSE otherwise.
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
BOOL IgmpIsMyPacket(
    Node* node,
    NodeAddress grpAddr,
    Int32 intfId)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    BOOL retVal = FALSE;
    IgmpInterfaceType igmpInterfaceType = IGMP_DEFAULT_INTERFACE;

    if (grpAddr == IGMP_ALL_SYSTEMS_GROUP_ADDR)
    {
        retVal = TRUE;
    }
    else if (igmp->igmpInterfaceInfo[intfId]->igmpNodeType == IGMP_ROUTER)
    {
        retVal = TRUE;
    }
    else if (igmp->igmpInterfaceInfo[intfId]->igmpNodeType == IGMP_PROXY)
    {
        if (intfId ==
            NetworkIpGetInterfaceIndexFromAddress(
                node, igmp->igmpUpstreamInterfaceAddr))
        {
            igmpInterfaceType = IGMP_UPSTREAM;
        }
        else
        {
            igmpInterfaceType = IGMP_DOWNSTREAM;
            retVal = TRUE;                      // this interface will act as router
        }
    }

    if (igmp->igmpInterfaceInfo[intfId]->igmpNodeType == IGMP_HOST
        || igmpInterfaceType == IGMP_UPSTREAM)
    {
        IgmpGroupInfoForHost* tempGrpPtr = NULL;
        tempGrpPtr = igmp->igmpInterfaceInfo[intfId]->
                        igmpHost->groupListHeadPointer;

        while (tempGrpPtr)
        {
            if (tempGrpPtr->groupId == grpAddr)
            {
                retVal = TRUE;
                break;
            }
            tempGrpPtr = tempGrpPtr->next;
        }
    }

    return retVal;
}


static
clocktype IgmpSetRegularQueryTimerDelay(
    IgmpRouter* igmpRouter,
    Message* msg)
{
    return igmpRouter->timer.generalQueryInterval;
}


static
clocktype IgmpSetStartUpQueryTimerDelay(
    IgmpRouter* igmpRouter,
    Message* msg)
{
    char startupQueryCount;
    char* ptr;

    ptr = MESSAGE_ReturnInfo(msg) + sizeof(int);
    memcpy(&startupQueryCount, ptr, sizeof(char));

    // Increase Startup query count
    //startupQueryCount++;
    //memcpy(ptr, &startupQueryCount, sizeof(char));

    if (startupQueryCount == igmpRouter->startUpQueryCount) {
        igmpRouter->queryDelayTimeFunc =
            &IgmpSetRegularQueryTimerDelay;
        //as the [startupQueryCount] number of start up
        //queries have been sent, the query interval now
        //should be equal to General Query Interval and
        //not Start Up Query interval.
        return igmpRouter->timer.generalQueryInterval;
    }
    else
    {
        // Increase Startup query count
        startupQueryCount++;
    }
    memcpy(ptr, &startupQueryCount, sizeof(char));

    return igmpRouter->timer.startUpQueryInterval;
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpSetQueryTimer()
// PURPOSE      : sets the timer to send query message
// RETURN VALUE : None
// ASSUMPTION   : This function shouldn't be called from IgmpInit(...)
//---------------------------------------------------------------------------
static
void IgmpSetQueryTimer(
    Node* node,
    Int32 intfId)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpRouter* igmpRouter;
    Message* newMsg;
    char* dataPtr;
    clocktype delay;

    igmpRouter = igmp->igmpInterfaceInfo[intfId]->igmpRouter;

    // Allocate new message
    newMsg = MESSAGE_Alloc(
                node, NETWORK_LAYER, GROUP_MANAGEMENT_PROTOCOL_IGMP,
                MSG_NETWORK_IgmpQueryTimer);

    MESSAGE_InfoAlloc(node, newMsg, sizeof(int) + sizeof(char));
    dataPtr = MESSAGE_ReturnInfo(newMsg);

    memcpy(dataPtr, &intfId, sizeof(int));
    dataPtr += sizeof(int);

    // XXX: Forcefully sending RegularQueryTimer, assuming this function
    //      shouldn't be called from IgmpInit(...) and when a
    //      router's state transit from NON_QUERIER to QUERIER the router
    //      will not send [Startup Query Count] General Queries spaced
    //      closely together [Startup Query Interval]
    memcpy(dataPtr, &igmpRouter->startUpQueryCount, sizeof(char));
    igmpRouter->queryDelayTimeFunc = &IgmpSetRegularQueryTimerDelay;

    delay = (igmpRouter->queryDelayTimeFunc)(igmpRouter, newMsg);

    // Send timer to self
    MESSAGE_Send(node, newMsg, delay);
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpSetReportTimer()
//
// PURPOSE      : Set delay timer for the group specified
//                to send membership report.
//                Generate a random number between 0 to max response time as
//                specified in the query message, and send a self timer.
//
// RETURN VALUE : None
// ASSUMPTION   : Maximum clock granularity is 100 millisecond.
//---------------------------------------------------------------------------
static
void IgmpSetReportTimer(
    Node* node,
    unsigned char maxResponseTime,
    IgmpGroupInfoForHost* tempGrpPtr,
    Int32 intfId)
{
    Message* newMsg = NULL;
    Int32 random;
    char* ptr;


    random = RANDOM_nrand(tempGrpPtr->seed);
    tempGrpPtr->groupDelayTimer = (clocktype)
            (((random) % (maxResponseTime)) * 100 * MILLI_SECOND);

    tempGrpPtr->lastQueryReceive = getSimTime(node);

    if (DEBUG)
    {
        char timeStr[MAX_STRING_LENGTH];
        char ipStr[MAX_ADDRESS_STRING_LENGTH];

        TIME_PrintClockInSecond(tempGrpPtr->groupDelayTimer, timeStr);
        IO_ConvertIpAddressToString(tempGrpPtr->groupId, ipStr);
        printf("    Setting delay timer for group %s to %ss\n",
            ipStr, timeStr);
    }

    // Allocate new message
    newMsg = MESSAGE_Alloc(
                node, NETWORK_LAYER, GROUP_MANAGEMENT_PROTOCOL_IGMP,
                MSG_NETWORK_IgmpGroupReplyTimer);

    MESSAGE_InfoAlloc(node, newMsg, (sizeof(NodeAddress) + sizeof(Int32)));
    memcpy(MESSAGE_ReturnInfo(newMsg),
           &tempGrpPtr->groupId,
           sizeof(NodeAddress));
    ptr = MESSAGE_ReturnInfo(newMsg) + sizeof(NodeAddress);
    *ptr = (char)intfId;

    // Send timer to self with delay
    MESSAGE_Send(node, newMsg, tempGrpPtr->groupDelayTimer);
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpSetGroupMembershipTimer()
// PURPOSE      : sets the timer for the membership of the specified group.
//                timer value = [Group Membership Interval].
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpSetGroupMembershipTimer(
    Node* node,
    IgmpGroupInfoForRouter* grpPtr,
    Int32 intfId)
{
    Message* newMsg;
    char* infoPtr;

    // Allocate new message
    newMsg = MESSAGE_Alloc(
                node, NETWORK_LAYER, GROUP_MANAGEMENT_PROTOCOL_IGMP,
                MSG_NETWORK_IgmpGroupMembershipTimer);

    MESSAGE_InfoAlloc(node, newMsg, sizeof(NodeAddress) + sizeof(int));

    infoPtr = MESSAGE_ReturnInfo(newMsg);
    memcpy(infoPtr, &grpPtr->groupId, sizeof(NodeAddress));
    memcpy(infoPtr + sizeof(NodeAddress), &intfId, sizeof(int));

    // Send timer to self
    MESSAGE_Send(node, newMsg, grpPtr->groupMembershipInterval);
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpSetLastMembershipTimer()
// PURPOSE      : sets the timer for the membership to the
//                [Group Membership Interval].
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpSetLastMembershipTimer(
    Node* node,
    NodeAddress grpAddr,
    Int32 intfId,
    short count,
    UInt8 lastMemberQueryInterval)
{
    Message* newMsg;
    char* dataPtr;

    // Allocate new message
    newMsg = MESSAGE_Alloc(
                node, NETWORK_LAYER, GROUP_MANAGEMENT_PROTOCOL_IGMP,
                MSG_NETWORK_IgmpLastMemberTimer);

    MESSAGE_InfoAlloc(
        node, newMsg, sizeof(NodeAddress) + sizeof(int) + sizeof(short));
    dataPtr = MESSAGE_ReturnInfo(newMsg);

    memcpy(dataPtr, &grpAddr, sizeof(NodeAddress));
    dataPtr += sizeof(NodeAddress);
    memcpy(dataPtr, &intfId, sizeof(int));
    dataPtr += sizeof(int);
    memcpy(dataPtr, &count, sizeof(short));

    // Send timer to self
    MESSAGE_Send(node, newMsg, (lastMemberQueryInterval / 10)*SECOND);
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpHostSetOrResetMembershipTimer()
//
// PURPOSE      : Set delay timer for the specified group. If a timer for the
//                group is already running, it is reset to the random value
//                only if the requested Max Response Time is less than the
//                remaining value of the running timer.
//
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpHostSetOrResetMembershipTimer(
    Node* node,
    IgmpMessage* igmpPkt,
    IgmpGroupInfoForHost* grpPtr,
    Int32 intfId)
{
    if (grpPtr->hostState == IGMP_HOST_IDLE_MEMBER)
    {
        // Host is in idle member state. Set timer
        grpPtr->hostState = IGMP_HOST_DELAYING_MEMBER;

        IgmpSetReportTimer(node, igmpPkt->maxResponseTime, grpPtr, intfId);
    }
    else
    {
        // If timer is already running reset it to the random
        // value only if the requested max response time is
        // less than the remaining value of the running timer
         clocktype remainingDelayTimer = grpPtr->groupDelayTimer -
                (getSimTime(node) - grpPtr->lastQueryReceive);

         if (remainingDelayTimer >
             (((igmpPkt->maxResponseTime)/10) * SECOND))
         {
            if (DEBUG) {
                printf("    Cancel previous timer\n");
            }

            IgmpSetReportTimer(
                node, igmpPkt->maxResponseTime, grpPtr, intfId);

            // Cancellation of previous timer is done at layer function. For
            // that we have make a convention that when a host sends a report
            // it must update it's lastReportSendOrReceive timer to current
            // value (i.e; the current simulation time).
        }
    }
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpInsertHostGroupList()
//
// PURPOSE      : Create a new group entry in this list.
//
// RETURN VALUE : Group database pointer of new group.
// ASSUMPTION   : None.
//---------------------------------------------------------------------------
static
IgmpGroupInfoForHost* IgmpInsertHostGroupList(
    Node* node,
    NodeAddress grpAddr,
    Int32 intfId)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpHost* igmpHost;
    IgmpGroupInfoForHost* tmpGrpPtr;

    if (!igmp->igmpInterfaceInfo[intfId])
    {
        char errStr[MAX_STRING_LENGTH];
        char ipStr[MAX_ADDRESS_STRING_LENGTH];

        IO_ConvertIpAddressToString(
            NetworkIpGetInterfaceAddress(node, intfId),
            ipStr);
        sprintf(errStr, "Node %u: Error inserting group\n"
            "         IGMP not running over interface %s!!\n",
            node->nodeId, ipStr);
        ERROR_Assert(FALSE, errStr);
        return NULL;
    }

    igmpHost = igmp->igmpInterfaceInfo[intfId]->igmpHost;

    // Allocate space for new group
    tmpGrpPtr = (IgmpGroupInfoForHost*)
        MEM_malloc(sizeof(IgmpGroupInfoForHost));

    // Assign group information
    tmpGrpPtr->groupId = grpAddr;
    tmpGrpPtr->hostState = IGMP_HOST_DELAYING_MEMBER;
    tmpGrpPtr->groupDelayTimer = IGMP_UNSOLICITED_REPORT_INTERVAL;
    tmpGrpPtr->lastQueryReceive = (clocktype) 0;
    tmpGrpPtr->lastReportSendOrReceive = getSimTime(node);
    tmpGrpPtr->isLastHost = TRUE;

    RANDOM_SetSeed(tmpGrpPtr->seed,
                   node->globalSeed,
                   node->nodeId,
                   GROUP_MANAGEMENT_PROTOCOL_IGMP,
                   intfId); // RAND, I don't know if this is unique

    // Insert at top of list
    tmpGrpPtr->next = igmpHost->groupListHeadPointer;
    igmpHost->groupListHeadPointer = tmpGrpPtr;

    igmpHost->groupCount += 1;
    igmp->igmpInterfaceInfo[intfId]->igmpStat.igmpTotalGroupJoin += 1;

    if (DEBUG) {
        char grpStr[20];
        char intfStr[20];

        IO_ConvertIpAddressToString(grpAddr, grpStr);
        IO_ConvertIpAddressToString(
            NetworkIpGetInterfaceAddress(node, intfId),
            intfStr);
        printf("Group %s has been added in host %u's list on "
            "interface %s\n\n",
            grpStr, node->nodeId, intfStr);
    }

    return tmpGrpPtr;
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpDeleteHostGroupList()
//
// PURPOSE      : Remove a group entry from this list.
//
// RETURN VALUE : TRUE if successfully deleted, FALSE otherwise.
// ASSUMPTION   : None.
//---------------------------------------------------------------------------
static
BOOL IgmpDeleteHostGroupList(
    Node* node,
    NodeAddress grpAddr,
    Int32 intfId,
    BOOL* checkLastHostStatus)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpHost* igmpHost;
    IgmpGroupInfoForHost* tmpGrpPtr;
    IgmpGroupInfoForHost* prev;

    *checkLastHostStatus = FALSE;

    if (!igmp->igmpInterfaceInfo[intfId])
    {
        char errStr[MAX_STRING_LENGTH];
        char ipStr[MAX_ADDRESS_STRING_LENGTH];

        IO_ConvertIpAddressToString(
            NetworkIpGetInterfaceAddress(node, intfId),
            ipStr);
        sprintf(errStr, "Node %u: Error deleting group\n"
            "         IGMP not running over interface %s!!\n",
            node->nodeId, ipStr);
        ERROR_Assert(FALSE, errStr);
        return FALSE;
    }

    igmpHost = igmp->igmpInterfaceInfo[intfId]->igmpHost;

    tmpGrpPtr = igmpHost->groupListHeadPointer;
    prev = igmpHost->groupListHeadPointer;

    while (tmpGrpPtr && tmpGrpPtr->groupId != grpAddr)
    {
        prev = tmpGrpPtr;
        tmpGrpPtr = tmpGrpPtr->next;
    }

    // Return if entry doesn't exist.
    if (!tmpGrpPtr) {
        if (DEBUG) {
            printf("    No entry found for this group\n");
        }
        return FALSE;
    }

    // If this was first element of the list
    if (prev == tmpGrpPtr) {
        igmpHost->groupListHeadPointer = tmpGrpPtr->next;
    } else {
        prev->next = tmpGrpPtr->next;
    }

    if (tmpGrpPtr->isLastHost) {
        *checkLastHostStatus = TRUE;
    }

    MEM_free(tmpGrpPtr);

    igmpHost->groupCount -= 1;
    igmp->igmpInterfaceInfo[intfId]->igmpStat.igmpTotalGroupLeave += 1;

    if (DEBUG) {
        char grpStr[20];
        char intfStr[20];

        IO_ConvertIpAddressToString(grpAddr, grpStr);
        IO_ConvertIpAddressToString(
            NetworkIpGetInterfaceAddress(node, intfId),
            intfStr);
        printf("Group %s has been removed from host %u's list on "
            "interface %s\n\n",
            grpStr, node->nodeId, intfStr);
    }

    return TRUE;
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpCheckHostGroupList()
//
// PURPOSE      : Check whether the specified group exist in this list.
//
// RETURN VALUE : Group database pointer if found, NULL otherwise.
// ASSUMPTION   : None.
//---------------------------------------------------------------------------
static
IgmpGroupInfoForHost* IgmpCheckHostGroupList(
    Node* node,
    NodeAddress grpAddr,
    IgmpGroupInfoForHost* listHead)
{
    IgmpGroupInfoForHost* tmpGrpPtr;

    tmpGrpPtr = listHead;

    while (tmpGrpPtr)
    {
        if (tmpGrpPtr->groupId == grpAddr)
        {
            return tmpGrpPtr;
        }

        tmpGrpPtr = tmpGrpPtr->next;
    }

    return NULL;
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpInsertRtrGroupList()
//
// PURPOSE      : Create a new group entry in this list.
//
// RETURN VALUE : Group database pointer of new group.
// ASSUMPTION   : None.
//---------------------------------------------------------------------------
static
IgmpGroupInfoForRouter* IgmpInsertRtrGroupList(
    Node* node,
    NodeAddress grpAddr,
    Int32 intfId,
    Int32 robustnessVariable)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpRouter* igmpRouter;
    IgmpGroupInfoForRouter* tmpGrpPtr;

    if (!igmp->igmpInterfaceInfo[intfId])
    {
        char errStr[MAX_STRING_LENGTH];
        char ipStr[MAX_ADDRESS_STRING_LENGTH];

        IO_ConvertIpAddressToString(
            NetworkIpGetInterfaceAddress(node, intfId),
            ipStr);
        sprintf(errStr, "Node %u: Error inserting group\n"
            "         IGMP not running over interface %s!!\n",
            node->nodeId, ipStr);
        ERROR_Assert(FALSE, errStr);
        return NULL;
    }

    igmpRouter = igmp->igmpInterfaceInfo[intfId]->igmpRouter;

    // Allocate space for new group
    tmpGrpPtr = (IgmpGroupInfoForRouter*)
        MEM_malloc(sizeof(IgmpGroupInfoForRouter));

    // Assign group information
    tmpGrpPtr->groupId = grpAddr;
    tmpGrpPtr->membershipState = IGMP_ROUTER_MEMBERS;
    tmpGrpPtr->lastMemberQueryCount = (short)robustnessVariable;
    //The default value for Query Response interval is 100 which on
    //converting into units comes out to be 10 seconds. In QualNet at
    //initialization it is stored as 100 or converting the value configured
    //by user (in seconds) to the normal value i.e. dividing by seconds and
    //multiplying by 10.Hence,it is required to divide the current value in
    //queryResponseInterval by 10 and multiply by SECOND.
    tmpGrpPtr->groupMembershipInterval = (clocktype)
                                    ((robustnessVariable * igmpRouter->queryInterval)
                              + (igmpRouter->queryResponseInterval * SECOND)
                               /IGMP_QUERY_RESPONSE_SCALE_FACTOR);
    tmpGrpPtr->lastMemberQueryInterval = igmpRouter->lastMemberQueryInterval;
    tmpGrpPtr->lastReportReceive = getSimTime(node);

    // Insert at top of list
    tmpGrpPtr->next = igmpRouter->groupListHeadPointer;
    igmpRouter->groupListHeadPointer = tmpGrpPtr;

    igmpRouter->numOfGroups += 1;
    igmp->igmpInterfaceInfo[intfId]->igmpStat.igmpTotalMembership += 1;

    if (DEBUG) {
        char grpStr[20];
        char intfStr[20];

        IO_ConvertIpAddressToString(grpAddr, grpStr);
        IO_ConvertIpAddressToString(
            NetworkIpGetInterfaceAddress(node, intfId),
            intfStr);
        printf("Group %s has been added in router %u's list on "
            "interface %s\n\n",
            grpStr, node->nodeId, intfStr);
    }

    return tmpGrpPtr;
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpDeleteRtrGroupList()
//
// PURPOSE      : Remove a group entry from this list.
//
// RETURN VALUE : TRUE if successfully deleted, FALSE otherwise.
// ASSUMPTION   : None.
//---------------------------------------------------------------------------
static
BOOL IgmpDeleteRtrGroupList(
    Node* node,
    NodeAddress grpAddr,
    Int32 intfId)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpRouter* igmpRouter;
    IgmpGroupInfoForRouter* tmpGrpPtr;
    IgmpGroupInfoForRouter* prev;

    if (!igmp->igmpInterfaceInfo[intfId])
    {
        char errStr[MAX_STRING_LENGTH];
        char ipStr[MAX_ADDRESS_STRING_LENGTH];

        IO_ConvertIpAddressToString(
            NetworkIpGetInterfaceAddress(node, intfId),
            ipStr);
        sprintf(errStr, "Node %u: Error deleting group\n"
            "         IGMP not running over interface %s!!\n",
            node->nodeId, ipStr);
        ERROR_Assert(FALSE, errStr);
        return FALSE;
    }

    igmpRouter = igmp->igmpInterfaceInfo[intfId]->igmpRouter;

    tmpGrpPtr = igmpRouter->groupListHeadPointer;
    prev = igmpRouter->groupListHeadPointer;

    while (tmpGrpPtr && tmpGrpPtr->groupId != grpAddr)
    {
        prev = tmpGrpPtr;
        tmpGrpPtr = tmpGrpPtr->next;
    }

    // Return if entry doesn't exist.
    if (!tmpGrpPtr) {
        return FALSE;
    }

    // If this was first element of the list
    if (prev == tmpGrpPtr)
    {
        igmpRouter->groupListHeadPointer = tmpGrpPtr->next;
    }
    else
    {
        prev->next = tmpGrpPtr->next;
    }

    MEM_free(tmpGrpPtr);

    igmpRouter->numOfGroups -= 1;

    if (DEBUG) {
        char grpStr[20];
        char intfStr[20];

        IO_ConvertIpAddressToString(grpAddr, grpStr);
        IO_ConvertIpAddressToString(
            NetworkIpGetInterfaceAddress(node, intfId),
            intfStr);
        printf("Group %s has been removed from router %u's list on "
            "interface %s\n\n",
            grpStr, node->nodeId, intfStr);
    }

    return TRUE;
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpCheckRtrGroupList()
//
// PURPOSE      : Check whether the specified group exist in this list.
//
// RETURN VALUE : Group database pointer if found, NULL otherwise.
// ASSUMPTION   : None.
//---------------------------------------------------------------------------
static
IgmpGroupInfoForRouter* IgmpCheckRtrGroupList(
    Node* node,
    NodeAddress grpAddr,
    IgmpGroupInfoForRouter* listHead)
{
    IgmpGroupInfoForRouter* tmpGrpPtr;

    tmpGrpPtr = listHead;

    while (tmpGrpPtr)
    {
        if (tmpGrpPtr->groupId == grpAddr)
        {
            return tmpGrpPtr;
        }

        tmpGrpPtr = tmpGrpPtr->next;
    }

    return NULL;
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpCreatePacket()
// PURPOSE      : To create an IGMP packet
// RETURN VALUE : None.
// ASSUMPTION   : Checksum field is always 0
//---------------------------------------------------------------------------
//static
void IgmpCreatePacket(
    Message* msg,
    unsigned char type,
    unsigned char maxResponseTime,
    unsigned groupAddress)
{
    IgmpMessage* ptr = NULL;

    ptr = (IgmpMessage*) MESSAGE_ReturnPacket(msg);

    // Assign packet values
    ptr->ver_type = type;
    ptr->maxResponseTime = maxResponseTime;
    ptr->checksum = 0;
    ptr->groupAddress = groupAddress;
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpSendQuery()
// PURPOSE      : Send Igmp Query Message on specified interface.
// RETURN VALUE : None.
// ASSUMPTION   : None.
//---------------------------------------------------------------------------
static
void IgmpSendQuery(
    Node* node,
    NodeAddress grpAddr,
    Int32 intfId,
    UInt8 queryResponseInterval,
    UInt8 lastMemberQueryInterval)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpInterfaceInfoType* thisInterface;
    Message* newMsg;
    UInt8 maxResponseTime;
    NodeAddress srcAddr;
    NodeAddress dstAddr;

// RtrJoinStart
    IgmpMessage igmpPkt;
// RtrJoinEnd

    //Red IAHEP Node Can Not Send Query To IAHEP Node
#ifdef CYBER_CORE
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    if (ip->iahepEnabled)
    {
        if ((ip->iahepData->nodeType == RED_NODE &&
        IsIAHEPRedSecureInterface(node, intfId)) ||
        ip->iahepData->nodeType == IAHEP_NODE)
        {
           return;
        }
    }

#endif //CYBER_CORE

    thisInterface = igmp->igmpInterfaceInfo[intfId];

    if (grpAddr == 0) {
        maxResponseTime = (queryResponseInterval);
    } else {
        maxResponseTime = lastMemberQueryInterval;
    }

    // Allocate new message
    newMsg = MESSAGE_Alloc(
                node, NETWORK_LAYER, GROUP_MANAGEMENT_PROTOCOL_IGMP,
                MSG_NETWORK_IgmpData);

    MESSAGE_PacketAlloc(node, newMsg, sizeof(IgmpMessage), TRACE_IGMP);

    // Allocate IGMP packet
    IgmpCreatePacket(
        newMsg, IGMP_QUERY_MSG, maxResponseTime, grpAddr);

// RtrJoinStart
    memcpy(&igmpPkt, newMsg->packet, sizeof(IgmpMessage));
// RtrJoinEnd

    // Get IP address for this interface
    srcAddr = NetworkIpGetInterfaceAddress(node, intfId);

    // Group specific query is sent to group address.
    // For general query (group address is 0) it is sent to all systems
    // group multicast address
    if (grpAddr) {
        dstAddr = grpAddr;
    }
    else {
        dstAddr = IGMP_ALL_SYSTEMS_GROUP_ADDR;
    }

    if (DEBUG) {
        char netStr[20];
        char timeStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(getSimTime(node), timeStr);
        IO_ConvertIpAddressToString(
            NetworkIpGetInterfaceNetworkAddress(node, intfId),
            netStr);

        if (thisInterface->igmpStat.igmpTotalMsgSend == 0)
        {
            printf("Node %u starts up as a Querier on network %s\n",
                node->nodeId, netStr);
        }

        if (grpAddr == 0) {
            printf("Node %u send general query to subnet %s at %ss\n\n",
                node->nodeId, netStr, timeStr);
        } else {
            char grpStr[MAX_ADDRESS_STRING_LENGTH];

            IO_ConvertIpAddressToString(grpAddr, grpStr);
            printf("Node %u send group specific query for group %s "
                "to subnet %s at %ss\n\n",
                node->nodeId, grpStr, netStr, timeStr);
        }
    }

    // Send query message.
    thisInterface->igmpStat.igmpTotalMsgSend += 1;
    thisInterface->igmpStat.igmpTotalQuerySend += 1;

    if (grpAddr == 0) {
        thisInterface->igmpStat.igmpGenQuerySend += 1;
    } else {
        thisInterface->igmpStat.igmpGrpSpecificQuerySend += 1;
    }

    NetworkIpSendRawMessageToMacLayer(
        node, newMsg, srcAddr, dstAddr, IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_IGMP, 1, intfId, ANY_DEST);

// RtrJoinStart
    // Act as if we've received a Query

    //If this is a host also then the
    //stats for query received should be incremented

    if (igmpPkt.groupAddress == 0 &&
        thisInterface->igmpHost->groupCount != 0)
    {
        thisInterface->igmpStat.igmpGenQueryReceived += 1;
        thisInterface->igmpStat.igmpTotalQueryReceived += 1;
        igmp->igmpInterfaceInfo[intfId]->igmpStat.igmpTotalMsgReceived += 1;
    IgmpHostHandleQuery(node, &igmpPkt, thisInterface, intfId);
    }
    else if (igmpPkt.groupAddress != 0 &&
        thisInterface->igmpHost->groupCount != 0)
    {
        IgmpGroupInfoForHost* tempGrpPtr;
        tempGrpPtr = IgmpCheckHostGroupList(
                        node, igmpPkt.groupAddress,
                        thisInterface->igmpHost->groupListHeadPointer);

        // If the group exist,then only we have to
        //increment the stats for group specific query
        if (tempGrpPtr)
        {
            thisInterface-> igmpStat.igmpGrpSpecificQueryReceived += 1;
            thisInterface->igmpStat.igmpTotalQueryReceived += 1;
            igmp->igmpInterfaceInfo[intfId]->igmpStat.igmpTotalMsgReceived += 1;
    IgmpHostHandleQuery(node, &igmpPkt, thisInterface, intfId);
        }
    }
    //IgmpHostHandleQuery(node, &igmpPkt, thisInterface, intfId);
// RtrJoinEnd
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpSendMembershipReport()
// PURPOSE      : Send Igmp Membership Report Message on specified interface.
// RETURN VALUE : None.
// ASSUMPTION   : None.
//---------------------------------------------------------------------------
static
void IgmpSendMembershipReport(
    Node* node,
    NodeAddress grpAddr,
    Int32 intfId)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpInterfaceInfoType* thisInterface;
    Message* newMsg;
    NodeAddress srcAddr;
    NodeAddress dstAddr;
    IgmpInterfaceType igmpInterfaceType = IGMP_DEFAULT_INTERFACE;

// RtrJoinStart
    IgmpMessage igmpPkt;
// RtrJoinEnd

    thisInterface = igmp->igmpInterfaceInfo[intfId];

    // Allocate new message
    newMsg = MESSAGE_Alloc(
                node, NETWORK_LAYER, GROUP_MANAGEMENT_PROTOCOL_IGMP,
                MSG_NETWORK_IgmpData);

    MESSAGE_PacketAlloc(node, newMsg, sizeof(IgmpMessage), TRACE_IGMP);

    // Allocate IGMP packet
    IgmpCreatePacket(
        newMsg, IGMP_MEMBERSHIP_REPORT_MSG, 0, grpAddr);

// RtrJoinStart
    if (igmp->igmpInterfaceInfo[intfId]->igmpNodeType == IGMP_ROUTER) {
        memcpy(&igmpPkt, newMsg->packet, sizeof(IgmpMessage));
    }
// RtrJoinEnd

    // Get IP address for this interface
    srcAddr = NetworkIpGetInterfaceAddress(node, intfId);

    // Membership report should be sent to router as well as members of the
    // group.So it is to be sent to all system group.

    //according to RFC 2236, a host multicasts membership report to the
    //group. Hence the destination address should be the group address in
    //the packet
    dstAddr = grpAddr;

    if (DEBUG){
        char grpStr[20];
        char netStr[20];
        char timeStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(getSimTime(node), timeStr);
        IO_ConvertIpAddressToString(grpAddr, grpStr);
        IO_ConvertIpAddressToString(
            NetworkIpGetInterfaceNetworkAddress(node, intfId),
            netStr);

        printf("Node %u send membership report for group %s "
            "to subnet %s at %ss\n\n",
            node->nodeId, grpStr, netStr, timeStr);
    }

    if (igmp->igmpInterfaceInfo[intfId]->igmpNodeType == IGMP_PROXY)
    {
        if (intfId ==
            NetworkIpGetInterfaceIndexFromAddress(
               node,igmp->igmpUpstreamInterfaceAddr))
        {
            igmpInterfaceType = IGMP_UPSTREAM;
        }
        else
        {
            igmpInterfaceType  =IGMP_DOWNSTREAM;
        }
    }

    if (!MAC_IsWirelessAdHocNetwork(node, intfId)
        || igmp->igmpInterfaceInfo[intfId]->igmpNodeType == IGMP_HOST
        || igmpInterfaceType == IGMP_UPSTREAM)
    {
        // Send membership report.
        thisInterface->igmpStat.igmpTotalMsgSend += 1;
        thisInterface->igmpStat.igmpTotalReportSend += 1;
        thisInterface->igmpStat.igmpMembershipReportSend += 1;
#ifdef CYBER_CORE
        NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
        if (ip->iahepEnabled && ip->iahepData->nodeType == IAHEP_NODE &&
            IsIAHEPBlackSecureInterface(node, intfId))
        {
            NodeAddress mappedGrpAddr = IAHEPGetMulticastBroadcastAddressMapping(
                                            node, ip->iahepData, grpAddr);

            // TODO: upgrade to IGMPv3, cannot include src address in IGMPv2
            IAHEPCreateIgmpJoinLeaveMessage(
                node,
                mappedGrpAddr,
                IGMP_MEMBERSHIP_REPORT_MSG,
                NULL);

            ip->iahepData->grpAddressList->insert(grpAddr);

            ip->iahepData->updateAmdTable->getAMDTable(0)->
                    iahepStats.iahepIGMPReportsSent ++;

            if (IAHEP_DEBUG)
            {
                char addrStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(mappedGrpAddr, addrStr);
                printf("\nIAHEP Node: %d", node->nodeId);
                printf("\tJoins Group: %s\n", addrStr);
            }

        }
        //In case of IGMP-BYPASS Mode, join must Be sent to IAHEP Node
        else if (ip->iahepEnabled && ip->iahepData->nodeType == RED_NODE &&
            (dstAddr == BROADCAST_MULTICAST_MAPPEDADDRESS))
        {
            intfId = IAHEPGetIAHEPRedInterfaceIndex(node);
            NetworkIpSendRawMessageToMacLayer(
                node, newMsg, srcAddr, dstAddr, IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_IGMP, 1, intfId, ANY_DEST);
        }
        else
        {
                NetworkIpSendRawMessageToMacLayer(
                    node, newMsg, srcAddr, dstAddr, IPTOS_PREC_INTERNETCONTROL,
                    IPPROTO_IGMP, 1, intfId, ANY_DEST);
            }
#else
                NetworkIpSendRawMessageToMacLayer(
                    node, newMsg, srcAddr, dstAddr, IPTOS_PREC_INTERNETCONTROL,
                    IPPROTO_IGMP, 1, intfId, ANY_DEST);
#endif //CYBER_CORE
            }
    else
    {
        // free the packet
        MESSAGE_Free(node, newMsg);
    }

// RtrJoinStart
    // Act as if we've received a Report
    if (igmp->igmpInterfaceInfo[intfId]->igmpNodeType == IGMP_ROUTER) {

        //As the router is receiving report here
        //hence stats should be incremented
        thisInterface->igmpStat.igmpMembershipReportReceived += 1;
        thisInterface->igmpStat.igmpTotalReportReceived += 1;
        igmp->igmpInterfaceInfo[intfId]->igmpStat.igmpTotalMsgReceived += 1;
        IgmpRouterHandleReport(node,
                               &igmpPkt,
                               thisInterface->igmpRouter,
                               srcAddr, intfId,
                               thisInterface->robustnessVariable);
    }
// RtrJoinEnd
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpSendLeaveReport()
// PURPOSE      : Send Igmp Leave Report Message on specified interface.
// RETURN VALUE : None.
// ASSUMPTION   : None.
//---------------------------------------------------------------------------
static
void IgmpSendLeaveReport(
    Node* node,
    NodeAddress grpAddr,
    Int32 intfId)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpInterfaceInfoType* thisInterface;
    Message* newMsg;
    NodeAddress srcAddr;
    NodeAddress dstAddr;

// RtrJoinStart
    IgmpMessage igmpPkt;
// RtrJoinEnd

    thisInterface = igmp->igmpInterfaceInfo[intfId];

    // Allocate new message
    newMsg = MESSAGE_Alloc(
                node, NETWORK_LAYER, GROUP_MANAGEMENT_PROTOCOL_IGMP,
                MSG_NETWORK_IgmpData);

    MESSAGE_PacketAlloc(node, newMsg, sizeof(IgmpMessage), TRACE_IGMP);

    // Allocate IGMP packet
    IgmpCreatePacket(
        newMsg, IGMP_LEAVE_GROUP_MSG, 0, grpAddr);

// RtrJoinStart
    if (igmp->igmpInterfaceInfo[intfId]->igmpNodeType == IGMP_ROUTER) {
        memcpy(&igmpPkt, newMsg->packet, sizeof(IgmpMessage));
    }
// RtrJoinEnd

    // Get IP address for this interface
    srcAddr = NetworkIpGetInterfaceAddress(node, intfId);

    // Get destination address
    dstAddr = IGMP_ALL_ROUTERS_GROUP_ADDR;

    if (DEBUG) {
        char grpStr[20];
        char netStr[20];
        char timeStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(getSimTime(node), timeStr);
        IO_ConvertIpAddressToString(grpAddr, grpStr);
        IO_ConvertIpAddressToString(
            NetworkIpGetInterfaceNetworkAddress(node, intfId),
            netStr);

        printf("Node %u send leave report for group %s "
            "to subnet %s at %ss\n\n",
            node->nodeId, grpStr, netStr, timeStr);
    }

    // Send membership report.
    thisInterface->igmpStat.igmpTotalMsgSend += 1;
    thisInterface->igmpStat.igmpTotalReportSend += 1;
    thisInterface->igmpStat.igmpLeaveReportSend += 1;

    NetworkIpSendRawMessageToMacLayer(
        node, newMsg, srcAddr, dstAddr, IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_IGMP, 1, intfId, ANY_DEST);

// RtrJoinStart
    // Act as if we've received a Leave Report
    if (igmp->igmpInterfaceInfo[intfId]->igmpNodeType == IGMP_ROUTER) {
        //the statistics for leave messages received should
        //be incremented here
        thisInterface->igmpStat.igmpTotalReportReceived += 1;
        thisInterface->igmpStat.igmpLeaveReportReceived += 1;
        igmp->igmpInterfaceInfo[intfId]->igmpStat.igmpTotalMsgReceived += 1;
        IgmpRouterHandleLeave(
            node, &igmpPkt, thisInterface->igmpRouter, srcAddr, intfId);
    }
#ifdef ADDON_DB
        HandleMulticastDBStatus(node, intfId, "Leave", srcAddr, grpAddr);
#endif
// RtrJoinEnd
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpHostInit()
// PURPOSE      : To initialize the IGMP host
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpHostInit(
    Node* node,
    IgmpHost* igmpHost,
    const NodeInput* nodeInput,
    int interfaceIndex)
{
    int unsolicitedReportCount = 0;
    BOOL retVal = FALSE;
    NodeAddress interfaceAddress;
    interfaceAddress = NetworkIpGetInterfaceAddress(node,interfaceIndex);

    igmpHost->groupCount = 0;
    igmpHost->groupListHeadPointer = NULL;

    // Get how many times we should send unsolicited report message
    IO_ReadInt(node->nodeId,
        interfaceAddress,
               nodeInput,
               "IGMP-UNSOLICITED-REPORT-COUNT",
               &retVal,
               &unsolicitedReportCount);

    if (!retVal)
    {
       if (DEBUG) {
           printf("Node %u: Unsolicited report count is not specified. "
                "Default value(2) is taken\n", node->nodeId);
     }

    unsolicitedReportCount = IGMP_UNSOLICITED_REPORT_COUNT;
    }

    igmpHost->unsolicitedReportCount = (char) unsolicitedReportCount;
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpRouterInit()
// PURPOSE      : To initialize the IGMP router
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpRouterInit(
    Node* node,
    IgmpRouter* igmpRouter,
    int igmpInterfaceIndex,
    const NodeInput* nodeInput,
    Int32 robustnessVariable,
    clocktype queryInterval,
    UInt8 queryResponseInterval,
    UInt8 lastMemberQueryInterval)
{
    Message* newMsg = NULL;
    char* ptr;
    char startupQueryCount = 1;
    Int32 random;
    clocktype delay;

    // Initially all routers starts up as a querier
    igmpRouter->routersState = IGMP_QUERIER;

    // Initializes various timer value.
    igmpRouter->queryInterval = queryInterval;
    igmpRouter->queryResponseInterval = queryResponseInterval;
    igmpRouter->timer.startUpQueryInterval =
                             (clocktype) IGMP_STARTUP_QUERY_INTERVAL;

    igmpRouter->timer.generalQueryInterval = queryInterval;
    //The default value for Query Response interval is 100 which on
    //converting into units comes out to be 10 seconds. In QualNet at
    //initialization it is stored as 100 or converting the value configured
    //by user (in seconds) to the normal value i.e. dividing by seconds and
    //multiplying by 10.Hence,it is required to divide the current value in
    //queryResponseInterval by 10 and multiply by SECOND.
    igmpRouter->timer.otherQuerierPresentInterval =
                            (clocktype) ((robustnessVariable * queryInterval)
                             +( 0.5 * (igmpRouter->queryResponseInterval * SECOND)
                             /IGMP_QUERY_RESPONSE_SCALE_FACTOR));
    igmpRouter->lastMemberQueryInterval = lastMemberQueryInterval;
    igmpRouter->timer.lastQueryReceiveTime = (clocktype) 0;

    // Set startup query count.
    // The default value is the Robustness Variable
    igmpRouter->startUpQueryCount = (char)robustnessVariable;
    igmpRouter->numOfGroups = 0;
    igmpRouter->queryDelayTimeFunc = &IgmpSetStartUpQueryTimerDelay;
    igmpRouter->groupListHeadPointer = NULL;

    // Sending a self message for initiating of messaging system
    newMsg = MESSAGE_Alloc(
                node, NETWORK_LAYER, GROUP_MANAGEMENT_PROTOCOL_IGMP,
                MSG_NETWORK_IgmpQueryTimer);

    MESSAGE_InfoAlloc(node, newMsg, sizeof(int) + sizeof(char));
    ptr = MESSAGE_ReturnInfo(newMsg);

    // Assign start up query count and interface index
    memcpy(ptr, &igmpInterfaceIndex, sizeof(int));
    ptr += sizeof(int);
    memcpy(ptr, &startupQueryCount, sizeof(char));

    RANDOM_SetSeed(igmpRouter->timerSeed,
                   node->globalSeed,
                   node->nodeId,
                   GROUP_MANAGEMENT_PROTOCOL_IGMP,
                   igmpInterfaceIndex);

    // Send self timer
    random = RANDOM_nrand(igmpRouter->timerSeed);
    delay = (clocktype) ((random) % SECOND);

    MESSAGE_Send(node, newMsg, delay);
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpJoinGroup()
// PURPOSE      : This function will be called by ip.c when an multicast
//                application wants to join a group. The function needs to be
//                called at the very start of the simulation.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
void IgmpJoinGroup(
    Node* node,
    Int32 interfaceId,
    NodeAddress groupToJoin,
    clocktype timeToJoin)
{
    Message* newMsg = NULL;
    char* ptr;
    NetworkDataIp* ipLayer = NULL;

    ipLayer = (NetworkDataIp*) node->networkData.networkVar;

    // If IGMP not enable, then ignore joining group using IGMP.
    if (ipLayer->isIgmpEnable == FALSE)
    {
        char errStr[MAX_STRING_LENGTH];
        char buf[MAX_ADDRESS_STRING_LENGTH];

        IO_ConvertIpAddressToString(groupToJoin, buf);
        sprintf(errStr, "Node %u: IGMP is not enabled.\n"
            "    Unable to initiate join message for group %s\n",
            node->nodeId, buf);
        ERROR_ReportWarning(errStr);

        return;
    }

    // Allocate new message
    newMsg = MESSAGE_Alloc(
                node, NETWORK_LAYER, GROUP_MANAGEMENT_PROTOCOL_IGMP,
                MSG_NETWORK_IgmpJoinGroupTimer);

    // Put the group address in info field
    MESSAGE_InfoAlloc(node, newMsg, (sizeof(NodeAddress) + sizeof(Int32)+ 1));
    memcpy(MESSAGE_ReturnInfo(newMsg), &groupToJoin, sizeof(NodeAddress));

    ptr = MESSAGE_ReturnInfo(newMsg) + sizeof(NodeAddress);
    *ptr = (char)interfaceId;
    ptr += sizeof(Int32);
    *ptr = 0;

    // Send timer to self
    MESSAGE_Send(node, newMsg, timeToJoin);
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpLeaveGroup()
// PURPOSE      : This function will be called by ip.c when an multicast
//                application wants to leave a group. The function needs to
//                be called at the very start of the simulation.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
void IgmpLeaveGroup(
    Node* node,
    Int32 interfaceId,
    NodeAddress groupToLeave,
    clocktype timeToLeave)
{
    Message* newMsg = NULL;
    NetworkDataIp* ipLayer = NULL;
    char* ptr;

    ipLayer = (NetworkDataIp *) node->networkData.networkVar;

    // If IGMP not enable, then ignore joining group using IGMP.
    if (ipLayer->isIgmpEnable == FALSE)
    {
        char errStr[MAX_STRING_LENGTH];
        char buf[MAX_ADDRESS_STRING_LENGTH];

        IO_ConvertIpAddressToString(groupToLeave, buf);
        sprintf(errStr, "Node %u: IGMP is not enabled.\n"
            "    Unable to initiate leave message for group %s\n",
            node->nodeId, buf);
        ERROR_ReportWarning(errStr);

        return;
    }

    // Allocate new message
    newMsg = MESSAGE_Alloc(
                node, NETWORK_LAYER, GROUP_MANAGEMENT_PROTOCOL_IGMP,
                MSG_NETWORK_IgmpLeaveGroupTimer);

    // Put the group address in msg-.info field
    MESSAGE_InfoAlloc(node, newMsg, (sizeof(NodeAddress) + sizeof(Int32)));
    memcpy(MESSAGE_ReturnInfo(newMsg), &groupToLeave, sizeof(NodeAddress));

    ptr = MESSAGE_ReturnInfo(newMsg) + sizeof(NodeAddress);
    *ptr = (char)interfaceId;

    // Send timer to self
    MESSAGE_Send(node, newMsg, timeToLeave);
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpHandleQueryTimer()
// PURPOSE      : Take action when MSG_NETWORK_IgmpQueryTimer expire.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpHandleQueryTimer(
    Node* node,
    Message* msg)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpRouter* igmpRouter;
    Int32 intfId;

    memcpy(&intfId, MESSAGE_ReturnInfo(msg), sizeof(int));

    ERROR_Assert(igmp->igmpInterfaceInfo[intfId],
        "Invalid interface in MSG_NETWORK_IgmpQueryTimer\n");
    igmpRouter = igmp->igmpInterfaceInfo[intfId]->igmpRouter;

    if (igmpRouter->routersState == IGMP_QUERIER)
    {
        Message* selfMsg;
        clocktype delay;

        if (DEBUG) {
            printf("Node %u got MSG_NETWORK_IgmpQueryTimer expired\n",
                node->nodeId);
        }

        // Send general query message to all-systems multicast group
        IgmpSendQuery(node,
                      0,
                      intfId,
                      igmpRouter->queryResponseInterval,
                      igmpRouter->lastMemberQueryInterval);

        delay = (igmpRouter->queryDelayTimeFunc)(igmpRouter, msg);

        selfMsg = MESSAGE_Duplicate(node, msg);

        // Send timer to self
        MESSAGE_Send(node, selfMsg, delay);
    }
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpHandleOtherQuerierPresentTimer()
// PURPOSE      : Take action when MSG_NETWORK_IgmpOtherQuerierPresentTimer
//                expire.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpHandleOtherQuerierPresentTimer(
    Node* node,
    Message* msg)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpRouter* igmpRouter;
    IgmpRouterTimer* igmpTimer;
    Int32 intfId;

    // Retrieve information
    memcpy(&intfId, MESSAGE_ReturnInfo(msg), sizeof(int));

    ERROR_Assert(igmp->igmpInterfaceInfo[intfId],
        "Invalid interface in MSG_NETWORK_IgmpOtherQuerierPresentTimer\n");
    igmpRouter = igmp->igmpInterfaceInfo[intfId]->igmpRouter;

    igmpTimer = &(igmpRouter->timer);

    // Check if Other Querier Present Timer has been expired.
    // If so, then set Routers status as IGMP_QUERIER
    if (((getSimTime(node) - igmpTimer->lastQueryReceiveTime) >=
        igmpTimer->otherQuerierPresentInterval)
        && (igmpRouter->routersState == IGMP_NON_QUERIER))
    {
        igmpRouter->routersState = IGMP_QUERIER;

        if (DEBUG) {
            char timeStr[MAX_STRING_LENGTH];
            char netStr[20];

            TIME_PrintClockInSecond(getSimTime(node), timeStr);
            IO_ConvertIpAddressToString(
                NetworkIpGetInterfaceNetworkAddress(node, intfId),
                netStr);
            printf("Node %u got MSG_NETWORK_IgmpOtherQuerierPresentTimer "
                "expired\n"
                "Node %u become Querier again on network %s "
                "at time %ss\n",
                node->nodeId, node->nodeId, netStr, timeStr);
        }

        // Send general query message to all-systems multicast group
        IgmpSendQuery(node,
                      0,
                      intfId,
                      igmpRouter->queryResponseInterval,
                      igmpRouter->lastMemberQueryInterval);

        // Set general query timer
        IgmpSetQueryTimer(node, intfId);
    }
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpHandleGroupReplyTimer()
// PURPOSE      : Take action when MSG_NETWORK_IgmpGroupReplyTimer expire.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpHandleGroupReplyTimer(
    Node* node,
    Message* msg)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpHost* igmpHost;
    IgmpGroupInfoForHost* grpPtr;
    NodeAddress grpAddr;
    char* dataPtr;
    Int32 intfId;

    dataPtr = MESSAGE_ReturnInfo(msg);
    memcpy(&grpAddr, dataPtr, sizeof(NodeAddress));
    dataPtr += sizeof(NodeAddress);
    memcpy(&intfId, dataPtr, sizeof(Int32));


    ERROR_Assert(igmp->igmpInterfaceInfo[intfId],
        "Invalid interface in MSG_NETWORK_IgmpGroupReplyTimer\n");

    igmpHost = igmp->igmpInterfaceInfo[intfId]->igmpHost;

    // Search for specified group
    grpPtr = IgmpCheckHostGroupList(
                node, grpAddr, igmpHost->groupListHeadPointer);

    // Send report if the timer has not been cancelled yet
    if (grpPtr && grpPtr->hostState == IGMP_HOST_DELAYING_MEMBER)
    {
        IgmpSendMembershipReport(node, grpAddr, intfId);

        grpPtr->lastReportSendOrReceive = getSimTime(node);
        grpPtr->isLastHost = TRUE;
        grpPtr->hostState = IGMP_HOST_IDLE_MEMBER;
    }
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpHandleGroupMembershipTimer()
// PURPOSE      : Take action when MSG_NETWORK_IgmpGroupMembershipTimer
//                expire.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpHandleGroupMembershipTimer(
    Node* node,
    Message* msg)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpRouter* igmpRouter;
    IgmpGroupInfoForRouter* grpPtr;
    char* dataPtr;
    NodeAddress grpAddr;
    Int32 intfId;

    dataPtr = MESSAGE_ReturnInfo(msg);
    memcpy(&grpAddr, dataPtr, sizeof(NodeAddress));
    dataPtr += sizeof(NodeAddress);
    memcpy(&intfId, dataPtr, sizeof(int));

    ERROR_Assert(igmp->igmpInterfaceInfo[intfId],
        "Invalid interface in MSG_NETWORK_IgmpGroupMembershipTimer\n");

    igmpRouter = igmp->igmpInterfaceInfo[intfId]->igmpRouter;

    // Search for specified group
    grpPtr = IgmpCheckRtrGroupList(
                node, grpAddr, igmpRouter->groupListHeadPointer);

    if (grpPtr && grpPtr->membershipState == IGMP_ROUTER_MEMBERS)
    {
        if ((getSimTime(node) - grpPtr->lastReportReceive) >=
            grpPtr->groupMembershipInterval)
        {
            Int32 mcastIntfId = intfId;
            MulticastProtocolType notifyMcastRtProto =
                IgmpGetMulticastProtocolInfo(node, intfId);

            IgmpDeleteRtrGroupList(node, grpAddr, intfId);
#ifdef ADDON_DB
            HandleStatsDBIgmpSummary(node, "Delete", 0, grpAddr);
#endif
            if (igmp->igmpInterfaceInfo[intfId]->igmpNodeType == IGMP_PROXY)
            {
                if (IgmpCheckSubscriptionInAllOtherDownstreamInterface(
                    node, igmp, grpAddr, intfId))
                {
                    return;
                }
                else
                {
                    Int32 upstreamIntfId = 0;

                    upstreamIntfId =
                        (Int32)NetworkIpGetInterfaceIndexFromAddress(
                                     node, igmp->igmpUpstreamInterfaceAddr);

                    //notify upstream. just call igmpleavegroup
                    //with timetoleave = 0
                    IgmpLeaveGroup(node,
                                   upstreamIntfId,
                                   grpAddr,
                                   (clocktype)0);
                }
            }
            else
            {
                // Notify multicast routing protocol
                if (notifyMcastRtProto)
                {
                    (notifyMcastRtProto)(
                        node, grpAddr, intfId, LOCAL_MEMBER_LEAVE_GROUP);
                }
            }
        }
    }
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpHandleJoinGroupTimer()
// PURPOSE      : Take action when MSG_NETWORK_IgmpJoinGroupTimer expire.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpHandleJoinGroupTimer(
    Node* node,
    Message* msg)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpHost* igmpHost;
    IgmpGroupInfoForHost* grpPtr;
    char* dataPtr;
    NodeAddress grpAddr;
    char unsolicitedReportCount;
    Int32 intfId;

    dataPtr = MESSAGE_ReturnInfo(msg);
    memcpy(&grpAddr, dataPtr, sizeof(NodeAddress));
    dataPtr += sizeof(NodeAddress);
    memcpy(&intfId, dataPtr, sizeof(Int32));

#ifdef ADDON_BOEINGFCS
    if (node->macData[intfId]->macProtocol ==
        MAC_PROTOCOL_LINK_EPLRS)
    {
        return;
    }
#endif

    dataPtr += sizeof(Int32);
    memcpy(&unsolicitedReportCount, dataPtr, sizeof(char));

    ERROR_Assert(igmp->igmpInterfaceInfo[intfId],
        "Invalid interface in MSG_NETWORK_IgmpJoinGroupTimer\n");

    igmpHost = igmp->igmpInterfaceInfo[intfId]->igmpHost;

    grpPtr = IgmpCheckHostGroupList(
                node, grpAddr, igmpHost->groupListHeadPointer);

    switch (unsolicitedReportCount)
    {
        case 0:
        {
            // If the group exist, do nothing
            if (grpPtr) {
                    if (DEBUG) {
                    char grpStr[20];
                    char intfStr[20];

                    IO_ConvertIpAddressToString(grpAddr, grpStr);
                    IO_ConvertIpAddressToString(
                        NetworkIpGetInterfaceAddress(node, intfId),
                        intfStr);
                    printf("Host %u is already a member of group %s "
                        "on interface %s\n",
                        node->nodeId, grpStr, intfStr);
                }

                break;
            }

            grpPtr = IgmpInsertHostGroupList(node, grpAddr, intfId);

#ifdef ADDON_DB
            // Get IP address for this interface
            NodeAddress srcAddr = NetworkIpGetInterfaceAddress(node, intfId);
            HandleMulticastDBStatus(node, intfId, "Join", srcAddr, grpAddr);
#endif
            // Fall through case
        }
        default:
        {
           // Group member may leave the group while unsolicited report
           // is being sent so grpPtr to be checked for NULL value.If node
           // left the group then no needs to send unsolicited report

           if (!grpPtr){
              return;
            }

           if (DEBUG) {
                printf("Host %u sending unsolicited report\n", node->nodeId);
            }

           //Report should be sent only when the timer has not
           //been cancelled i.e. it is in the delaying member state
           //However, we do need to schedule the next report
           if ((unsolicitedReportCount == 0) ||
              (grpPtr->hostState == IGMP_HOST_DELAYING_MEMBER))
           {
            // Send unsolicited membership report
            IgmpSendMembershipReport(node, grpAddr, intfId);

            grpPtr->lastReportSendOrReceive = getSimTime(node);
            grpPtr->isLastHost = TRUE;
            grpPtr->hostState = IGMP_HOST_IDLE_MEMBER;
           }
            unsolicitedReportCount++;

            if (unsolicitedReportCount < igmpHost->unsolicitedReportCount)
            {
                Message* selfMsg;

                grpPtr->hostState = IGMP_HOST_DELAYING_MEMBER;

                // Act as a query just has been received
                grpPtr->lastQueryReceive = getSimTime(node);

                // Allocate message to trigger self
                memcpy(dataPtr, &unsolicitedReportCount, sizeof(char));
                selfMsg = MESSAGE_Duplicate(node, msg);

                // unsolicited Reports are sent after every
                // IGMP_UNSOLICITED_REPORT_INTERVAL

                // Send timer to self
                MESSAGE_Send(node, selfMsg, IGMP_UNSOLICITED_REPORT_INTERVAL);
            }
            break;
        }
    } // switch
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpHandleLeaveGroupTimer()
// PURPOSE      : Take action when MSG_NETWORK_IgmpLeaveGroupTimer expire.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpHandleLeaveGroupTimer(
    Node* node,
    Message* msg)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpHost* igmpHost;
    char* dataPtr;
    NodeAddress grpAddr;
    Int32 intfId;
    BOOL retVal = FALSE;
    BOOL isLastHost = FALSE;

    dataPtr = MESSAGE_ReturnInfo(msg);
    memcpy(&grpAddr, dataPtr, sizeof(NodeAddress));
    dataPtr += sizeof(NodeAddress);
    memcpy(&intfId, dataPtr, sizeof(int));

#ifdef ADDON_BOEINGFCS
    if (node->macData[intfId]->macProtocol ==
        MAC_PROTOCOL_LINK_EPLRS)
    {
        return;
    }
#endif

    ERROR_Assert(igmp->igmpInterfaceInfo[intfId],
        "Invalid interface in MSG_NETWORK_IgmpLeaveGroupTimer\n");

    igmpHost = igmp->igmpInterfaceInfo[intfId]->igmpHost;

    if (DEBUG) {
        char timeStr[MAX_STRING_LENGTH];
        char grpStr[20];

        printf("Node %u got MSG_NETWORK_IgmpLeaveGroupTimer expired\n",
            node->nodeId);

        TIME_PrintClockInSecond(getSimTime(node), timeStr);
        IO_ConvertIpAddressToString(grpAddr, grpStr);
        printf("Node %u : Igmp receive group leave request from "
            "upper layer at time %s for group %s\n",
            node->nodeId, timeStr, grpStr);
    }

    // Delete group entry
    retVal = IgmpDeleteHostGroupList(
                node, grpAddr, intfId, &isLastHost);

    if (retVal && isLastHost) {
        // Send leave report
        IgmpSendLeaveReport(node, grpAddr, intfId);
    }
#ifdef CYBER_CORE
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;

    if (ip->iahepEnabled && ip->iahepData->nodeType == IAHEP_NODE)
    {
        NodeAddress mappedGrpAddr = IAHEPGetMulticastBroadcastAddressMapping(
                                            node, ip->iahepData, grpAddr);

        IAHEPCreateIgmpJoinLeaveMessage(
            node,
            mappedGrpAddr,
            IGMP_LEAVE_GROUP_MSG,
            NULL);

        ip->iahepData->grpAddressList->erase(grpAddr);

        ip->iahepData->updateAmdTable->getAMDTable(0)->
                    iahepStats.iahepIGMPLeavesSent ++;

        if (IAHEP_DEBUG)
        {
            char addrStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(mappedGrpAddr, addrStr);
            printf("\nIAHEP Node: %d", node->nodeId);
            printf("\tLeaves Group: %s\n", addrStr);
        }
}
#endif //CYBER_CORE

}
//---------------------------------------------------------------------------
// FUNCTION     : IgmpHandleLastMemberTimer()
// PURPOSE      : Take action when MSG_NETWORK_IgmpLastMemberTimer expire.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpHandleLastMemberTimer(
    Node* node,
    Message* msg)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpRouter* igmpRouter;
    IgmpGroupInfoForRouter* grpPtr;
    char* dataPtr;
    NodeAddress grpAddr;
    Int32 intfId;
    short count;

    dataPtr = MESSAGE_ReturnInfo(msg);
    memcpy(&grpAddr, dataPtr, sizeof(NodeAddress));
    dataPtr += sizeof(NodeAddress);
    memcpy(&intfId, dataPtr, sizeof(int));
    dataPtr += sizeof(int);
    memcpy(&count, dataPtr, sizeof(short));

    ERROR_Assert(igmp->igmpInterfaceInfo[intfId],
        "Invalid interface in MSG_NETWORK_IgmpLastMemberTimer\n");

    igmpRouter = igmp->igmpInterfaceInfo[intfId]->igmpRouter;

    // Search for specified group
    grpPtr = IgmpCheckRtrGroupList(
                node, grpAddr, igmpRouter->groupListHeadPointer);

    if (!grpPtr) {
        char errStr[MAX_STRING_LENGTH];

        sprintf(errStr, "Node %u: Group shouldn't be deleted before sending "
            "[Last Member Query Count] times group specific query\n",
            node->nodeId);
        ERROR_Assert(FALSE, errStr);
        return;
    }

    count++;

    // Keep setting timer until we've received a report or we've
    // sent the Group specific query [Last Member Query Count] times.

    if (((getSimTime(node) - grpPtr->lastReportReceive) >
        (count * (grpPtr->lastMemberQueryInterval / 10)*SECOND))
        && (count < grpPtr->lastMemberQueryCount))
    {
        // Send Group specific Query message
        IgmpSendQuery(node,
                      grpAddr,
                      intfId,
                      igmpRouter->queryResponseInterval,
                      igmpRouter->lastMemberQueryInterval);

        IgmpSetLastMembershipTimer(node,
                                   grpAddr,
                                   intfId,
                                   count,
                                   igmpRouter->lastMemberQueryInterval);
    }
    else if ((count == grpPtr->lastMemberQueryCount)
        && ((getSimTime(node) - grpPtr->lastReportReceive) >
            (count * (grpPtr->lastMemberQueryInterval / 10)*SECOND)))
    {
        MulticastProtocolType notifyMcastRtProto =
            IgmpGetMulticastProtocolInfo(node, intfId);

        ERROR_Assert(grpPtr->membershipState ==
            IGMP_ROUTER_CHECKING_MEMBERSHIP,
            "Group state should be IGMP_ROUTER_CHECKING_MEMBERSHIP\n");

        IgmpDeleteRtrGroupList(node, grpAddr, intfId);
#ifdef ADDON_DB
        HandleStatsDBIgmpSummary(node, "Delete", 0, grpAddr);
#endif
        if (igmp->igmpInterfaceInfo[intfId]->igmpNodeType == IGMP_PROXY)
        {
            if (IgmpCheckSubscriptionInAllOtherDownstreamInterface(
                node, igmp, grpAddr, intfId))
            {
                return;
            }
            else
            {
                Int32 upstreamIntfId = 0;

                upstreamIntfId =
                    (Int32)NetworkIpGetInterfaceIndexFromAddress(
                                 node, igmp->igmpUpstreamInterfaceAddr);

                //notify upstream. just call igmpleavegroup
                //with timetoleave = 0
                IgmpLeaveGroup(node,
                               upstreamIntfId,
                               grpAddr,
                               (clocktype)0);
            }
        }
        else
        {
        // Notify multicast routing protocol
        if (notifyMcastRtProto)
        {
            (notifyMcastRtProto)(
                node, grpAddr, intfId, LOCAL_MEMBER_LEAVE_GROUP);
        }
    }
}
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpIsRouter()
// PURPOSE      : Check whether the node is a router
// RETURN VALUE : TRUE if this node is a router. FALSE otherwise.
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
BOOL IgmpIsRouter(
    Node* node,
    const NodeInput* nodeInput,
    Int32 interfaceId)
{
    char currentLine[200*MAX_STRING_LENGTH];

    // For IO_GetDelimitedToken
    char iotoken[MAX_STRING_LENGTH];
    char* next;

    const char *delims = "{,} \t\n";
    char *token = NULL;
    char *ptr = NULL;
    unsigned int id;
    int i;
    Address srcAddr;
    NodeAddress nodeId;
    BOOL isSrcNodeId,isIpv6Addr;
    Int32 intfId;

    for (i = 0; i < nodeInput->numLines; i++)
    {
        if (strcmp(nodeInput->variableNames[i], "IGMP-ROUTER-LIST") == 0)
        {
            if ((ptr = strchr(nodeInput->values[i],'{')) == NULL)
            {
                char errorStr[MAX_STRING_LENGTH];
                sprintf(errorStr,
                        "Could not find '{' character:\n  %s\n",
                        nodeInput->values[i]);
                ERROR_ReportError(errorStr);
            }
            else
            {
                strcpy(currentLine, ptr);
                token = IO_GetDelimitedToken(
                            iotoken, currentLine, delims, &next);
                if (!token)
                {
                    char errorStr[MAX_STRING_LENGTH];
                    sprintf(errorStr,
                            "Cann't find nodeId list, e.g. {1, 2, .. "
                            "}:\n%s\n",
                            currentLine);
                    ERROR_ReportError(errorStr);
                }
                while (TRUE)
                {
                    intfId = interfaceId;
                    IO_ParseNodeIdHostOrNetworkAddress(token,
                                                      &srcAddr,
                                                      &isSrcNodeId);
                    if (!isSrcNodeId)
                    {
                        intfId =
                          MAPPING_GetInterfaceIndexFromInterfaceAddress(node,
                                                                    srcAddr);
                        nodeId =
                          MAPPING_GetNodeIdFromInterfaceAddress(node,
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
                                IO_ParseNodeIdHostOrNetworkAddress(token,
                                                 &srcAddr.interfaceAddr.ipv6,
                                                 &isIpv6Addr,
                                                 &nodeId);
                            }
                        }
                    }
                    if (node->nodeId == nodeId && intfId == interfaceId)
                    {
                        return TRUE;
                    }
                    token = IO_GetDelimitedToken(iotoken,
                                                 next,
                                                 delims,
                                                 &next);
                    if (!token)
                    {
                        break;
                    }
                    if (strncmp("thru", token, 4) == 0)
                    {
                        if (!isSrcNodeId)
                        {
                            char errorStr[MAX_STRING_LENGTH];
                            sprintf(errorStr,
                                    "thru should be used with Node Id \n%s",
                                    currentLine);
                            ERROR_ReportError(errorStr);
                        }
                        else
                        {
                            unsigned int maxNodeId;
                            unsigned int minNodeId;

                            minNodeId = nodeId;
                            token = IO_GetDelimitedToken(iotoken,
                                                         next,
                                                         delims,
                                                         &next);
                            if (!token)
                            {
                                char errorStr[MAX_STRING_LENGTH];
                                sprintf(errorStr,
                                        "Bad string input\n%s",
                                        currentLine);
                                ERROR_ReportError(errorStr);
                            }
                            maxNodeId = atoi(token);
                            if (maxNodeId < minNodeId)
                            {
                                char errorStr[MAX_STRING_LENGTH];
                                sprintf(errorStr,
                                        "Bad string input\n%s",
                                        currentLine);
                                ERROR_ReportError(errorStr);
                            }
                            if ((node->nodeId >= minNodeId)
                                              && (node->nodeId <= maxNodeId))
                            {
                                return TRUE;
                            }
                            else
                            {
                                token = IO_GetDelimitedToken(iotoken,
                                                             next,
                                                             delims,
                                                             &next);
                            }
                            if (!token)
                            {
                                break;
                            }
                        }
                    }
                }// End while
            }// End else
        }
    }// End for
    return FALSE;
}


void IgmpSearchLocalGroupDatabase(
    Node* node,
    NodeAddress grpAddr,
    int interfaceId,
    BOOL* localMember)
{
    IgmpData* igmp;
    IgmpRouter* igmpRouter = NULL;
    NetworkDataIp *ipLayer = (NetworkDataIp *) node->networkData.networkVar;

    *localMember = FALSE;

    // If IGMP is not running in this interface or if it's not a router
    if (ipLayer->isIgmpEnable == FALSE) {
        return;
    }

    igmp = IgmpGetDataPtr(node);
    if ((interfaceId < 0)
        || !igmp
        || !igmp->igmpInterfaceInfo[interfaceId]
        || igmp->igmpInterfaceInfo[interfaceId]->igmpNodeType != IGMP_ROUTER)
    {
        return;
    }

    igmpRouter = igmp->igmpInterfaceInfo[interfaceId]->igmpRouter;

    if (IgmpCheckRtrGroupList(
            node, grpAddr, igmpRouter->groupListHeadPointer))
    {
        *localMember = TRUE;
    }
}


void IgmpSetMulticastProtocolInfo(
    Node* node,
    int interfaceId,
    MulticastProtocolType mcastProtoPtr)
{
    IgmpData* igmp = IgmpGetDataPtr(node);

    // If IGMP is not running in this interface
    if (!igmp || !igmp->igmpInterfaceInfo[interfaceId])
    {
        char errStr[MAX_STRING_LENGTH];
        char intfStr[MAX_ADDRESS_STRING_LENGTH];

        IO_ConvertIpAddressToString(
            NetworkIpGetInterfaceAddress(node, interfaceId),
            intfStr);
        sprintf(errStr, "Node %u: IGMP is not running over interface %s\n",
            node->nodeId, intfStr);
        ERROR_Assert(FALSE, errStr);
        return;
    }

    igmp->igmpInterfaceInfo[interfaceId]->multicastProtocol = mcastProtoPtr;
}


MulticastProtocolType IgmpGetMulticastProtocolInfo(
    Node* node,
    int interfaceId)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpInterfaceInfoType* thisIntf;

    // If IGMP is not running in this interface
    if (!igmp || !igmp->igmpInterfaceInfo[interfaceId])
    {
        char errStr[MAX_STRING_LENGTH];
        char intfStr[MAX_ADDRESS_STRING_LENGTH];

        IO_ConvertIpAddressToString(
            NetworkIpGetInterfaceAddress(node, interfaceId),
            intfStr);
        sprintf(errStr, "Node %u: IGMP is not running over interface %s\n",
            node->nodeId, intfStr);
        ERROR_Assert(FALSE, errStr);
        return NULL;
    }

    thisIntf = igmp->igmpInterfaceInfo[interfaceId];

    if (thisIntf->multicastProtocol)
    {
        return (thisIntf->multicastProtocol);
    }

    return NULL;
}


BOOL IgmpGrpIsInList(Node* node, LinkedList* grpList, NodeAddress groupId)
{
    ListItem* tempItem = NULL;
    NodeAddress* groupAddr = NULL;


    tempItem = grpList->first;

    while (tempItem)
    {
        groupAddr = (NodeAddress *) tempItem->data;

        if (*groupAddr == groupId)
        {
            return TRUE;
        }

        tempItem = tempItem->next;
    }

    return FALSE;
}


void IgmpFillGroupList(Node *node, LinkedList** grpList, int interfaceId)
{
    NetworkDataIp *ipLayer = (NetworkDataIp *) node->networkData.networkVar;
    IgmpGroupInfoForRouter *tempPtr = NULL;
    int i;
    int count = 0;


    ListInit(node, grpList);

    /* If IGMP is not running in this node or the node is not igmp router */
    IgmpNodeType nodeType =
          ipLayer->igmpDataPtr->igmpInterfaceInfo[interfaceId]->igmpNodeType;
    if ((ipLayer->isIgmpEnable == FALSE)
        || (nodeType != IGMP_ROUTER))
    {
        return;
    }

    for (i = 0; i < node->numberInterfaces; i++)
    {
        /*
         * If IGMP is not running over this interface, then skip this
         * interface
        */
        if (ipLayer->igmpDataPtr->igmpInterfaceInfo[i] == NULL)
        {
            continue;
        }

        if (interfaceId != ANY_INTERFACE && interfaceId != i)
        {
            continue;
        }

        tempPtr = ipLayer->igmpDataPtr->igmpInterfaceInfo[i]
                          ->igmpRouter->groupListHeadPointer;

        while (tempPtr)
        {
            NodeAddress *groupAddr;

            if ((i != 0)
                && (IgmpGrpIsInList(node,
                                           *grpList,
                                           tempPtr->groupId)))

            {
                tempPtr = tempPtr->next;
                continue;
            }

            groupAddr = (NodeAddress *) MEM_malloc(sizeof(NodeAddress));

            memcpy(groupAddr, &tempPtr->groupId, sizeof(NodeAddress));

            ListInsert(node, *grpList, 0, (void *) groupAddr);

            count++;
            tempPtr = tempPtr->next;
        }
    }

    ERROR_Assert(count == (*grpList)->size, "Fail to fill up group list\n");
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpHostHandleQuery()
// PURPOSE      : Host processing a received query.
// RETURN VALUE : TRUE if successfully process, FALSE otherwise.
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpHostHandleQuery(
    Node* node,
    IgmpMessage* igmpPkt,
    IgmpInterfaceInfoType* thisInterface,
    Int32 intfId)
{
    IgmpGroupInfoForHost* tempGrpPtr;
    IgmpHost* igmpHost = NULL;

    igmpHost = thisInterface->igmpHost;

    if (DEBUG) {
        printf("    Host processing of IGMP_QUERY_MSG\n");
    }

    if (igmpPkt->groupAddress == 0)
    {
        // General query received
        if (DEBUG) {
            printf("    General query\n");
        }

#ifdef CYBER_CORE

        NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
        Message* newMsg = NULL;
        NodeAddress grpAddr = ANY_ADDRESS;
        set<NodeAddress>::iterator grpAddrIter;
        map<NodeAddress, NodeAddress>::iterator amdIter;
            map<NodeAddress, NodeAddress> tempGrpMapTable;

        if (ip->iahepEnabled && ip->iahepData->nodeType == IAHEP_NODE
            && IsIAHEPBlackSecureInterface(node, intfId))
        {

            amdIter = ip->iahepData->multicastAmdMapping->find(ANY_ADDRESS);
            if (amdIter !=  ip->iahepData->multicastAmdMapping->end())
            {
                    tempGrpMapTable.insert(
                    pair<NodeAddress, NodeAddress>
                    (ANY_ADDRESS, (NodeAddress)amdIter->second));
                }
            else
            {
                tempGrpMapTable.insert(
                    pair<NodeAddress, NodeAddress>
                    (ANY_ADDRESS, BROADCAST_MULTICAST_MAPPEDADDRESS));
            }

            for (grpAddrIter = ip->iahepData->grpAddressList->begin();
                            grpAddrIter != ip->iahepData->grpAddressList->end();
                            ++grpAddrIter)
            {
                grpAddr = *grpAddrIter;
                if (grpAddr != ANY_ADDRESS)
                {
                    amdIter = ip->iahepData->multicastAmdMapping->find(grpAddr);

                    if (amdIter !=  ip->iahepData->multicastAmdMapping->end())
                    {
                        tempGrpMapTable[grpAddr] = (NodeAddress)amdIter->second;
                    }
                    else
                    {
                        tempGrpMapTable[grpAddr] = grpAddr +
                            DEFAULT_RED_BLACK_MULTICAST_MAPPING;
                    }
                }
            }


            map<NodeAddress, NodeAddress>::iterator it;

            for (it = tempGrpMapTable.begin(); it != tempGrpMapTable.end();
                    ++it)
            {
                IAHEPCreateIgmpJoinLeaveMessage(
                    node,
                    it->second,
                    IGMP_MEMBERSHIP_REPORT_MSG,
                    NULL);

                thisInterface->igmpStat.igmpTotalMsgSend += 1;
                thisInterface->igmpStat.igmpTotalReportSend += 1;
                thisInterface->igmpStat.igmpMembershipReportSend += 1;

                ip->iahepData->updateAmdTable->getAMDTable(0)->
                    iahepStats.iahepIGMPReportsSent ++;

                if (IAHEP_DEBUG)
                {
                    char addrStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(it->second, addrStr);
                    printf("\nIAHEP Node: %d", node->nodeId);
                    printf("\tJoins Group: %s\n", addrStr);
                }
            }
        }
        else
        {
#endif //CYBER_CORE
        if (igmpHost->groupCount == 0)
        {
            if (DEBUG) {
                printf("    Currently no members\n");
            }

            // No member exist. Nothing to do.
            return;
        }

        tempGrpPtr = igmpHost->groupListHeadPointer;

        // Set delay timer for each group of which it is a member
        while (tempGrpPtr != NULL)
        {
            IgmpHostSetOrResetMembershipTimer(
                node, igmpPkt, tempGrpPtr, intfId);

            tempGrpPtr = tempGrpPtr->next;
        }
#ifdef CYBER_CORE
        }
#endif //CYBER_CORE
    }
    else
    {
        // Group specific query
        if (DEBUG) {
            printf("    Group specific query\n");
        }

        // Search for specified group
        tempGrpPtr = IgmpCheckHostGroupList(
                        node, igmpPkt->groupAddress,
                        igmpHost->groupListHeadPointer);

        // If the group exist, set timer for sending report
        if (tempGrpPtr)
        {
            IgmpHostSetOrResetMembershipTimer(
                node, igmpPkt, tempGrpPtr, intfId);
        }
        else
        {
            IgmpData* igmp = IgmpGetDataPtr(node);

            if (igmp->igmpInterfaceInfo[intfId]->igmpNodeType == IGMP_HOST) {
                if (DEBUG) {
                    printf("    Received bad query\n");
                }

                thisInterface->igmpStat.igmpBadQueryReceived += 1;
                thisInterface->igmpStat.igmpBadMsgReceived += 1;
            }
        }
    }
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpHostHandleReport()
// PURPOSE      : Host processing a received report.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpHostHandleReport(
    Node* node,
    IgmpMessage* igmpPkt,
    IgmpInterfaceInfoType* thisInterface)
{
    IgmpHost* igmpHost = NULL;
    IgmpGroupInfoForHost* tempGrpPtr = NULL;

    igmpHost = thisInterface->igmpHost;

    //This stat has already been incremented at the
    //time of calling this function

    //thisInterface->igmpStat.igmpTotalReportReceived += 1;

    if (DEBUG) {
        printf("    Host processing of IGMP_MEMBERSHIP_REPORT_MSG\n");
    }

    // Search for specified group
    tempGrpPtr = IgmpCheckHostGroupList(
                    node, igmpPkt->groupAddress,
                    igmpHost->groupListHeadPointer);

    // If the group exist cancel timer to prevent sending
    // of duplicate report in the subnet
    if (tempGrpPtr)
    {
        tempGrpPtr->lastReportSendOrReceive = getSimTime(node);
        tempGrpPtr->hostState = IGMP_HOST_IDLE_MEMBER;

        // Remember that we are not the last host to reply to Query
        tempGrpPtr->isLastHost = FALSE;

        if (DEBUG) {
            char timeStr[MAX_STRING_LENGTH];
            char ipStr[MAX_ADDRESS_STRING_LENGTH];

            TIME_PrintClockInSecond(getSimTime(node), timeStr);
            IO_ConvertIpAddressToString(igmpPkt->groupAddress, ipStr);
            printf("    Receive membership report for group %s at "
                "time %ss\n"
                "    Cancelling self timer",
                ipStr, timeStr);
        }
    }
    else
    {
        if (thisInterface->igmpNodeType == IGMP_HOST) {
            if (DEBUG) {
                printf("    Received bad membership report\n");
            }
            thisInterface->igmpStat.igmpBadReportReceived += 1;
            thisInterface->igmpStat.igmpBadMsgReceived += 1;
        }
    }
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpHandlePacketForHost()
// PURPOSE      : Host processing a incoming IGMP packet.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpHandlePacketForHost(
    Node* node,
    Message* msg,
    NodeAddress srcAddr,
    NodeAddress grpAddr,
    Int32 intfId)
{
    IgmpMessage* igmpPkt = NULL;
    IgmpData* igmp = NULL;
    IgmpInterfaceInfoType* thisInterface = NULL;

    igmp = IgmpGetDataPtr(node);
    thisInterface = igmp->igmpInterfaceInfo[intfId];

    igmpPkt = (IgmpMessage*) MESSAGE_ReturnPacket(msg);

    switch (igmpPkt->ver_type)
    {
        case IGMP_QUERY_MSG:
        {
            thisInterface->igmpStat.igmpTotalQueryReceived += 1;

            if (igmpPkt->groupAddress == 0) {
                thisInterface->igmpStat.igmpGenQueryReceived += 1;
            } else {
                thisInterface-> igmpStat.igmpGrpSpecificQueryReceived += 1;
            }

            IgmpHostHandleQuery(node, igmpPkt, thisInterface, intfId);

            break;
        }
        case IGMP_MEMBERSHIP_REPORT_MSG:
        {
            thisInterface->igmpStat.igmpTotalReportReceived += 1;
            thisInterface->igmpStat.igmpMembershipReportReceived += 1;

            IgmpHostHandleReport(node, igmpPkt, thisInterface);

            break;
        }
        default:
        {
            thisInterface->igmpStat.igmpBadMsgReceived += 1;

            if (DEBUG) {
                printf("    Invalid Message type");
            }
            break;
        }
    }
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpRouterHandleQuery()
// PURPOSE      : Router processing a received query.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpRouterHandleQuery(
    Node* node,
    IgmpMessage* igmpPkt,
    IgmpRouter* igmpRouter,
    int srcAddr,
    Int32 intfId,
    Int32 robustnessVariable)
{
    IgmpData* igmp = IgmpGetDataPtr(node);
    IgmpGroupInfoForRouter* tempGrpPtr;
    NodeAddress intfAddr;
    Message* newMsg;

    intfAddr = NetworkIpGetInterfaceAddress(node, intfId);

    if (DEBUG) {
        printf("    Router processing of IGMP_QUERY_MSG\n");
    }

    // Querier Router selection
    if ((igmpPkt->groupAddress == 0) && (srcAddr <(signed) intfAddr) &&
        !MAC_IsWirelessAdHocNetwork(node, intfId))
    {
        if (igmpRouter->routersState == IGMP_QUERIER)
        {
            // If the router is in checking membership state
            // then ignore Querier to Non-Querier transition
            tempGrpPtr = igmpRouter->groupListHeadPointer;

            while ((tempGrpPtr != NULL)
                && (tempGrpPtr->membershipState !=
                    IGMP_ROUTER_CHECKING_MEMBERSHIP))
            {
                tempGrpPtr = tempGrpPtr->next;
            }

            if (!tempGrpPtr)
            {
                if (DEBUG) {
                    char netStr[20];

                    IO_ConvertIpAddressToString(
                        NetworkIpGetInterfaceNetworkAddress(node, intfId),
                        netStr);
                    printf("    Node %u become non querier on network %s\n",
                            node->nodeId, netStr);
                }

                // Make this router as Non-Querier
                igmpRouter->routersState = IGMP_NON_QUERIER;
            }
        }

        // Note the time when last query message receive
        igmpRouter->timer.lastQueryReceiveTime = getSimTime(node);

        newMsg = MESSAGE_Alloc(
                    node, NETWORK_LAYER, GROUP_MANAGEMENT_PROTOCOL_IGMP,
                    MSG_NETWORK_IgmpOtherQuerierPresentTimer);

        MESSAGE_InfoAlloc(node, newMsg, sizeof(int));
        memcpy(MESSAGE_ReturnInfo(newMsg), &intfId, sizeof(int));
    //The default value for Query Response interval is 100 which on
    //converting into units comes out to be 10 seconds. In QualNet at
    //initialization it is stored as 100 or converting the value configured
    //by user (in seconds) to the normal value i.e. dividing by seconds and
    //multiplying by 10.Hence,it is required to divide the current value in
    //queryResponseInterval by 10 and multiply by SECOND.
        MESSAGE_Send(node,
                     newMsg,
                     (clocktype) ((robustnessVariable * igmpRouter->queryInterval)
                      +( 0.5 * (igmpRouter->queryResponseInterval * SECOND)
                      /IGMP_QUERY_RESPONSE_SCALE_FACTOR)));

    }

    // If it is Group-Specific-Query and the router is Non-Querier
    if ((igmpPkt->groupAddress != 0)
        && (igmpRouter->routersState == IGMP_NON_QUERIER))
    {
        // Search for specific group
        tempGrpPtr = IgmpCheckRtrGroupList(
                        node, igmpPkt->groupAddress,
                        igmpRouter->groupListHeadPointer);

        if (tempGrpPtr)
        {
            clocktype remainingDelay;
            clocktype checkingMembershipDelay;

            remainingDelay =
                tempGrpPtr->groupMembershipInterval -
                (getSimTime(node) - tempGrpPtr->lastReportReceive);

            checkingMembershipDelay =
                tempGrpPtr->lastMemberQueryCount *
                ((igmpPkt->maxResponseTime /10)* SECOND);

            // When a non-Querier receives a Group-Specific Query
            // message, if its existing group membership timer is
            // greater than [Last Member Query Count] times the Max
            // Response Time specified in the message, it sets its
            // group membership timer to that value.
            if (remainingDelay > checkingMembershipDelay)
            {
                tempGrpPtr->groupMembershipInterval =
                    checkingMembershipDelay;

                if (DEBUG) {
                    char timeStr[MAX_STRING_LENGTH];

                    TIME_PrintClockInSecond(
                        checkingMembershipDelay, timeStr);
                    printf("    Non Querier router setting group delay "
                        "timer to new value %ss\n",
                        timeStr);
                }

                // Set timer for the membership
                IgmpSetGroupMembershipTimer(
                    node, tempGrpPtr, intfId);
            }
        }
    }
    //IAHEP node need to get original multicast Address in case of Query
    //message recieved from Black Node
#ifdef CYBER_CORE
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;

    if (ip->iahepEnabled && ip->iahepData->nodeType == IAHEP_NODE)
    {
        NodeAddress mappedGrpAddr = 0;
        map<NodeAddress,NodeAddress>::iterator it;

        if (igmpPkt->groupAddress)
        {
            for (it=ip->iahepData->multicastAmdMapping->begin();
            it != ip->iahepData->multicastAmdMapping->end(); it++)
            {
                if ((*it).second == igmpPkt->groupAddress)
                {
                    mappedGrpAddr = (*it).first;
                }
            }

            if (!mappedGrpAddr)
            {
                mappedGrpAddr = mappedGrpAddr
                    - DEFAULT_RED_BLACK_MULTICAST_MAPPING;
            }
        }

        IAHEPCreateIgmpJoinLeaveMessage(
            node,
            mappedGrpAddr,
            IGMP_QUERY_MSG,
            igmpPkt);

        if (IAHEP_DEBUG)
        {
            char addrStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(mappedGrpAddr, addrStr);
            printf("\nIAHEP Node: %d", node->nodeId);
            printf("\nSends Query To Red Node For Group: %s\n", addrStr);
        }
    }
#endif

}
#ifdef CYBER_CORE
/*!
 * \brief Get PIM SM Data
 *
 * \param node      Pointer to current node
 * \param intfId    Interface index
 *
 * \return     Pointer to the pim sm data
 *
 * Get pim sm data for the node. Called when it is to check if PIM SM
 * is enabled.
 */
static
PimDataSm* IgmpGetPimSmData(
               Node* node,
               Int32 intfId)
{
    PimData* pim = NULL;
    PimDataSm* pimDataSm = NULL;

    if (pim = (PimData*)NetworkIpGetMulticastRoutingProtocol(node,
        MULTICAST_PROTOCOL_PIM))
    {
        if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
        {
            pimDataSm = (PimDataSm*)pim->pimModePtr;
        }
        else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
        {
            pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;
        }
    }

    return pimDataSm;
}

/*!
 * \brief Check if PIM SM is enabled on the node
 *
 * \param node      Pointer to current node
 * \param intfId    Interface index
 *
 * \return     TRUE if PIM SM is enabled, otherwise FALSE
 *
 * Check if pim sm is enabled on the node. Called when black node receives
 * a report message.
 */
static
BOOL IgmpPimSmIsEnabled(
        Node* node,
        Int32 intfId)
{
    PimDataSm* pimDataSm = NULL;

    pimDataSm = IgmpGetPimSmData(node, intfId);
    if (pimDataSm)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*!
 * \brief Start the black network (S,G) PIM JOIN process by notifying
 *        PIM SM module at IAHEP node with (src,grp) info
 *
 * \param node      Pointer to current node
 * \param intfId    Interface index
 * \param blackMcstSrcAddr black source address
 * \param blackMcstGrpAddress black group address
 * \param event     JOIN or LEAVE message
 *
 * \return     NONE
 *
 * Start the black network (S,G) JOIN process. Called when black node receives
 * a report message.
 */
static
void IgmpStartPimSmSGJoinLeaveInBlackNetwork(
            Node* node,
            Int32 intfId,
            NodeAddress blackMcstSrcAddress,
            NodeAddress blackMcstGrpAddress,
            LocalGroupMembershipEventType event)
{
    PimDataSm* pimDataSm = NULL;
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;

    pimDataSm = IgmpGetPimSmData(node, intfId);

    // if a black router receivers a IGMP report with
    // blackSrcAddress inititate the PIM-SM SPT JOIN in black side
    if (pimDataSm
        && ip->iahepEnabled
        && ip->iahepData->nodeType == BLACK_NODE
        && IsIAHEPBlackSecureInterface(node, intfId))
    {
        Int32 mcastIntfId = intfId;

        for (int i = 0; i < node->numberInterfaces; i++)
        {
            if (IgmpGetMulticastProtocolInfo(node, i) != NULL)
            {
                mcastIntfId = i;
                break;
            }
        }

        MulticastProtocolType notifyMcastRtProto =
                IgmpGetMulticastProtocolInfo(node, mcastIntfId);
        if (notifyMcastRtProto)
        {
            if (!pimDataSm->srcGrpList)
            {
                ListInit(node, &pimDataSm->srcGrpList);
            }
            RoutingPimSourceGroupList srcGrpPair;
            srcGrpPair.srcAddr = blackMcstSrcAddress;
            srcGrpPair.groupAddr = blackMcstGrpAddress;
            ListInsert(node,
                       pimDataSm->srcGrpList,
                       getSimTime(node),
                       (void*)&srcGrpPair);
            (notifyMcastRtProto)(node,
                                 blackMcstGrpAddress,
                                 intfId,
                                 event);
        }
    }
}
#endif
//---------------------------------------------------------------------------
// FUNCTION     : IgmpRouterHandleReport()
// PURPOSE      : Router processing a received report.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpRouterHandleReport(
    Node* node,
    IgmpMessage* igmpPkt,
    IgmpRouter* igmpRouter,
    int srcAddr,
    Int32 intfId,
    Int32 robustnessVariable
#ifdef CYBER_CORE
    ,
    NodeAddress* mcstSrcAddress
#endif
                           )
{
    IgmpGroupInfoForRouter* grpPtr;
    NodeAddress grpAddr;
    NodeAddress intfAddr;
    IgmpData* igmp=NULL;
    int i;
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;

    intfAddr = NetworkIpGetInterfaceAddress(node, intfId);

    if (DEBUG) {
        printf("    Router processing of IGMP_MEMBERSHIP_REPORT_MSG\n");
    }

    grpAddr = igmpPkt->groupAddress;

    // Search for the specified group.
    grpPtr = IgmpCheckRtrGroupList(
                node, grpAddr, igmpRouter->groupListHeadPointer);

    if (grpPtr)
    {
        // If this group already exist in list
        if (DEBUG) {
            char grpStr[20];

            IO_ConvertIpAddressToString(grpAddr, grpStr);
            printf("    Receive Membership Report for Group %s\n",
                grpStr);
        }

        // Group is already exist. Check membership state for this group,
        // i.e; whether the state is checking membership or member present.
        if (grpPtr->membershipState == IGMP_ROUTER_CHECKING_MEMBERSHIP
            || grpPtr->membershipState == IGMP_ROUTER_MEMBERS)
        {
            grpPtr->membershipState = IGMP_ROUTER_MEMBERS;
            /*grpPtr->groupMembershipInterval = IGMP_GROUP_MEMBERSHIP_INTERVAL;*/
            //The default value for Query Response interval is 100 which on
            //converting into units comes out to be 10 seconds. In QualNet at
            //initialization it is stored as 100 or converting the value configured
            //by user (in seconds) to the normal value i.e. dividing by seconds and
            //multiplying by 10.Hence,it is required to divide the current value in
            //queryResponseInterval by 10 and multiply by SECOND.
            grpPtr->groupMembershipInterval = (clocktype)
                ((robustnessVariable * igmpRouter->queryInterval)
                + (igmpRouter->queryResponseInterval * SECOND)
                /IGMP_QUERY_RESPONSE_SCALE_FACTOR);

            // Note that we've receive a membership report
            grpPtr->lastReportReceive = getSimTime(node);

            // Set timer for the membership
            IgmpSetGroupMembershipTimer(node, grpPtr, intfId);
        }
        else
        {
            char errStr[MAX_STRING_LENGTH];
            char grpStr[MAX_ADDRESS_STRING_LENGTH];

            IO_ConvertIpAddressToString(grpAddr, grpStr);
            sprintf(errStr, "Unknown router state at node %u for group %s\n",
                node->nodeId, grpStr);
            ERROR_Assert(FALSE, errStr);
        }
    }
    else
    {
        MulticastProtocolType notifyMcastRtProto =
            IgmpGetMulticastProtocolInfo(node, intfId);

        // No entry found for this group, so add it to list.
        grpPtr = IgmpInsertRtrGroupList(node,
                                        grpAddr,
                                        intfId,
                                        robustnessVariable);
        if (DEBUG) {
            char grpStr[20];

            IO_ConvertIpAddressToString(grpAddr, grpStr);
            printf("    Receive Membership Report for Group %s\n",
                grpStr);
        }

        // Set timer for the membership
        IgmpSetGroupMembershipTimer(node, grpPtr, intfId);

        igmp = IgmpGetDataPtr(node);

        if (igmp->igmpInterfaceInfo[intfId]->igmpNodeType == IGMP_PROXY)
        {
            if (IgmpCheckSubscriptionInAllOtherDownstreamInterface(
                node, igmp, grpAddr, intfId))
            {
                return;
            }
            else
            {
                Int32 upstreamIntfId = 0;

                upstreamIntfId =
                    (Int32)NetworkIpGetInterfaceIndexFromAddress(
                                     node, igmp->igmpUpstreamInterfaceAddr);

                //notify upstream. just call igmpjoingroup with timetojoin=0
                IgmpJoinGroup(node, upstreamIntfId, grpAddr, (clocktype)0);
            }
        }
        else
        {
#ifdef CYBER_CORE
            BOOL pimSmEnabled = FALSE;
            pimSmEnabled = IgmpPimSmIsEnabled(node, intfId);
            if (pimSmEnabled &&
                ip->iahepData &&
                ip->iahepData->nodeType == BLACK_NODE &&
                mcstSrcAddress != NULL)
            {
                IgmpStartPimSmSGJoinLeaveInBlackNetwork(
                        node, intfId, *mcstSrcAddress, grpAddr,
                        LOCAL_MEMBER_JOIN_GROUP);
            }
            else
#endif
            // Notify multicast routing protocol about the new membership.
            if (notifyMcastRtProto)
            {
                (notifyMcastRtProto)(node,
                                     grpAddr,
                                     intfId,
                                     LOCAL_MEMBER_JOIN_GROUP);
            }
        }
    }

#ifdef ADDON_DB
    HandleStatsDBIgmpSummary(node, "Join", srcAddr, grpAddr);
#endif

#ifdef CYBER_CORE

    if (ip->iahepEnabled)
    {
        if (ip->iahepData->nodeType == IAHEP_NODE)
        {
            NodeAddress mappedGrpAddr = IAHEPGetMulticastBroadcastAddressMapping(
                                            node, ip->iahepData, grpAddr);

            NodeAddress blackSrcAddress;
            NodeAddress *mappedSrcAddr = NULL;

            if (mcstSrcAddress)
            {
                IAHEPAmdMappingType* amdEntry =
                        IAHEPGetAMDInfoForDestinationRedAddress(node, *mcstSrcAddress);

                if (amdEntry)
                {
                    blackSrcAddress = amdEntry->destBlackInterface;
                    mappedSrcAddr = &blackSrcAddress;
                }
            }
            IAHEPCreateIgmpJoinLeaveMessage(
                node,
                mappedGrpAddr,
                IGMP_MEMBERSHIP_REPORT_MSG,
                NULL,
                mappedSrcAddr);

            ip->iahepData->grpAddressList->insert(grpAddr);

            ip->iahepData->updateAmdTable->getAMDTable(0)->
                    iahepStats.iahepIGMPReportsSent ++;

            if (IAHEP_DEBUG)
            {
                char addrStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(mappedGrpAddr, addrStr);
                printf("\nIAHEP Node: %d", node->nodeId);
                printf("\tJoins Group: %s\n", addrStr);
            }
        }
    }
#endif
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpRouterHandleLeave()
// PURPOSE      : Router processing a leave group message.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpRouterHandleLeave(
    Node* node,
    IgmpMessage* igmpPkt,
    IgmpRouter* igmpRouter,
    int srcAddr,
    Int32 intfId)
{
    IgmpGroupInfoForRouter* grpPtr;
    NodeAddress grpAddr;
    NodeAddress intfAddr;

    intfAddr = NetworkIpGetInterfaceAddress(node, intfId);
    grpAddr = igmpPkt->groupAddress;

    if (DEBUG) {
        printf("    Router processing of IGMP_LEAVE_MSG\n");
    }

    if (igmpRouter->routersState == IGMP_QUERIER)
    {
        // Search for the specified group.
        grpPtr = IgmpCheckRtrGroupList(
                        node, grpAddr, igmpRouter->groupListHeadPointer);

        if (grpPtr)
        {
            clocktype delayTime;
            IgmpStateType originalState = grpPtr->membershipState;

            grpPtr->membershipState = IGMP_ROUTER_CHECKING_MEMBERSHIP;
            grpPtr->groupMembershipInterval =
                grpPtr->lastMemberQueryCount *
                (grpPtr->lastMemberQueryInterval / 10)*SECOND;
            delayTime = 0;

            if (DEBUG) {
                char grpStr[20];
                char intfStr[20];

                IO_ConvertIpAddressToString(grpAddr, grpStr);
                IO_ConvertIpAddressToString(intfAddr, intfStr);
                printf("    Receive leave report for group %s on "
                    "interface %s\n",
                    grpStr, intfStr);
            }

            // Send Group Specific Query message
            IgmpSendQuery(node,
                          grpAddr,
                          intfId,
                          igmpRouter->queryResponseInterval,
                          igmpRouter->lastMemberQueryInterval);

            // Set timer for the membership
            //IgmpSetGroupMembershipTimer(node, grpPtr, intfId);

            if (originalState != IGMP_ROUTER_CHECKING_MEMBERSHIP)
            {
                // Set last membership timer
                IgmpSetLastMembershipTimer(node,
                                           grpAddr,
                                           intfId,
                                           0,
                                           igmpRouter->lastMemberQueryInterval);
            }
//#ifdef ADDON_DB
        //HandleMulticastDBStatus(node, intfId, "Leave", srcAddr, grpAddr);
//#endif
        }
        else
        {
            IgmpData* igmp = IgmpGetDataPtr(node);

            igmp->igmpInterfaceInfo[intfId]->
                igmpStat.igmpBadReportReceived += 1;
            igmp->igmpInterfaceInfo[intfId]->
                igmpStat.igmpBadMsgReceived += 1;
        }
    }
    else
    {
        if (DEBUG) {
            printf("    Ignore packet as it is Leave Report\n");
        }
    }

#ifdef ADDON_DB
    HandleStatsDBIgmpSummary(node, "Leave", srcAddr, grpAddr);
#endif


#ifdef CYBER_CORE
    NetworkDataIp* ip = (NetworkDataIp *) node->networkData.networkVar;

    if (ip->iahepEnabled && ip->iahepData->nodeType == IAHEP_NODE)
    {
        NodeAddress mappedGrpAddr = IAHEPGetMulticastBroadcastAddressMapping(
                                        node, ip->iahepData, grpAddr);

        IAHEPCreateIgmpJoinLeaveMessage(
            node,
            mappedGrpAddr,
            IGMP_LEAVE_GROUP_MSG,
            NULL);

        ip->iahepData->grpAddressList->erase(grpAddr);

        //Update Stats
        ip->iahepData->updateAmdTable->getAMDTable(0)->
            iahepStats.iahepIGMPLeavesSent ++;
        ip->iahepData->updateAmdTable->getAMDTable(0)->
            iahepStats.iahepIGMPLevingGrp = mappedGrpAddr;

        if (IAHEP_DEBUG)
        {
            char addrStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(mappedGrpAddr, addrStr);
            printf("\nIAHEP Node: %d", node->nodeId);
            printf("\tLeaves Group: %s\n", addrStr);
        }
    }
#endif //CYBER_CORE
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpHandlePacketForRouter()
// PURPOSE      : Router processing a incoming IGMP packet.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpHandlePacketForRouter(
    Node* node,
    Message* msg,
    NodeAddress srcAddr,
    NodeAddress grpAddr,
    Int32 intfId)
{
    IgmpData* igmp;
    IgmpMessage* igmpPkt;
    IgmpRouter* igmpRouter;
    IgmpInterfaceInfoType *thisInterface;

    igmp = IgmpGetDataPtr(node);

    thisInterface = igmp->igmpInterfaceInfo[intfId];
    igmpRouter = thisInterface->igmpRouter;
    igmpPkt = (IgmpMessage*) MESSAGE_ReturnPacket(msg);

    switch (igmpPkt->ver_type)
    {
        case IGMP_QUERY_MSG:
        {
            // Router receives a query message from another router
            thisInterface->igmpStat.igmpTotalQueryReceived += 1;

            if (igmpPkt->groupAddress == 0) {
                thisInterface->igmpStat.igmpGenQueryReceived += 1;
            } else {
                thisInterface-> igmpStat.igmpGrpSpecificQueryReceived += 1;
            }

            IgmpRouterHandleQuery(node,
                                  igmpPkt,
                                  igmpRouter,
                                  srcAddr, intfId,
                                  thisInterface->robustnessVariable);

// RtrJoinStart
            IgmpHostHandleQuery(node, igmpPkt, thisInterface, intfId);
// RtrJoinEnd

            break;
        }
        case IGMP_MEMBERSHIP_REPORT_MSG:
        {
            // Router receives a Membership report
            thisInterface->igmpStat.igmpTotalReportReceived += 1;
            thisInterface->igmpStat.igmpMembershipReportReceived += 1;
#ifdef CYBER_CORE
            NodeAddress mcstSrcAddress = ANY_DEST;
            if (MESSAGE_ReturnInfo(msg, INFO_TYPE_SourceAddr))
            {
                memcpy(&mcstSrcAddress,
                       MESSAGE_ReturnInfo(msg, INFO_TYPE_SourceAddr),
                                          sizeof(NodeAddress));
                IgmpRouterHandleReport(node,
                                    igmpPkt,
                                    igmpRouter,
                                    srcAddr,
                                    intfId,
                                    thisInterface->robustnessVariable,
                                    &mcstSrcAddress);
            }
            else
#endif
            IgmpRouterHandleReport(node,
                                   igmpPkt,
                                   igmpRouter,
                                   srcAddr,
                                   intfId,
                                   thisInterface->robustnessVariable);

// RtrJoinStart
            IgmpHostHandleReport(node, igmpPkt, thisInterface);
// RtrJoinEnd

            break;
        }
        case IGMP_LEAVE_GROUP_MSG:
        {
            // Router receives a Leave report
            thisInterface->igmpStat.igmpTotalReportReceived += 1;
            thisInterface->igmpStat.igmpLeaveReportReceived += 1;

            IgmpRouterHandleLeave(
                node, igmpPkt, igmpRouter, srcAddr, intfId);
            break;
        }
        default:
        {
            thisInterface->igmpStat.igmpBadMsgReceived += 1;
            break;
        }
     }//end switch//
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpPrintStat()
// PURPOSE      : Print the statistics of IGMP protocol.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpPrintStat(Node* node)
{
    int i;
    char buf[MAX_STRING_LENGTH];
    char nodeType[MAX_STRING_LENGTH];
    IgmpData* igmp;
    IgmpInterfaceInfoType* thisInterface;

    igmp = IgmpGetDataPtr(node);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        NodeAddress interfaceAddress = NetworkIpGetInterfaceAddress(node, i);

        if (!igmp || !igmp->igmpInterfaceInfo[i])
        {
            continue;
        }

        thisInterface = igmp->igmpInterfaceInfo[i];

        if (thisInterface->igmpNodeType == IGMP_HOST) {
            sprintf(nodeType, "IGMP-HOST");
        }
        else if (thisInterface->igmpNodeType == IGMP_PROXY) {
            sprintf(nodeType, "IGMP-PROXY");
        }
        else {
            sprintf(nodeType, "IGMP-ROUTER");
        }

        sprintf(buf, "Membership created in local network = %d",
                      thisInterface->igmpStat.igmpTotalMembership);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Total Group Joins = %d",
                      thisInterface->igmpStat.igmpTotalGroupJoin);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Total Group Leaves = %d",
                      thisInterface->igmpStat.igmpTotalGroupLeave);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);
        sprintf(buf, "Total Messages Sent = %d",
                      thisInterface->igmpStat.igmpTotalMsgSend);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Total Messages Received = %d",
                      thisInterface->igmpStat.igmpTotalMsgReceived);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Bad Messages Received = %d",
                      thisInterface->igmpStat.igmpBadMsgReceived);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Total Reports Received = %d",
                      thisInterface->igmpStat.igmpTotalReportReceived);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Bad Reports Received = %d",
                      thisInterface->igmpStat.igmpBadReportReceived);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Membership Reports Received = %d",
                      thisInterface->igmpStat.igmpMembershipReportReceived);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Leave Reports Received = %d",
                      thisInterface->igmpStat.igmpLeaveReportReceived);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Total Queries Received = %d",
                      thisInterface->igmpStat.igmpTotalQueryReceived);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Bad Queries Received = %d",
                      thisInterface->igmpStat.igmpBadQueryReceived);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "General Queries Received = %d",
                      thisInterface->igmpStat.igmpGenQueryReceived);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Group Specific Queries Received = %d",
                      thisInterface->igmpStat.igmpGrpSpecificQueryReceived);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Total Reports Sent = %d",
                      thisInterface->igmpStat.igmpTotalReportSend);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Membership Reports Sent = %d",
                      thisInterface->igmpStat.igmpMembershipReportSend);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Leave Reports Sent = %d",
                      thisInterface->igmpStat.igmpLeaveReportSend);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Total Queries Sent = %d",
                      thisInterface->igmpStat.igmpTotalQuerySend);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "General Queries Sent = %d",
                      thisInterface->igmpStat.igmpGenQuerySend);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);

        sprintf(buf, "Group Specific Queries Sent = %d",
                      thisInterface->igmpStat.igmpGrpSpecificQuerySend);
        IO_PrintStat(
            node,
            "Network",
            nodeType,
            interfaceAddress,
            -1 /* instanceId */,
            buf);
    }
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpInit()
// PURPOSE      : To initialize IGMP protocol.
// RETURN VALUE : None
// ASSUMPTION   : All routers and hosts support IGMPv2.
//---------------------------------------------------------------------------
void IgmpInit(
    Node* node,
    const NodeInput* nodeInput,
    IgmpData** igmpLayerPtr,
    int interfaceIndex)
{
    Int32 robustnessVariable;
    char error[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    clocktype queryResponseInterval;
    clocktype lastMemberQueryInterval;
    BOOL retVal = FALSE;
    Int32 matchType;
    NetworkDataIp* ipLayer = NULL;
    IgmpData* igmp = NULL;
    IgmpInterfaceInfoType* thisInterface = NULL;
    clocktype queryInterval;
    IgmpInterfaceType igmpInterfaceType = IGMP_DEFAULT_INTERFACE;
    NodeAddress igmpUpstreamInterfaceAddr = 0;
    BOOL isNodeId = FALSE;
    int numHostBits = 0;
    clocktype  interval;
    int intVal = 0;

#ifdef ADDON_BOEINGFCS
    if (node->macData[interfaceIndex]->macProtocol ==
        MAC_PROTOCOL_LINK_EPLRS)
    {
        return;
    }
#endif

#ifdef ADDON_BOEINGFCS
    if (node->macData[interfaceIndex]->macProtocol ==
        MAC_PROTOCOL_LINK_EPLRS)
    {
        return;
    }
#endif

    ipLayer = (NetworkDataIp*) node->networkData.networkVar;

    // Allocate internal structure if initializes for the first time
    //if (ipLayer->isIgmpEnable == FALSE)
    if (*igmpLayerPtr == NULL)
    {
        // Set IGMP flag
        ipLayer->isIgmpEnable = TRUE;

        igmp = (IgmpData*) MEM_malloc(sizeof(IgmpData));

        memset(igmp, 0, sizeof(IgmpData));
        *igmpLayerPtr = igmp;

        // Check input file for IGMP-STATISTICS flag
        IO_ReadString(
            node->nodeId, (unsigned)ANY_INTERFACE, nodeInput, "IGMP-STATISTICS",
            &retVal, buf);

        if (!retVal || !strcmp (buf, "NO"))
        {
            igmp->showIgmpStat = FALSE;
        }
        else if (!strcmp (buf, "YES"))
        {
            igmp->showIgmpStat = TRUE;
        }
        else
        {
            ERROR_ReportError(
                "IGMP-STATISTICS: Unknown value in configuration file.\n");
        }
    }
    else
    {
       // ERROR_Assert(*igmpLayerPtr, "Initialization error within IGMP\n");
        igmp = *igmpLayerPtr;
    }

    // All multicast nodes join the all-systems-group
    NetworkIpAddToMulticastGroupList(node, IGMP_ALL_SYSTEMS_GROUP_ADDR);

    // Allocate interface structure for this interface
    thisInterface = (IgmpInterfaceInfoType*)
        MEM_malloc(sizeof(IgmpInterfaceInfoType));

    igmp->igmpInterfaceInfo[interfaceIndex] = thisInterface;

    memset(thisInterface, 0, sizeof(IgmpInterfaceInfoType));

    // check if this is an ad hoc wireless network
    igmp->igmpInterfaceInfo[interfaceIndex]->isAdHocMode = FALSE;
    if (MAC_IsWirelessAdHocNetwork(node, interfaceIndex))
    {
        igmp->igmpInterfaceInfo[interfaceIndex]->isAdHocMode = TRUE;
    }

    // Check input file for IGMP-PROXY flag
    IO_ReadString(node->nodeId,
                  (unsigned)ANY_INTERFACE,
                  nodeInput,
                  "IGMP-PROXY",
                  &retVal,
                  buf);

    if (retVal && !strcmp(buf,"YES"))
    {
        igmp->igmpInterfaceInfo[interfaceIndex]->igmpNodeType = IGMP_PROXY;
    }
    else
    {
        // Check input file to determine if this node is a router
        retVal = IgmpIsRouter(node, nodeInput,interfaceIndex);

        if (retVal) 
        {
            igmp->igmpInterfaceInfo[interfaceIndex]->igmpNodeType = 
                                                                 IGMP_ROUTER;
        } 
        else 
        {
            igmp->igmpInterfaceInfo[interfaceIndex]->igmpNodeType = 
                                                                   IGMP_HOST;
        }
    }

    if (DEBUG) {
        char buf[20];
        char hostType[20];

        if (igmp->igmpInterfaceInfo[interfaceIndex]->igmpNodeType 
                     == IGMP_ROUTER) 
        {
            sprintf(hostType, "%s", "Router");
        } 
        else 
        {
            sprintf(hostType, "%s", "Host");
        }

        IO_ConvertIpAddressToString(
            NetworkIpGetInterfaceNetworkAddress(node, interfaceIndex),
            buf);
        printf("Node %u initializes as %s for subnet %s\n",
            node->nodeId, hostType, buf);
    }

    if (igmp->igmpInterfaceInfo[interfaceIndex]->igmpNodeType == IGMP_PROXY)
    {
        IO_ReadString(
            node->nodeId, 
            NetworkIpGetInterfaceAddress(node,interfaceIndex),
            nodeInput,
            "IGMP-PROXY-UPSTREAM-INTERFACE",
            &retVal,
            buf);

        if (retVal)
        {
            IO_ParseNodeIdHostOrNetworkAddress(buf,
                &igmpUpstreamInterfaceAddr, &numHostBits, &isNodeId);

            if (isNodeId)
            {
                // Error
                sprintf(buf,"IGMP-PROXY: node %d\n\t"
                    "IGMP-PROXY-UPSTREAM-INTERFACE must be interface"
                    " address", node->nodeId);
                ERROR_ReportError(buf);
            }

            igmp->igmpUpstreamInterfaceAddr = igmpUpstreamInterfaceAddr;
        }
        else
        {
           // Error
            sprintf(buf,"IGMP-PROXY: node %d\n\t"
                "IGMP-PROXY-UPSTREAM-INTERFACE must be specified.",
                node->nodeId);

            ERROR_ReportError(buf);
        }
        if (igmpUpstreamInterfaceAddr ==
            NetworkIpGetInterfaceAddress(node,interfaceIndex))
        {
            igmpInterfaceType = IGMP_UPSTREAM;
        }
        else
        {
            igmpInterfaceType = IGMP_DOWNSTREAM;
        }
        IgmpSetInterfaceType(node, igmp, interfaceIndex, igmpInterfaceType);

        // set a dummy multicast router function
        NetworkIpSetMulticastRouterFunction(
            node, &IgmpProxyRoutingFunction, interfaceIndex);
    }
    else
    {
        IgmpSetInterfaceType(
            node, igmp, interfaceIndex, IGMP_DEFAULT_INTERFACE);
    }
    retVal = FALSE;
    IO_ReadInt(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "IGMP-ROBUSTNESS-VARIABLE",
        &retVal,
        &intVal);


    if (retVal)
    {
        //Robustness variable cannot be equal to zero.
        robustnessVariable = intVal;
        if (robustnessVariable <= 0)
        {
            sprintf(error, "The Robustness Variable should be greater than zero\n");
            ERROR_ReportError(error);
        }
        igmp->igmpInterfaceInfo[interfaceIndex]->robustnessVariable =
                                                    robustnessVariable;
    }
    else
    {
        igmp->igmpInterfaceInfo[interfaceIndex]->robustnessVariable = IGMP_ROBUSTNESS_VAR;
    }


    // If this node is a router.
    if (igmp->igmpInterfaceInfo[interfaceIndex]->igmpNodeType == IGMP_ROUTER
        || igmpInterfaceType == IGMP_DOWNSTREAM)
    {
        retVal = FALSE;
        IO_ReadTime(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "IGMP-QUERY-INTERVAL",
            &retVal,
            &interval);


        if (retVal)
        {
            queryInterval = interval;
            if (queryInterval <= 0)
            {
                sprintf(error, "The Query Interval should be greater than zero\n");
                ERROR_ReportError(error);
            }
            queryInterval = (queryInterval/1000000000) * SECOND;
        }
        else
        {
            queryInterval = (clocktype)IGMP_QUERY_INTERVAL;
        }
        retVal = FALSE;
        IO_ReadTime(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "IGMP-QUERY-RESPONSE-INTERVAL",
            &retVal,
            &interval);


        if (retVal)
        {
            queryResponseInterval = interval;
            queryResponseInterval = (queryResponseInterval/SECOND)*10;
        }
        else
        {
            queryResponseInterval = IGMP_QUERY_RESPONSE_INTERVAL;
        }

        if ((queryResponseInterval/10) >= (queryInterval/SECOND))
        {
            sprintf(error, "The number of seconds represented by the "
            "Query Response Interval must be less than "
            "the Query Interval\n");
            ERROR_ReportError(error);
        }
        retVal = FALSE;
        IO_ReadTime(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "IGMP-LAST-MEMBER-QUERY-INTERVAL",
            &retVal,
            &interval);

        if (retVal)
        {
            lastMemberQueryInterval = interval;
            if (lastMemberQueryInterval <= 0)
            {
                sprintf(error, "The Last Member Query Interval should be "
                    "greater than zero\n");
                ERROR_ReportError(error);
            }
            lastMemberQueryInterval = (lastMemberQueryInterval/SECOND)*10;
            if (lastMemberQueryInterval > 255)
            {
                sprintf(error, "The Last Member Query Interval should be "
                    "less than 25.5 seconds\n");
                ERROR_ReportError(error);
            }
        }
        else
        {
            lastMemberQueryInterval = IGMP_LAST_MEMBER_QUERY_INTERVAL;
        }
        // All multicast routers join the all-routers-group
        NetworkIpAddToMulticastGroupList(node, IGMP_ALL_ROUTERS_GROUP_ADDR);

        // Allocate router structure
        thisInterface->igmpRouter = (IgmpRouter*)
            MEM_malloc(sizeof(IgmpRouter));

        // Initializes Igmp router structure.
        IgmpRouterInit(
            node,
            thisInterface->igmpRouter,
            interfaceIndex,
            nodeInput,
            igmp->igmpInterfaceInfo[interfaceIndex]->robustnessVariable,
            queryInterval,
            (UInt8)queryResponseInterval,
            (UInt8)lastMemberQueryInterval);
    }

    // Every node have host behavior.
    // Allocate host structure.
    thisInterface->igmpHost = (IgmpHost*)
        MEM_malloc(sizeof(IgmpHost));

    // Initializes Igmp host structure
    IgmpHostInit(node, thisInterface->igmpHost, nodeInput, interfaceIndex);

    if (interfaceIndex == node->numberInterfaces-1
        && igmp->igmpInterfaceInfo[interfaceIndex]->igmpNodeType 
                            == IGMP_PROXY)
    {
        if (!IgmpValidateProxyParameters(node, igmp) )
        {
            ERROR_ReportError("IGMP_PROXY: Not properly initialised.");
        }
    }
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpLayer()
// PURPOSE      : Interface function to receive all the self messages
//              : given by the IP layer to IGMP to handle self message
//              : and timer events.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
void IgmpLayer(Node* node, Message* msg)
{
    switch (msg->eventType)
    {
        case MSG_NETWORK_IgmpQueryTimer:
        {
            IgmpHandleQueryTimer(node, msg);
            break;
        }
        case MSG_NETWORK_IgmpOtherQuerierPresentTimer:
        {
            IgmpHandleOtherQuerierPresentTimer(node, msg);
            break;
        }
        case MSG_NETWORK_IgmpGroupReplyTimer:
        {
            IgmpHandleGroupReplyTimer(node, msg);
            break;
        }
        case MSG_NETWORK_IgmpGroupMembershipTimer:
        {
            IgmpHandleGroupMembershipTimer(node, msg);
            break;
        }
        case MSG_NETWORK_IgmpJoinGroupTimer:
        {
            if (DEBUG) {
                printf("Node %u got MSG_NETWORK_IgmpJoinGroupTimer "
                    "expired\n",
                    node->nodeId);
            }

            IgmpHandleJoinGroupTimer(node, msg);
            break;
        }
        case MSG_NETWORK_IgmpLeaveGroupTimer:
        {
            IgmpHandleLeaveGroupTimer(node, msg);
            break;
        }
        case MSG_NETWORK_IgmpLastMemberTimer:
        {
            IgmpHandleLastMemberTimer(node, msg);
            break;
        }

#ifdef ADDON_DB
        case MSG_STATS_IGMP_InsertSummary:
        {
            StatsDb* db = node->partitionData->statsDb;

            if (db == NULL ||
                !db->statsIgmpTable->createIgmpSummaryTable)
            {
                break;
            }

            HandleStatsDBIgmpSummaryTableInsertion(node);

            MESSAGE_Send(node,
                         MESSAGE_Duplicate(node, msg),
                         db->statsIgmpTable->igmpSummaryInterval);

            break;
        }
#endif

        default:
        {
            char errStr[MAX_STRING_LENGTH];

            sprintf(errStr, "Unknown Event-Type for node %u", node->nodeId);
            ERROR_Assert(FALSE, errStr);

            break;
        }
    }

    MESSAGE_Free(node, msg);
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpHandleProtocolPacket()
// PURPOSE      : Function to process IGMP packets received from IP.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
void IgmpHandleProtocolPacket(
    Node* node,
    Message* msg,
    NodeAddress srcAddr,
    NodeAddress dstAddr,
    Int32 intfId)
{
    IgmpData* igmp;
    IgmpMessage* igmpMessage = (IgmpMessage*) MESSAGE_ReturnPacket(msg);

#ifdef ENTERPRISE_LIB
    // Check for DVMRP message
    if (igmpMessage->ver_type == IGMP_DVMRP)
    {
       RoutingDvmrpHandleProtocolPacket(node, msg, srcAddr, dstAddr, intfId);
       return;
    }
#endif // ENTERPRISE_LIB
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;


    if (ip->isIgmpEnable == FALSE)
    {
        return;
    }

    igmp = IgmpGetDataPtr(node);

    // Sanity check
    if (!igmp || !igmp->igmpInterfaceInfo[intfId])
    {
        MESSAGE_Free(node, msg);
        return;
    }

    // Check whether packet come from attached subnet
    if (!NetworkIpIsPointToPointNetwork(node, intfId)
        && !IgmpIsSrcInAttachedSubnet(node, srcAddr))
    {
        MESSAGE_Free(node, msg);
        return;
    }

    if (IgmpIsMyPacket(node, dstAddr, intfId))
    {
        if (DEBUG) {
            char srcStr[20];
            char intfStr[20];
            char timeStr[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(srcAddr, srcStr);
            IO_ConvertIpAddressToString(
                NetworkIpGetInterfaceAddress(node, intfId),
                intfStr);
            TIME_PrintClockInSecond(getSimTime(node), timeStr);
            printf("Node %u: Receive packet from %s on "
                "interface %s at %ss\n",
                node->nodeId, srcStr, intfStr, timeStr);
        }

        igmp->igmpInterfaceInfo[intfId]->igmpStat.igmpTotalMsgReceived += 1;

        if (igmp->igmpInterfaceInfo[intfId]->igmpNodeType == IGMP_ROUTER)
        {
            IgmpHandlePacketForRouter(
                node, msg, srcAddr, dstAddr, intfId);
        }
        else if (igmp->igmpInterfaceInfo[intfId]->igmpNodeType  == IGMP_HOST)
        {
            IgmpHandlePacketForHost(
                node, msg, srcAddr, dstAddr, intfId);
        }
        else if (igmp->igmpInterfaceInfo[intfId]->igmpNodeType == IGMP_PROXY)
        {
            IgmpHandlePacketForProxy(
                node, msg, srcAddr, dstAddr, intfId);
        }
                            else
        {
            char errStr[MAX_STRING_LENGTH];

            sprintf(errStr, "Node %u is not initialized or it's not IGMP "
                "enabled", node->nodeId);
            ERROR_Assert(FALSE, errStr);
        }
    }

    MESSAGE_Free(node, msg);
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpFinalize()
// PURPOSE      : Print the statistics of IGMP protocol.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
void IgmpFinalize(Node* node)
{
    IgmpData* igmp;
    int i;

    igmp = IgmpGetDataPtr(node);

    // Sanity checking
    if (!igmp)
    {
        return;
    }

    if (igmp->showIgmpStat)
    {
        IgmpPrintStat(node);
    }

    if (DEBUG)
    {
        printf("----------------------------------------------------------\n"
            "Node %u: Printing related group information\n"
            "------------------------------------------------------------",
            node->nodeId);

        for (i = 0; i < node->numberInterfaces; i++)
        {
            IgmpInterfaceInfoType* thisInterface;
            IgmpGroupInfoForHost* tempPtr;
            int j = 1;
            char buf[20];

            thisInterface = igmp->igmpInterfaceInfo[i];

            if (!thisInterface) {
                continue;
            }

            tempPtr = thisInterface->igmpHost->groupListHeadPointer;

            if (tempPtr)
            {
                IO_ConvertIpAddressToString(
                    NetworkIpGetInterfaceAddress(node, i),
                    buf);
                printf("--------------------------------------------------\n"
                    "Membership information for interface %s :\n"
                    "--------------------------------------------------\n",
                    buf);
            }

            while (tempPtr != NULL)
            {
                IO_ConvertIpAddressToString(tempPtr->groupId, buf);
                printf("%d.\t%15s\n", j, buf);
                tempPtr = tempPtr->next;
                j++;
            }

            if (thisInterface->igmpRouter)
            {

                int j = 1;
                IgmpGroupInfoForRouter* tempPtr;
                tempPtr = thisInterface->igmpRouter->groupListHeadPointer;

                printf("\nNode %u is router\n", node->nodeId);

                if (tempPtr)
                {
                    IO_ConvertIpAddressToString(
                        NetworkIpGetInterfaceAddress(node, i),
                        buf);
                    printf("\nGroup responsibility as a router on "
                        "interface %s\n",
                        buf);
                }
                while (tempPtr != NULL)
                {
                    IO_ConvertIpAddressToString(tempPtr->groupId, buf);
                    printf("%d.\t%15s\n", j, buf);
                    tempPtr = tempPtr->next;
                    j++;
                }

            }
            else
            {
                printf("\nNode %u is host\n", node->nodeId);
            }
        }
        printf("\n");
    }
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpSetInterfaceType()
// PURPOSE      : Sets igmpInterfaceType
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static void IgmpSetInterfaceType(Node* node,
                                 IgmpData* igmpData,
                                 Int32 interfaceIndex,
                                 IgmpInterfaceType igmpInterfaceType)
{
    //set the igmpInterfaceType of the corresponding interface
    igmpData->igmpInterfaceInfo[interfaceIndex]->igmpInterfaceType =
        igmpInterfaceType;
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpGetInterfaceType()
// PURPOSE      : Checks igmpInterfaceType
// RETURN VALUE : IGMP_UPSTREAM/IGMP_DOWNSTREAM/IGMP_DEFAULT_INTERFACE
// ASSUMPTION   : None
//---------------------------------------------------------------------------

static
IgmpInterfaceType IgmpGetInterfaceType(
                                 Node* node,
                                 IgmpData* igmpData,
                                 Int32 interfaceIndex)
{
    IgmpInterfaceType igmpReqInterfaceType;

    //read the igmpInterfaceType of the corresponding interface
    igmpReqInterfaceType =
        igmpData->igmpInterfaceInfo[interfaceIndex]->igmpInterfaceType;

    if (igmpReqInterfaceType == IGMP_UPSTREAM
        || igmpReqInterfaceType == IGMP_DOWNSTREAM)
    {
        return igmpReqInterfaceType;
    }

    return IGMP_DEFAULT_INTERFACE;
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpValidateProxyParameters ()
// PURPOSE      : Checks whether IGMP_PROXY device has been
//                initialised correctly by checking the
//                igmpInterfaceTypes and making sure that only one
//                upstream interface is present
// RETURN VALUE : TRUE if properly initialised. FALSE otherwise.
// ASSUMPTION   : None
//---------------------------------------------------------------------------


static
BOOL IgmpValidateProxyParameters(Node* node,
                                 IgmpData* igmpData)
{
    int upstreamCount = 0;
    Int32 i =0;
    IgmpInterfaceType igmpInterfaceType;

    for (i = 0; i < (Int32)node->numberInterfaces; i++)
    {
        igmpInterfaceType = IgmpGetInterfaceType(node, igmpData, i);

        if (igmpInterfaceType == IGMP_UPSTREAM)
        {
            upstreamCount++;
        }
        else if (igmpInterfaceType == IGMP_DEFAULT_INTERFACE)
        {
            //set corresponding interfaceType to IGMP_DOWNSTREAM
            IgmpSetInterfaceType(node, igmpData, i, IGMP_DOWNSTREAM);
        }
    }
    if (upstreamCount != 1)
    {
        return FALSE;
    }
    return TRUE;
}

//---------------------------------------------------------------------------
//FUNCTION    : IgmpCheckSubscriptionInAllOtherDownstreamInterface ()
//PURPOSE     : Checks whether IGMP_PROXY device has been initialised
//              correctly by checking the igmpInterfaceTypes and
//              making sure that only one upstream interface is
//              present
//RETURN VALUE: TRUE if grouId is present in any other downstream
//              interface. FALSE otherwise.
//ASSUMPTION  : None
//---------------------------------------------------------------------------

static BOOL IgmpCheckSubscriptionInAllOtherDownstreamInterface(
                                   Node* node,
                                   IgmpData* igmpData,
                                   NodeAddress groupId,
                                   Int32 interfaceIndex)
{
    Int32 i = 0;
    IgmpInterfaceType igmpInterfaceType;
    IgmpRouter* igmpRouter;

    for (i = 0; i < (Int32)node->numberInterfaces; i++)
    {
        if (i == interfaceIndex) //all interfaces except interfaceIndex
        {
            continue;
        }

        igmpInterfaceType = IgmpGetInterfaceType(node, igmpData, i);

        if (igmpInterfaceType == IGMP_UPSTREAM)
        {
            continue;
        }

        igmpRouter = igmpData->igmpInterfaceInfo[i]->igmpRouter;

        if (IgmpCheckRtrGroupList(
            node, groupId, igmpRouter->groupListHeadPointer) )
        {
            return TRUE;
        }
    }
    return FALSE;
}


//---------------------------------------------------------------------------
// FUNCTION     : IgmpHandlePacketForProxy()
// PURPOSE      : Proxy processing a incoming IGMP packet.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
static
void IgmpHandlePacketForProxy(
    Node* node,
    Message* msg,
    NodeAddress srcAddr,
    NodeAddress grpAddr,
    Int32 intfId)
{
    IgmpData* igmp = NULL;

    igmp = IgmpGetDataPtr(node);

    if (intfId ==
        (Int32)NetworkIpGetInterfaceIndexFromAddress(
                    node, igmp->igmpUpstreamInterfaceAddr))
    {
        IgmpHandlePacketForHost(node, msg, srcAddr, grpAddr, intfId);
    }
    else
    {
        IgmpHandlePacketForRouter(node, msg, srcAddr, grpAddr, intfId);
    }
}


// --------------------------------------------------------------------------
// FUNCTION     :IgmpGetSubscription()
// PURPOSE      :Checks whether a particular interface is subscribed to a
//               multicast group or not
// ASSUMPTION   :None
// RETURN VALUE :TRUE if specified interface is subscribed, else FALSE
// --------------------------------------------------------------------------


static
BOOL IgmpGetSubscription(Node* node,
                         Int32 interfaceIndex,
                         NodeAddress grpAddr)
{
    IgmpData* igmp;
    IgmpHost* igmpHost;
    IgmpRouter* igmpRouter;
    IgmpInterfaceType igmpInterfaceType = IGMP_UPSTREAM;

    igmp = IgmpGetDataPtr(node);

    if (igmp->igmpInterfaceInfo[interfaceIndex]->igmpNodeType == IGMP_PROXY)
    {
        igmpInterfaceType =
            IgmpGetInterfaceType(node, igmp, interfaceIndex);
    }

    if (igmp->igmpInterfaceInfo[interfaceIndex]->igmpNodeType == IGMP_HOST 
        || igmpInterfaceType == IGMP_UPSTREAM)
    {
        igmpHost = igmp->igmpInterfaceInfo[interfaceIndex]->igmpHost;

        if (IgmpCheckHostGroupList(
            node, grpAddr, igmpHost->groupListHeadPointer) )
        {
            return TRUE;
        }
    }
    else if (igmp->igmpInterfaceInfo[interfaceIndex]->igmpNodeType
             == IGMP_ROUTER
             || igmpInterfaceType == IGMP_DOWNSTREAM)
    {
        igmpRouter = igmp->igmpInterfaceInfo[interfaceIndex]->igmpRouter;

        if (IgmpCheckRtrGroupList(
            node, grpAddr, igmpRouter->groupListHeadPointer) )
        {
            return TRUE;
        }
    }
    return FALSE;
}



// --------------------------------------------------------------------------
// FUNCTION     :IgmpProxyRoutingFunction()
// PURPOSE      :Handle multicast forwarding of packet
// ASSUMPTION   :None
// RETURN VALUE :NULL
// --------------------------------------------------------------------------

void IgmpProxyRoutingFunction(Node* node,
                              Message* msg,
                              NodeAddress grpAddr,
                              Int32 interfaceIndex,
                              BOOL* packetWasRouted,
                              NodeAddress prevHop)
{
    Int32 i = 0;
    IgmpData* igmp = NULL;

    igmp = IgmpGetDataPtr(node);

    if (igmp->igmpInterfaceInfo[interfaceIndex]->igmpNodeType == IGMP_PROXY)
    {

        for (i = 0; i < (Int32)node->numberInterfaces; i++)
        {
            if (i == interfaceIndex)
            {
#ifdef ADDON_BOEINGFCS
                // for RIP packet self generated by SRW interface
                // due to the IGMP Proxy
                // the RIP packet should still be forwarded to the SRW subent
                if (node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CES_SRW_PORT &&
                    prevHop == ANY_IP)
                {
                    IgmpProxyForwardPacketOnInterface(
                        node,
                        MESSAGE_Duplicate(node, msg),
                        interfaceIndex,
                        i);
                }
                else
                {
                    continue;
                }

#else
                continue;
#endif
            }

            if (NetworkIpGetInterfaceAddress(node,i) ==
                igmp->igmpUpstreamInterfaceAddr )
            {
                //send packet
                IgmpProxyForwardPacketOnInterface(
                    node,
                    MESSAGE_Duplicate(node, msg),
                    interfaceIndex,
                    i);
            }
            else if (IgmpGetSubscription(node, i, grpAddr))
            {
                if (IGMP_QUERIER ==
                    igmp->igmpInterfaceInfo[i]->igmpRouter->routersState)
                {
                    IgmpProxyForwardPacketOnInterface(
                        node,
                        MESSAGE_Duplicate(node, msg),
                        interfaceIndex,
                        i);
                }
            }
        }

        *packetWasRouted = TRUE;
        MESSAGE_Free(node, msg);
    }
    else
    {
        ERROR_ReportWarning("IGMP-PROXY routing function "
            "called for a non-proxy device");
    }
}

// --------------------------------------------------------------------------
// FUNCTION     :IgmpProxyForwardPacketOnInterface()
// PURPOSE      :Forwards packet on specified interface
// ASSUMPTION   :None
// RETURN VALUE :NULL
// --------------------------------------------------------------------------
static
void IgmpProxyForwardPacketOnInterface(Node* node,
                                       Message* msg,
                                       Int32 incomingIntfId,
                                       Int32 outgoingIntfId)
{
    NodeAddress nextHop;

    nextHop = NetworkIpGetInterfaceBroadcastAddress(node, outgoingIntfId);

    NetworkIpSendPacketToMacLayer(node, msg, outgoingIntfId, nextHop);
}



#ifdef ADDON_DB
/**
* To get the first node on each partition that is actually running IGMP.
*
* @param partition Data structure containing partition information
* @return First node on the partition that is running IGMP
*/
Node* StatsDBIgmpGetInitialNodePointer(PartitionData* partition)
{
    Node* traverseNode = partition->firstNode;
    IgmpData* igmp = NULL;


    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        NetworkDataIp* ip = (NetworkDataIp*)
                                        traverseNode->networkData.networkVar;

        if (ip->isIgmpEnable)
        {
            igmp = IgmpGetDataPtr(traverseNode);
        }

        if (igmp == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        // we've found a node that has IGMP on it!
        return traverseNode;
    }

    return NULL;
}
#endif
