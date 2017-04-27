#include "VA_FSClusterMetadata.h"
#include <bitset>
#include <algorithm>

bool VA_FSClusterMetadata::fromString(const std::string& metaString)
{
	auto str = metaString.c_str();
	size_t pos = 0;
	size_t size = metaString.size();

	if (pos + sizeof size_t > size)
	{
		return false;
	}

	size_t numOfElements = *reinterpret_cast<const size_t*>(str + pos);
	size_t numOfBlocks = (numOfElements + 7) / 8;
	pos += sizeof size_t;

	std::vector<bool> vector;

	for (size_t i = 0; i < numOfBlocks; i++)
	{
		if (pos + 1 > size)
		{
			return false;
		}
		std::bitset<8> byte = *reinterpret_cast<const std::bitset<8>*>(str + pos);
		pos++;

		for (auto j = 0; j < 8; j++)
		{
			vector.push_back(byte[j]);
		}
	}

	while (vector.size() > numOfElements)
	{
		vector.pop_back();
	}

	cl_data = vector;

	return true;
}

std::string VA_FSClusterMetadata::toString() const
{
	auto size = cl_data.size();
	auto sizePtr = reinterpret_cast<char*>(&size);
	std::string s(sizePtr, sizePtr + sizeof size);

	std::bitset<8> byte;
	size_t i = 0, j = 0;
	while (true)
	{
		if ((i + j) >= cl_data.size())
		{
			if (j > 0)
			{
				s += static_cast<char>(byte.to_ulong());
			}
			break;
		}

		if (j >= 8)
		{
			i += 8;
			j = 0;
			s += static_cast<char>(byte.to_ulong());
		}

		byte[j] = cl_data[i + j];
		j++;
	}
	return s;
}

BigSize VA_FSClusterMetadata::getFreeBlockNum() const
{
	return count(cl_data.begin(), cl_data.end(), true);
}

BlockPtr VA_FSClusterMetadata::lockBlock()
{
	auto it = find(cl_data.begin(), cl_data.end(), true);
	if (it == cl_data.end())
	{
		throw 1;
	}
	*it = false;
	return it - cl_data.begin();
}

void VA_FSClusterMetadata::freeBlock(const BlockPtr& num)
{
	cl_data[num] = true;
}
