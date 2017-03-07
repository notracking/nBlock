#ifndef ECAP_NBLOCK_DEBUGGER_H
#define ECAP_NBLOCK_DEBUGGER_H

#include <libecap/common/log.h>
#include <iosfwd>

using libecap::ilNormal;
using libecap::ilCritical;
using libecap::flXaction;
using libecap::flApplication;
using libecap::mslLarge;

class Debugger {
	public:
		explicit Debugger(const libecap::LogVerbosity lv); // opens
		~Debugger(); // closes

		// logs a message if host enabled debugging at the specified level
		template <class T>
		const Debugger &operator <<(const T &msg) const {
			if (debug)
				*debug << msg;
			return *this;
		}

	private:
		/* prohibited and not implemented */
		Debugger(const Debugger&);
		Debugger &operator=(const Debugger&);

		std::ostream *debug; // host-provided debug ostream or nil
};

#endif
