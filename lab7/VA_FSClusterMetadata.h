#pragma once
#include <vector>
#include "types.h"

struct VA_FSClusterMetadata
{
	std::vector<bool> cl_data;

	bool fromString(const std::string& metaString);
	std::string toString() const;

	BigSize getFreeBlockNum() const;
	BlockPtr lockBlock();
	void freeBlock(const BlockPtr& num);
};
