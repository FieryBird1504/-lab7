#include "VA_FSFileWayMetadata.h"

bool VA_FSFileWayMetadata::fromString(const std::string& metaString)
{
	auto str = metaString.c_str();
	size_t pos = 0;
	size_t size = metaString.size();

	if (pos + sizeof size_t >= size)
	{
		return false;
	}

	size_t numOfElements = *reinterpret_cast<const size_t*>(str + pos);
	pos += sizeof size_t;

	std::map<std::string, BlockPtr> map;

	for (size_t i = 0; i < numOfElements; i++)
	{
		if (pos + sizeof size_t > size)
		{
			return false;
		}
		size_t elementSize = *reinterpret_cast<const size_t*>(str + pos);
		pos += sizeof size_t;

		if (pos + elementSize > size)
		{
			return false;
		}
		std::string element(str + pos, str + pos + elementSize);
		pos += elementSize;

		if (pos + sizeof BlockPtr > size)
		{
			return false;
		}
		BlockPtr startPos = *reinterpret_cast<const BlockPtr*>(str + pos);
		pos += sizeof BlockPtr;

		map.insert(std::pair<std::string, BlockPtr>(element, startPos));
	}

	cl_ways = map;
	return true;
}

std::string VA_FSFileWayMetadata::toString() const
{
	auto size = cl_ways.size();
	auto sizePtr = reinterpret_cast<char*>(&size);
	std::string s(sizePtr, sizePtr + sizeof size);

	for (auto it = cl_ways.begin(); it != cl_ways.end(); ++it)
	{
		auto keySize = it->first.size();
		auto key = it->first;
		auto value = it->second;
		sizePtr = reinterpret_cast<char*>(&keySize);
		s += std::string(sizePtr, sizePtr + sizeof keySize) + key;
		sizePtr = reinterpret_cast<char*>(&value);
		s += std::string(sizePtr, sizePtr + sizeof value);
	}

	return s;
}
