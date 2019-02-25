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

#ifndef IGMP_H
#define IGMP_H

 // Each multicast routing protocol should provide a function that would
 // handle the event when an IGMP router receive a Group Membership Report
 // for a new group or an existing group leaves the subnet. IGMP will call
 // this function through a function pointer when a new group membership is
 // created or an existing group leaves. This function pointer will
 // initializes at beginning of simulation within Initialization function of
 // different multicast routing protocol.
 //
 // IgmpSetMulticastProtocolInfo() will set this pointer

typedef enum
{
    LOCAL_MEMBER_JOIN_GROUP,
    LOCAL_MEMBER_LEAVE_GROUP
} LocalGroupMembershipEventType;


typedef
void (*MulticastProtocolType) (
        Node *node,
        NodeAddress groupAddr,
        int interfaceId,
        LocalGroupMembershipEventType event);


struct network_igmp_router_str;

typedef
clocktype (*QueryDelayFuncType) (
    struct network_igmp_router_str* igmpRouter,
    Message* msg);


// TBD: Should be removed after mospf.* removes GROUP_ID with NodeAddress
typedef NodeAddress  GROUP_ID;

// all-systems-group (224.0.0.1)
#define IGMP_ALL_SYSTEMS_GROUP_ADDR   0xE0000001

// all-routers-group (224.0.0.2)
#define IGMP_ALL_ROUTERS_GROUP_ADDR   0xE0000002

// Default value of unsolicited report count
#define IGMP_UNSOLICITED_REPORT_COUNT    2

#define IGMP_QUERY_RESPONSE_SCALE_FACTOR    10


// Various timer values
#define IGMP_ROBUSTNESS_VAR                   2
#define IGMP_QUERY_INTERVAL                   ( 125 * SECOND )
#define IGMP_QUERY_RESPONSE_INTERVAL          100
#define IGMP_LAST_MEMBER_QUERY_INTERVAL       10
#define IGMP_UNSOLICITED_REPORT_INTERVAL      ( 10 * SECOND )
#define IGMP_MAX_RESPONSE_TIME                10.0
#define IGMP_STARTUP_QUERY_INTERVAL           ( 0.25 * IGMP_QUERY_INTERVAL )
#define IGMP_OTHER_QUERIER_PRESENT_INTERVAL   ((IGMP_ROBUSTNESS_VAR \
        *IGMP_QUERY_INTERVAL )+( 0.5 * IGMP_QUERY_RESPONSE_INTERVAL))
#define IGMP_STARTUP_QUERY_COUNT              IGMP_ROBUSTNESS_VAR
#define IGMP_LAST_MEMBER_QUERY_COUNT          IGMP_ROBUSTNESS_VAR
#define IGMP_GROUP_MEMBERSHIP_INTERVAL        ((IGMP_ROBUSTNESS_VAR \
        *IGMP_QUERY_INTERVAL) + IGMP_QUERY_RESPONSE_INTERVAL)


// Igmp message types
#define IGMP_QUERY_MSG                 0x11
#define IGMP_MEMBERSHIP_REPORT_MSG     0x16
#define IGMP_LEAVE_GROUP_MSG           0x17


// DVMRP Packet
#define IGMP_DVMRP                     0x13


// Node type enumerations
typedef enum {

    IGMP_HOST = 0,
    IGMP_ROUTER,
    IGMP_PROXY,

    // is not used, just kept in place of the default type
    IGMP_NORMAL_NODE
} IgmpNodeType;


// State type enumerations for Igmp
typedef enum {
    IGMP_HOST_NON_MEMBER = 0,
    IGMP_HOST_DELAYING_MEMBER,
    IGMP_HOST_IDLE_MEMBER,

    IGMP_QUERIER,
    IGMP_NON_QUERIER,

    IGMP_ROUTER_NO_MEMBERS,
    IGMP_ROUTER_MEMBERS,
    IGMP_ROUTER_CHECKING_MEMBERSHIP,

    // any new state definitions MUST BE added before this
    IGMP_DEFAULT
} IgmpStateType;


// Igmp message type
typedef struct IgmpMessage_str
{
    unsigned char   ver_type;
    unsigned char   maxResponseTime;
    short int       checksum;
    unsigned        groupAddress;
} IgmpMessage;


// Structure for holding group information in host
typedef struct network_igmp_group_info_for_host
{
    NodeAddress                                 groupId;
    IgmpStateType                               hostState;
    clocktype                                   groupDelayTimer;
    clocktype                                   lastQueryReceive;
    clocktype                                   lastReportSendOrReceive;
    BOOL                                        isLastHost;
    RandomSeed                                  seed;
    struct network_igmp_group_info_for_host*    next;
} IgmpGroupInfoForHost;


// Igmp host structure
typedef struct
{
    IgmpGroupInfoForHost*    groupListHeadPointer;
    int                      groupCount;
    char                     unsolicitedReportCount;
} IgmpHost;


// Routers timer
typedef struct
{
    clocktype           startUpQueryInterval;
    clocktype           generalQueryInterval;
    clocktype           otherQuerierPresentInterval;
    clocktype           lastQueryReceiveTime;
} IgmpRouterTimer;


// Structure for holding group information in router
typedef struct network_igmp_group_info_for_router
{
    NodeAddress                                 groupId;
    IgmpStateType                               membershipState;
    short                                       lastMemberQueryCount;
    clocktype                                   groupMembershipInterval;
    clocktype                                   lastMemberQueryInterval;
    clocktype                                   lastReportReceive;

    struct network_igmp_group_info_for_router*  next;
} IgmpGroupInfoForRouter;


// Igmp router structure
typedef struct network_igmp_router_str
{
    IgmpStateType            routersState;
    char                     startUpQueryCount;
    IgmpRouterTimer          timer;
    int                      numOfGroups;
    QueryDelayFuncType       queryDelayTimeFunc;

    IgmpGroupInfoForRouter*  groupListHeadPointer;

    RandomSeed               timerSeed;
    clocktype                queryInterval;
    UInt8                    queryResponseInterval;
    UInt8                    lastMemberQueryInterval;
} IgmpRouter;


// Interface specific statistics structure
typedef struct
{
    int         igmpTotalMsgSend;
    int         igmpTotalMsgReceived;
    int         igmpBadMsgReceived;

    // Query message statistics
    int         igmpTotalQueryReceived;
    int         igmpGrpSpecificQueryReceived;
    int         igmpGenQueryReceived;
    int         igmpBadQueryReceived;
    int         igmpTotalQuerySend;
    int         igmpGrpSpecificQuerySend;
    int         igmpGenQuerySend;

    // Report message statistics
    int         igmpTotalReportReceived;
    int         igmpLeaveReportReceived;
    int         igmpMembershipReportReceived;
    int         igmpBadReportReceived;
    int         igmpTotalReportSend;
    int         igmpLeaveReportSend;
    int         igmpMembershipReportSend;

    // Host behavior
    int         igmpTotalGroupJoin;
    int         igmpTotalGroupLeave;

    // Only Router specific statistics
    int         igmpTotalMembership;
} IgmpStatistics;


//IGMP interface type enumeration
typedef enum {
    IGMP_UPSTREAM = 0,
    IGMP_DOWNSTREAM,

    // igmp interface type for igmp host and igmp router
    IGMP_DEFAULT_INTERFACE
} IgmpInterfaceType;


// Interface data structure.
typedef struct
{
    IgmpHost*               igmpHost;
    IgmpRouter*             igmpRouter;
    MulticastProtocolType   multicastProtocol;
    IgmpStatistics          igmpStat;
    BOOL                    isAdHocMode;     // is ad hoc wireless network
    Int32 robustnessVariable;
    IgmpInterfaceType       igmpInterfaceType;
    IgmpNodeType            igmpNodeType;    // Host or Router or Proxy
} IgmpInterfaceInfoType;


// Main layer structure.
typedef struct
{
    BOOL                    showIgmpStat;
    IgmpInterfaceInfoType*  igmpInterfaceInfo[MAX_NUM_INTERFACES];
    NodeAddress             igmpUpstreamInterfaceAddr;
} IgmpData;


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
    int interfaceIndex);


//---------------------------------------------------------------------------
// FUNCTION     : IgmpLayer()
// PURPOSE      : Interface function to receive all the self messages
//              : given by the IP layer to IGMP to handle self message
//              : and timer events.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
void IgmpLayer(Node* node, Message* msg);


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
    int intfId);


//---------------------------------------------------------------------------
// FUNCTION     : IgmpFinalize()
// PURPOSE      : Print the statistics of IGMP protocol.
// RETURN VALUE : None
// ASSUMPTION   : None
//---------------------------------------------------------------------------
void IgmpFinalize(Node* node);


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
    clocktype timeToJoin);


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
    clocktype timeToLeave);


void IgmpSearchLocalGroupDatabase(
    Node* node,
    NodeAddress grpAddr,
    int interfaceId,
    BOOL* localMember);

void IgmpFillGroupList(
    Node* node,
    LinkedList** grpList,
    int interfaceId);

void IgmpSetMulticastProtocolInfo(
    Node* node,
    int interfaceId,
    MulticastProtocolType mcastProtoPtr);

MulticastProtocolType IgmpGetMulticastProtocolInfo(
    Node* node,
    int interfaceId);

void IgmpCreatePacket(
    Message* msg,
    unsigned char type,
    unsigned char maxResponseTime,
    unsigned groupAddress);
//---------------------------------------------------------------------------
// FUNCTION     : IgmpGetDataPtr()
// PURPOSE      : retrieves the IGMP data ptr from node structure.
// RETURN VALUE : Pointer to IgmpData
// ASSUMPTION   : None
//---------------------------------------------------------------------------
IgmpData* IgmpGetDataPtr(Node* node);

#endif // IGMP_H


