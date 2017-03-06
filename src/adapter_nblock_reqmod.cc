#include "autoconf.h"
#include "Debugger.h"
#include "NetFilterDns.h"
#include "NetFilterAdblock.h"

#include <iostream>
#include <string.h>
#include <libecap/common/registry.h>
#include <libecap/common/errors.h>
#include <libecap/common/message.h>
#include <libecap/common/header.h>
#include <libecap/common/names.h>
#include <libecap/common/named_values.h>
#include <libecap/host/host.h>
#include <libecap/adapter/service.h>
#include <libecap/adapter/xaction.h>
#include <libecap/host/xaction.h>
#include <chrono> // for measuring execution time

namespace Adapter { // not required, but adds clarity

class Service: public libecap::adapter::Service {
	public:
		Service(const std::string &aMode);
	
		// About
		virtual std::string uri() const; // unique across all vendors
		virtual std::string tag() const; // changes with version and config
		virtual void describe(std::ostream &os) const; // free-format info

		// Configuration
		virtual void configure(const libecap::Options &cfg);
		virtual void reconfigure(const libecap::Options &cfg);
		void setOne(const libecap::Name &name, const libecap::Area &valArea);
		const std::string mode; // REQMOD or RESPMOD (for unique service URI)

		// Lifecycle
		virtual void start(); // expect makeXaction() calls
		virtual void stop(); // no more makeXaction() calls until start()
		virtual void retire(); // no more makeXaction() calls

		// Scope (XXX: this may be changed to look at the whole header)
		virtual bool wantsUrl(const char *url) const;

		// Work
		virtual MadeXactionPointer makeXaction(libecap::host::Xaction *hostx);
		
	private:
};

// Calls Service::setOne() for each host-provided configuration option.
// See Service::configure().
class Cfgtor: public libecap::NamedValueVisitor {
	public:
		Cfgtor(Service &aSvc): svc(aSvc) {}
		virtual void visit(const libecap::Name &name, const libecap::Area &value) {
			svc.setOne(name, value);
		}
		Service &svc;
};

// TODO: libecap should provide an adapter::HeaderOnlyXact convenience class

// a minimal adapter transaction
class Xaction: public libecap::adapter::Xaction {
	public:
		Xaction(libecap::host::Xaction *x);
		virtual ~Xaction();

		// meta-info for the host transaction
		virtual const libecap::Area option(const libecap::Name &name) const;
		virtual void visitEachOption(libecap::NamedValueVisitor &visitor) const;

		// lifecycle
		virtual void start();
		virtual void stop();

		// adapted body transmission control
		virtual void abDiscard() { noBodySupport(); }
		virtual void abMake() { noBodySupport(); }
		virtual void abMakeMore() { noBodySupport(); }
		virtual void abStopMaking() { noBodySupport(); }

		// adapted body content extraction and consumption
		virtual libecap::Area abContent(libecap::size_type, libecap::size_type) { noBodySupport(); return libecap::Area(); }
		virtual void abContentShift(libecap::size_type)  { noBodySupport(); }

		// virgin body state notification
		virtual void noteVbContentDone(bool) { noBodySupport(); }
		virtual void noteVbContentAvailable() { noBodySupport(); }

	protected:
		void noBodySupport() const;

	private:
		libecap::host::Xaction *hostx; // Host transaction rep
};

} // namespace Adapter

static const std::string CfgErrorPrefix = "nBlock configuration error: ";
	
std::string Adapter::Service::uri() const {
	return "ecap://nBlock/ecap/services/?mode=" + mode;
}

Adapter::Service::Service(const std::string &aMode):
	mode(aMode)
{
}

std::string Adapter::Service::tag() const {
	return PACKAGE_VERSION;
}

void Adapter::Service::describe(std::ostream &os) const {
	os << "nblock " << mode << " adapter: " << PACKAGE_NAME << " v" << PACKAGE_VERSION;
}

void Adapter::Service::configure(const libecap::Options &cfg) {
	Cfgtor cfgtor(*this);
	cfg.visitEachOption(cfgtor);

	// check for post-configuration errors and inconsistencies
	if (NetFilterDns::getInstance().localCacheSize == 0 || NetFilterAdblock::getInstance().localCacheSize == 0)
	{
		throw libecap::TextException(CfgErrorPrefix + "cache value can not be 0");
	}
}

void Adapter::Service::reconfigure(const libecap::Options &cfg) {
	configure(cfg);
}

void Adapter::Service::setOne(const libecap::Name &name, const libecap::Area &valArea) {
	if (this->mode != "CLIENT_REQUEST_MODE")
		return;
	
	const std::string value = valArea.toString();
	
	if (name == "cache")
	{
		try
		{
			NetFilterDns::getInstance().localCacheSize = std::stoi(value);
			NetFilterAdblock::getInstance().localCacheSize = std::stoi(value);
			Debugger(ilNormal | flApplication) << "[nBlock] Cache: " << std::to_string(std::stoi(value)) << " entries";
		}
		catch (...)
		{
			throw libecap::TextException(CfgErrorPrefix + "[nBlock] Invalid value for 'cache': " + value);
		}
	}
	/* can use start_with for future loading of multiple lists */
	else if (name == "list_hostnames")
	{
		NetFilterDns::getInstance().LoadHostnames(value);
	}
	else if (name == "list_domains")
	{
		NetFilterDns::getInstance().LoadDomains(value);
	}
	else if (name == "list_adblockplus")
	{
		NetFilterAdblock::getInstance().LoadAdblockList(value);
	}
	else if (name.assignedHostId())
		; // skip host-standard options we do not know or care about
	else
	{
		throw libecap::TextException(CfgErrorPrefix + "[nBlock] Unsupported configuration parameter: " + name.image());
	}
}

void Adapter::Service::start() {
	libecap::adapter::Service::start();
	
	// custom code
}

void Adapter::Service::stop() {
	// custom code would go here, but this service does not have one
	libecap::adapter::Service::stop();
}

void Adapter::Service::retire() {
	// custom code would go here, but this service does not have one
	libecap::adapter::Service::stop();
}

bool Adapter::Service::wantsUrl(const char *url) const {
	return true; // minimal adapter is applied to all messages
}

Adapter::Service::MadeXactionPointer
Adapter::Service::makeXaction(libecap::host::Xaction *hostx) {
	return Adapter::Service::MadeXactionPointer(new Adapter::Xaction(hostx));
}


Adapter::Xaction::Xaction(libecap::host::Xaction *x): hostx(x) {
}

Adapter::Xaction::~Xaction() {
	if (libecap::host::Xaction *x = hostx) {
		hostx = 0;
		x->adaptationAborted();
	}
}

const libecap::Area Adapter::Xaction::option(const libecap::Name &) const {
	return libecap::Area(); // this transaction has no meta-information
}

void Adapter::Xaction::visitEachOption(libecap::NamedValueVisitor &) const {
	// this transaction has no meta-information to pass to the visitor
}

void Adapter::Xaction::start() {
	Must(hostx);
	
	typedef const libecap::RequestLine *CLRLP;
	if (CLRLP requestLine = dynamic_cast<CLRLP>(&hostx->virgin().firstLine()))
	{
		// Use the dns based blocklist to see if the requested Host should be blocked.
		// This filter is extremely fast, using local caching for recurrinng requests.
		static const libecap::Name headerHost("Host");
		std::string requestHost = hostx->virgin().header().value(headerHost).toString();
		
		if (NetFilterDns::getInstance().IsBlackListed(requestHost))
		{
			hostx->blockVirgin(); // block access!
			Debugger(ilNormal | flApplication) << "!! NetFilterDns Blocked request to host: " << requestHost;

			return;
		} 
		
		std::string requestUri = requestLine->uri().toString();
		
		// Ignore initial https connections (those should be covered by the dns filters)
		// Example: Requested URI: cdns.gigya.com:443
		if (requestUri.compare(0, 4, "http") == 0) // Moving this check into the IsBlackListed() function will corrupt the requestUri string for some strange reason...
		{
	
			// And finally use the external Ad-block client to check if the host is black/white listed.
			// TODO: our local interface also caches recurring entries, for a decent speed upgrade since the adblock parser is still relatively slow
			if (NetFilterAdblock::getInstance().IsBlackListed(requestUri, hostx->virgin().header()))
			{
				hostx->blockVirgin(); // block access!
				Debugger(ilNormal | flApplication) << "!! NetFilterAdblock Blocked request: " << requestUri;

				return;
			}
		}
		
		//auto startTime = std::chrono::high_resolution_clock::now();
		//Debugger(ilNormal | flApplication) << "Execution Time AdblockFilter: " << (std::chrono::high_resolution_clock::now() - startTime).count() << "us";
	}
	
	// Make this adapter non-callable
	libecap::host::Xaction *x = hostx;
	hostx = 0;
	
	// Tell the host to use the virgin message (request is not blacklisted)
	x->useVirgin();
}

void Adapter::Xaction::stop() {
	hostx = 0;
	// the caller will delete
}

void Adapter::Xaction::noBodySupport() const {
	Must(!"must not be called: nblock reqmod adapter offers no body support");
	// not reached
}

// create the adapter and register with libecap to reach the host application
static bool Register(const std::string &mode) {
	libecap::RegisterVersionedService(new Adapter::Service(mode));
	return true;
}

static const bool RegisteredReqmod = Register("CLIENT_REQUEST_MODE");
static const bool RegisteredRespmod = Register("SERVER_RESPONSE_MODE");