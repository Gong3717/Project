#ifndef _EPC_FX_H_
#define _EPC_FX_H_

#include "node.h"
#include "epc_fx_common.h"

// /**
// FUNCTION   :: EpcFxGetEpcData
// LAYER      :: EPC
// PURPOSE    :: get EPC data
// PARAMETERS ::
// + node      : Node*       : Pointer to node.
// RETURN     :: EpcData*    : EPC data on specified node
// **/
EpcFxData*
EpcFxGetEpcData(Node* node);

// /**
// FUNCTION   :: EpcFxInit
// LAYER      :: EPC
// PURPOSE    :: initialize EPC
// PARAMETERS ::
// + node                   : Node*       : Pointer to node.
// + interfaceIndex         : int         : interface index
// + sgwmmeNodeId           : NodeAddress : node ID of SGW/MME
// + sgwmmeInterfaceIndex   : int         : interface index of SGW/MME
// + tagetNodeId      : NodeAddress : Node ID
// RETURN     :: void : NULL
// **/
void
EpcFxInit(Node* node, int interfaceIndex,
	NodeAddress sgwmmeNodeId, int sgwmmeInterfaceIndex);

// /**
// FUNCTION   :: EpcFxFinalize
// LAYER      :: EPC
// PURPOSE    :: Finalize EPC
// PARAMETERS ::
// + node      : Node*       : Pointer to node.
// RETURN     :: void        : NULL
// **/
void
EpcFxFinalize(Node* node);

// /**
// FUNCTION   :: EpcFxProcessAttachUE
// LAYER      :: EPC
// PURPOSE    :: process for message AttachUE
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + ueRnti    : const fxRnti& : UE
// + enbRnti   : const fxRnti& : eNodeB
// RETURN     :: void : NULL
// **/
void
EpcFxProcessAttachUE(Node* node,
	const fxRnti& ueRnti, const fxRnti& enbRnti);
// /**
// FUNCTION   :: EpcFxProcessDetachUE
// LAYER      :: EPC
// PURPOSE    :: process for message DetachUE
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + ueRnti    : const fxRnti& : UE
// + enbRnti   : const fxRnti& : eNodeB
// RETURN     :: void : NULL
// **/
void
EpcFxProcessDetachUE(Node* node,
	const fxRnti& ueRnti, const fxRnti& enbRnti);

// /**
// FUNCTION   :: EpcFxProcessPathSwitchRequest
// LAYER      :: EPC
// PURPOSE    :: process for message DetachUE
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + hoParticipator   : const FXHandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void
EpcFxProcessPathSwitchRequest(
	Node* node,
	const FXHandoverParticipator& hoParticipator);

// /**
// FUNCTION   :: EpcFxSwitchDLPath
// LAYER      :: EPC
// PURPOSE    :: switch DL path
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + hoParticipator   : const FXHandoverParticipator&: participators of H.O.
// RETURN     :: BOOL : result
// **/
BOOL
EpcFxSwitchDLPath(
	Node* node,
	const FXHandoverParticipator& hoParticipator);
// /**
// FUNCTION   :: EpcFxSendPathSwitchRequestAck
// LAYER      :: EPC
// PURPOSE    :: send path switch request ack
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + hoParticipator   : const FXHandoverParticipator&: participators of H.O.
// + result           : BOOL     : path switch result
// RETURN     :: void : NULL
// **/
void
EpcFxSendPathSwitchRequestAck(
	Node* node,
	const FXHandoverParticipator& hoParticipator,
	BOOL result);

#endif    // _EPC_Fx_H_