#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"

#include "layer2_fx.h"



FxRrcConfig* GetFxRrcConfig(Node* node, int interfaceIndex)
{
	Layer2DataFx* layer2Data =
		FxLayer2GetLayer2DataFx(node, interfaceIndex);
	ERROR_Assert(layer2Data != NULL, "");
	return layer2Data->rrcConfig;
}

FxSacConfig* GetFxSacConfig(Node* node, int interfaceIndex)
{
	return &(GetFxRrcConfig(node, interfaceIndex)->fxSacConfig);
}

FxSlcConfig* GetFxSlcConfig(Node* node, int interfaceIndex)
{
	return &(GetFxRrcConfig(node, interfaceIndex)->fxSlcConfig);
}

FxSmacConfig* GetFxSmacConfig(Node* node, int interfaceIndex)
{
	return &(GetFxRrcConfig(node, interfaceIndex)->fxSmacConfig);
}

FxPhyConfig* GetFxPhyConfig(Node* node, int interfaceIndex)
{
	return &(GetFxRrcConfig(node, interfaceIndex)->fxPhyConfig);
}

void SetFxRrcConfig(Node* node, int interfaceIndex, FxRrcConfig& rrcConfig)
{
	FxRrcConfig* currentRrcConfig = GetFxRrcConfig(node, interfaceIndex);
	*currentRrcConfig = rrcConfig;
}
