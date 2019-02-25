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

#ifndef _BELLMANFORD_H_
#define _BELLMANFORD_H_

//-----------------------------------------------------------------------------
// DEFINES (none)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// STRUCTS, ENUMS, AND TYPEDEFS (none)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// INLINED FUNCTIONS (none)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PROTOTYPES FOR FUNCTIONS WITH EXTERNAL LINKAGE
//-----------------------------------------------------------------------------

void
RoutingBellmanfordInit(Node *node);

void
RoutingBellmanfordLayer(Node *node, Message *msg);

void
RoutingBellmanfordFinalize(Node *node, int interfaceIndex);

/*
// FUNCTION   :: RoutingBellmanfordInitTrace
// PURPOSE    :: Bellmanford initialization  for tracing
// PARAMETERS ::
// + node : Node* : Pointer to node
// + nodeInput    : const NodeInput* : Pointer to NodeInput
// RETURN ::  void : NULL
*/
void RoutingBellmanfordInitTrace(Node* node, const NodeInput* nodeInput);

//Interface Utility Functions
//
//
// FUNCTION     BellmanfordHookToRedistribute
// PURPOSE      Added for route redistribution
// Parameters:
//      node:
//      destAddress:
//      destAddressMask:
//      nextHopAddress:
//      interfaceIndex:
//      routeCost:
//      Changed by C Shaw
void BellmanfordHookToRedistribute(
        Node* node,
        NodeAddress destAddress,
        NodeAddress destAddressMask,
        NodeAddress nextHopAddress,
        int interfaceIndex,
        void* routeCost);

//
// FUNCTION   :: RoutingBellmanfordPrintTraceXML
// PURPOSE    :: Print packet trace information in XML format
// PARAMETERS ::
// + node     : Node*    : Pointer to node
// + msg      : Message* : Pointer to packet to print headers from
// RETURN     ::  void   : NULL
//
void RoutingBellmanfordPrintTraceXML(Node* node, Message* msg);

#endif // _BELLMANFORD_H_
