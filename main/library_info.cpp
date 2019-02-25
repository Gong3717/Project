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
// PACKAGE :: Library Information
// DESCRIPTION :: This file describes the APIs used to determined some
//                library related information.
// **/

#include <stdio.h>
#include <stdlib.h>

#include "library_info.h"

// /**
// FUNCTION     :: INFO_MultimediaEnterpriseLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether MultiMedia & Enterprise Library is enabled
//                 in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_MultimediaEnterpriseLibCompiled()
{
#ifdef ENTERPRISE_LIB
    return TRUE;
#else
    return FALSE;
#endif
}

// /**
// FUNCTION     :: INFO_WirelessLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Wireless Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_WirelessLibCompiled()
{
#ifdef WIRELESS_LIB
    return TRUE;
#else
    return FALSE;
#endif
}

// /**
// FUNCTION     :: INFO_AdvancedWirelessLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Advanced Wireless Library is enabled
//                 in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_AdvancedWirelessLibCompiled()
{
#ifdef ADVANCED_WIRELESS_LIB
    return TRUE;
#else
    return FALSE;
#endif
}

// /**
// FUNCTION     :: INFO_AleAsapsLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Ale/Asaps Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_AleAsapsLibCompiled()
{
#ifdef ALE_ASAPS_LIB
    return TRUE;
#else
    return FALSE;
#endif
}

// /**
// FUNCTION     :: INFO_CellularLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Cellular Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_CellularLibCompiled()
{
#ifdef CELLULAR_LIB
    return TRUE;
#else
    return FALSE;
#endif
}

// /**
// FUNCTION     :: INFO_CyberLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Cyber Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_CyberLibCompiled()
{
#ifdef CYBER_LIB
    return TRUE;
#else
    return FALSE;
#endif
}

// /**
// FUNCTION     :: INFO_SatelliteLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Satellite Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_SatelliteLibCompiled()
{
#ifdef SATELLITE_LIB
    return TRUE;
#else
    return FALSE;
#endif
}

// /**
// FUNCTION     :: INFO_SensorNetworksLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Sensor Networks Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_SensorNetworksLibCompiled()
{
#ifdef SENSOR_NETWORKS_LIB
    return TRUE;
#else
    return FALSE;
#endif
}

// /**
// FUNCTION     :: INFO_TiremLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether TIREM Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_TiremLibCompiled()
{
#ifdef TIREM_LIB
    return TRUE;
#else
    return FALSE;
#endif
}

// /**
// FUNCTION     :: INFO_UmtsLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether UMTS Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_UmtsLibCompiled()
{
#ifdef UMTS_LIB
    return TRUE;
#else
    return FALSE;
#endif
}

// /**
// FUNCTION     :: INFO_UrbanPropLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Urban Propagation Library is enabled
//                 in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_UrbanPropLibCompiled()
{
#ifdef URBAN_LIB
    return TRUE;
#else
    return FALSE;
#endif
}

// /**
// FUNCTION     :: INFO_MilitaryRadiosLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Military Radios Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_MilitaryRadiosLibCompiled()
{
#ifdef MILITARY_RADIOS_LIB
    return TRUE;
#else
    return FALSE;
#endif
}

// /**
// FUNCTION     :: INFO_DisInterfaceCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether DIS interface is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_DisInterfaceCompiled()
{
#ifdef DIS_INTERFACE
    return TRUE;
#else
    return FALSE;
#endif
}

// /**
// FUNCTION     :: INFO_HlaInterfaceCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether HLA interface is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_HlaInterfaceCompiled()
{
#ifdef HLA_INTERFACE
    return TRUE;
#else
    return FALSE;
#endif
}

// /**
// FUNCTION     :: INFO_AgiInterfaceCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether AGI interface is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_AgiInterfaceCompiled()
{
#ifdef AGI_INTERFACE
    return TRUE;
#else
    return FALSE;
#endif
}

// /**
// FUNCTION     :: INFO_LteCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether LTE is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_LteCompiled()
{
#ifdef LTE_LIB
    return TRUE;
#else
    return FALSE;
#endif
}
