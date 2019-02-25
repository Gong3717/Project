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

// /**
// PROTOCOL     :: SIP
//
// SUMMARY      :: Session Initiation Protocol(SIP) is the signalling
//                 protocol used to set up, modify and terminate multimedia
//                 sessions over TCP/IP network. It is highly scalable,
//                 flexible to extension and simpler than similar protocols.
//
// LAYER        :: Application
//
// STATISTICS   ::
// + inviteSent  : Number of INVITE requests sent
// + ackSent     : Number of ACK requests sent
// + byeSent     : Number of BYE requests sent
//
// + inviteRecd  : Number of INVITE requests received
// + ackRecd     : Number of ACK requests received
// + byeRecd     : Number of BYE requests received
//
// + tryingSent  : Number of TRYING responses sent
// + ringingSent : Number of RINGING responses sent
// + okSent      : Number of OK responses sent
//
// + tryingRecd  : Number of TRYING responses received
// + ringingRecd : Number of RINGING responses received
// + okRecd      : Number of OK responses received
//
// + reqForwarded  : Number of requests forwarded
// + reqRedirected : Number of requests redireced
// + reqDropped    : Number of requests dropped
//
// + respForwarded : Number of responses forwarded
// + respDropped   : Number of responses dropped
//
// + callAttempted : Number of calls/sessions attempted
// + callConnected : Number of calls/sessions successfully connected
// + callRejected  : Number of calls/sessions rejected

// CONFIG_PARAM ::
// + MULTIMEDIA-SIGNALLING-PROTOCOL : SIP|H323 : This parameter selects
// one of the available signalling protocols
// + SIP-PROXYLIST : list of int : Comma delimited list of Proxy servers
// + SIP-TRANSPORT-LAYER-PROTOCOL : TCP|UDP|SCTP : selects one of
// transport protocols.
// + SIP-CALL-MODEL : DIRECT|PROXY-ROUTED : Specifies whether the call
// proceeds directly to the called party bypassing proxies or goes
// through the proxies.
// + SIP-STATISTICS : BOOLEAN : Selects if the statistics generated after
// simulation is to be printed or not.
// + TERMINAL-ALIAS-ADDRESS-FILE : TERMINAL-ALIAS-ADDRESS-FILE : Specifies
// the file containing details of the sip nodes
//
// VALIDATION   ::
//
// IMPLEMENTED_FEATURES ::
// + Transport layer protocol : SIP implemented over TCP
// + Two modes of call Establishment : SIP can route calls directly to
// the callee or it can be routed through proxy server
//
// OMITTED_FEATURES     ::
// + Transport layer protocol : UDP/ SCTP are not implemented
// + CANCEL, REGISTER, OPT : These three requests not implemented
// + Inter-ProxyServer Routing : Routing in between two proxy servers
//
// ASSUMPTIONS  ::
// + The users are not mobile,i.e. they are hooked to a single host or
// IP address and it is read initially from the above alias address file
//
// RELATED  ::
// H.323, RTP, RTCP, MGCP
// **/

#ifndef SIP_H
#define SIP_H

#include "app_util.h"
#include "multimedia_sipdata.h"
#include "multimedia_sipsession.h"

//---------------------------------------------------------------------------
// PROTOTYPES FOR INTERFACES(WITH EXTERNAL LINKAGE)
//---------------------------------------------------------------------------
// /**
// FUNCTION   :: SipInit
// LAYER      :: APPLICATION
// PURPOSE    :: Initializes the nodes running SIP protocol
// PARAMETERS ::
// + node      : Node* : Pointer to the node to be initialized
// + nodeInput : NodeInput* : Pointer to input configuration file
// + hostType  : SipHostType : Type of Sip Host
// RETURN     :: void : NULL
// **/
void        SipInit(Node* node, const NodeInput *nodeInput,
                    SipHostType hostType);


// /**
// FUNCTION   :: SipProxyInit
// LAYER      :: APPLICATION
// PURPOSE    :: Initializes the SIP proxy nodes
// PARAMETERS ::
// + firstNode : Node* : Pointer to the first node
// + nodeInput : NodeInput* : Pointer to input configuration file
// RETURN     :: void  : NULL
// **/
void        SipProxyInit(Node* firstNode, const NodeInput* nodeInput);


// /**
// FUNCTION   :: SipProcessEvent
// LAYER      :: APPLICATION
// PURPOSE    :: Handles the SIP related events generated during simulation
// PARAMETERS ::
// + node      : Node* : Pointer to the node that receives the event
// + msg       : Message* : Pointer to the received message
// RETURN     :: void : NULL
// **/
void        SipProcessEvent(Node* node, Message* msg);


// /**
// FUNCTION   :: SipFinalize
// LAYER      :: APPLICATION
// PURPOSE    :: Prints the statistics data from simulation for a node
// PARAMETERS ::
// + node      : Node* : Pointer to the node getting finalized
// RETURN     :: void : NULL
// **/
void        SipFinalize(Node* node);


// /**
// FUNCTION   :: SipInitiateConnection
// LAYER      :: APPLICATION
// PURPOSE    :: Interface, for the third party application(now VoIP)
// using which a SIP node is informed of desire for call initiation
// PARAMETERS ::
// + node      : Node* : Pointer to the calling node
// + voipData  : void* : Pointer to calling application's data struct.
// RETURN     :: void : NULL
// **/
void        SipInitiateConnection(Node* node, void* voipData);


// /**
// FUNCTION   :: SipTerminateConnection
// LAYER      :: APPLICATION
// PURPOSE    :: Interface, for the third party application(now VoIP)
// using which a SIP node is informed of desire for call termination.
// PARAMETERS ::
// + node      : Node* : Pointer to the node wishing to terminate call
// + voipData  : void* : Pointer to calling application's data struct.
// RETURN     :: void : NULL
// **/
void        SipTerminateConnection(Node* node, void* voipData);

struct SipCallInfo
{
    char        from[SIP_STR_SIZE64];
    char        to[SIP_STR_SIZE64];
    char        callId[SIP_STR_SIZE64];
    int connectionId;
};

#endif  /* SIP_H */
