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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "app_util.h"
#include "partition.h"
#include "network_ip.h"
#include "qualnet_error.h"
#include "transport_udp.h"
#include "transport_tcp.h"
#include "multimedia_h323.h"
#include "multimedia_h225_ras.h"

#define H225_RAS_DEBUG 0
#define H225_RAS_DEBUG_DETAIL 0


//-------------------------------------------------------------------------
// NAME:        H225RasPrintGateKeeperInfoList.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasPrintGateKeeperInfoList(Node* node)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    H225RasGatekeeperData* h225RasGatekeeperData =
        (H225RasGatekeeperData *) h225Ras->h225RasCommon;

    LinkedList* infoList = h225RasGatekeeperData->gateKeeperInfoList;
    ListItem* item = infoList->first;

    printf("\nGateKeeper Registration List of Node %d\n", node->nodeId);
    printf("Gatekeeper               Endpoint\n");
    printf("Addr            RasPort  Addr            RasPort AliasAddr\n");
    printf("----------------------------------------------------------\n");

    while (item != NULL)
    {
        char gatekeeperIpAddr[MAX_STRING_LENGTH];
        char terminalIpAddr[MAX_STRING_LENGTH];

        H225RasGateKeeperRegnItem* regList =
            (H225RasGateKeeperRegnItem*) item->data;

        IO_ConvertIpAddressToString(
            regList->gatekeeperCallSignalTransAddr.ipAddr, gatekeeperIpAddr);

        IO_ConvertIpAddressToString(
            regList->terminalCallSignalTransAddr.ipAddr, terminalIpAddr);

        printf("%-16s%d     %-16s%d    %s\n",
            gatekeeperIpAddr,
            regList->gatekeeperRasTransAddr.port,
            terminalIpAddr,
            regList->terminalRasTransAddr.port,
            regList->terminalAliasAddr);

        item = item->next;
    }
}


//-------------------------------------------------------------------------
// NAME:        H225RasPrintTerminalInfo
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasPrintTerminalInfo(Node* node)
{
    char gatekeeperIpAddr[MAX_STRING_LENGTH];
    char terminalIpAddr[MAX_STRING_LENGTH];

    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225TerminalData* h225Terminal = h323->h225Terminal;

    H225RasTerminalData* h225RasTerminalData =
        (H225RasTerminalData *) h225Ras->h225RasCommon;

    IO_ConvertIpAddressToString(
        h225RasTerminalData->gatekeeperRasTransAddr.ipAddr,
        gatekeeperIpAddr);

    IO_ConvertIpAddressToString(
        h225Terminal->terminalCallSignalTransAddr.ipAddr,
        terminalIpAddr);

    printf("\nTerminal Information of node %d\n", node->nodeId);

    printf("Gatekeeper Addr      = %s\n"
           "Gatekeeper RasPort   = %d\n"
           "Terminal Addr        = %s\n"
           "Terminal CallSigPort = %d\n"
           "Terminal RasPort     = %d\n"
           "Terminal AliasAddr   = %s\n",
        gatekeeperIpAddr,
        h225RasTerminalData->gatekeeperRasTransAddr.port,
        terminalIpAddr,
        h225Terminal->terminalCallSignalTransAddr.port,
        h225RasTerminalData->terminalRasTransAddr.port,
        h225Terminal->terminalAliasAddr);
}


//-------------------------------------------------------------------------
// NAME:        H225RasPrintEndpoint
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasPrintEndpoint(Node* node)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    // GateKeeper Registration List maintains only for Gatekeeper.
    switch (h323->endpointType)
    {
        case Gatekeeper:
        {
            H225RasPrintGateKeeperInfoList(node);
            break;
        }

        case Terminal:
        {
            H225RasPrintTerminalInfo(node);
            break;
        }

        default:
            break;
    }
}


///////////////////////////////////////////////////////////////////////////
//////////////////////////// RAS timer messages ///////////////////////////
///////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------------
// NAME:        H225RasSendGRQPeriodicRefreshTimer
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasSendGRQPeriodicRefreshTimer(
    Node* node,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr)
{
    Message* refreshMsg;
    H225SourceDestTransAddrInfo* sdInfo;

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);
        printf("Node %u sending MSG_APP_H225_RAS_GRQ_PeriodicRefreshTimer"
            " at %s\n", node->nodeId, clockStr);
    }

    refreshMsg = MESSAGE_Alloc(
                            node,
                            APP_LAYER,
                            APP_H225_RAS_UNICAST,
                            MSG_APP_H225_RAS_GRQ_PeriodicRefreshTimer);

    MESSAGE_InfoAlloc(node, refreshMsg,
        (sizeof(H225SourceDestTransAddrInfo)));

    sdInfo = (H225SourceDestTransAddrInfo*) MESSAGE_ReturnInfo(refreshMsg);

    H323CopyTransAddr(&sdInfo->sourceTransAddr, &sourceTransAddr);
    H323CopyTransAddr(&sdInfo->destTransAddr, &destTransAddr);

    MESSAGE_Send(node, refreshMsg,
        H225_RAS_GRQ_PERIODIC_REFRESH - GATEKEEPER_REQUEST_TIMER);
}


//-------------------------------------------------------------------------
// NAME:        H225RasCreateGRQTimerMsg.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasCreateGRQTimerMsg(
    Node* node,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr)
{
    Message* gatekeeperRequestMsg;
    H225SourceDestTransAddrInfo* sdInfo;

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);
        printf("Node %u sending MSG_APP_H225_RAS_GatekeeperRequestTimer"
            " at %s\n", node->nodeId, clockStr);
    }

    gatekeeperRequestMsg = MESSAGE_Alloc(
                            node,
                            APP_LAYER,
                            APP_H225_RAS_UNICAST,
                            MSG_APP_H225_RAS_GatekeeperRequestTimer);

    MESSAGE_InfoAlloc(node, gatekeeperRequestMsg,
        (sizeof(H225SourceDestTransAddrInfo)));

    sdInfo = (H225SourceDestTransAddrInfo*)
             MESSAGE_ReturnInfo(gatekeeperRequestMsg);

    H323CopyTransAddr(&sdInfo->sourceTransAddr, &sourceTransAddr);
    H323CopyTransAddr(&sdInfo->destTransAddr, &destTransAddr);

    MESSAGE_Send(node, gatekeeperRequestMsg, GATEKEEPER_REQUEST_TIMER);
}


//-------------------------------------------------------------------------
// NAME:        H225RasCreateRRQTimerMsg.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasCreateRRQTimerMsg(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr)
{
    H225SourceDestTransAddrInfo* sdInfo;
    Message* registrationRequestMsg;

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);
        printf("Node %u sending "
            "MSG_APP_H225_RAS_RegistrationRequestTimer at %s\n",
            node->nodeId, clockStr);
    }

    registrationRequestMsg = MESSAGE_Alloc(
                            node,
                            APP_LAYER,
                            APP_H225_RAS_UNICAST,
                            MSG_APP_H225_RAS_RegistrationRequestTimer);

    MESSAGE_InfoAlloc(node, registrationRequestMsg,
        (sizeof(H225SourceDestTransAddrInfo)));

    sdInfo = (H225SourceDestTransAddrInfo*)
             MESSAGE_ReturnInfo(registrationRequestMsg);

    H323CopyTransAddr(&sdInfo->sourceTransAddr, &sourceTransAddr);
    H323CopyTransAddr(&sdInfo->destTransAddr, &destTransAddr);

    MESSAGE_Send(node, registrationRequestMsg, REGISTRATION_REQUEST_TIMER);
}


///////////////////////////////////////////////////////////////////////////
/////////////////// RAS messages for gatekeeper discovery /////////////////
///////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendGCF.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasCreateAndSendGCF(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225RasStat* h225Stat = h225Ras->h225RasStat;

    H225GatekeeperRequest* h225GRQ =
        (H225GatekeeperRequest*) MESSAGE_ReturnPacket(msg);

    UdpToAppRecv* udpMsg = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

    // Allocating space for the GatekeeperConfirm packet
    unsigned char* h225GCF =
        (unsigned char *) MEM_malloc (SIZE_OF_H225_RAS_GCF);

    unsigned char* temph225GCFptr = h225GCF;

    memset(h225GCF, 0, SIZE_OF_H225_RAS_GCF);

    Q931PduCreate((Q931Message*) h225GCF,
                  1,
                  (unsigned char) Q931MessageGetCallRefValue(
                  ((Q931Message*) h225GRQ)->callRefFlgVal),
                  GatekeeperConfirm);

    // move the pointer after Q931 header
    temph225GCFptr += sizeof(Q931Message);

    // sequence number
    H323PutInt(&temph225GCFptr, h225GRQ->requestSeqNum);

    // H225_ProtocolID
    H323PutString(&temph225GCFptr, H225_ProtocolID, IDENTIFIER_SIZE);

    if (sourceTransAddr.ipAddr == GATEKEEPER_DISCOVERY_MULTICAST_ADDRESS)
    {
        sourceTransAddr.ipAddr = NetworkIpGetInterfaceAddress(
            node, udpMsg->incomingInterfaceIndex);
    }

    // rasAddress
    H323PutTransAddr(&temph225GCFptr, &sourceTransAddr);

    h225Stat->gatekeeperConfirmed++;

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u sending GCF Packet"
               " at time %s\n", node->nodeId, clockStr);
    }

    if (H225_RAS_DEBUG_DETAIL)
    {
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(sourceTransAddr.ipAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(destTransAddr.ipAddr, destIpAddr);

        printf("  SourceAddr = %s\n"
               "  SourcePort = %u\n"
               "  DestAddr   = %s\n"
               "  DestPort   = %u\n\n",
               sourceIpAddr,
               sourceTransAddr.port,
               destIpAddr,
               destTransAddr.port);
    }

    // Sending the GCF packet to the receiver
    APP_UdpSendNewData(
        node,
        APP_H225_RAS_UNICAST,
        sourceTransAddr.ipAddr,
        sourceTransAddr.port,
        destTransAddr.ipAddr,
        (char *) h225GCF,
        SIZE_OF_H225_RAS_GCF,
        0,
        TRACE_H323);

    MEM_free(h225GCF);
}


//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendGRJ.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
// TBD
static
void H225RasCreateAndSendGRJ(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225RasStat* h225Stat = h225Ras->h225RasStat;

    h225Stat->gatekeeperRejected++;
}


//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendGRQReply.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasCreateAndSendGRQReply(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr,
    H225RasGatekeeperData* h225RasGatekeeperData)
{
    // Gatekeepers may respond with the Gatekeeper Confirmation (GCF)
    // message indicating "I can be your Gatekeeper" and containing
    // the Transport Address of the Gatekeeper's RAS Channel.
    //
    // If a Gatekeeper does not want the endpoint to register to it,
    // it shall return Gatekeeper Reject (GRJ).

    if (TRUE)    // TBD
    {

        H225RasProcessEntry* anEntry = (H225RasProcessEntry*)
            MEM_malloc(sizeof(H225RasProcessEntry));

        // store terminal RAS address
        H323CopyTransAddr(&anEntry->terminalRasTransAddr, &destTransAddr);

        // store gatekeeper assigned RAS port
        anEntry->rasAssignedPort = sourceTransAddr.port;

        ListInsert(node,
           h225RasGatekeeperData->rasProcessList,
           getSimTime(node),
           anEntry);

        H225RasCreateAndSendGCF(
            node,
            msg,
            sourceTransAddr,
            destTransAddr);
    }
    else
    {
        H225RasCreateAndSendGRJ(
            node,
            msg,
            sourceTransAddr,
            destTransAddr);
    }
}


//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendGRQ.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasCreateAndSendGRQ(
    Node* node,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr)
{
    unsigned char callRefValue;

    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    H225RasStat* h225Stat = h225Ras->h225RasStat;

    // Allocating space for the GatekeeperRequest packet
    unsigned char* h225GRQ =
        (unsigned char*) MEM_malloc (SIZE_OF_H225_RAS_GRQ);

    unsigned char* tempH225GRQptr = h225GRQ;

    memset(h225GRQ, 0, SIZE_OF_H225_RAS_GRQ);

    callRefValue = (unsigned char) ((h225Ras->requestSeqNum)
                    % (MAX_CALL_REF_VALUE));

    h225Stat->gatekeeperRequested++;

    Q931PduCreate((Q931Message*) h225GRQ,
                  0,
                  callRefValue,
                  GatekeeperRequest);

    // Fill the value in GatekeeperRequest packet
    tempH225GRQptr += sizeof(Q931Message);

    // requestSeqNum
    H323PutInt(&tempH225GRQptr, h225Ras->requestSeqNum);
    h225Ras->requestSeqNum++;

    // H225_ProtocolID
    H323PutString(&tempH225GRQptr, H225_ProtocolID, IDENTIFIER_SIZE);

    // rasAddress
    H323PutTransAddr(&tempH225GRQptr, &sourceTransAddr);

    // endpoint type
    H323PutInt(&tempH225GRQptr, Terminal);

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(sourceTransAddr.ipAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(destTransAddr.ipAddr, destIpAddr);
        ctoa(getSimTime(node), clockStr);

        printf("Node %u sending GRQ packet at %s\n"
               "  SourceAddress      = %s\n"
               "  SourceRasPort      = %u\n"
               "  DestinationAddress = %s\n\n",
               node->nodeId,
               clockStr,
               sourceIpAddr,
               sourceTransAddr.port,
               destIpAddr);
    }

    // Sending the GatekeeperRequest packet to the receiver
    APP_UdpSendNewData(
        node,
        (AppType) destTransAddr.port,
        sourceTransAddr.ipAddr,
        sourceTransAddr.port,
        destTransAddr.ipAddr,
        (char *) h225GRQ,
        SIZE_OF_H225_RAS_GRQ,
        0,
        TRACE_H323);

    MEM_free(h225GRQ);
}


///////////////////////////////////////////////////////////////////////////
////////////////// RAS messages for endpoint registration /////////////////
///////////////////////////////////////////////////////////////////////////


static
unsigned short H225RasSearchProcessList(
    H225RasGatekeeperData* h225RasGatekeeperData,
    H225TransportAddress* rasAddr)
{
    ListItem* item;

    LinkedList* infoList = h225RasGatekeeperData->rasProcessList;
    item = infoList->first;

    while (item != NULL)
    {
        H225RasProcessEntry* anEntry = (H225RasProcessEntry*) item->data;

        if ((anEntry->terminalRasTransAddr.ipAddr == rasAddr->ipAddr) &&
            (anEntry->terminalRasTransAddr.port == rasAddr->port))
        {
            return anEntry->rasAssignedPort;
        }
        item = item->next;
    }
    return 0;
}


//-------------------------------------------------------------------------
// NAME:        H225RasRegisterInGateKeeperInfoList.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
H225RasGateKeeperRegnItem* H225RasRegisterInGateKeeperInfoList(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    char* terminalAlias,
    H225RasGatekeeperData* h225RasGatekeeperData)
{
    H225TransportAddress* callSignalAddr = (H225TransportAddress*)
        (MESSAGE_ReturnPacket(msg) + 26);

    H225TransportAddress* rasAddress = (H225TransportAddress*)
        (MESSAGE_ReturnPacket(msg) + 32);

    H225RasGateKeeperRegnItem* newRegList = (H225RasGateKeeperRegnItem*)
        MEM_malloc(sizeof (H225RasGateKeeperRegnItem));

    memset(newRegList, 0, sizeof (H225RasGateKeeperRegnItem));

    newRegList->gatekeeperRasTransAddr.ipAddr = sourceTransAddr.ipAddr;
    newRegList->gatekeeperRasTransAddr.port =
        H225RasSearchProcessList(h225RasGatekeeperData, rasAddress);

    newRegList->gatekeeperCallSignalTransAddr.ipAddr =
        sourceTransAddr.ipAddr;

    newRegList->gatekeeperCallSignalTransAddr.port =
        node->appData.nextPortNum++;

    H323CopyTransAddr(&newRegList->terminalRasTransAddr, rasAddress);
    H323CopyTransAddr(&newRegList->terminalCallSignalTransAddr,
                      callSignalAddr);

    strcpy(newRegList->terminalAliasAddr, terminalAlias);

    ListInsert(node,
               h225RasGatekeeperData->gateKeeperInfoList,
               getSimTime(node),
               newRegList);

    return newRegList;
}


//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendRCF.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasCreateAndSendRCF(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225RasStat* h225Stat = h225Ras->h225RasStat;

    H225RegistrationRequest* h225RRQ =
        (H225RegistrationRequest*) MESSAGE_ReturnPacket(msg);

    char* terminalAlias = (((char*) h225RRQ) + 42);

    // Allocating space for the GatekeeperConfirm packet
    unsigned char* h225RCF =
        (unsigned char *) MEM_malloc (SIZE_OF_H225_RAS_RCF);

    unsigned char* temph225RCFptr = h225RCF;

    H225RasGateKeeperRegnItem* regList =
        H225RasRegisterInGateKeeperInfoList(
            node,
            msg,
            sourceTransAddr,
            terminalAlias,
            (H225RasGatekeeperData*) h225Ras->h225RasCommon);

    memset(h225RCF, 0, SIZE_OF_H225_RAS_RCF);

    Q931PduCreate((Q931Message*) h225RCF,
                  1,
                  (unsigned char) Q931MessageGetCallRefValue(
                  ((Q931Message*) h225RRQ)->callRefFlgVal),
                  RegistrationConfirm);

    // Fill the value in Registration Confirm packet
    temph225RCFptr += sizeof (Q931Message);

    // sequeunce number
    H323PutInt(&temph225RCFptr, h225RRQ->requestSeqNum);

    // H225_ProtocolID
    H323PutString(&temph225RCFptr, H225_ProtocolID, IDENTIFIER_SIZE);

    // call signal transport address
    H323PutTransAddr(&temph225RCFptr,
                     &regList->gatekeeperCallSignalTransAddr);

    // endpoint type
    H323PutInt(&temph225RCFptr, Terminal);

    // willRespondToIRR
    H323PutInt(&temph225RCFptr, TRUE);

    // preGrantedARQ
    H323PutInt(&temph225RCFptr, FALSE);

    // maintainConnection
    H323PutInt(&temph225RCFptr, TRUE);

    h225Stat->registrationConfirmed++;

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u sending RCF Packet"
               " at time %s\n", node->nodeId, clockStr);
    }

    if (H225_RAS_DEBUG_DETAIL)
    {
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(
            regList->gatekeeperRasTransAddr.ipAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(destTransAddr.ipAddr, destIpAddr);

        printf("  SourceAddr    = %s\n"
               "  SourceRasPort = %u\n"
               "  DestAddr      = %s\n"
               "  DestRasPort   = %u\n\n",
               sourceIpAddr,
               regList->gatekeeperRasTransAddr.port,
               destIpAddr,
               destTransAddr.port);
    }

    // Sending the Registration Confirm packet to the receiver
    APP_UdpSendNewData(
        node,
        APP_H225_RAS_UNICAST,
        regList->gatekeeperRasTransAddr.ipAddr,
        regList->gatekeeperRasTransAddr.port,
        destTransAddr.ipAddr,
        (char *) h225RCF,
        SIZE_OF_H225_RAS_RCF,
        0,
        TRACE_H323);

    MEM_free(h225RCF);

}


//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendRRJ
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
// TBD
static
void H225RasCreateAndSendRRJ(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225RasStat* h225Stat = h225Ras->h225RasStat;

    h225Stat->registrationRejected++;
}


//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendRRQReply.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasCreateAndSendRRQReply(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr)
{
    // The Gatekeeper shall respond with either a Registration
    // Confirmation (RCF) or a Registration Reject (RRJ).
    if (TRUE)    // TBD
    {
        H225RasCreateAndSendRCF(
            node,
            msg,
            sourceTransAddr,
            destTransAddr);
    }
    else
    {
        H225RasCreateAndSendRRJ(
            node,
            msg,
            sourceTransAddr,
            destTransAddr);
    }
}


//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendRRQ.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasCreateAndSendRRQ(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr)
{
    unsigned char callRefValue;

    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225TerminalData* h225Terminal = h323->h225Terminal;

    H225RasTerminalData* h225RasTerminalData =
        (H225RasTerminalData *) h225Ras->h225RasCommon;

    H225RasStat* h225Stat = h225Ras->h225RasStat;

    // Allocating space for packet
    unsigned char* h225RRQ =
        (unsigned char*) MEM_malloc (SIZE_OF_H225_RAS_RRQ);

    unsigned char* tempH225RRQptr = h225RRQ;

    memset(h225RRQ, 0, SIZE_OF_H225_RAS_RRQ);

    callRefValue = (unsigned char) ((h225Ras->requestSeqNum)
        % (MAX_CALL_REF_VALUE));

    h225Stat->registrationRequested++;

    Q931PduCreate((Q931Message*) h225RRQ,
                  0,
                  callRefValue,
                  RegistrationRequest);

    // Fill the value in the packet
    tempH225RRQptr += sizeof (Q931Message);

    // requestSeqNum
    H323PutInt(&tempH225RRQptr, h225Ras->requestSeqNum);
    h225Ras->requestSeqNum++;

    // H225_ProtocolID
    H323PutString(&tempH225RRQptr, H225_ProtocolID, IDENTIFIER_SIZE);

    // discoveryComplete
    H323PutInt(&tempH225RRQptr, TRUE);

    // callSignalAddress
    H323PutTransAddr(&tempH225RRQptr,
                     &h225Terminal->terminalCallSignalTransAddr);

    // rasAddress
    H323PutTransAddr(&tempH225RRQptr,
                     &h225RasTerminalData->terminalRasTransAddr);

    // endpoint type
    H323PutInt(&tempH225RRQptr, Terminal);

    // terminalAliasAddr
    H323PutString(&tempH225RRQptr,
                  h225Terminal->terminalAliasAddr,
                  MAX_ALIAS_ADDR_SIZE);

    // keepAlive
    H323PutInt(&tempH225RRQptr, TRUE);

    // willSupplyUUIEs
    H323PutInt(&tempH225RRQptr, FALSE);

    // maintainConnection
    H323PutInt(&tempH225RRQptr, TRUE);

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(sourceTransAddr.ipAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(destTransAddr.ipAddr, destIpAddr);
        ctoa(getSimTime(node), clockStr);

        printf("Node %u sending RRQ packet at %s\n"
               "  SourceAddress      = %s\n"
               "  SourceRasPort      = %u\n"
               "  DestinationAddress = %s\n\n",
               node->nodeId,
               clockStr,
               sourceIpAddr,
               sourceTransAddr.port,
               destIpAddr);
    }

    // Sending the GatekeeperRequest packet to the receiver
    APP_UdpSendNewData(
        node,
        APP_H225_RAS_UNICAST,
        sourceTransAddr.ipAddr,
        sourceTransAddr.port,
        destTransAddr.ipAddr,
        (char *) h225RRQ,
        SIZE_OF_H225_RAS_RRQ,
        0,
        TRACE_H323);

    MEM_free(h225RRQ);
}


//-------------------------------------------------------------------------
// NAME:        H225RasStartEndpointRegistration
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasStartEndpointRegistration(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr)
{
    H225RasCreateAndSendRRQ(
            node,
            msg,
            sourceTransAddr,
            destTransAddr);

    H225RasCreateRRQTimerMsg(
            node,
            msg,
            sourceTransAddr,
            destTransAddr);
}


///////////////////////////////////////////////////////////////////////////
//////////////////// RAS messages for endpoint location ///////////////////
///////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendLCF.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasCreateAndSendLCF(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr,
    H225RasGateKeeperRegnItem* regList)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225RasStat* h225Stat = h225Ras->h225RasStat;

    H225LocationRequest* h225LRQ =
        (H225LocationRequest*) MESSAGE_ReturnPacket(msg);

    UdpToAppRecv* udpMsg = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

    // Allocating space for the GatekeeperConfirm packet
    unsigned char* h225LCF =
        (unsigned char *) MEM_malloc (SIZE_OF_H225_RAS_LCF);

    unsigned char* temph225LCFptr = h225LCF;

    memset(h225LCF, 0, SIZE_OF_H225_RAS_LCF);

    Q931PduCreate((Q931Message*) h225LCF, 1,
                  (unsigned char) Q931MessageGetCallRefValue(
                  ((Q931Message*) h225LRQ)->callRefFlgVal),
                  LocationConfirm);

    // Fill the value in Location Confirm packet
    temph225LCFptr += sizeof (Q931Message);

    // requestSeqNum
    H323PutInt(&temph225LCFptr, h225LRQ->requestSeqNum);

    if (sourceTransAddr.ipAddr == GATEKEEPER_DISCOVERY_MULTICAST_ADDRESS)
    {
        sourceTransAddr.ipAddr = NetworkIpGetInterfaceAddress(
                                    node,
                                    udpMsg->incomingInterfaceIndex);
    }

    // callSignalAddress of terminal
    if (h323->callModel == Direct)
    {
        H323PutTransAddr(&temph225LCFptr,
                         &regList->terminalCallSignalTransAddr);
    }
    else
    {   // GatekeeperRouted, call signal address of gatekeeper
        H323PutTransAddr(&temph225LCFptr,
                         &regList->gatekeeperCallSignalTransAddr);
    }

    // rasAddress
    H323PutTransAddr(&temph225LCFptr, &regList->gatekeeperRasTransAddr);

    h225Stat->locationConfirmed++;

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u sending LCF Packet"
               " at time %s\n", node->nodeId, clockStr);
    }

    if (H225_RAS_DEBUG_DETAIL)
    {
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(sourceTransAddr.ipAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(destTransAddr.ipAddr, destIpAddr);

        printf("  SourceAddr    = %s\n"
               "  SourceRasPort = %u\n"
               "  DestAddr      = %s\n"
               "  DestPort      = %u\n\n",
               sourceIpAddr,
               sourceTransAddr.port,
               destIpAddr,
               destTransAddr.port);
    }

    // Sending the LCF packet to the requested gatekeeper
    APP_UdpSendNewData(
        node,
        APP_H225_RAS_UNICAST,
        sourceTransAddr.ipAddr,
        sourceTransAddr.port,
        destTransAddr.ipAddr,
        (char *) h225LCF,
        SIZE_OF_H225_RAS_LCF,
        0,
        TRACE_H323);

    MEM_free(h225LCF);
}


//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendLRJ.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasCreateAndSendLRJ(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225RasStat* h225Stat = h225Ras->h225RasStat;

    h225Stat->locationRejected++;

    // right now there is no rejection. may be required later
}


//-------------------------------------------------------------------------
// NAME:        H225RasSearchReceiverAddr.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
H225RasGateKeeperRegnItem* H225RasSearchReceiverAddr(
    Node* node,
    H225RasGatekeeperData* h225RasGatekeeperData,
    char* aliasAddr)
{
    LinkedList* infoList;
    ListItem* item;
    H225RasGateKeeperRegnItem* regList;

    infoList = h225RasGatekeeperData->gateKeeperInfoList;
    item = infoList->first;

    while (item != NULL)
    {
        regList = (H225RasGateKeeperRegnItem*) item->data;

        if (strcmp(regList->terminalAliasAddr, aliasAddr) == 0)
        {
            if (H225_RAS_DEBUG)
            {
                char clockStr[MAX_STRING_LENGTH];
                ctoa(getSimTime(node), clockStr);

                printf("Node %u got endpoint address from Aliasaddress"
                       " at time %s\n", node->nodeId, clockStr);
            }

            if (H225_RAS_DEBUG_DETAIL)
            {
                char gatekeeperIpAddr[MAX_STRING_LENGTH];
                char terminalIpAddr[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(
                    regList->gatekeeperCallSignalTransAddr.ipAddr,
                    gatekeeperIpAddr);

                IO_ConvertIpAddressToString(
                    regList->terminalCallSignalTransAddr.ipAddr,
                    terminalIpAddr);

                printf("  GatkeeperAddress = %s\n"
                       "  EndpointAddress  = %s\n"
                       "  AliasAddress     = %s\n\n",
                        gatekeeperIpAddr,
                        terminalIpAddr,
                        aliasAddr);
            }
            return regList;
        }
        item = item->next;
    }
    return NULL;
}


static
void H225RasSearchGatekeeperAddr(
    H225RasGatekeeperData* h225RasGatekeeperData,
    NodeAddress terminalAddr,
    unsigned short terminalPort,
    H225TransportAddress* gatekeeperRasTransAddr)
{
    LinkedList* infoList;
    ListItem* item;
    H225RasGateKeeperRegnItem* regList;

    infoList = h225RasGatekeeperData->gateKeeperInfoList;
    item = infoList->first;

    while (item != NULL)
    {
        regList = (H225RasGateKeeperRegnItem*) item->data;

        if ((regList->terminalRasTransAddr.ipAddr == terminalAddr) &&
            (regList->terminalRasTransAddr.port == terminalPort))
        {
            H323CopyTransAddr(gatekeeperRasTransAddr,
                              &regList->gatekeeperRasTransAddr);
            return;
        }
        item = item->next;
    }
}



//-------------------------------------------------------------------------
// NAME:        H225RasPrintCallList.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasPrintCallList(Node* node)
{
    ListItem* item;
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    H225RasGatekeeperData* h225RasGatekeeperData =
        (H225RasGatekeeperData *) h225Ras->h225RasCommon;

    item = h225RasGatekeeperData->callEntryList->first;
    printf("Call List of Node %d\n", node->nodeId);

    while (item != NULL)
    {

        H225RasCallEntry* callList = (H225RasCallEntry*) item->data;

        printf("requestSeqNum = %u\n",callList->requestSeqNum);

        printf("initiator ipAddr = %x\n",
            callList->initiatorCallSignalTransAddr.ipAddr);
        printf("initiator port = %u\n",
            callList->initiatorCallSignalTransAddr.port);

        printf("local ipAddr = %x\n",
            callList->localCallSignalTransAddr.ipAddr);
        printf("local port = %u\n",
            callList->localCallSignalTransAddr.port);

        printf("receiver ipAddr = %x\n",
            callList->receiverCallSignalTransAddr.ipAddr);
        printf("receiver port = %u\n",
            callList->receiverCallSignalTransAddr.port);

        printf("prev ipAddr = %x\n",
            callList->prevCallSignalTransAddr.ipAddr);
        printf("nextDeliver ipAddr = %x\n",
            callList->nextCallSignalDeliverAddr);

        printf("ingresConnectionId = %d\n",callList->ingresConnectionId);
        printf("egresConnectionId = %d\n",callList->egresConnectionId);
        printf("isMarkedForDel = %d\n",callList->isMarkedForDel);

        item = item->next;
    }
}


//-------------------------------------------------------------------------
// NAME:        H225RasInsertNewCall.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
H225RasCallEntry* H225RasInsertNewCall(
    Node* node,
    unsigned int requestSeqNum,
    H225RasGatekeeperData* h225RasGatekeeperData,
    H225TransportAddress* initiatorCallSignalTransAddr,
    H225TransportAddress* prevTransAddr,
    H225RasGateKeeperRegnItem* regList)
{
    H225RasCallEntry* newEntry
        = (H225RasCallEntry*) MEM_malloc(sizeof(H225RasCallEntry));

    memset(newEntry, 0, sizeof(H225RasCallEntry));

    newEntry->requestSeqNum = requestSeqNum;

    H323CopyTransAddr(&newEntry->initiatorCallSignalTransAddr,
                      initiatorCallSignalTransAddr);

    H323CopyTransAddr(&newEntry->prevCallSignalTransAddr, prevTransAddr);

    if (regList)
    {
        H323CopyTransAddr(&newEntry->receiverCallSignalTransAddr,
                          &regList->terminalCallSignalTransAddr);

        H323CopyTransAddr(&newEntry->localCallSignalTransAddr,
                          &regList->gatekeeperCallSignalTransAddr);

        newEntry->nextCallSignalDeliverAddr =
            regList->terminalCallSignalTransAddr.ipAddr;
    }

    newEntry->ingresConnectionId = INVALID_ID;
    newEntry->egresConnectionId = INVALID_ID;

    newEntry->isMarkedForDel = FALSE;

    ListInsert(node,
               h225RasGatekeeperData->callEntryList,
               getSimTime(node),
               newEntry);

    return newEntry;
}


//-------------------------------------------------------------------------
// NAME:        H225RasSearchAndMarkCallEntry.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
int
H225RasSearchAndMarkCallEntry(
    H225RasData* h225Ras,
    int connectionId,
    H225ConectionIdType connectionIdType)
{
    ListItem* item;
    int oppositeConnectionId = INVALID_ID;
    H225RasGatekeeperData* h225RasGatekeeperData =
        (H225RasGatekeeperData*) h225Ras->h225RasCommon;

    item = h225RasGatekeeperData->callEntryList->first;

    while (item != NULL)
    {
        H225RasCallEntry* newEntry = (H225RasCallEntry*) item->data;

        switch (connectionIdType)
        {
            case INGRES_TO_EGRES:
            {
                if (newEntry->ingresConnectionId == connectionId)
                {
                    oppositeConnectionId = newEntry->egresConnectionId;
                    newEntry->isMarkedForDel = TRUE;
                    break;
                }
            }

            case EGRES_TO_INGRES:
            {
                if (newEntry->egresConnectionId == connectionId)
                {
                    oppositeConnectionId = newEntry->ingresConnectionId;
                    newEntry->isMarkedForDel = TRUE;
                    break;
                }
            }

            default:
                break;
        }
        item = item->next;
    }
    return oppositeConnectionId;
}


//-------------------------------------------------------------------------
// NAME:        H225RasSearchAndRemoveCallEntry.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
int
H225RasSearchAndRemoveCallEntry(
    Node* node,
    H225RasData* h225Ras,
    int connectionId)
{
    ListItem* item;
    int oppositeConnectionId = INVALID_ID;
    H225RasGatekeeperData* h225RasGatekeeperData =
        (H225RasGatekeeperData*) h225Ras->h225RasCommon;

    item = h225RasGatekeeperData->callEntryList->first;

    while (item != NULL)
    {
        H225RasCallEntry* newEntry = (H225RasCallEntry*) item->data;

        if (newEntry->isMarkedForDel)
        {
                if (newEntry->ingresConnectionId == connectionId)
                {
                    oppositeConnectionId = newEntry->egresConnectionId;

                    ListGet(node,
                            h225RasGatekeeperData->callEntryList,
                            item,
                            TRUE,
                            FALSE);

                    break;
                }
                if (newEntry->egresConnectionId == connectionId)
                {
                    oppositeConnectionId = newEntry->ingresConnectionId;

                    ListGet(node,
                            h225RasGatekeeperData->callEntryList,
                            item,
                            TRUE,
                            FALSE);

                    break;
                }
        }
        item = item->next;
    }
    return oppositeConnectionId;
}


//-------------------------------------------------------------------------
// NAME:        H225RasSearchCallList.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
H225RasCallEntry* H225RasSearchCallList(
    H225RasData* h225Ras,
    NodeAddress sourceAddr,
    unsigned short sourcePort,
    H225RasSearchType searchType)
{
    ListItem* item;
    H225RasGatekeeperData* h225RasGatekeeperData =
        (H225RasGatekeeperData*) h225Ras->h225RasCommon;

    ERROR_Assert(h225RasGatekeeperData->gateKeeperInfoList,
        "Invalid gatekeeper info structure\n");

    item = h225RasGatekeeperData->callEntryList->first;

    while (item != NULL)
    {
        H225RasCallEntry* newEntry = (H225RasCallEntry*) item->data;

        if (newEntry->isMarkedForDel)
        {
            item = item->next;
            continue;
        }

        switch (searchType)
        {
            case PREVIOUS:
            {
                if ((newEntry->prevCallSignalTransAddr.ipAddr ==
                        sourceAddr)
                    && (newEntry->prevCallSignalTransAddr.port ==
                        sourcePort))
                {
                    return newEntry;
                }
                break;
            }
            case CURRENT:
            {
                if ((newEntry->localCallSignalTransAddr.ipAddr ==
                        sourceAddr)
                    && (newEntry->localCallSignalTransAddr.port ==
                        sourcePort))
                {
                    return newEntry;
                }
                break;
            }
            default:
                break;
        }
        item = item->next;
    }
    return NULL;
}


//-------------------------------------------------------------------------
// NAME:        H225RasSearchCallListForReceiverAddr.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
H225RasCallEntry* H225RasSearchCallListForReceiverAddr(
    H225RasData* h225Ras,
    NodeAddress receiverAddr,
    unsigned short receiverPort)
{
    ListItem* item;
    H225RasGatekeeperData* h225RasGatekeeperData =
        (H225RasGatekeeperData*) h225Ras->h225RasCommon;

    ERROR_Assert(h225RasGatekeeperData->gateKeeperInfoList,
        "Invalid gatekeeper info structure\n");

    item = h225RasGatekeeperData->callEntryList->first;

    while (item != NULL)
    {
        H225RasCallEntry* newEntry = (H225RasCallEntry*) item->data;

        if (newEntry->isMarkedForDel)
        {
            item = item->next;
            continue;
        }

        if ((newEntry->receiverCallSignalTransAddr.ipAddr == receiverAddr)
            && (newEntry->receiverCallSignalTransAddr.port == receiverPort))
        {
            return newEntry;
        }
        item = item->next;
    }
    return NULL;
}


//-------------------------------------------------------------------------
// NAME:        H225RasSearchCallListForConnectionId.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
int H225RasSearchCallListForConnectionId(
    H225RasData* h225Ras,
    int connectionId,
    H225ConectionIdType connectionIdType)
{
    ListItem* item;
    H225RasGatekeeperData* h225RasGatekeeperData =
        (H225RasGatekeeperData*) h225Ras->h225RasCommon;

    ERROR_Assert(h225RasGatekeeperData->gateKeeperInfoList,
        "Invalid gatekeeper info structure\n");

    item = h225RasGatekeeperData->callEntryList->first;

    while (item != NULL)
    {
        H225RasCallEntry* newEntry = (H225RasCallEntry*) item->data;

        if (newEntry->isMarkedForDel)
        {
            item = item->next;
            continue;
        }

        switch (connectionIdType)
        {
            case INGRES_TO_EGRES:
            {
                if (newEntry->ingresConnectionId == connectionId)
                {
                    return newEntry->egresConnectionId;
                }
                break;
            }
            case EGRES_TO_INGRES:
            {
                if (newEntry->egresConnectionId == connectionId)
                {
                    return newEntry->ingresConnectionId;
                }
                break;
            }
            default:
                break;
        }
        item = item->next;
    }
    return INVALID_ID;
}



//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendLRQReply.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasCreateAndSendLRQReply(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr,
    H225RasGateKeeperRegnItem* regList)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    H225RasGatekeeperData* h225RasGatekeeperData =
        (H225RasGatekeeperData *) h225Ras->h225RasCommon;

    H225LocationRequest* h225LRQ;

    H225TransportAddress prevCallSignalTransAddr;

    // GateKeeper Registration List maintains only for Gatekeeper.
    if (h323->endpointType != Gatekeeper)
    {
        ERROR_Assert(FALSE, "Node should be Gatekeeper, Invalid LRQ\n");
    }

    h225LRQ = (H225LocationRequest*) MESSAGE_ReturnPacket(msg);

    prevCallSignalTransAddr.ipAddr = (h225LRQ->replyAddress).ipAddr ;
    prevCallSignalTransAddr.port = (h225LRQ->replyAddress).port ;

    if (regList)
    {
        if (h323->callModel == GatekeeperRouted)
        {

            // destTransAddr holding initiator address
            H225RasInsertNewCall(node,
                                 h225LRQ->requestSeqNum,
                                 h225RasGatekeeperData,
                                 &prevCallSignalTransAddr,
                                 &destTransAddr,
                                 regList);
        }

        H225RasCreateAndSendLCF(
            node,
            msg,
            sourceTransAddr,
            destTransAddr,
            regList);
    }
    else
    {
        if (H225_RAS_DEBUG)
        {
            char clockStr[MAX_STRING_LENGTH];
            ctoa(getSimTime(node), clockStr);

            printf("Node %u did not get any endpoint address from"
                   " aliasaddress at time %s\n\n", node->nodeId, clockStr);
        }

        H225RasCreateAndSendLRJ(
            node,
            msg,
            sourceTransAddr,
            destTransAddr);
    }
}


//-------------------------------------------------------------------------
// NAME:        H225RasSearchCallListForSeqNum.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
H225RasCallEntry* H225RasSearchCallListForSeqNum(
    Node* node,
    unsigned int requestSeqNum)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    H225RasGatekeeperData* h225RasGatekeeperData =
        (H225RasGatekeeperData *) h225Ras->h225RasCommon;

    ListItem* item = h225RasGatekeeperData->callEntryList->first;

    while (item != NULL)
    {
        H225RasCallEntry* anEntry = (H225RasCallEntry*) item->data;

        if (anEntry->requestSeqNum == requestSeqNum)
        {
            return anEntry;
        }
        item = item->next;
    }
    return NULL;
}


//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendLRQ.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasCreateAndSendLRQ(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr,
    char* dstAliasAddress,
    unsigned int requestSeqNum)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    H225RasStat* h225Stat = h225Ras->h225RasStat;

    unsigned char callRefValue;

    H225RasCallEntry* anEntry;

    // Allocating space for the Location Request packet
    unsigned char* h225LRQ =
        (unsigned char *) MEM_malloc (SIZE_OF_H225_RAS_LRQ);

    unsigned char* tempH225LRQptr = h225LRQ;

    memset(h225LRQ, 0, SIZE_OF_H225_RAS_LRQ);

    callRefValue = (unsigned char) ((h225Ras->requestSeqNum)
        % (MAX_CALL_REF_VALUE));

    h225Stat->locationRequested++;

    Q931PduCreate((Q931Message*) h225LRQ,
                  0,
                  callRefValue,
                  LocationRequest);

    // Fill the value in GatekeeperRequest packet
    tempH225LRQptr += sizeof (Q931Message);

    // requestSeqNum
    H323PutInt(&tempH225LRQptr, requestSeqNum);

    // endpoint type
    H323PutInt(&tempH225LRQptr, Terminal);

    // dstAliasAddress
    H323PutString(&tempH225LRQptr, dstAliasAddress, MAX_ALIAS_ADDR_SIZE);

    anEntry = H225RasSearchCallListForSeqNum(node, requestSeqNum);

    // replyAddress
    if (anEntry)
    {
        H323PutTransAddr(&tempH225LRQptr,
                         &anEntry->localCallSignalTransAddr);
    }

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(sourceTransAddr.ipAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(destTransAddr.ipAddr, destIpAddr);
        ctoa(getSimTime(node), clockStr);

        printf("Node %u sending LRQ packet at %s\n"
               "  SsourceAddress     = %s\n"
               "  SourceRasPort      = %u\n"
               "  DestinationAddress = %s\n"
               "  DestAliasAddress   = %s\n\n",
               node->nodeId,
               clockStr,
               sourceIpAddr,
               sourceTransAddr.port,
               destIpAddr,
               dstAliasAddress);
    }

    // Sending the LocationRequest packet to the receiver
    APP_UdpSendNewData(
        node,
        (AppType) destTransAddr.port,
        sourceTransAddr.ipAddr,
        sourceTransAddr.port,
        destTransAddr.ipAddr,
        (char *) h225LRQ,
        SIZE_OF_H225_RAS_LRQ,
        0,
        TRACE_H323);

    MEM_free(h225LRQ);
}


///////////////////////////////////////////////////////////////////////////
//////////////////// RAS messages for admission control ///////////////////
///////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendACF.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasCreateAndSendACF(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr,
    H225TransportAddress receiverCallSignalTransAddr,
    H225TransportAddress *initiatorTransAddr,
    BOOL sendToInitiator)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225RasStat* h225Stat = h225Ras->h225RasStat;

    H225AdmissionRequest* h225ARQ =
        (H225AdmissionRequest*) MESSAGE_ReturnPacket(msg);

    // dummy, 4 not allowed
    unsigned int* requestSeqNum =
        (unsigned int*) (((char*) h225ARQ) + 4);

    // Allocating space for the AdmissionConfirm packet
    unsigned char* h225ACF =
        (unsigned char *) MEM_malloc (SIZE_OF_H225_RAS_ACF);

    unsigned char* temph225ACFptr = h225ACF;

    memset(h225ACF, 0, SIZE_OF_H225_RAS_ACF);

    Q931PduCreate((Q931Message*) h225ACF,
                  1,
                  (unsigned char) Q931MessageGetCallRefValue(
                  ((Q931Message*) h225ARQ)->callRefFlgVal),
                  AdmissionConfirm);

    // Fill the value in AdmissionConfirm packet
    temph225ACFptr += sizeof (Q931Message);

    // requestSeqNum
    H323PutInt(&temph225ACFptr, *requestSeqNum);

    // bandWidth
    H323PutInt(&temph225ACFptr, 1);

    // callModel
    H323PutInt(&temph225ACFptr, h323->callModel);

    // destCallSignalAddress
    if (h323->callModel == Direct)
    {
        if (sendToInitiator) // indicates ARQ called from initiator
        {
            // for initiator send receiver address
            H323PutTransAddr(&temph225ACFptr, &receiverCallSignalTransAddr);
        }
        else
        {
            if (initiatorTransAddr)
            {
                // for receiver send initiator address
                H323PutTransAddr(&temph225ACFptr, initiatorTransAddr);
            }
        }
    }
    else
    {   // GatekeeperRouted
        H323PutTransAddr(&temph225ACFptr, &sourceTransAddr);
    }

    // willRespondToIRR
    H323PutInt(&temph225ACFptr, TRUE);

    // uuiesRequested
    H323PutInt(&temph225ACFptr, TRUE);

    h225Stat->admissionConfirmed++;

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u sending ACF Packet"
               " at time %s\n", node->nodeId, clockStr);
    }

    if (H225_RAS_DEBUG_DETAIL)
    {
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(sourceTransAddr.ipAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(destTransAddr.ipAddr, destIpAddr);

        printf("  SourceAddr    = %s\n"
               "  SourceRasPort = %u\n"
               "  DestAddr      = %s\n"
               "  DestPort      = %u\n\n",
               sourceIpAddr,
               sourceTransAddr.port,
               destIpAddr,
               destTransAddr.port);
    }

    // Sending the Admission Confirm packet to the receiver
    APP_UdpSendNewData(
        node,
        //destTransAddr.port,
        APP_H225_RAS_UNICAST,
        sourceTransAddr.ipAddr,
        sourceTransAddr.port,
        destTransAddr.ipAddr,
        (char *) h225ACF,
        SIZE_OF_H225_RAS_ACF,
        0,
        TRACE_H323);

    MEM_free(h225ACF);
}


//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendARJ.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
// TBD
static
void H225RasCreateAndSendARJ(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225RasStat* h225Stat = h225Ras->h225RasStat;

    h225Stat->admissionRejected++;
}






//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendARQReply
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasCreateAndSendARQReply(
    Node* node,
    Message* msg,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    H225AdmissionRequest* h225ARQ =
        (H225AdmissionRequest*) MESSAGE_ReturnPacket(msg);

    UdpToAppRecv* udpMsg = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

    H225RasGatekeeperData* h225RasGatekeeperData =
        (H225RasGatekeeperData*) h225Ras->h225RasCommon;

    H225RasGateKeeperRegnItem* regList =
        H225RasSearchReceiverAddr(node,
                              h225RasGatekeeperData,
                              h225ARQ->aliasAddress);

    unsigned int requestSeqNum;

    H225TransportAddress* initiatorCallSignalTransAddr =
        (H225TransportAddress*) (((char*) h225ARQ) + 48);

    if (regList)
    {
        H225RasCallEntry* callEntry;

        if (h323->callModel == GatekeeperRouted)
        {
            if (Q931MessageGetCallRefFlag(
                h225ARQ->q931Pdu.callRefFlgVal) == H323_INITIATOR)
            {
                // destTransAddr holding initiator address
                H225RasInsertNewCall(node,
                                     h225ARQ->requestSeqNum,
                                     h225RasGatekeeperData,
                                     initiatorCallSignalTransAddr,
                                     initiatorCallSignalTransAddr,
                                     regList);
            }

            callEntry = H225RasSearchCallListForReceiverAddr(
                h225Ras,
                GetIPv4Address(udpMsg->sourceAddr),
                udpMsg->sourcePort);

            if (callEntry)
            {
                H225RasCreateAndSendACF(
                    node,
                    msg,
                    sourceTransAddr,
                    destTransAddr,
                    regList->terminalCallSignalTransAddr,
                    &(callEntry->initiatorCallSignalTransAddr),
                    FALSE);
            }
            else
            {
                H225RasCreateAndSendACF(
                    node,
                    msg,
                    sourceTransAddr,
                    destTransAddr,
                    regList->terminalCallSignalTransAddr,
                    NULL,
                    FALSE);
            }
        }
        else
        {
            H225RasCreateAndSendACF(
                node,
                msg,
                sourceTransAddr,
                destTransAddr,
                regList->terminalCallSignalTransAddr,
                initiatorCallSignalTransAddr,
                TRUE);
        }
    }
    else
    {
        H225TransportAddress gatekeeperRasTransAddr;
        H225RasCallEntry* callEntry;

        H225RasSearchGatekeeperAddr(
                              h225RasGatekeeperData,
                              GetIPv4Address(udpMsg->sourceAddr),
                              udpMsg->sourcePort,
                              &gatekeeperRasTransAddr);

        H323CopyTransAddr(&sourceTransAddr, &gatekeeperRasTransAddr);

        destTransAddr.ipAddr = GATEKEEPER_DISCOVERY_MULTICAST_ADDRESS;
        destTransAddr.port = APP_H225_RAS_MULTICAST;

        requestSeqNum = h225Ras->requestSeqNum++;

        callEntry = H225RasInsertNewCall(node,
                             requestSeqNum,
                             h225RasGatekeeperData,
                             initiatorCallSignalTransAddr,
                             initiatorCallSignalTransAddr,
                             0);

        callEntry->localCallSignalTransAddr.ipAddr =
            gatekeeperRasTransAddr.ipAddr;
        callEntry->localCallSignalTransAddr.port =
            node->appData.nextPortNum++;

        H225RasCreateAndSendLRQ(
            node,
            msg,
            sourceTransAddr,
            destTransAddr,
            h225ARQ->aliasAddress,
            requestSeqNum);
    }
}


//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendARQ.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasCreateAndSendARQ(
    Node* node,
    H225TransportAddress sourceTransAddr,
    H225TransportAddress destTransAddr,
    VoipHostData* voip,
    char* destAliasAddr,
    H323HostState callRefFlag)
{
    unsigned char callRefValue;

    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    H225RasStat* h225Stat = h225Ras->h225RasStat;

    // Allocating space for the GatekeeperRequest packet
    unsigned char* h225ARQ =
        (unsigned char *) MEM_malloc (SIZE_OF_H225_RAS_ARQ);

    unsigned char* tempH225ARQptr = h225ARQ;

    memset(h225ARQ, 0, SIZE_OF_H225_RAS_ARQ);

    callRefValue = (unsigned char) 1;

    h225Stat->admissionRequested++;

    Q931PduCreate((Q931Message*) h225ARQ,
                  (char) callRefFlag,
                  callRefValue,
                  AdmissionRequest);

    // Fill the value in GatekeeperRequest packet
    tempH225ARQptr += sizeof (Q931Message);

    // requestSeqNum
    H323PutInt(&tempH225ARQptr, h225Ras->requestSeqNum);

    // callType
    H323PutInt(&tempH225ARQptr, pointToPoint);

    // endpoint Type
    H323PutInt(&tempH225ARQptr, Terminal);

    // terminalAliasAddr
    H323PutString(&tempH225ARQptr,
                  destAliasAddr,
                  MAX_ALIAS_ADDR_SIZE);

    // source call signal trans addr
    H323PutTransAddrItemwise(&tempH225ARQptr,
                             voip->callSignalAddr,
                             voip->callSignalPort);

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(sourceTransAddr.ipAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(destTransAddr.ipAddr, destIpAddr);
        ctoa(getSimTime(node), clockStr);

        printf("Node %u sending ARQ packet at %s\n"
               "  SourceAddress      = %s\n"
               "  SourceRasPort      = %u\n"
               "  DestinationAddress = %s\n\n",
               node->nodeId,
               clockStr,
               sourceIpAddr,
               sourceTransAddr.port,
               destIpAddr);
    }

    // Sending the ARQ packet to the receiver
    APP_UdpSendNewData(
        node,
        APP_H225_RAS_UNICAST,
        sourceTransAddr.ipAddr,
        sourceTransAddr.port,
        destTransAddr.ipAddr,
        (char *) h225ARQ,
        SIZE_OF_H225_RAS_ARQ,
        0,
        TRACE_H323);

    MEM_free(h225ARQ);
}


//-------------------------------------------------------------------------
// NAME:        H225RasCreateAndSendARQForReceiver.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
void H225RasCreateAndSendARQForReceiver(Node* node, VoipHostData* voip)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225TerminalData* h225Terminal = h323->h225Terminal;

    H225RasTerminalData* h225RasTerminalData =
        (H225RasTerminalData *) h225Ras->h225RasCommon;

    H225RasCreateAndSendARQ(
        node,
        h225RasTerminalData->terminalRasTransAddr,
        h225RasTerminalData->gatekeeperRasTransAddr,
        voip,
        h225Terminal->terminalAliasAddr,
        H323_RECEIVER);
}


//-------------------------------------------------------------------------
// NAME:        H225RasAdmissionRequest
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
void H225RasAdmissionRequest(
    Node* node,
    VoipHostData* voip)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    H225RasTerminalData* h225RasTerminalData =
        (H225RasTerminalData *) h225Ras->h225RasCommon;

    h323->callSignalPort = voip->callSignalPort;

    H225RasCreateAndSendARQ(
        node,
        h225RasTerminalData->terminalRasTransAddr,
        h225RasTerminalData->gatekeeperRasTransAddr,
        voip,
        voip->destAliasAddr,
        H323_INITIATOR);
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////// RAS init ////////////////////////////////
///////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------------
// NAME:        H225RasStatsInit.
//
// PURPOSE:     This function is called to initialize all the
//              statistics variables/flags for H225Ras Stats.
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasStatsInit(Node* node, H323Data* h323)
{
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    H225RasStat* h225Stat = (H225RasStat*)
        MEM_malloc (sizeof (H225RasStat));

    h225Ras->h225RasStat = h225Stat;
    memset(h225Ras->h225RasStat, 0, sizeof(H225RasStat));
}


//-------------------------------------------------------------------------
// NAME:        H323ReadEndpointList
//
// PURPOSE:
//
// PARAMETERS:  node, node which is message/event appears
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
BOOL H323ReadEndpointList(
    PartitionData * partitionData,
    NodeAddress thisNodeId,
    const NodeInput* nodeInput,
    char* terminalAliasAddr)
{
    BOOL wasFound;
    NodeInput placementInput;
    int i;
    int numValues;
    Node* firstNode;

    firstNode = partitionData->firstNode;
    IO_ReadCachedFile(
        thisNodeId,
        ANY_ADDRESS,
        nodeInput,
        "TERMINAL-ALIAS-ADDRESS-FILE",
        &wasFound,
        &placementInput);

    if (wasFound == FALSE)
    {
        ERROR_ReportError("Cannot open TERMINAL-ALIAS-ADDRESS-FILE.");
    }

    for (i = 0; i < placementInput.numLines; i++)
    {
        NodeAddress nodeId;
        NodeAddress sourceAddr;
        char filename[MAX_STRING_LENGTH];
        char errorMsg[MAX_STRING_LENGTH];
        BOOL wasFound2;

        numValues = sscanf(placementInput.inputStrings[i], "%u %s",
            &nodeId, terminalAliasAddr);

        sourceAddr = MAPPING_GetDefaultInterfaceAddressFromNodeId(
                          firstNode, nodeId);

        if (sourceAddr == INVALID_MAPPING)
        {
            IO_ReadString(
                thisNodeId,
                ANY_ADDRESS,
                nodeInput,
                "TERMINAL-ALIAS-ADDRESS-FILE",
                &wasFound2,
                filename);

            sprintf(errorMsg,
                    "Source node %u does not exist:Wrong configuration"
                    " in file '%s'\n",
                    nodeId,
                    filename);
            ERROR_ReportError(errorMsg);
        }

        if (numValues != 2)
        {
            IO_ReadString(
                thisNodeId,
                ANY_ADDRESS,
                nodeInput,
                "TERMINAL-ALIAS-ADDRESS-FILE",
                &wasFound2,
                filename);

            sprintf(errorMsg,
                    "Wrong configuration format in file '%s'\nString :"
                    "%s\n", filename, placementInput.inputStrings[i]);
            ERROR_ReportError(errorMsg);
        }

        if (nodeId == thisNodeId)
        {
            return TRUE;
        }
    }
    terminalAliasAddr [0] = '\0';
    return FALSE;
}


//-------------------------------------------------------------------------
// NAME:        H225RasTerminalInit.
//
// PURPOSE:     Handling all Endpoint initializations needed for H225Ras
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
void H225RasTerminalInit(
    Node* node,
    const NodeInput* nodeInput,
    H323Data* h323,
    NodeAddress homeUnicastAdr)
{
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];
    int numHostBits;
    BOOL isNodeId;
    H225TransportAddress sourceTransAddr;
    H225TransportAddress destTransAddr;
    NodeAddress gatekeeperAddress;
    unsigned short destPort;
    H225RasTerminalData* h225RasTerminalData;

    H225RasData* h225RasData =
        (H225RasData*) MEM_malloc (sizeof (H225RasData));

    h323->h225Ras = (void *) h225RasData;

    h225RasTerminalData = (H225RasTerminalData*)
        MEM_malloc (sizeof (H225RasTerminalData));

    h225RasData->h225RasCommon = (void *) h225RasTerminalData;

    h225RasData->requestSeqNum = 1;

    h225RasTerminalData->terminalRasTransAddr.ipAddr = homeUnicastAdr;
    h225RasTerminalData->terminalRasTransAddr.port =
        node->appData.nextPortNum++;

    h225RasTerminalData->GRQRetryCount = 0;
    h225RasTerminalData->h225RasGotGateKeeperResponse = FALSE;
    h225RasTerminalData->isAlreadyRegistered = FALSE;
    h225RasTerminalData->RRQRetryCount = 0;
    h225RasTerminalData->h225RasGotRegistrationResponse = FALSE;
    h225RasTerminalData->gatekeeperRasTransAddr.ipAddr = 0;
    h225RasTerminalData->gatekeeperRasTransAddr.port = 0;

    if (H225_RAS_DEBUG)
    {
        printf("Node %u is a Endpoint\n", node->nodeId);
    }

    // Initialize all the state variables for statistics
    H225RasStatsInit(node, h323);

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "GATEKEEPER-ADDRESS",
        &retVal,
        buf);

    if (retVal == TRUE)
    {
        IO_ParseNodeIdHostOrNetworkAddress(
            buf,
            &gatekeeperAddress,
            &numHostBits,
            &isNodeId);

        destPort = APP_H225_RAS_UNICAST;
        h225RasTerminalData->gateKeeperUnicastAddAvailable = TRUE;
    }
    else
    {
        gatekeeperAddress = GATEKEEPER_DISCOVERY_MULTICAST_ADDRESS;
        destPort = APP_H225_RAS_MULTICAST;
        h225RasTerminalData->gateKeeperUnicastAddAvailable = FALSE;
    }

    H323CopyTransAddr(&sourceTransAddr,
                      &h225RasTerminalData->terminalRasTransAddr);

    destTransAddr.ipAddr = gatekeeperAddress;
    destTransAddr.port = destPort;

    H225RasCreateAndSendGRQ(
        node,
        sourceTransAddr,
        destTransAddr);

    H225RasCreateGRQTimerMsg(
        node,
        sourceTransAddr,
        destTransAddr);

}


//-------------------------------------------------------------------------
// NAME:        H225RasGatekeeperInit
//
// PURPOSE:     Handling all Gatekeeper initializations
//              needed for H225Ras
//
// PARAMETERS:  node,      Node which is to be initialized.
//              h323,      H323 pointer
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
void H225RasGatekeeperInit(Node* node, H323Data* h323)
{
    H225RasGatekeeperData* h225RasGatekeeperData;

    H225RasData* h225RasData =
        (H225RasData*) MEM_malloc (sizeof (H225RasData));

    h323->h225Ras = (void *) h225RasData;

    h225RasGatekeeperData = (H225RasGatekeeperData*)
        MEM_malloc (sizeof (H225RasGatekeeperData));

    h225RasData->h225RasCommon = (void *) h225RasGatekeeperData;

    h225RasData->requestSeqNum = 1;

    // Initialize the list for RAS service started info
    ListInit(node, &h225RasGatekeeperData->rasProcessList);

    // Initialize the list for Gatekeeper information
    ListInit(node, &h225RasGatekeeperData->gateKeeperInfoList);

    // Initialize the list of call entry
    ListInit(node, &h225RasGatekeeperData->callEntryList);

    if (H225_RAS_DEBUG)
    {
        printf("Node %u is a Gatekeeper\n", node->nodeId);
    }

    // Initialize all the state variables for statistics
    H225RasStatsInit(node, h323);

    NetworkIpJoinMulticastGroup(node,
                              GATEKEEPER_DISCOVERY_MULTICAST_ADDRESS,
                              MULTICAST_GROUP_JOINING_TIME);
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////// RAS layer ///////////////////////////////
///////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------------
// NAME:        H225RasHandleGRQ
//
// PURPOSE:     H225Ras GRQ packet handling routine
//
// PARAMETERS:  node, node which is message/event appears
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
static
void H225RasHandleGRQ(Node* node, Message* msg)
{
    H225TransportAddress sourceTransAddr;
    H225TransportAddress destTransAddr;
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    H225RasGatekeeperData* h225RasGatekeeperData =
        (H225RasGatekeeperData *) h225Ras->h225RasCommon;

    UdpToAppRecv* udpMsg = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

    if (h323->endpointType != Gatekeeper)
    {
        if (H225_RAS_DEBUG)
        {
            printf("node %u is not a Gatekeeper, So discard the GRQ "
                "packet\n", node->nodeId);
        }

        return;
    }

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u got GRQ"
               " at time %s\n", node->nodeId, clockStr);
    }

    if (H225_RAS_DEBUG_DETAIL)
    {
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&udpMsg->sourceAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(&udpMsg->destAddr, destIpAddr);

        printf("  SourceAddr    = %s\n"
               "  SourceRasPort = %u\n"
               "  DestAddr      = %s\n"
               "  DestPort      = %u\n\n",
               sourceIpAddr,
               udpMsg->sourcePort,
               destIpAddr,
               udpMsg->destPort);
    }

    sourceTransAddr.ipAddr = GetIPv4Address(udpMsg->destAddr);
    sourceTransAddr.port = node->appData.nextPortNum++;
    destTransAddr.ipAddr = GetIPv4Address(udpMsg->sourceAddr);
    destTransAddr.port = udpMsg->sourcePort;

    H225RasCreateAndSendGRQReply(
        node,
        msg,
        sourceTransAddr,
        destTransAddr,
        h225RasGatekeeperData);
}


//-------------------------------------------------------------------------
// NAME:        H225RasHandleGCF
//
// PURPOSE:     H225Ras GRQ packet handling routine
//
// PARAMETERS:  node, node which is message/event appears
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
static
void H225RasHandleGCF(Node* node, Message* msg)
{
    H225TransportAddress destTransAddr;

    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225RasTerminalData* h225RasTerminalData =
        (H225RasTerminalData *) h225Ras->h225RasCommon;

    UdpToAppRecv* udpMsg = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

    H225TransportAddress* rasAddr = (H225TransportAddress*)
        (MESSAGE_ReturnPacket(msg) + 22);

    h225RasTerminalData->h225RasGotGateKeeperResponse = TRUE;

    if (!h225RasTerminalData->isAlreadyRegistered)
    {
        h225RasTerminalData->isAlreadyRegistered = TRUE;
    }
    else
    {
        return;
    }

    if (h323->endpointType != Terminal)
    {
        ERROR_Assert(FALSE, "Node should be Terminal, Invalid GCF\n");
    }

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u got GCF"
               " at time %s\n", node->nodeId, clockStr);
    }

    if (H225_RAS_DEBUG_DETAIL)
    {
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&udpMsg->sourceAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(&udpMsg->destAddr, destIpAddr);

        printf("  SourceAddr    = %s\n"
               "  SourceRasPort = %u\n"
               "  DestAddr      = %s\n"
               "  DestPort      = %u\n\n",
               sourceIpAddr,
               udpMsg->sourcePort,
               destIpAddr,
               udpMsg->destPort);
    }

    H323CopyTransAddr(&h225RasTerminalData->gatekeeperRasTransAddr, rasAddr);

    destTransAddr.ipAddr = GetIPv4Address(udpMsg->sourceAddr);
    destTransAddr.port = udpMsg->sourcePort;

    H225RasStartEndpointRegistration(
        node,
        msg,
        h225RasTerminalData->terminalRasTransAddr,
        destTransAddr);
}


//-------------------------------------------------------------------------
// NAME:        H225RasHandleGRJ
//
// PURPOSE:     H225Ras GRJ packet handling routine
//
// PARAMETERS:  node, node which is message/event appears
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
// TBD
static
void H225RasHandleGRJ(Node* node, Message* msg)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225RasTerminalData* h225RasTerminalData =
        (H225RasTerminalData *) h225Ras->h225RasCommon;

    UdpToAppRecv* udpMsg = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

    if (h323->endpointType != Terminal)
    {
        ERROR_Assert(FALSE, "Node should be Terminal, Invalid GRJ\n");
    }

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u got GRJ"
               " at time %s\n", node->nodeId, clockStr);
    }

    if (H225_RAS_DEBUG_DETAIL)
    {
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&udpMsg->sourceAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(&udpMsg->destAddr, destIpAddr);

        printf("  SourceAddr             = %s\n"
               "  SourcePort             = %u\n"
               "  DestAddr               = %s\n"
               "  DestPort               = %u\n\n",
               sourceIpAddr,
               udpMsg->sourcePort,
               destIpAddr,
               udpMsg->destPort);
    }

    h225RasTerminalData->h225RasGotGateKeeperResponse = FALSE;
    h225RasTerminalData->isAlreadyRegistered = FALSE;
}


//-------------------------------------------------------------------------
// NAME:        H225RasHandleRRQ
//
// PURPOSE:     H225Ras GRQ packet handling routine
//
// PARAMETERS:  node, node which is message/event appears
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
static
void H225RasHandleRRQ(Node* node, Message* msg)
{
    H225TransportAddress sourceTransAddr;
    H225TransportAddress destTransAddr;

    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    UdpToAppRecv* udpMsg = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

    if (h323->endpointType != Gatekeeper)
    {
        ERROR_Assert(FALSE, "Node should be Gatekeeper, Invalid RRQ\n");
    }

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u got RRQ"
               " at time %s\n", node->nodeId, clockStr);
    }

    if (H225_RAS_DEBUG_DETAIL)
    {
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&udpMsg->sourceAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(&udpMsg->destAddr, destIpAddr);

        printf("  SourceAddr    = %s\n"
               "  SourceRasPort = %u\n"
               "  DestAddr      = %s\n"
               "  DestPort      = %u\n\n",
               sourceIpAddr,
               udpMsg->sourcePort,
               destIpAddr,
               udpMsg->destPort);
    }

    sourceTransAddr.ipAddr = GetIPv4Address(udpMsg->destAddr);
    sourceTransAddr.port = udpMsg->destPort;
    destTransAddr.ipAddr = GetIPv4Address(udpMsg->sourceAddr);
    destTransAddr.port = udpMsg->sourcePort;

    H225RasCreateAndSendRRQReply(
        node,
        msg,
        sourceTransAddr,
        destTransAddr);
}


//-------------------------------------------------------------------------
// NAME:        H225RasHandleRCF
//
// PURPOSE:     H225Ras GRQ packet handling routine
//
// PARAMETERS:  node, node which is message/event appears
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
static
void H225RasHandleRCF(Node* node, Message* msg)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225TerminalData* h225Terminal = h323->h225Terminal;

    H225RasTerminalData* h225RasTerminalData =
        (H225RasTerminalData *) h225Ras->h225RasCommon;

    H225TransportAddress* callSignalAddr = (H225TransportAddress*)
        (MESSAGE_ReturnPacket(msg) + 22);

    UdpToAppRecv* udpMsg = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

    if (h323->endpointType != Terminal)
    {
        ERROR_Assert(FALSE, "Node should be Terminal, Invalid RCF\n");
    }

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u got RCF"
               " at time %s\n", node->nodeId, clockStr);
    }

    if (H225_RAS_DEBUG_DETAIL)
    {
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&udpMsg->sourceAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(&udpMsg->destAddr, destIpAddr);

        printf("  SourceAddr    = %s\n"
               "  SourceRasPort = %u\n"
               "  DestAddr      = %s\n"
               "  DestPort      = %u\n\n",
               sourceIpAddr,
               udpMsg->sourcePort,
               destIpAddr,
               udpMsg->destPort);
    }

    h225RasTerminalData->h225RasGotRegistrationResponse = TRUE;

    H323CopyTransAddr(&h225Terminal->gatekeeperCallSignalTransAddr,
                      callSignalAddr);
}


//-------------------------------------------------------------------------
// NAME:        H225RasHandleRRJ
//
// PURPOSE:     H225Ras GRQ packet handling routine
//
// PARAMETERS:  node, node which is message/event appears
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
// TBD
static
void H225RasHandleRRJ(Node* node, Message* msg)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225RasTerminalData* h225RasTerminalData =
        (H225RasTerminalData *) h225Ras->h225RasCommon;

    UdpToAppRecv* udpMsg = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

    if (h323->endpointType != Terminal)
    {
        ERROR_Assert(FALSE, "Node should be Terminal, Invalid RRJ\n");
    }

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u got RRJ"
               " at time %s\n", node->nodeId, clockStr);
    }

    if (H225_RAS_DEBUG_DETAIL)
    {
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&udpMsg->sourceAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(&udpMsg->destAddr, destIpAddr);

        printf("  SourceAddr    = %s\n"
               "  SourceRasPort = %u\n"
               "  DestAddr      = %s\n"
               "  DestRasPort   = %u\n\n",
               sourceIpAddr,
               udpMsg->sourcePort,
               destIpAddr,
               udpMsg->destPort);
    }

    h225RasTerminalData->h225RasGotRegistrationResponse = TRUE;
}


//-------------------------------------------------------------------------
// NAME:        H225RasHandleLRQ
//
// PURPOSE:     H225Ras GRQ packet handling routine
//
// PARAMETERS:  node, node which is message/event appears
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
static
void H225RasHandleLRQ(Node* node, Message* msg)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225RasGatekeeperData* h225RasGatekeeperData =
        (H225RasGatekeeperData *) h225Ras->h225RasCommon;

    H225RasGateKeeperRegnItem* regList;
    UdpToAppRecv* udpMsg = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

    H225LocationRequest* h225LRQ =
        (H225LocationRequest*) MESSAGE_ReturnPacket(msg);

   H225TransportAddress replyTransAddr;
    replyTransAddr.ipAddr = (h225LRQ->replyAddress).ipAddr ;
    replyTransAddr.port = (h225LRQ->replyAddress).port ;

    ERROR_Assert(h323->endpointType == Gatekeeper,
        "Node receives invalid LRQ message\n");

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u got LRQ"
               " at time %s\n", node->nodeId, clockStr);
    }

    if (H225_RAS_DEBUG_DETAIL)
    {
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&udpMsg->sourceAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(&udpMsg->destAddr, destIpAddr);

        printf("  SourceAddr    = %s\n"
               "  SourceRasPort = %u\n"
               "  DestAddr      = %s\n"
               "  DestRasPort   = %u\n\n",
               sourceIpAddr,
               udpMsg->sourcePort,
               destIpAddr,
               udpMsg->destPort);
    }

    regList = H225RasSearchReceiverAddr(
                        node,
                        h225RasGatekeeperData,
                        h225LRQ->destinationInfo);

    if (regList)
    {
        H225RasCreateAndSendLRQReply(
            node,
            msg,
            regList->gatekeeperRasTransAddr,
            replyTransAddr,
            regList);
    }

}


static
void H225RasHandleLCF(Node* node, Message* msg)
{
    H225TransportAddress sourceTransAddr;

    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    unsigned int* requestSeqNum =
        (unsigned int*) (MESSAGE_ReturnPacket(msg) + 4);

    H225TransportAddress* receiverCallSignalTransAddr =
        (H225TransportAddress*) (requestSeqNum + 1);

    UdpToAppRecv* udpMsg = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

    H225RasCallEntry* anEntry;
    //int interfaceId;

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u got LCF"
               " at time %s\n", node->nodeId, clockStr);
    }

    sourceTransAddr.ipAddr = GetIPv4Address(udpMsg->destAddr);
    sourceTransAddr.port = udpMsg->destPort;

    anEntry = H225RasSearchCallListForSeqNum(
                node,
                *requestSeqNum);

    if (h323->callModel == Direct)
    {
        H323CopyTransAddr(&anEntry->receiverCallSignalTransAddr,
                          receiverCallSignalTransAddr);
    }

    anEntry->nextCallSignalDeliverAddr = receiverCallSignalTransAddr->ipAddr;

    H225RasCreateAndSendACF(
            node,
            msg,
            sourceTransAddr,
            anEntry->initiatorCallSignalTransAddr,
            *receiverCallSignalTransAddr,
            NULL,
            TRUE);  // TRUE indicates call from LCF
}


//-------------------------------------------------------------------------
// NAME:        H225RasHandleLRJ
//
// PURPOSE:     H225Ras GRQ packet handling routine
//
// PARAMETERS:  node, node which is message/event appears
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
static
void H225RasHandleLRJ(Node* node, Message* msg)
{
    //TBD: not required
}


//-------------------------------------------------------------------------
// NAME:        H225RasHandleARQ
//
// PURPOSE:     H225Ras GRQ packet handling routine
//
// PARAMETERS:  node, node which is message/event appears
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
static
void H225RasHandleARQ(Node* node, Message* msg)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    H225RasGatekeeperData* h225RasGatekeeperData =
        (H225RasGatekeeperData *) h225Ras->h225RasCommon;

    UdpToAppRecv* udpMsg = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

    H225TransportAddress destTransAddr;
    H225TransportAddress gatekeeperRasTransAddr = {0};

    if (h323->endpointType != Gatekeeper)
    {
        ERROR_Assert(FALSE, "Node should be Gatekeeper, Invalid ARQ\n");
    }

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u got ARQ"
               " at time %s\n", node->nodeId, clockStr);
    }

    if (H225_RAS_DEBUG_DETAIL)
    {
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&udpMsg->sourceAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(&udpMsg->destAddr, destIpAddr);

        printf("  SourceAddr    = %s\n"
               "  SourceRasPort = %u\n"
               "  DestAddr      = %s\n"
               "  DestRasPort   = %u\n\n",
               sourceIpAddr,
               udpMsg->sourcePort,
               destIpAddr,
               udpMsg->destPort);
    }

    H225RasSearchGatekeeperAddr(
                              h225RasGatekeeperData,
                              GetIPv4Address(udpMsg->sourceAddr),
                              udpMsg->sourcePort,
                              &gatekeeperRasTransAddr);

    destTransAddr.ipAddr = GetIPv4Address(udpMsg->sourceAddr);
    destTransAddr.port = APP_H225_RAS_UNICAST;

    H225RasCreateAndSendARQReply(
        node,
        msg,
        gatekeeperRasTransAddr,
        destTransAddr);
}


//-------------------------------------------------------------------------
// NAME:        H225RasHandleACF
//
// PURPOSE:     H225Ras GRQ packet handling routine
//
// PARAMETERS:  node, node which is message/event appears
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
static
void H225RasHandleACF(Node* node, Message* msg)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    H225TransportAddress* h225ACF =
        (H225TransportAddress*) (MESSAGE_ReturnPacket(msg));

    UdpToAppRecv* udpMsg = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

    H225TransportAddress* nextDeliverAddr =
        (H225TransportAddress*) ((char*)h225ACF + 16);

    if (h323->endpointType != Terminal)
    {
        ERROR_Assert(FALSE, "Node should be Terminal, Invalid ACF\n");
    }

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u got ACF"
               " at time %s\n", node->nodeId, clockStr);
    }

    if (H225_RAS_DEBUG_DETAIL)
    {
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&udpMsg->sourceAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(&udpMsg->destAddr, destIpAddr);

        printf("  SourceAddr    = %s\n"
               "  SourceRasPort = %u\n"
               "  DestAddr      = %s\n"
               "  DestRasPort   = %u\n\n",
               sourceIpAddr,
               udpMsg->sourcePort,
               destIpAddr,
               udpMsg->destPort);
    }


    if (h323->hostState == H323_RECEIVER)
    {
        H323InitiateAlertingMessage(node, h323->connectionId);
    }
    else
    {   // INITIATOR
        VoipHostData* voip =
            VoipGetInitiatorDataForCallSignal(node, h323->callSignalPort);

        h323->hostState = H323_INITIATOR;
        h323->h323Stat.callInitiated++;

        voip->nextDeliverAddr = nextDeliverAddr->ipAddr;

        if (H225_RAS_DEBUG)
        {
            char buf[MAX_STRING_LENGTH];
            char sourceIpAddr[MAX_STRING_LENGTH];
            char destIpAddr[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(
                voip->callSignalAddr,
                sourceIpAddr);

            IO_ConvertIpAddressToString(
                voip->nextDeliverAddr,
                destIpAddr);

            ctoa(getSimTime(node), buf);

            printf("Node %u initiates to open TCP connection at %s\n"
                   "  SourceAddress      = %s\n"
                   "  Sourceport         = %u\n"
                   "  DestinationAddress = %s\n\n",
                        node->nodeId,
                        buf,
                        sourceIpAddr,
                        voip->callSignalPort,
                        destIpAddr);
        }

        // open TCP connection
        // here dest address (*h225ACF) comes from gatekeeper info list.
        APP_TcpOpenConnection(
            node,
            APP_H323,
            voip->callSignalAddr,
            voip->callSignalPort,
            voip->nextDeliverAddr,
            (short) APP_H323,
            voip->callSignalPort,
            (clocktype) 0);
    }
}


//-------------------------------------------------------------------------
// NAME:        H225RasHandleARJ
//
// PURPOSE:     H225Ras GRQ packet handling routine
//
// PARAMETERS:  node, node which is message/event appears
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
// TBD
static
void H225RasHandleARJ(Node* node, Message* msg)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    UdpToAppRecv* udpMsg = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

    if (h323->endpointType != Terminal)
    {
        ERROR_Assert(FALSE, "Node should be Terminal, Invalid ARJ\n");
    }

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u got ARJ"
               " at time %s\n", node->nodeId, clockStr);
    }

    if (H225_RAS_DEBUG_DETAIL)
    {
        char sourceIpAddr[MAX_STRING_LENGTH];
        char destIpAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&udpMsg->sourceAddr, sourceIpAddr);
        IO_ConvertIpAddressToString(&udpMsg->destAddr, destIpAddr);

        printf("  sourceAddr             = %s\n"
               "  sourcePort             = %u\n"
               "  destAddr               = %s\n"
               "  destPort               = %u\n\n",
               sourceIpAddr,
               udpMsg->sourcePort,
               destIpAddr,
               udpMsg->destPort);
    }
}


//-------------------------------------------------------------------------
// NAME:        H225RasHandleProtocolPacket
//
// PURPOSE:     main H225Ras packet handling routine
//
// PARAMETERS:  node, node which is message/event appears
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
static
void H225RasHandleProtocolPacket(Node* node, Message* msg)
{

    Q931Message* q931Pdu = (Q931Message *) MESSAGE_ReturnPacket(msg);

    switch (Q931MessageGetMsgType(q931Pdu->msgFlgType))
    {
        case GatekeeperRequest:
        {
            H225RasHandleGRQ(node, msg);
            break;
        }

        case GatekeeperConfirm:
        {
            H225RasHandleGCF(node, msg);
            break;
        }

        case GatekeeperReject:
        {
            H225RasHandleGRJ(node, msg);
            break;
        }

        case RegistrationRequest:
        {
            H225RasHandleRRQ(node, msg);
            break;
        }

        case RegistrationConfirm:
        {
            H225RasHandleRCF(node, msg);
            break;
        }

        case RegistrationReject:
        {
            H225RasHandleRRJ(node, msg);
            break;
        }

        case LocationRequest:
        {
            H225RasHandleLRQ(node, msg);
            break;
        }

        case LocationConfirm:
        {
            H225RasHandleLCF(node, msg);
            break;
        }

        case LocationReject:
        {
            H225RasHandleLRJ(node, msg);
            break;
        }

        case AdmissionRequest:
        {
            H225RasHandleARQ(node, msg);
            break;
        }

        case AdmissionConfirm:
        {
            H225RasHandleACF(node, msg);
            break;
        }

        case AdmissionReject:
        {
            H225RasHandleARJ(node, msg);
            break;
        }

        default:
        {
#ifndef EXATA
            ERROR_Assert(FALSE, "Unknown RAS message\n");
#endif
        }
    }
}


//-------------------------------------------------------------------------
// NAME:        H225RasHandleGRQPeriodicRefreshDelay
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasHandleGRQPeriodicRefreshDelay(Node* node, Message* msg)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225RasTerminalData* h225RasTerminalData =
        (H225RasTerminalData *) h225Ras->h225RasCommon;

    H225SourceDestTransAddrInfo* sdInfo =
        (H225SourceDestTransAddrInfo*) MESSAGE_ReturnInfo(msg);

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u got MSG_APP_H225_RAS_GRQ_PeriodicRefreshTimer"
               " event at time %s\n", node->nodeId, clockStr);
    }

    H225RasCreateAndSendGRQ(
        node,
        sdInfo->sourceTransAddr,
        sdInfo->destTransAddr);

    h225RasTerminalData->GRQRetryCount = 0;

    H225RasCreateGRQTimerMsg(
        node,
        sdInfo->sourceTransAddr,
        sdInfo->destTransAddr);
}


//-------------------------------------------------------------------------
// NAME:        H225RasHandleGRQTimer.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasHandleGRQTimer(Node* node, Message* msg)
{
    H225RasData* h225Ras;
    H323Data* h323;
    H225RasTerminalData* h225RasTerminalData;

    H225SourceDestTransAddrInfo* sdInfo =
        (H225SourceDestTransAddrInfo*) MESSAGE_ReturnInfo(msg);

    h323 = (H323Data*) node->appData.multimedia->sigPtr;
    h225Ras = (H225RasData*) h323->h225Ras;
    h225RasTerminalData = (H225RasTerminalData *) h225Ras->h225RasCommon;

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u got MSG_APP_H225_RAS_GatekeeperRequestTimer"
               " event at time %s\n", node->nodeId, clockStr);
    }

    if (h225RasTerminalData->h225RasGotGateKeeperResponse == FALSE)
    {
        if (h225RasTerminalData->GRQRetryCount < GRQ_MAX_RETRY_COUNT)
        {
            H225RasCreateAndSendGRQ(
                node,
                sdInfo->sourceTransAddr,
                sdInfo->destTransAddr);

            h225RasTerminalData->GRQRetryCount++;

            H225RasCreateGRQTimerMsg(
                node,
                sdInfo->sourceTransAddr,
                sdInfo->destTransAddr);
        }
        else
        {
            H225RasSendGRQPeriodicRefreshTimer(
                node,
                sdInfo->sourceTransAddr,
                sdInfo->destTransAddr);
        }
    }
}


//-------------------------------------------------------------------------
// NAME:        H225RasHandleRRQTimer.
//
// PURPOSE:
//
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H225RasHandleRRQTimer(Node* node, Message* msg)
{
    H225RasData* h225Ras;
    H323Data* h323;
    H225RasTerminalData* h225RasTerminalData;

    H225SourceDestTransAddrInfo* sdInfo =
        (H225SourceDestTransAddrInfo*) MESSAGE_ReturnInfo(msg);

    h323 = (H323Data*) node->appData.multimedia->sigPtr;
    h225Ras = (H225RasData*) h323->h225Ras;
    h225RasTerminalData = (H225RasTerminalData *) h225Ras->h225RasCommon;

    if (H225_RAS_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(getSimTime(node), clockStr);

        printf("Node %u got MSG_APP_H225_RAS_RegistrationRequestTimer"
               " event at time %s\n", node->nodeId, clockStr);
    }

    if (h225RasTerminalData->h225RasGotRegistrationResponse == FALSE
        && h225RasTerminalData->RRQRetryCount < RRQ_MAX_RETRY_COUNT)
    {
        h225RasTerminalData->RRQRetryCount++;

        H225RasCreateAndSendRRQ(
            node,
            msg,
            sdInfo->sourceTransAddr,
            sdInfo->destTransAddr);

        H225RasCreateRRQTimerMsg(
            node,
            msg,
            sdInfo->sourceTransAddr,
            sdInfo->destTransAddr);
    }
}


//-------------------------------------------------------------------------
// NAME:        H225RasLayer
//
// PURPOSE:     main H323 message and event handling routine
//
// PARAMETERS:  node, node which is message/event appears
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
void H225RasLayer(Node* node, Message* msg)
{
    switch (MESSAGE_GetEvent(msg))
    {

        case MSG_APP_FromTransport:
        {
            //RAS message processing
            H225RasHandleProtocolPacket(node, msg);
            break;
        }

        case MSG_APP_H225_RAS_GRQ_PeriodicRefreshTimer:
        {
            H225RasHandleGRQPeriodicRefreshDelay(node, msg);
            break;
        }

        case MSG_APP_H225_RAS_GatekeeperRequestTimer:
        {
            H225RasHandleGRQTimer(node, msg);
            break;
        }

        case MSG_APP_H225_RAS_RegistrationRequestTimer:
        {
            H225RasHandleRRQTimer(node, msg);
            break;
        }

        default:
        {
            ERROR_Assert(FALSE, "Unknown message\n");
        }
    }
    MESSAGE_Free(node, msg);
}


///////////////////////////////////////////////////////////////////////////
/////////////////////////////// RAS finalize //////////////////////////////
///////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------------
// NAME:        H225RasFinalize
//
// PURPOSE:     Printing statistical information
//
// PARAMETERS:  node, node which is printing the statistical information
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
void H225RasFinalize(Node* node)
{
    char buf[MAX_STRING_LENGTH];

    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225RasStat* h225Stat = h225Ras->h225RasStat;

    sprintf(buf, "Number Of GRQ Sent = %d",
            h225Stat->gatekeeperRequested);

    IO_PrintStat(
        node,
        "Application",
        "H225Ras",
        ANY_DEST,
        -1, // instance Id
        buf);


    sprintf(buf, "Number Of GCF Sent = %d",
            h225Stat->gatekeeperConfirmed);

    IO_PrintStat(
        node,
        "Application",
        "H225Ras",
        ANY_DEST,
        -1, // instance Id
        buf);


    sprintf(buf, "Number Of GRJ Sent = %d",
            h225Stat->gatekeeperRejected);

    IO_PrintStat(
        node,
        "Application",
        "H225Ras",
        ANY_DEST,
        -1, // instance Id
        buf);

    sprintf(buf, "Number Of RRQ Sent = %d",
            h225Stat->registrationRequested);

    IO_PrintStat(
        node,
        "Application",
        "H225Ras",
        ANY_DEST,
        -1, // instance Id
        buf);

    sprintf(buf, "Number Of RCF Sent = %d",
            h225Stat->registrationConfirmed);

    IO_PrintStat(
        node,
        "Application",
        "H225Ras",
        ANY_DEST,
        -1, // instance Id
        buf);


    sprintf(buf, "Number Of RRJ Sent = %d",
            h225Stat->registrationRejected);


    IO_PrintStat(
        node,
        "Application",
        "H225Ras",
        ANY_DEST,
        -1, // instance Id
        buf);


    sprintf(buf, "Number Of LRQ Sent = %d",
            h225Stat->locationRequested);

    IO_PrintStat(
        node,
        "Application",
        "H225Ras",
        ANY_DEST,
        -1, // instance Id
        buf);


    sprintf(buf, "Number Of LCF Sent = %d",
            h225Stat->locationConfirmed);

    IO_PrintStat(
        node,
        "Application",
        "H225Ras",
        ANY_DEST,
        -1, // instance Id
        buf);

    sprintf(buf, "Number Of LRJ Sent = %d",
            h225Stat->locationRejected);

    IO_PrintStat(
        node,
        "Application",
        "H225Ras",
        ANY_DEST,
        -1, // instance Id
        buf);


    sprintf(buf, "Number Of ARQ Sent = %d",
            h225Stat->admissionRequested);

    IO_PrintStat(
        node,
        "Application",
        "H225Ras",
        ANY_DEST,
        -1, // instance Id
        buf);


    sprintf(buf, "Number Of ACF Sent = %d",
            h225Stat->admissionConfirmed);

    IO_PrintStat(
        node,
        "Application",
        "H225Ras",
        ANY_DEST,
        -1, // instance Id
        buf);


    sprintf(buf, "Number Of ARJ Sent = %d",
            h225Stat->admissionRejected);

    IO_PrintStat(
        node,
        "Application",
        "H225Ras",
        ANY_DEST,
        -1, // instance Id
        buf);

    if (H225_RAS_DEBUG)
    {
        H225RasPrintEndpoint(node);
    }
}

