/* RFA logging target.
 */

#ifndef __LOG_HH__
#define __LOG_HH__

#pragma once

/* Boost noncopyable base class */
#include <boost/utility.hpp>

/* RFA 7.2 */
#include <rfa.hh>

#include "event_queue.hh"

namespace nezumi
{

	class log_t :
		public rfa::common::Client,
		boost::noncopyable
	{
	public:
		log_t();
		~log_t();

		bool init (const char* application_name_);
		bool registerLoggerClient (event_queue_t& event_queue_) throw (rfa::common::InvalidUsageException, rfa::common::InvalidConfigurationException);

/* RFA event callback */
		void processEvent (const rfa::common::Event& event_);

	private:

		void processLoggerNotifyEvent (const rfa::logger::LoggerNotifyEvent& event_);

/* Log router to target a custom client */
		rfa::logger::ApplicationLogger*	router;

/* Log monitor to filter events from a log router */
		rfa::logger::AppLoggerMonitor* monitor;
	};

} /* namespace nezumi */

#endif /* __LOG_HH__ */

/* eof */
