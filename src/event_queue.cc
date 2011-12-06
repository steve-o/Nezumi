/* RFA asynchronous event queue.
 */

#include <cassert>

#include "event_queue.hh"

using rfa::common::RFA_String;

static const RFA_String kEventQueueName ("This is not an event queue");

nezumi::event_queue_t::event_queue_t() :
	event_queue (nullptr)
{
}

/* 6.2.1.1.4 Shutdown Event Queue.
 * An application can also call deactivate() on the Event Queue prior to
 * calling to destroy(), which prevents any more Events from being put onto the
 * Event Queue. If the application desires, it could dispatch any remaining
 * Events before destroying the Event Queue. However, it must not destroy() an
 * Event Queue while it has active event streams.
 */
nezumi::event_queue_t::~event_queue_t()
{
	if (nullptr != event_queue) {
		event_queue->deactivate();
		event_queue->destroy();
	}
}

/* 6.2.1.1.1 Initialize Event Queue.
 * The application may also not specify a name, in which case an internal name
 * is generated.
 */
bool
nezumi::event_queue_t::init()
{
	event_queue = rfa::common::EventQueue::create (kEventQueueName);
	assert (nullptr != event_queue);

	return true;
}

/* 6.2.1.1.2 Dispatch from Event Queue.
 */
long
nezumi::event_queue_t::dispatch (
	long	timeOut_
	)
{
	return event_queue->dispatch (timeOut_);
}

/* Wrapper for rfa::logger::AppLoggerMonitor::registerLoggerClient
 */
rfa::common::Handle*
nezumi::event_queue_t::registerLoggerClient (
	rfa::logger::AppLoggerMonitor& pAppLoggerMonitor_,
	rfa::logger::AppLoggerInterestSpec& interestSpec_,
	rfa::common::Client& client_,
	void* closure_
	)
{
	return pAppLoggerMonitor_.registerLoggerClient (*event_queue, interestSpec_, client_, closure_);
}

/* Wrapper for rfa::sessionLayer::OMMProvider::RegisterClient
 */
rfa::common::Handle*
nezumi::event_queue_t::registerOMMIntSpecClient (
	rfa::sessionLayer::OMMProvider& pOMMProvider_,
	const rfa::sessionLayer::OMMIntSpec& interestSpec_,
	rfa::common::Client& client_,
	void *closure_
	)
{
	return pOMMProvider_.registerClient (event_queue, &interestSpec_, client_, closure_);
}

/* eof */
