#include "NetFilterAdblock.h"

#include <libecap/common/name.h>
#include <libecap/common/area.h>
#include <libecap/common/options.h>
#include <cstring>
#include <string>
#include <stdexcept>
#include <iterator>
#include <algorithm>
#include <chrono>

// TODO: Filter out all netblock rules, ignore cosmetic filters completely.
// TODO: Optimize list, remove duped entries that are already present in the DNS block lists, since that trumps all.
bool NetFilterAdblock::LoadAdblockList(std::string fileName)
{
	RequestCacheWhiteList.clear();
	RequestCacheBlackList.clear();
	
	std::ifstream adBlockListFile(fileName);
	std::string rules;
	
	for (std::string line; std::getline(adBlockListFile, line);)
	{
		// Do not add any cosmetic filters here since we cannot do anything with them in the REQMOD
		if (line.find_first_of('#') == std::string::npos)
		{
			// TODO: should also check for conditional filters like iframes etc
			// TODO: Remove filters that are already covered by NetFilterDns
			rules += line + "\n"; // Newlines are stripped by getline(), these are required for the list parser to work
		}
	}
	adBlockListFile.close();
	AdBlockNetfilterClient.parse(rules.c_str());
	
	Debugger(ilNormal | flApplication) << "[nBlock] Loaded " << AdBlockNetfilterClient.numFilters << " filters in the Adblock parser";
	
	return true;
}

void NetFilterAdblock::PushCache(std::unordered_set<std::string> &cacheSet, std::string newValue)
{
	mutexCache.lock();
	
	if (cacheSet.size() > localCacheSize)
		cacheSet.erase(cacheSet.begin());

	cacheSet.insert(newValue);
	
	mutexCache.unlock();
}

bool NetFilterAdblock::IsCached(std::unordered_set<std::string> &cacheSet, std::string uniqueRequestIdentifier)
{
	mutexCache.lock();
	
	if (cacheSet.find(uniqueRequestIdentifier) != cacheSet.end())
	{
		mutexCache.unlock();
		return true;
	}	
	
	mutexCache.unlock();
	return false;
}

bool NetFilterAdblock::IsBlackListed(std::string requestedUri, const libecap::Header &header)
{
	// Extract some header info from the virgin request.
	static const libecap::Name headerAccept("Accept");
	static const libecap::Name headerXRequest("X-Requested-With");
	static const libecap::Name headerContentType("Content-Type");
	static const libecap::Name headerReferer("Referer");
	
	std::string requestAccept = header.value(headerAccept).toString();
	std::string requestXRequest = header.value(headerXRequest).toString();
	std::string requestContentType = header.value(headerContentType).toString();
	std::string requestReferer = header.value(headerReferer).toString();
	
	FilterOption options = FONoFilterOption;
	
	// Try to detect the correct request type, this is somewhat limited compared to browser-based ad-blockers, because we do not
	// have the full context of the initiating page that's sending out requests. https / http mixed sites will for example not send a referrer.
	// In the future we could do page / request correlation by setting special cookies; or manipulating custom headers after we analyse the html content.
	//
	// Use Firefox developer consoles Network view to see how requests are categorized (HTML, SCRIPT, CSS etc..)
	//
	// Valid file/resource type options from ad-block, see: "filter.h"
	//	enum FilterOption {
	//	(f) FONoFilterOption = 0,
	//	(f) FOScript = 01,
	//	(f) FOImage = 02,
	//	(f) FOStylesheet = 04,
	//	FOObject = 010, // content handled by browser plugins, e.g. Flash or Java
	//	(f) FOXmlHttpRequest = 020,		// X-Requested-With header
	//	FOObjectSubrequest = 040,	 // should be merged with FOObject
	//	FOSubdocument = 0100, // iFrames
	//	(f) FODocument = 0200, // only used for whitelisting
	//	FOOther = 0400,
	//	FOXBL = 01000,
	//	FOCollapse = 02000,
	//	FODoNotTrack = 04000,
	//	FOElemHide = 010000,
	//	(f) FOThirdParty = 020000,  // Used internally only, do not use
	//	(f) FONotThirdParty = 040000,  // Used internally only, do not use
	//	(f) FOPing = 0100000,  // Not supported, but we will ignore these rules
	//	(f) FOResourcesOnly = FOScript|FOImage|FOStylesheet|FOObject|FOXmlHttpRequest|
	//	(f) FOObjectSubrequest|FOSubdocument|FODocument|FOOther|FOXBL,
	//	(f) FOUnsupported = FOPing
	//	};
	
	// Scripts, images and CSS are detected only based if there is a 'file extension' in the requested uri, eg http://host.com/script.js
	// This could also be determined by a server response header 'Content-Type', but we do not want to send out a request at all.
	// Only other possibility is to examine the main documents html code and scrap <script> tags, though they could also be manipulated by javascript.
	// TODO: These checks are tested only with Firefox / Internet Explorer, other browsers might behave slightly differently.
	
	if (std::regex_search(requestedUri, std::regex("\\.(js|json)")))
	{
		options = (FilterOption)(options | FOScript);
	}
	
	else if (std::regex_search(requestedUri, std::regex("\\.(jpeg|jpg|gif|png|bmp|svg)")))
	{
		options = (FilterOption)(options | FOImage);
	}
	
	// When a browser requests a CSS source, it will actually set the 'Accept' header, allowing us to get a better prediction for this file type.
	else if (std::regex_search(requestedUri, std::regex("\\.(css)")) || std::regex_search(requestAccept, std::regex("text/css")))
	{
		options = (FilterOption)(options | FOStylesheet);
	}

	// FOXmlHttpRequest can be detected using 2 methods:
	// - 'X-Requested-With' request header is set to: "XMLHttpRequest"
	// - POST requests made by javascript will set the ContentType to x-www-form-urlencoded, while regular POST requests do not.
	if (requestXRequest == "XMLHttpRequest" || requestContentType == "application/x-www-form-urlencoded")
	{
		options = (FilterOption)(options | FOXmlHttpRequest);
	}
	
	if (requestReferer == "" && std::regex_search(requestAccept, std::regex("text/html")))
	{
		options = (FilterOption)(options | FODocument);
	}

	/*
	// For debugging, display the content types for each request
	std::string contentString;
	if (options == FONoFilterOption)
		contentString = "FONoFilterOption";
	
	if ((FilterOption)(options & FOScript) == FOScript)
		contentString = contentString + "$script";
	
	if ((FilterOption)(options & FOImage) == FOImage)
		contentString = contentString + "$image";

	if ((FilterOption)(options & FOStylesheet) == FOStylesheet)
	contentString = contentString + "$stylesheet";
	
	if ((FilterOption)(options & FOXmlHttpRequest) == FOXmlHttpRequest)
		contentString = contentString + "$xmlhttprequest";
	
	if ((FilterOption)(options & FODocument) == FODocument)
		contentString = contentString + "$document";
	
	Debugger(ilNormal | flApplication) << "(" << contentString << ") " << requestedUri;
	//Debugger(ilNormal | flApplication) << "(Referer) " << requestReferer;
	*/
	
	// Get the domain / hostname of the referring url.
	std::string referringHost = "";
	if (requestReferer != "")
	{
		// Get hostname (or so called domain according to ad-block..) from referer
		// Ad-block client uses this to determine if it's dealing with a 3rd party request.
		int posStart = requestReferer.find_first_of(":/", 4);
		int posEnd = requestReferer.find_first_of("/", posStart+6);
		referringHost = requestReferer.substr(posStart+3, posEnd - posStart - 3);
	}
	
	//Debugger(ilNormal | flApplication) << "Execution Time premodding AdblockFilter: " << (std::chrono::high_resolution_clock::now() - startTime).count() << "us";

	// Generate a request unique string with all parameters that are of influence to the ad-block lib for local caching
	std::string uniqueIdentifier = requestedUri + std::to_string(options) + referringHost;
	
	// First check our local host whitelist cache for previous checked entries
	if (IsCached(RequestCacheWhiteList, uniqueIdentifier))
	{
		return false;
	}
	
	// And check our local host blacklist cache for previous checked entries
	if (IsCached(RequestCacheBlackList, uniqueIdentifier))
	{
		return true;
	}
	
	//Debugger(ilNormal | flApplication) << requestedUri;
	
	if (AdBlockNetfilterClient.matches(requestedUri.c_str(), options, referringHost.c_str()))
	{
		PushCache(RequestCacheBlackList, uniqueIdentifier);
		return true;
	}
	
	// Request is allowed, cache it to our whitelist the the next time it will be requested
	PushCache(RequestCacheWhiteList, uniqueIdentifier);
	
	return false;
}
