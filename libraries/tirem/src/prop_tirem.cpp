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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "prop_tirem.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG (1)

#ifndef _WIN32

//
// Fortran Initialization
//
void f_init ();
#else
#include <windows.h>
HINSTANCE tiremLib;
CalcTiremLossFuncType CalcTiremLoss;
GetTiremNumFuncType GetTiremNum;
#endif

// Longley-Rice / TIREM related parameters:
//
//                               Climate Refractivity
// Equatorial                    1       360
// Continental Subtropical       2       320
// Maritime Tropical             3       370
// Desert                        4       280
// Continental Temperate         5       301
// Maritime Temperate, Over Land 6       320
// Maritime Temperate, Over Sea  7       350
//
//                Dielectric Constant Ground Conductivity
//                (Permittivity)
// Average Ground 15                  0.005
// Poor Ground     4                  0.001
// Good Ground    25                  0.02
// Fresh Water    81                  0.01
// Salt Water     81                  5.0


void TiremInitialize(
    PropChannel *propChannel,
    int channelIndex,
    const NodeInput *nodeInput)
{
    PropProfile* propProfile = propChannel->profile;
    BOOL wasFound;
    double elevationSamplingDistance;
    int climate;
    double refractivity;
    double conductivity;
    double permittivity;
    double humidity;
    BOOL horizontalPolarization;
    char polarization[MAX_STRING_LENGTH];
    char errorStr[MAX_STRING_LENGTH];
    char dllFilename[MAX_STRING_LENGTH];

    static int init_libs = 1;

    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-SAMPLING-DISTANCE",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        &elevationSamplingDistance);

    if (wasFound) {
        propProfile->elevationSamplingDistance =
            (float)elevationSamplingDistance;
    }
    else {
        propProfile->elevationSamplingDistance =
            DEFAULT_SAMPLING_DISTANCE;
    }

    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-REFRACTIVITY",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        &refractivity);

    if (wasFound) {
        propProfile->refractivity = refractivity;
    }
    else {
        propProfile->refractivity = DEFAULT_REFRACTIVITY;
    }

    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-CONDUCTIVITY",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        &conductivity);

    if (wasFound) {
        propProfile->conductivity = conductivity;
    }
    else {
        propProfile->conductivity = DEFAULT_CONDUCTIVITY;
    }

    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-PERMITTIVITY",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        &permittivity);

    if (wasFound) {
        propProfile->permittivity = permittivity;
    }
    else {
        propProfile->permittivity = DEFAULT_PERMITTIVITY;
    }

    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-HUMIDITY",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        &humidity);

    if (wasFound) {
        propProfile->humidity = humidity;
    }
    else {
        propProfile->humidity = DEFAULT_HUMIDITY;
    }

    IO_ReadIntInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-CLIMATE",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        &climate);

    if (wasFound) {
        assert(climate >= 1 && climate <= 7);
        propProfile->climate = climate;
    }
    else {
        propProfile->climate = DEFAULT_CLIMATE_NUM;
    }

    //
    // This must be an antenna property..
    //
    IO_ReadStringInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "ANTENNA-POLARIZATION",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        polarization);

    if (wasFound) {
        if (strcmp(polarization, "HORIZONTAL")) {
            propProfile->polarization = HORIZONTAL;
            propProfile->polarizationString = "H   ";
        }
        else if (strcmp(polarization, "VERTICAL")) {
            propProfile->polarization = VERTICAL;
            propProfile->polarizationString = "V   ";
        }
        else {
            ERROR_ReportError("Unrecognized polarization\n");
        }
    }
    else {
        propProfile->polarization = VERTICAL;
        propProfile->polarizationString = "V   ";
    }

    if (init_libs) {
#ifndef _WIN32
        if (DEBUG) {
            printf("f_init()\n");
        }
        f_init();
#else
        IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "TIREM-DLL-FILENAME",
            &wasFound,
            dllFilename);

        if (!wasFound)
        {
            strcpy(dllFilename, DEFAULT_TIREM_DLL_FILENAME);
        }
        tiremLib = LoadLibrary(dllFilename);
        if (tiremLib != NULL)
        {
            CalcTiremLoss = (CalcTiremLossFuncType)GetProcAddress(tiremLib, "CalcTiremLoss");
            GetTiremNum = (GetTiremNumFuncType)GetProcAddress(tiremLib, "GetTiremNum");
            if ((CalcTiremLoss == NULL) || (GetTiremNum == NULL))
            {
                ERROR_ReportError(
                    "TIREM Init: Unable to GetProcAddress for CalcTiremLoss");
            }
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr, "TIREM Init: Unable to LoadLibrary \"%s\"", dllFilename);
            ERROR_ReportError(errorStr);
        }
#endif
        init_libs = 0;
    }

    if ((propProfile->refractivity < 200) ||
        (propProfile->refractivity > 450))
    {
        sprintf(errorStr, "Surface refractivity out of range: %f\n",
                refractivity);
        ERROR_ReportError(errorStr);
    }

    if ((propProfile->conductivity < .00001) ||
        (propProfile->conductivity > 100))
    {
        sprintf(errorStr, "Conductivity out of range: %f\n", conductivity);
        ERROR_ReportError(errorStr);
    }

    return;
}

double PathlossTirem(
    long numSamples,
    float distance,
    double elevationArray[],
    float txAntennaHeight,
    float rxAntennaHeight,
    char polarizationString[],
    float humidity,
    float permittivity,
    float conductivity,
    float frequency,
    float refractivity)
{
    float f_elevationArray[MAX_NUM_ELEVATION_SAMPLES];
    float distances[MAX_NUM_ELEVATION_SAMPLES];
    int j;
    long extnsn = 0;   // not a reused path
                       // NOTE:  DO NOT CHANGE THIS VALUE

    // TIREM outputs
    char version[8];    /* tirem version */
    char mode[4];      /* operation mode */
    float path_loss = 0.0,
          freespace_loss = 0.0;

    /*
     *
     * Check the frequency range before passing to tirem3() since tirem3()
     * does NO range checking and will crash if an invalid value is passed.
     * These ranges were taken from Table 1. TIREM Input Argument
     * List Variables found in the TIREM/SEM Programmer's Reference Manual
     * (Supplement 1) dated June 1993
     *
     */
    ERROR_Assert( ((txAntennaHeight >= 0) && (txAntennaHeight <= 30000) &&
                   (rxAntennaHeight >= 0) && (rxAntennaHeight <= 30000)),
                  "Antenna height out of range (<0 or > 30000m)");

    ERROR_Assert( ((frequency >= 1) && (frequency <= 20000)),
                  "Center frequency out of range (<1 or >20000)");
    for (j = 0; j < numSamples; j++)
    {
        f_elevationArray[j] = (float) elevationArray[j];
        distances[j] = distance * ((float) j / (numSamples - 1));
    }
#ifdef _WIN32
    (CalcTiremLoss)(
#else
    tirem3_(
#endif
    &txAntennaHeight,
    &rxAntennaHeight,
    &frequency,
    &numSamples,
    f_elevationArray,
    distances,
    &extnsn,
    &refractivity,
    &conductivity,
    &permittivity,
    &humidity,
    polarizationString,
    version,
    mode,
    &path_loss,
    &freespace_loss);

    return (double) path_loss;
}

#ifdef __cplusplus
}
#endif
