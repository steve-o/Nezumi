/* RFA context.
 */

#ifndef __RFA_HH__
#define __RFA_HH__

#pragma once

/* Boost noncopyable base class */
#include <boost/utility.hpp>

/* RFA 7.2 */
#include <rfa.hh>

#include "config.hh"
#include "deleter.hh"

namespace nezumi
{

	class rfa_t :
		boost::noncopyable
	{
	public:
		rfa_t (const config_t& config);
		~rfa_t();

		bool init() throw (rfa::common::InvalidUsageException);

	private:

		const config_t& config_;		

/* Live config database */
		std::unique_ptr<rfa::config::ConfigDatabase, internal::release_deleter> rfa_config_;
	};

} /* namespace nezumi */

#endif /* __RFA_HH__ */

/* eof */
