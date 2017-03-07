#include "Debugger.h"
#include <libecap/common/registry.h>
#include <libecap/host/host.h>
#include <iostream>

Debugger::Debugger(const libecap::LogVerbosity lv):
	debug(libecap::MyHost().openDebug(lv)) {
}

Debugger::~Debugger() {
	if (debug)
		libecap::MyHost().closeDebug(debug);
}
