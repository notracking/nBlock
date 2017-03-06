#ifndef ECAP_NBLOCK_NetFilterAdblock
#define ECAP_NBLOCK_NetFilterAdblock

#include "Debugger.h"
#include "ad_block_client.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <set>
#include <string.h>
#include <unordered_set>
#include <mutex>
#include <regex>
#include <libecap/common/header.h>
#include <libecap/common/names.h>

class NetFilterAdblock {
public:
	static NetFilterAdblock& getInstance()
	{
		static NetFilterAdblock instance;
		return instance;
	}

	bool LoadAdblockList(std::string fileName);
	bool IsBlackListed(std::string requestedUrl, const libecap::Header &header);

	unsigned int localCacheSize;

protected:

private:
	AdBlockClient AdBlockNetfilterClient;
	
	std::mutex mutexCache;
	std::unordered_set<std::string> RequestCacheWhiteList;
	std::unordered_set<std::string> RequestCacheBlackList;
	
	void PushCache(std::unordered_set<std::string> &cacheSet, std::string newValue);
	bool IsCached(std::unordered_set<std::string> &cacheSet, std::string host);
};

#endif
