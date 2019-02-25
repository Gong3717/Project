// Copyright (c) 2001-2008, Scalable Network Technologies, Inc.  All Rights Reserved.
//                          6100 Center Drive West
//                          Suite 1250
//                          Los Angeles, CA 90045
//                          sales@scalable-networks.com
//
// This source code is licensed, not sold, and is subject to a written
// license agreement.  Among other things, no portion of this source
// code may be copied, transmitted, disclosed, displayed, distributed,
// translated, used as the basis for a derivative work, or used, in
// whole or in part, for any program or purpose other than its intended
// use in compliance with the license agreement as part of the EXata
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.
#ifndef NETWORK_IPFILTER_H
#define NETWORK_IPFILTER_H

#define MAX_FILTERS 10
enum {
    NetworkIp_Filter_Visualizer = 0x1,
    NetworkIpFilter_PAS = 0x2,
    NetworkIpFilter_Drop = 0x4
};

typedef struct {
    NodeAddress srcAddress;
    NodeAddress destAddress;
    int filterType;
} NetworkIpFilter;


BOOL NetworkIpAddFilter(
    Node *node,
    NodeAddress sourceAddress,
    NodeAddress destAddress,
    int filterType);

void NetworkIpRemoveFilter(
    Node *node,
    NodeAddress sourceAddress,
    NodeAddress destAddress,
    int filterType);

BOOL NetworkIpEmptyFilter(
    Node *node);

BOOL NetworkIpCheckFilter(
    Node *node,
    NodeAddress sourceAddress,
    NodeAddress destAddress,
    int *filterType);

#endif
