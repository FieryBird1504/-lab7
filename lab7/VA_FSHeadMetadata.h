#pragma once
#include "types.h"

struct VA_FSHeadMetadata
{
	LittleSize cl_clusterSize;
	BlockPtr cl_numClusters;
	BlockPtr cl_freeClustersPos;
	BlockPtr cl_fileWaysPos;

	VA_FSHeadMetadata();

	void reset();
};
