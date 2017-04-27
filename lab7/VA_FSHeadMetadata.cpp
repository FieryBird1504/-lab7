#include "VA_FSHeadMetadata.h"
#include "VA_FSCluster.h"

VA_FSHeadMetadata::VA_FSHeadMetadata()
{
	reset();
}

void VA_FSHeadMetadata::reset()
{
	cl_clusterSize = VA_FSCluster::cl_maxClusterDataSize + sizeof VA_FSClusterHeadMetadata;
	cl_fileWaysPos = 0;
	cl_freeClustersPos = 0;
	cl_numClusters = 1 << 15;
}
