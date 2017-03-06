/* eCAP ClamAV Adapter                                http://www.e-cap.org/
 * Copyright (C) 2011 The Measurement Factory.
 * Distributed under GPL v2 without any warranty.                        */

#include "Debugger.h"
#include <libecap/common/registry.h>
#include <libecap/host/host.h>
#include <iostream>

// TODO: support automated prefixing of log messages

Debugger::Debugger(const libecap::LogVerbosity lv):
	debug(libecap::MyHost().openDebug(lv)) {
}

Debugger::~Debugger() {
	if (debug)
		libecap::MyHost().closeDebug(debug);
}
