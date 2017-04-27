#include "VA_FSCluster.h"

const LittleSize VA_FSCluster::cl_maxClusterDataSize = (1 << 15) - sizeof VA_FSClusterHeadMetadata;

VA_FSCluster::VA_FSCluster(): cl_data(new char[cl_maxClusterDataSize])
{
}

VA_FSCluster::VA_FSCluster(const VA_FSCluster& other)
{
	cl_head = other.cl_head;
	cl_data = new char[cl_maxClusterDataSize];
	memcpy(cl_data, other.cl_data, cl_maxClusterDataSize);
}

VA_FSCluster::~VA_FSCluster()
{
	if (cl_data)
	{
		delete [] cl_data;
	}
}
