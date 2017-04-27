#pragma once
#include "types.h"

struct VA_FSClusterHeadMetadata
{
	BlockPtr prev;
	BlockPtr next;
	LittleSize size;
};
