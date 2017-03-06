#ifndef ECAP_NBLOCK_NETFILTERDNS
#define ECAP_NBLOCK_NETFILTERDNS

#include "Debugger.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <set>
#include <string.h>
#include <unordered_set>
#include <mutex>

class NetFilterDns {
public:
	static NetFilterDns& getInstance()
	{
		static NetFilterDns instance;
		return instance;
	}

	bool LoadHostnames(std::string fileName);
	bool LoadDomains(std::string fileName);
	bool LoadAdblockList(std::string fileName);

	bool IsBlackListed(std::string host);

	unsigned int localCacheSize;

protected:

private:
	std::mutex mutexBlocklists;
	std::unordered_set<std::string> HostnameBlockList;
	std::unordered_set<std::string> DomainBlockList;

	std::mutex mutexCache;
	std::unordered_set<std::string> HostCacheWhiteList;
	std::unordered_set<std::string> HostCacheBlackList;
	
	void PushCache(std::unordered_set<std::string> &cacheSet, std::string newValue);
	bool IsCached(std::unordered_set<std::string> &cacheSet, std::string host);
	bool IsInBlockList(std::unordered_set<std::string> &blocklistSet, std::string host);
};

#endif
