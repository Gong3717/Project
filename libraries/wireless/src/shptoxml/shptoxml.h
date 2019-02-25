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

#ifndef SHPTOXML_H
#define SHPTOXML_H

#include <cfloat>

void
ProcessCommandLineArguments(
    ShptoxmlData& sxData, unsigned argc, char* argv[]);

void
ShowUsage();

bool
ParseStringArg(
    int i,
    int argc,
    const char* arg,
    char* value,
    size_t bufSize);

bool
ParseUintArg(
    int i,
    int argc,
    const char* arg,
    unsigned &value);

bool
ParseDblArg(
    int i,
    int argc,
    const char* arg,
    double &value,
    double min = -DBL_MAX,
    double max = DBL_MAX);

#endif /* SHPTOXML_H */
