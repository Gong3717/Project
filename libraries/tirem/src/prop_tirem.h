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


#ifndef TIREM_LIB_H
#define TIREM_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#define HORIZONTAL 0
#define VERTICAL 1

#define DEFAULT_SAMPLING_DISTANCE 100.0

#define DEFAULT_CLIMATE_NUM  1
#define DEFAULT_REFRACTIVITY 360
#define DEFAULT_PERMITTIVITY 15
#define DEFAULT_CONDUCTIVITY 0.005
#define DEFAULT_HUMIDITY     10

#define DEFAULT_TIREM_DLL_FILENAME "TIREM315DLL.DLL"
//
// TIREM path loss calculation
//
#ifdef _WIN32

typedef void (*CalcTiremLossFuncType)(
    float*,
    float*,
    float*,
    long*,
    float*,
    float*,
    long*,
    float*,
    float*,
    float*,
    float*,
    char*,
    char*,
    char*,
    float*,
    float*);

typedef void (*GetTiremNumFuncType)(
    char*);

extern CalcTiremLossFuncType CalcTiremLoss;
extern GetTiremNumFuncType GetTiremNum;

#else
int tirem3_ (float *tantht, float *rantht, float *propfq, long *numelv,
             float *elev, float *dist, long *extnsn, float *refrac,
             float *conduc, float *permit, float *humid, char *polarz,
             char *vrsion, char *mode, float *ploss, float *fsloss);
#endif

void TiremInitialize(
    PropChannel *propChannel,
    int channelIndex,
    const NodeInput *nodeInput);

double PathlossTirem(
    long numSamples,
    float distanceBetwSamples,
    double elevationArray[],
    float txAntennaHeight,
    float rxAntennaHeight,
    char polarizationString[],
    float humidity,
    float permittivity,
    float conductivity,
    float frequency,
    float refractivity);


#ifdef __cplusplus
}
#endif

#endif /*TIREM_LIB_H*/
