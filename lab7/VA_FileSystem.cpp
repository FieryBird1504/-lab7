#include "VA_FileSystem.h"
#include "VA_FSClusterMetadata.h"
#include "VA_FSCluster.h"
#include <algorithm>

void VA_FileSystem::resetBeginMetadata()
{
	cl_beginMetadata.reset();
}

void VA_FileSystem::readBeginMetadata()
{
	cl_f.seekg(0);
	cl_f.read(reinterpret_cast<char*>(&cl_beginMetadata), sizeof cl_beginMetadata);
}

void VA_FileSystem::writeBeginMetadata()
{
	cl_f.seekp(0);
	cl_f.write(reinterpret_cast<char*>(&cl_beginMetadata), sizeof cl_beginMetadata);
}

VA_FileSystem::VA_FileSystem(const std::string& wayToFile)
	: cl_wayToFileOnDisk(wayToFile), cl_f(cl_wayToFileOnDisk, std::ios::in | std::ios::out | std::ios::binary), cl_zeroClusterStartPos(sizeof cl_beginMetadata)
{
	readBeginMetadata();
	readClusterMetadata();
	readFileWayMetadata();
}

VA_FileSystem::~VA_FileSystem()
{
	writeFileWayMetadata();
	writeClusterMetadata();
	cl_f.close();
}

std::set<std::string> VA_FileSystem::getListOfFiles() const
{
	std::set<std::string> res;
	std::for_each(cl_fileWayMetadata.cl_ways.begin(), cl_fileWayMetadata.cl_ways.end(), [&res](const std::pair<std::string, BlockPtr>& pair)
	              {
		              res.insert(pair.first);
	              });
	return res;
}

void VA_FileSystem::format()
{
	resetBeginMetadata();

	cl_clusterMetadata = VA_FSClusterMetadata();
	cl_clusterMetadata.cl_data = std::vector<bool>(cl_beginMetadata.cl_numClusters, true);

	firstClusterMetadataProcessing();

	firstFileWayMetadataProcessing();

	writeClusterMetadata();
	writeBeginMetadata();
}

bool VA_FileSystem::move(const std::string& startWay, const std::string& destinationWay)
{
	//todo
	if (startWay.empty() || destinationWay.empty() || startWay[0] != '/' && destinationWay[0] != '/')
	{
		return false;
	}

	if (startWay[0] == '/' && destinationWay[0] == '/')
	{
		return moveInFS(startWay, destinationWay);
	}

	VA_File f;
	bool res = true;
	switch (startWay[0])
	{
	case '/':
		res = readFromFS(startWay, f);
		break;
	default:
		res = readFromExternal(startWay, f);
	}

	if (!res)
	{
		return false;
	}

	switch (destinationWay[0])
	{
	case '/':
		res = writeToFS(destinationWay, f);
		break;
	default:
		res = writeToExternal(destinationWay, f);
	}

	if (res)
	{
		switch (startWay[0])
		{
		case '/':
			res = deleteFromFS(startWay);
			break;
		default:
			res = deleteFromExternal(startWay);
		}
	}

	return res;
}

bool VA_FileSystem::copy(const std::string& startWay, const std::string& destinationWay)
{
	if (startWay.empty() || destinationWay.empty() || startWay[0] != '/' && destinationWay[0] != '/')
	{
		return false;
	}

	VA_File f;
	bool res = true;
	switch (startWay[0])
	{
	case '/':
		res = readFromFS(startWay, f);
		break;
	default:
		res = readFromExternal(startWay, f);
	}

	if (!res)
	{
		return false;
	}

	switch (destinationWay[0])
	{
	case '/':
		res = writeToFS(destinationWay, f);
		break;
	default:
		res = writeToExternal(destinationWay, f);
	}

	return res;
}

bool VA_FileSystem::deleteF(const std::string& way)
{
	if (way.empty() || way[0] != '/')
	{
		return false;
	}

	return deleteFromFS(way);
}

BigSize VA_FileSystem::calculateNumOfBlocks(const VA_File& f)
{
	return (f.size() + VA_FSCluster::cl_maxClusterDataSize - 1) / VA_FSCluster::cl_maxClusterDataSize;
}

BigSize VA_FileSystem::calculatePosByBlockIndex(const BigSize& num) const
{
	return sizeof VA_FSHeadMetadata + cl_beginMetadata.cl_clusterSize * num;
}

void VA_FileSystem::setPosToRead(const BigSize& clusterNum)
{
	cl_f.seekg(calculatePosByBlockIndex(clusterNum));
}

void VA_FileSystem::setPosToWrite(const BigSize& clusterNum)
{
	cl_f.seekp(calculatePosByBlockIndex(clusterNum));
}

BlockPtr VA_FileSystem::generateNewStartingPos()
{
	auto pos = cl_clusterMetadata.lockBlock();

	VA_FSClusterHeadMetadata meta;
	meta.next = meta.prev = pos;
	meta.size = 0;

	writeBlockHead(pos, meta);

	return pos;
}

VA_FSClusterHeadMetadata VA_FileSystem::readBlockHead(const BigSize& num)
{
	setPosToRead(num);
	VA_FSClusterHeadMetadata res;
	cl_f.read(reinterpret_cast<char*>(&res), sizeof res);
	return res;
}

void VA_FileSystem::writeBlockHead(const BigSize& num, const VA_FSClusterHeadMetadata& meta)
{
	setPosToWrite(num);
	cl_f.write(reinterpret_cast<const char*>(&meta), sizeof meta);
}

VA_FSCluster VA_FileSystem::readBlock(const BigSize& num)
{
	setPosToRead(num);
	VA_FSCluster cluster;

	cl_f.read(reinterpret_cast<char*>(&cluster.cl_head), sizeof cluster.cl_head);
	cl_f.read(cluster.cl_data, VA_FSCluster::cl_maxClusterDataSize);

	return cluster;
}

void VA_FileSystem::writeBlock(const BigSize& num, const VA_FSCluster& cluster)
{
	setPosToWrite(num);

	cl_f.write(reinterpret_cast<const char*>(&cluster.cl_head), sizeof cluster.cl_head);
	cl_f.write(cluster.cl_data, VA_FSCluster::cl_maxClusterDataSize);
}

void VA_FileSystem::firstClusterMetadataProcessing()
{
	//first clusterMetadata lock
	auto clusterMetaSize = calculateNumOfBlocks(cl_clusterMetadata.toString());
	std::vector<BlockPtr> clusterMetaBlocks;
	for (BigSize i = 0; i < clusterMetaSize; ++i)
	{
		clusterMetaBlocks.push_back(cl_clusterMetadata.lockBlock());
	}

	cl_beginMetadata.cl_freeClustersPos = clusterMetaBlocks.front();

	for (auto it = clusterMetaBlocks.begin(); it != clusterMetaBlocks.end(); ++it)
	{
		auto meta = readBlockHead(*it);
		if (it == clusterMetaBlocks.begin())
		{
			meta.prev = *it;
		}
		else
		{
			meta.prev = *(it - 1);
		}

		if (it == clusterMetaBlocks.end() - 1)
		{
			meta.next = *it;
			meta.size = static_cast<LittleSize>(cl_clusterMetadata.toString().size() - (clusterMetaSize - 1) * cl_beginMetadata.cl_clusterSize);
		}
		else
		{
			meta.prev = *(it + 1);
			meta.size = VA_FSCluster::cl_maxClusterDataSize;
		}
		writeBlockHead(*it, meta);
	}
}

void VA_FileSystem::firstFileWayMetadataProcessing()
{
	cl_fileWayMetadata.cl_ways.clear();
	cl_beginMetadata.cl_fileWaysPos = generateNewStartingPos();
	write(cl_fileWayMetadata.toString(), cl_beginMetadata.cl_fileWaysPos);
}

void VA_FileSystem::readClusterMetadata()
{
	VA_File string;
	read(string, cl_beginMetadata.cl_freeClustersPos);
	cl_clusterMetadata.fromString(string);
}

void VA_FileSystem::writeClusterMetadata()
{
	auto nextBlockPos = cl_beginMetadata.cl_freeClustersPos;
	auto blockPos = nextBlockPos + 1; //чтобы не повторялось
	auto string = cl_clusterMetadata.toString();
	BigSize strPos = 0;

	do
	{
		blockPos = nextBlockPos;
		auto block = readBlock(blockPos);
		nextBlockPos = block.cl_head.next;
		memcpy(block.cl_data, string.c_str() + strPos, block.cl_head.size);
		strPos += block.cl_head.size;
		writeBlock(blockPos, block);
	}
	while (blockPos != nextBlockPos);
}

void VA_FileSystem::readFileWayMetadata()
{
	VA_File f;
	read(f, cl_beginMetadata.cl_fileWaysPos);
	cl_fileWayMetadata.fromString(f);
}

void VA_FileSystem::writeFileWayMetadata()
{
	write(cl_fileWayMetadata.toString(), cl_beginMetadata.cl_fileWaysPos);
}

void VA_FileSystem::freeBlocks(const BlockPtr& startingPos)
{
	auto nextBlock = startingPos;
	auto currentBlock = startingPos + 1;

	while (nextBlock != currentBlock)
	{
		currentBlock = nextBlock;
		auto blockHead = readBlockHead(currentBlock);
		nextBlock = blockHead.next;
		blockHead.prev = blockHead.next = currentBlock;
		blockHead.size = 0;
		writeBlockHead(currentBlock, blockHead);
		cl_clusterMetadata.freeBlock(currentBlock);
	}
}

bool VA_FileSystem::read(VA_File& file, const BlockPtr& startingPos)
{
	auto nextBlockPos = startingPos;
	auto blockPos = nextBlockPos;
	std::string string;

	do
	{
		blockPos = nextBlockPos;
		auto block = readBlock(blockPos);
		nextBlockPos = block.cl_head.next;
		string += std::string(block.cl_data, block.cl_data + block.cl_head.size);
	}
	while (blockPos != nextBlockPos);

	file = string;
	return true;
}

bool VA_FileSystem::write(const VA_File& file, const BlockPtr& startingPos)
{
	BigSize filePos = 0;
	auto nextBlock = startingPos;
	auto currentBlock = startingPos;

	if (calculateNumOfBlocks(file) > cl_clusterMetadata.getFreeBlockNum())
	{
		return false;
	}

	do
	{
		auto block = readBlock(nextBlock);
		block.cl_head.prev = currentBlock;
		currentBlock = nextBlock;
		nextBlock = block.cl_head.next;
		if (filePos + VA_FSCluster::cl_maxClusterDataSize > file.size())
		{
			block.cl_head.size = static_cast<LittleSize>(file.size() - filePos);
			memcpy(block.cl_data, file.c_str() + filePos, block.cl_head.size);
			block.cl_head.next = currentBlock;
			writeBlock(currentBlock, block);
			break;
		}

		block.cl_head.size = VA_FSCluster::cl_maxClusterDataSize;
		memcpy(block.cl_data, file.c_str() + filePos, block.cl_head.size);
		filePos += VA_FSCluster::cl_maxClusterDataSize;

		if (currentBlock == nextBlock)
		{
			try
			{
				nextBlock = generateNewStartingPos();
				block.cl_head.next = nextBlock;
			}
			catch (const int&)
			{
				return false;
			}
		}

		writeBlock(currentBlock, block);
	}
	while (true);

	if (currentBlock != nextBlock)
	{
		freeBlocks(nextBlock);
	}

	return true;
}

bool VA_FileSystem::readFromFS(const std::string& way, VA_File& file)
{
	auto map = &cl_fileWayMetadata.cl_ways;
	auto it = map->find(way);
	if (it == map->end())
	{
		return false;
	}

	return read(file, it->second);
}

bool VA_FileSystem::writeToFS(const std::string& way, const VA_File& file)
{
	auto map = &cl_fileWayMetadata.cl_ways;
	auto it = map->find(way);
	if (it != map->end())
	{
		return false;
	}

	map->insert(make_pair(way, generateNewStartingPos()));
	if (write(file, map->find(way)->second))
	{
		return true;
	}
	deleteFromFS(way);
	return false;
}

bool VA_FileSystem::moveInFS(const std::string& sourceWay, const std::string& destinationWay)
{
	auto map = &cl_fileWayMetadata.cl_ways;
	auto it = map->find(destinationWay);
	if (it != map->end())
	{
		return false;
	}

	it = map->find(sourceWay);
	if (it == map->end())
	{
		return false;
	}

	auto value = (*map)[sourceWay];
	map->erase(it);
	map->emplace(make_pair(destinationWay, value));

	return true;
}

bool VA_FileSystem::deleteFromFS(const std::string& way)
{
	auto map = &cl_fileWayMetadata.cl_ways;
	auto it = map->find(way);
	if (it == map->end())
	{
		return false;
	}
	freeBlocks(it->second);
	map->erase(way);
	return true;
}

bool VA_FileSystem::readFromExternal(const std::string& way, VA_File& file)
{
	std::fstream f(way, std::ios::in | std::ios::binary);
	if (!f)
	{
		return false;
	}

	while (f)
	{
		file.push_back(f.get());
	}

	file.pop_back();
	f.close();
	return true;
}

bool VA_FileSystem::writeToExternal(const std::string& way, const VA_File& file)
{
	std::fstream f(way, std::ios::out | std::ios::trunc | std::ios::binary);
	if (!f)
	{
		return false;
	}

	f.write(file.c_str(), file.size());

	f.close();
	return true;
}

bool VA_FileSystem::deleteFromExternal(const std::string& way)
{
	return remove(way.c_str()) == 0;
}
