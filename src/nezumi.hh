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
#include "logging.hh"
#include "rfa.hh"
#include "rfa_logging.hh"
#include "provider.hh"

namespace nezumi
{
/* Basic example structure for application state of an item stream. */
	struct broadcast_stream_t : item_stream_t
	{
		broadcast_stream_t () :
			count (0)
		{
		}

		uint64_t	count;
	};

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
		int run (int argc, const char* argv[]);
		void processTimer (void* closure);

	private:

/* Run core event loop. */
		void mainLoop();

/* Broadcast out message. */
		bool sendRefresh() throw (rfa::common::InvalidUsageException);

/* Application configuration. */
		const config_t config_;

/* RFA asynchronous event queue. */
		rfa::common::EventQueue* event_queue_;

/* RFA provider */
		provider_t* provider_;
	
/* Starter Common timer queue. */
		Timer* timer_;

/* Item stream. */
		broadcast_stream_t msft_stream_;

/* Publish fields. */
		rfa::data::FieldList fields_;
	};

} /* namespace nezumi */

#endif /* __NEZUMI_HH__ */

/* eof */
