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

#include <cstdint>

/* Boost Chrono. */
#include <boost/chrono.hpp>

/* Boost noncopyable base class */
#include <boost/utility.hpp>

/* Boost threading. */
#include <boost/thread.hpp>

/* RFA 7.2 */
#include <rfa/rfa.hh>

#include "chromium/logging.hh"

#include "config.hh"
#include "provider.hh"

namespace logging
{
	class LogEventProvider;
}

namespace nezumi
{
	class rfa_t;
	class provider_t;

/* Basic example structure for application state of an item stream. */
	class broadcast_stream_t : public item_stream_t
	{
	public:
		broadcast_stream_t () :
			count (0)
		{
		}

		uint64_t	count;
	};

/* Periodic timer event source */
	template<class Clock, class Duration = typename Clock::duration>
	class time_base_t
	{
	public:
		virtual bool processTimer (const boost::chrono::time_point<Clock, Duration>& t) = 0;
	};

	template<class Clock, class Duration = typename Clock::duration>
	class time_pump_t
	{
	public:
		time_pump_t (const boost::chrono::time_point<Clock, Duration>& due_time, Duration td, time_base_t<Clock, Duration>* cb) :
			due_time_ (due_time),
			td_ (td),
			cb_ (cb)
		{
			CHECK(nullptr != cb_);
		}

		void operator()()
		{
			try {
				while (true) {
					boost::this_thread::sleep_until (due_time_);
					if (!cb_->processTimer (due_time_))
						break;
					due_time_ += td_;
				}
			} catch (boost::thread_interrupted const&) {
				LOG(INFO) << "Timer thread interrupted.";
			}
		}

	private:
		boost::chrono::time_point<Clock, Duration> due_time_;
		Duration td_;
		time_base_t<Clock, Duration>* cb_;
	};

	class nezumi_t :
		public time_base_t<boost::chrono::system_clock>,
		boost::noncopyable
	{
	public:
		~nezumi_t();

/* Run the provider with the given command-line parameters.
 * Returns the error code to be returned by main().
 */
		int run();
		void clear();

/* Configured period timer entry point. */
		bool processTimer (const boost::chrono::time_point<boost::chrono::system_clock>& t) override;

	private:

/* Run core event loop. */
		void mainLoop();

/* Broadcast out message. */
		bool sendRefresh() throw (rfa::common::InvalidUsageException);

/* Application configuration. */
		config_t config_;

/* RFA context. */
		std::shared_ptr<rfa_t> rfa_;

/* RFA asynchronous event queue. */
		std::shared_ptr<rfa::common::EventQueue> event_queue_;

/* RFA logging */
		std::shared_ptr<logging::LogEventProvider> log_;

/* RFA provider */
		std::shared_ptr<provider_t> provider_;
	
/* Item stream. */
		std::shared_ptr<broadcast_stream_t> msft_stream_;

/* Publish fields. */
		rfa::data::FieldList fields_;

/* Thread timer. */
		std::unique_ptr<time_pump_t<boost::chrono::system_clock>> timer_;
		std::unique_ptr<boost::thread> timer_thread_;
	};

} /* namespace nezumi */

#endif /* __NEZUMI_HH__ */

/* eof */
