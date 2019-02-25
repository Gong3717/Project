#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network_ip.h"

#include "fx_common.h"

void FxSmacAddStatsDBInfo(Node* node,
	                     Message* msg)
{
	FxStatsDbSduPduInfo* info = (FxStatsDbSduPduInfo*)MESSAGE_ReturnInfo(
		msg,
		(UInt16)INFO_TYPE_FxStatsDbSduPduInfo);

	if (info)
	{
		// info already exist, no need to add again
		return;
	}

	info = (FxStatsDbSduPduInfo*)MESSAGE_AddInfo(
		node,
		msg,
		sizeof(FxStatsDbSduPduInfo),
		(UInt16)INFO_TYPE_FxStatsDbSduPduInfo);

	ERROR_Assert(info != NULL,
		"INFO_TYPE_FxStatsDbSduPduInfo adding failed");

	info->dataSize = 0;
	info->ctrlSize = 0;
	info->isCtrlPacket = false;
}

void FxSmacUpdateStatsDBInfo(Node* node,
	Message* msg,
	UInt32 dataSize,
	UInt32 ctrlSize,
	BOOL isCtrlPacket)
{
	FxStatsDbSduPduInfo* info = (FxStatsDbSduPduInfo*)MESSAGE_ReturnInfo(
		msg,
		(UInt16)INFO_TYPE_FxStatsDbSduPduInfo);


	if (!info)
	{
		char errStr[MAX_STRING_LENGTH];
		sprintf(errStr, "INFO_TYPE_FxStatsDbSduPduInfo not found while "
			" update, on Node %d.\n",
			node->nodeId);
		ERROR_ReportWarning(errStr);
		return;
	}

	info->dataSize = dataSize;
	info->ctrlSize = ctrlSize;
	info->isCtrlPacket = isCtrlPacket;
}