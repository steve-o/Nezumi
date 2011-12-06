/* RFA asynchronous event queue.
 */

#ifndef __EVENT_QUEUE_HH__
#define __EVENT_QUEUE_HH__

#pragma once

/* Boost noncopyable base class */
#include <boost/utility.hpp>

/* RFA 7.2 */
#include <rfa.hh>

namespace nezumi
{

	class event_queue_t :
		boost::noncopyable
	{
	public:
		event_queue_t();
		~event_queue_t();

		bool init();
		long dispatch (long timeOut_ = rfa::common::Dispatchable::NoWait);

		rfa::common::Handle* registerLoggerClient (rfa::logger::AppLoggerMonitor& pAppLoggerMonitor_, rfa::logger::AppLoggerInterestSpec& interestSpec_, rfa::common::Client& client_, void* closure_ = nullptr) throw (rfa::common::InvalidUsageException);
		rfa::common::Handle* registerOMMIntSpecClient (rfa::sessionLayer::OMMProvider& pOMMProvider_, const rfa::sessionLayer::OMMIntSpec& interestSpec_, rfa::common::Client& client_, void *closure_ = nullptr) throw (rfa::common::InvalidUsageException);

	private:
		rfa::common::EventQueue* event_queue;
	};

} /* namespace nezumi */

#endif /* __EVENT_QUEUE_HH__ */

/* eof */
