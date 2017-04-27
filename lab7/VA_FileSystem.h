#pragma once
#include <fstream>
#include "VA_FSHeadMetadata.h"
#include "VA_FSClusterMetadata.h"
#include "VA_FSClusterHeadMetadata.h"
#include "VA_FSFileWayMetadata.h"
#include <set>

struct VA_FSCluster;

class VA_FileSystem
{
	std::string cl_wayToFileOnDisk;
	std::fstream cl_f;

	VA_FSHeadMetadata cl_beginMetadata;
	VA_FSClusterMetadata cl_clusterMetadata;
	VA_FSFileWayMetadata cl_fileWayMetadata;
	const LittleSize cl_zeroClusterStartPos;

	static BigSize calculateNumOfBlocks(const VA_File& f);

	BigSize calculatePosByBlockIndex(const BigSize& num) const;
	void setPosToRead(const BigSize& clusterNum);
	void setPosToWrite(const BigSize& clusterNum);

	BlockPtr generateNewStartingPos();

	VA_FSClusterHeadMetadata readBlockHead(const BigSize& num);
	void writeBlockHead(const BigSize& num, const VA_FSClusterHeadMetadata& meta);

	VA_FSCluster readBlock(const BigSize& num);
	void writeBlock(const BigSize& num, const VA_FSCluster& cluster);

	void firstClusterMetadataProcessing();
	void firstFileWayMetadataProcessing();

	void readClusterMetadata();
	void writeClusterMetadata();

	void readFileWayMetadata();
	void writeFileWayMetadata();

	void freeBlocks(const BlockPtr& startingPos);

	bool read(VA_File& file, const BlockPtr& startingPos);
	bool write(const VA_File& file, const BlockPtr& startingPos);

	bool readFromFS(const std::string& way, VA_File& file);
	bool writeToFS(const std::string& way, const VA_File& file);
	bool moveInFS(const std::string& sourceWay, const std::string& destinationWay);
	bool deleteFromFS(const std::string& way);

	static bool readFromExternal(const std::string& way, VA_File& file);
	static bool writeToExternal(const std::string& way, const VA_File& file);
	static bool deleteFromExternal(const std::string& way);

	void resetBeginMetadata();
	void readBeginMetadata();
	void writeBeginMetadata();

public:
	VA_FileSystem(const std::string& wayToFile = "D:\\file.VA_FS.bin");
	~VA_FileSystem();

	std::set<std::string> getListOfFiles() const;

	void format();
	bool move(const std::string& startWay, const std::string& destinationWay);
	bool copy(const std::string& startWay, const std::string& destinationWay);
	bool deleteF(const std::string& way);
};
