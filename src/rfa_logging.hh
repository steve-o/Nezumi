/* RFA logging consumer.
 */

#ifndef __RFA_LOGGING_HH__
#define __RFA_LOGGING_HH__

#pragma once

/* Boost noncopyable base class */
#include <boost/utility.hpp>

/* RFA 7.2 */
#include <rfa.hh>

#include "config.hh"

namespace logging
{

	class LogEventProvider :
		public rfa::common::Client,
		boost::noncopyable
	{
	public:
		LogEventProvider (const nezumi::config_t& config, rfa::common::EventQueue& event_queue);
		~LogEventProvider ();

		bool Register () throw (rfa::common::InvalidUsageException, rfa::common::InvalidConfigurationException);
		bool Unregister();

/* RFA event callback. */
		void processEvent (const rfa::common::Event& event_);

	private:

		void processLoggerNotifyEvent (const rfa::logger::LoggerNotifyEvent& event_);

		const nezumi::config_t& config_;

/* RFA event queue. */
		rfa::common::EventQueue& event_queue_;

/* RFA "application logger", a logging transport. */
		rfa::logger::ApplicationLogger*	logger_;

/* RFA "application logger monitor", an RFA event source. */
		rfa::logger::AppLoggerMonitor* monitor_;

/* RFA log event consumer. */
		rfa::common::Handle* handle_;
	};

} /* namespace logging */

#endif /* __RFA_LOGGING_HH__ */

/* eof */
