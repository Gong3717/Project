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

#ifndef _CBR_APP_H_
#define _CBR_APP_H_

#include "types.h"
#include "dynamic.h"
#include "stats_app.h"

#ifdef ADDON_DB
class SequenceNumber;
#endif // ADDON_DB
/*
 * Header size defined to be consistent accross 32/64 bit platforms
 */
//#define CBR_HEADER_SIZE 32
#define CBR_HEADER_SIZE 40
/*
 * Data item structure used by cbr.
 */
typedef
struct struct_app_cbr_data
{
    short sourcePort;
    char type;
    BOOL isMdpEnabled;
    Int32 seqNo;
    clocktype txTime;
#ifdef ADDON_BOEINGFCS
    Int32 txPartitionId;
#endif // ADDON_BOEINGFCS

#ifdef ADVANCED_WIRELESS_LIB
    Int32 pktSize;
    clocktype interval;
#elif UMTS_LIB
    Int32 pktSize;
    clocktype interval;
#endif // ADVANCED_WIRELESS_LIB || UMTS_LIB
}
CbrData;


/* Structure containing cbr client information. */

typedef
struct struct_app_cbr_client_str
{
    Address localAddr;
    Address remoteAddr;
    D_Clocktype interval;
    clocktype sessionStart;
    clocktype sessionFinish;
    clocktype sessionLastSent;
    clocktype endTime;
    BOOL sessionIsClosed;
    UInt32 itemsToSend;
    UInt32 itemSize;
    short sourcePort;
    Int32 seqNo;
    D_UInt32 tos;
    int  uniqueId;
    Int32 mdpUniqueId;
    BOOL isMdpEnabled;
    std::string* applicationName;
#ifdef ADDON_DB
    Int32 sessionId;
    Int32 receiverId;
#endif // ADDON_DB

#ifdef ADDON_NGCNMS
    Message* lastTimer;
#endif
    STAT_AppStatistics* stats;
}
AppDataCbrClient;

/* Structure containing cbr related information. */

typedef
struct struct_app_cbr_server_str
{
    Address localAddr;
    Address remoteAddr;
    short sourcePort;
    clocktype sessionStart;
    clocktype sessionFinish;
    clocktype sessionLastReceived;
    BOOL sessionIsClosed;
    Int32 seqNo;
    int uniqueId ;

  //    clocktype lastInterArrivalInterval;
  //    clocktype lastPacketReceptionTime;

#ifdef ADDON_DB
    Int32 sessionId;
    Int32 sessionInitiator;
    Int32 hopCount;
    SequenceNumber *seqNumCache;
#endif // ADDON_DB


#ifdef ADDON_BOEINGFCS
    // The number of packets received from a different partition
    UInt32 numPktsRecvdDP;

    // Total end to end delay between nodes on different partitions
    clocktype totalEndToEndDelayDP;

    // Total jitter between nodes on different partitions
    clocktype totalJitterDP;

    // Used for calculating jitter for packets on different partitions
    clocktype lastPacketReceptionTimeDP;
    clocktype lastInterArrivalIntervalDP;

    // Whether or not to perform sequence number check on incoming data packets
    BOOL useSeqNoCheck;
#endif /* ADDON_BOEINGFCS */
       STAT_AppStatistics* stats;
}
AppDataCbrServer;


/*
 * NAME:        AppLayerCbrClient.
 * PURPOSE:     Models the behaviour of CbrClient Client on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  nodePtr - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
AppLayerCbrClient(Node *nodePtr, Message *msg);

/*
 * NAME:        AppCbrClientInit.
 * PURPOSE:     Initialize a CbrClient session.
 * PARAMETERS:  nodePtr - pointer to the node,
 *              serverAddr - address of the server,
 *              itemsToSend - number of items to send,
 *              itemSize - size of each packet,
 *              interval - interval of packet transmission rate.
 *              startTime - time until the session starts.
 *              endTime - time until the session end.
 *              tos - the contents for the type of service field.
 *              isRsvpTeEnabled - whether RSVP-TE is enabled
 *              appName - application alias name
 *              isMdpEnabled - true if MDP is enabled by user.
 *              isProfileNameSet - true if profile name is provided by user.
 *              profileName - mdp profile name.
 *              uniqueId - mdp session unique id.
 *              nodeInput - nodeInput for config file.
 * RETURN:      none.
 */
void
AppCbrClientInit(
    Node *node,
    Address clientAddr,
    Address serverAddr,
    Int32 itemsToSend,
    Int32 itemSize,
    clocktype interval,
    clocktype startTime,
    clocktype endTime,
    unsigned tos,
    BOOL isRsvpTeEnabled,
    char* appName,
    BOOL isMdpEnabled = FALSE,
    BOOL isProfileNameSet = FALSE,
    char* profileName = NULL,
    Int32 uniqueId = -1,
    const NodeInput* nodeInput = NULL);


/*
 * NAME:        AppCbrClientPrintStats.
 * PURPOSE:     Prints statistics of a CbrClient session.
 * PARAMETERS:  nodePtr - pointer to the this node.
 *              clientPtr - pointer to the cbr client data structure.
 * RETURN:      none.
 */
void
AppCbrClientPrintStats(Node *nodePtr, AppDataCbrClient *clientPtr);

/*
 * NAME:        AppCbrClientFinalize.
 * PURPOSE:     Collect statistics of a CbrClient session.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
AppCbrClientFinalize(Node *nodePtr, AppInfo* appInfo);

/*
 * NAME:        AppCbrClientGetCbrClient.
 * PURPOSE:     search for a cbr client data structure.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              sourcePort - source port of the cbr client.
 * RETURN:      the pointer to the cbr client data structure,
 *              NULL if nothing found.
 */
AppDataCbrClient *
AppCbrClientGetCbrClient(Node *nodePtr, short sourcePort);

/*
 * NAME:        AppCbrClientNewCbrClient.
 * PURPOSE:     create a new cbr client data structure, place it
 *              at the beginning of the application list.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              remoteAddr - remote address.
 *              itemsToSend - number of items to send,
 *              itemSize - size of data packets.
 *              interval - interdeparture time of packets.
 *              startTime - time when session is started.
 *              endTime - time when session is ended.
 * RETURN:      the pointer to the created cbr client data structure,
 *              NULL if no data structure allocated.
 */
AppDataCbrClient *
AppCbrClientNewCbrClient(
    Node *nodePtr,
    Address localAddr,
    Address remoteAddr,
    Int32 itemsToSend,
    Int32 itemSize,
    clocktype interval,
    clocktype startTime,
    clocktype endTime,
    TosType tos,
    char* appName);

/*
 * NAME:        AppCbrClientScheduleNextPkt.
 * PURPOSE:     schedule the next packet the client will send.  If next packet
 *              won't arrive until the finish deadline, schedule a close.
 * PARAMETERS:  nodePtr - pointer to the node,
 *              clientPtr - pointer to the cbr client data structure.
 * RETRUN:      none.
 */
void
AppCbrClientScheduleNextPkt(Node *nodePtr, AppDataCbrClient *clientPtr);


/*
 * NAME:        AppLayerCbrServer.
 * PURPOSE:     Models the behaviour of CbrServer Server on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  nodePtr - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
AppLayerCbrServer(Node *nodePtr, Message *msg);

/*
 * NAME:        AppCbrServerInit.
 * PURPOSE:     listen on CbrServer server port.
 * PARAMETERS:  nodePtr - pointer to the node.
 * RETURN:      none.
 */
void
AppCbrServerInit(Node *nodePtr);

/*
 * NAME:        AppCbrServerPrintStats.
 * PURPOSE:     Prints statistics of a CbrServer session.
 * PARAMETERS:  nodePtr - pointer to this node.
 *              serverPtr - pointer to the cbr server data structure.
 * RETURN:      none.
 */
void
AppCbrServerPrintStats(Node *nodePtr, AppDataCbrServer *serverPtr);

/*
 * NAME:        AppCbrServerFinalize.
 * PURPOSE:     Collect statistics of a CbrServer session.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
AppCbrServerFinalize(Node *nodePtr, AppInfo* appInfo);

/*
 * NAME:        AppCbrServerGetCbrServer.
 * PURPOSE:     search for a cbr server data structure.
 * PARAMETERS:  nodePtr - cbr server.
 *              sourcePort - source port of cbr client/server pair.
 *              remoteAddr - cbr client sending the data.
 * RETURN:      the pointer to the cbr server data structure,
 *              NULL if nothing found.
 */
AppDataCbrServer *
AppCbrServerGetCbrServer(
    Node *nodePtr,
    Address remoteAddr,
    short sourcePort);

/*
 * NAME:        AppCbrServerNewCbrServer.
 * PURPOSE:     create a new cbr server data structure, place it
 *              at the beginning of the application list.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              localAddr - local address.
 *              remoteAddr - remote address.
 *              sourcePort - source port of cbr client/server pair.
 * RETRUN:      the pointer to the created cbr server data structure,
 *              NULL if no data structure allocated.
 */
AppDataCbrServer *
AppCbrServerNewCbrServer(
    Node *nodePtr,
    Address localAddr,
    Address remoteAddr,
    short sourcePort);

 /*
 * NAME       :: AppLayerCbrPrintTraceXML
 * PURPOSE    :: Print packet trace information in XML format
 * PARAMETERS :: node     : Node*    : Pointer to node
 *                msg      : Message* : Pointer to packet to print headers
 * RETURN     ::  void   : NULL
*/

void AppLayerCbrPrintTraceXML(Node* node, Message* msg);

/*
 * FUNCTION   :: AppLayerCbrInitTrace
 * PURPOSE    :: Cbr initialization  for tracing
 * PARAMETERS :: node : Node* : Pointer to node
 *               nodeInput    : const NodeInput* : Pointer to NodeInput
 * RETURN ::  void : NULL
*/
void AppLayerCbrInitTrace(Node* node, const NodeInput* nodeInput);

#ifdef ADDON_NGCNMS
void
AppCbrClientReInit(Node* node, Address sourceAddr);

void AppCbrReset(Node* node);

#endif

#endif /* _CBR_APP_H_ */

