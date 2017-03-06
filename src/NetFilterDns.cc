#include "NetFilterDns.h"

#include <libecap/common/name.h>
#include <libecap/common/area.h>
#include <libecap/common/options.h>
#include <cstring>
#include <string>
#include <stdexcept>
#include <chrono>

bool NetFilterDns::LoadHostnames(std::string fileName)
{
	mutexBlocklists.lock();
	
	HostnameBlockList.clear();
	HostCacheWhiteList.clear();
	HostCacheBlackList.clear();

	std::ifstream HostnameBlockListFile(fileName);
	for (std::string line; std::getline(HostnameBlockListFile, line);)
	{
		if (strncmp(line.c_str(), "0.0.0.0 ", 8) == 0) // strncmp returns 0 on match
		{
			HostnameBlockList.insert(line.substr(8, line.size() - 8));
		}
	}
	HostnameBlockListFile.close();
	Debugger(ilNormal | flApplication) << "[nBlock] Loaded " << HostnameBlockList.size() << " entries in to hostname blocklist";

	mutexBlocklists.unlock();
	return true;
}

// TODO: should make a hostname importer out of this function: out of scope for first proto type.
bool NetFilterDns::LoadDomains(std::string fileName)
{
	mutexBlocklists.lock();
	
	DomainBlockList.clear();
	HostCacheWhiteList.clear();
	HostCacheBlackList.clear();

	std::ifstream DomainBlockListFile(fileName);
	for (std::string line; std::getline(DomainBlockListFile, line);)
	{
		if (strncmp(line.c_str(), "address=/", 9) == 0) // strncmp returns 0 on match
		{
			DomainBlockList.insert(line.substr(9, line.size() - (9 + 8)));
		}
	}
	DomainBlockListFile.close();
	mutexBlocklists.unlock();
	
	Debugger(ilNormal | flApplication) << "[nBlock] Loaded " << DomainBlockList.size() << " entries in to domain blocklist";
	return true;
}

void NetFilterDns::PushCache(std::unordered_set<std::string> &cacheSet, std::string newValue)
{
	mutexCache.lock();
	
	if (cacheSet.size() > localCacheSize)
		cacheSet.erase(cacheSet.begin());

	cacheSet.insert(newValue);
	
	mutexCache.unlock();
}

bool NetFilterDns::IsCached(std::unordered_set<std::string> &cacheSet, std::string host)
{
	mutexCache.lock();
	
	if (cacheSet.find(host) != cacheSet.end())
	{
		mutexCache.unlock();
		return true;
	}	
	
	mutexCache.unlock();
	return false;
}

bool NetFilterDns::IsInBlockList(std::unordered_set<std::string> &blocklistSet, std::string host)
{
	mutexBlocklists.lock();
	
	if (blocklistSet.find(host) != blocklistSet.end())
	{
		mutexBlocklists.unlock();
		return true;
	}	
	
	mutexBlocklists.unlock();
	return false;
}

bool NetFilterDns::IsBlackListed(std::string host)
{
	// Remove port part if present (for https requests)
	size_t port_pos = host.find_first_of(':');
	if (port_pos != std::string::npos)
	{
		host = host.substr(0, port_pos);
	}

	// First check our local host whitelist cache for previous checked entries
	if (IsCached(HostCacheWhiteList, host))
	{
		return false;
	}
	
	// And check our local host blacklist cache for previous checked entries
	if (IsCached(HostCacheBlackList, host))
	{
		return true;
	}

	// First check for matches with our hostnames list (faster)
	if (IsInBlockList(HostnameBlockList, host))
	{
		PushCache(HostCacheBlackList, host);
		return true;
	}

	// Now check for matching domain filters, matching against subdomains, eg: host.com, evil.host.com, a.evil.host.com
	size_t dotpos = host.find_last_of('.'); // First part is always skipped, do not block *.com
	std::string subhost;

	while (dotpos != std::string::npos)
	{
		dotpos = host.find_last_of('.', dotpos-1);

		if (dotpos != std::string::npos)
		{
			subhost = host.substr(dotpos + 1, host.size() - dotpos);
		}
		else subhost = host;

		// Check if the sub-host matches any domain in the block-lists
		if (IsInBlockList(DomainBlockList, subhost))
		{
			PushCache(HostCacheBlackList, host);			
			return true;
		}
	}

	// Host is found to be clear, add it to the local cache for faster filtering the next time it is requested
	PushCache(HostCacheWhiteList, host);

	return false;
}
