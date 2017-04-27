#pragma once
#include "VA_FSClusterHeadMetadata.h"

struct VA_FSCluster
{
	VA_FSClusterHeadMetadata cl_head;
	static const LittleSize cl_maxClusterDataSize;
	char* cl_data;

	VA_FSCluster();
	VA_FSCluster(const VA_FSCluster& other);
	~VA_FSCluster();
};
