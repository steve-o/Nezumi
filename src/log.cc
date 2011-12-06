/* RFA log system.
 *
 * The RFA Developers Guide, Chapter 9 Logger Package defines the minimum requirements
 * for performing logging.
 */

#include <cassert>

/* RFA 7.2 headers */
#include <rfa.hh>

/* RFA 7.2 additional library */

#include <StarterCommon/AppUtil.h>

#include "log.hh"

using rfa::common::RFA_String;

/* 9.2.1 Configuring the Logger Package
 * The config database is specified by the context name which must be "RFA".
 */
static const RFA_String kContextName ("RFA");
static const RFA_String kMonitorName ("This is not a monitor");

nezumi::log_t::log_t() :
	router (nullptr),
	monitor (nullptr)
{
}

/* 9.3.5 RFA Application Logger Shutdown
 * Application Logger Monitors must be destroyed in the reverse order of creation.
 */
nezumi::log_t::~log_t()
{
/* Instead of unregistering the monitor filter per 9.2.4.4, just destroy the monitor. */
	if (nullptr != monitor)
		monitor->destroy();
/* 9.2.3.2 Shutting down the application logger. */
	if (nullptr != router)
		router->release ();
}

bool
nezumi::log_t::init (
	const char* application_name_
	)
{
	return AppUtil::initLog (application_name_);
}

/* 9.2.2 Logging in Applications
 * We are not sending logs to RFA so we do not initialize the component logger.  We
 * setup enough to redirect to the "Starter Common" logging utility.
 */
bool
nezumi::log_t::registerLoggerClient (
	nezumi::event_queue_t& event_queue_
	)
{
/* 9.2.3.1 Initialize the Application logger.
 * The config database may be shared between other components, implying some form of
 * reference counting.
 */
	router = rfa::logger::ApplicationLogger::acquire (kContextName);
	assert (nullptr != router);

/* 9.2.4.2 Initialize Application Logger Monitor. */
	monitor = router->createApplicationLoggerMonitor (kMonitorName, false /* no completion events */);
	assert (nullptr != monitor);

/* 9.2.4.3 Registering for Log Events.
 * Setting minimum severity to "Success" is defined as everything.
 */
	rfa::logger::AppLoggerInterestSpec log_spec;
	log_spec.setMinSeverity (rfa::common::Success);
	rfa::common::Handle* handle = event_queue_.registerLoggerClient (*monitor, log_spec, *this, nullptr /* unused closure */);
	assert (nullptr != handle);

	return true;
}

void
nezumi::log_t::processEvent (
	const rfa::common::Event& event_
	)
{
	switch (event_.getType()) {
	case rfa::logger::LoggerNotifyEventEnum:
		processLoggerNotifyEvent (static_cast<const rfa::logger::LoggerNotifyEvent&> (event_));
		break;

        default:
                break;
        }
}

void
nezumi::log_t::processLoggerNotifyEvent (
	const rfa::logger::LoggerNotifyEvent& event_
	)
{
	int level;
	switch ((int)event_.getSeverity()) {
	case rfa::common::Success:	level = AppUtil::TRACE; break;
	case rfa::common::Information:	level = AppUtil::INFO; break;
	case rfa::common::Warning:	level = AppUtil::WARN; break;
	case rfa::common::Error:	level = AppUtil::ERR; break;
	}
	AppUtil::log (
		0,
		level,
		"LoggerNotifyEvent: { Component: \"%s\", LogId: \"%ld\", Text: \"%s\" }\n",
		event_.getComponentName().c_str(),
		event_.getLogID(),
		event_.getMessageText().c_str());
}

/* eof */
