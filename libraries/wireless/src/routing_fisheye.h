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

//
// File: fisheye.h
// This is the implementation of Fisheye routing protocol
// Reference: "Fisheye State Routing Protocol (FSR) for Ad Hoc Networks"
//            IETF internet draft <draft-ietf-manet-fsr-03.txt>
//            by Mario Gerla, Xiaoyan Hong, Guangyu Pei.
//

#ifndef _FISHEYE_H_
#define _FISHEYE_H_

// ---------------------------------------------------------------------------
//   DEFINES (none)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//   STRUCTS, ENUMS, AND TYPEDEFS (none)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//   INLINED FUNCTIONS (none)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//   PROTOTYPES FOR FUNCTIONS WITH EXTERNAL LINKAGE
// ---------------------------------------------------------------------------

void
RoutingFisheyeInit(Node *node,const NodeInput *nodeInput);

void
RoutingFisheyeLayer(Node *node, Message *msg);

void
RoutingFisheyeFinalize(Node *node);

#endif  // _FISHEYE_H_
