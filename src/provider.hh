/* RFA provider.
 */

#ifndef __PROVIDER_HH__
#define __PROVIDER_HH__

#pragma once

#include <cstdint>

/* Boost noncopyable base class */
#include <boost/utility.hpp>

/* RFA 7.2 */
#include <rfa.hh>

/* RFA 7.2 additional library */
#include <StarterCommon/Encoder.h>

#include "rfa.hh"
#include "config.hh"
#include "event_queue.hh"

namespace nezumi
{

	class provider_t :
		public rfa::common::Client,
		boost::noncopyable
	{
	public:
		provider_t (const char* application_name_, rfa_t& rfa_, Encoder& encoder_, const config_t& config_);
		~provider_t();

		bool init (event_queue_t& event_queue_) throw (rfa::common::InvalidConfigurationException, rfa::common::InvalidUsageException);
		rfa::sessionLayer::ItemToken& generateItemToken () throw (rfa::common::InvalidUsageException);
		uint32_t submit (rfa::common::Msg& msg, rfa::sessionLayer::ItemToken& token, void* closure = nullptr) throw (rfa::common::InvalidUsageException);

/* RFA event callback. */
		void processEvent (const rfa::common::Event& event_);

	private:
		void processOMMItemEvent (const rfa::sessionLayer::OMMItemEvent& event_);
                void processRespMsg (const rfa::message::RespMsg& msg_);
                void processLoginResponse (const rfa::message::RespMsg& msg_);
                void processLoginSuccess (const rfa::message::RespMsg& msg_);
                void processLoginSuspect (const rfa::message::RespMsg& msg_);
                void processLoginClosed (const rfa::message::RespMsg& msg_);
		void processOMMCmdErrorEvent (const rfa::sessionLayer::OMMCmdErrorEvent& event_);

		bool sendLoginRequest (event_queue_t& event_queue_) throw (rfa::common::InvalidUsageException);
		bool sendDirectoryResponse();

		const char* application_name;

		const config_t& config;

/* RFA context. */
		rfa_t& rfa;

/* RFA session defines one or more connections for horizontal scaling. */
		rfa::sessionLayer::Session* session;

/* RFA OMM provider interface. */
		rfa::sessionLayer::OMMProvider* provider;

/* Reuters Wire Format versions. */
		uint8_t rwf_major_version;
		uint8_t rwf_minor_version;

/* Starter common demo payload. */
		Encoder& encoder;

/* RFA will return a CmdError message if the provider application submits data
 * before receiving a login success message.  Mute downstream publishing until
 * permission is granted to submit data.
 */
		bool is_muted;
	};

} /* namespace nezumi */

#endif /* __PROVIDER_HH__ */

/* eof */
