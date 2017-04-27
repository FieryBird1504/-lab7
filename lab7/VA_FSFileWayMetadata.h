#pragma once
#include <map>
#include <string>

#include "types.h"

struct VA_FSFileWayMetadata
{
	std::map<std::string, BlockPtr> cl_ways;

	bool fromString(const std::string& metaString);
	std::string toString() const;
};
