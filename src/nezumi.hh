/* RFA non-interactive publisher.
 *
 * A choice is presented with broadcast feeds, either wait for a successful
 * downstream connetion then start upstream connectivity, which could be
 * subject to notable latency.  Or, one could connect upstream and simply
 * mute publishing until the downstream connection is ready, burning cycles
 * processing and subsequently dropping upstream data.  This module focuses
 * on the latter.
 */

#ifndef __NEZUMI_HH__
#define __NEZUMI_HH__

#pragma once

/* Boost noncopyable base class */
#include <boost/utility.hpp>

/* RFA 7.2 */
#include <rfa.hh>

/* RFA 7.2 additional library */
#include <StarterCommon/Timer.h>

#include "config.hh"
#include "rfa.hh"
#include "event_queue.hh"
#include "provider.hh"
#include "log.hh"

namespace nezumi
{

	class nezumi_t :
		public TimerClient,
		boost::noncopyable
	{
	public:
		nezumi_t ();
		~nezumi_t();

/* Run the provider with the given command-line parameters.
 * Returns the error code to be returned by main().
 */
		int run (int argc_, const char* argv_[]);

		void processTimer (void* closure_);

	private:

/* Clear state from previous run() */
		void clear();

/* Broadcast out message. */
		bool sendRefresh() throw (rfa::common::InvalidUsageException);

/* Application configuration. */
		const config_t config;

/* RFA context */
		rfa_t rfa;

/* RFA asynchronous event queue. */
		event_queue_t event_queue;

/* RFA logging */
		log_t log;

/* RFA provider */
		provider_t provider;

/* Starter common demo payload. */
		Encoder encoder;

/* Starter Common timer queue. */
		Timer timer;
	};

} /* namespace nezumi */

#endif /* __NEZUMI_HH__ */

/* eof */
