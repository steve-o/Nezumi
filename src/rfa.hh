/* RFA context.
 */

#ifndef __RFA_MISSING_HH__
#define __RFA_MISSING_HH__

#pragma once

/* Boost noncopyable base class */
#include <boost/utility.hpp>

/* RFA 7.2 */
#include <rfa.hh>

#include "config.hh"

namespace nezumi
{

	class rfa_t :
		boost::noncopyable
	{
	public:
		rfa_t (const config_t& config_);
		~rfa_t();

		bool init() throw (rfa::common::InvalidUsageException);
		const char* getSessionName();
		const char* getVendorName();

	private:

		const config_t& config;		

/* Live config database */
		rfa::config::ConfigDatabase* rfa_config;
	};

} /* namespace nezumi */

#endif /* __RFA_MISSING_HH__ */

/* eof */
