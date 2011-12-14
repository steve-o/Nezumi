/* RFA provider.
 */

#ifndef __PROVIDER_HH__
#define __PROVIDER_HH__

#pragma once

#include <cstdint>
#include <hash_map>
#include <map>

/* Boost noncopyable base class */
#include <boost/utility.hpp>

/* RFA 7.2 */
#include <rfa.hh>

#include "rfa.hh"
#include "config.hh"
#include "event_queue.hh"

namespace nezumi
{

	struct item_stream_t
	{
		item_stream_t () :
			token (nullptr)
		{
		}

/* Fixed name for this stream. */
		rfa::common::RFA_String name;
/* Session token which is valid from login success to login close. */
		rfa::sessionLayer::ItemToken* token;
	};

	class provider_t :
		public rfa::common::Client,
		boost::noncopyable
	{
	public:
		provider_t (const config_t& config, rfa_t& rfa, rfa::common::EventQueue& event_queue);
		~provider_t();

		bool init() throw (rfa::common::InvalidConfigurationException, rfa::common::InvalidUsageException);

		bool createItemStream (const char* name, item_stream_t& item_stream) throw (rfa::common::InvalidUsageException);
		bool send (item_stream_t& item_stream, rfa::common::Msg& msg) throw (rfa::common::InvalidUsageException);

/* RFA event callback. */
		void processEvent (const rfa::common::Event& event);

		uint8_t getRwfMajorVersion() {
			return rwf_major_version_;
		}
		uint8_t getRwfMinorVersion() {
			return rwf_minor_version_;
		}

	private:
		void processOMMItemEvent (const rfa::sessionLayer::OMMItemEvent& event);
                void processRespMsg (const rfa::message::RespMsg& msg);
                void processLoginResponse (const rfa::message::RespMsg& msg);
                void processLoginSuccess (const rfa::message::RespMsg& msg);
                void processLoginSuspect (const rfa::message::RespMsg& msg);
                void processLoginClosed (const rfa::message::RespMsg& msg);
		void processOMMCmdErrorEvent (const rfa::sessionLayer::OMMCmdErrorEvent& event);

		bool sendLoginRequest() throw (rfa::common::InvalidUsageException);
		bool sendDirectoryResponse();
		void getServiceDirectory (rfa::data::Map& map);
		void getServiceFilterList (rfa::data::FilterList& filterList);
		void getServiceInformation (rfa::data::ElementList& elementList);
		void getServiceCapabilities (rfa::data::Array& capabilities);
		void getServiceDictionaries (rfa::data::Array& dictionaries);
		void getServiceState (rfa::data::ElementList& elementList);
		bool resetTokens();

		uint32_t submit (rfa::common::Msg& msg, rfa::sessionLayer::ItemToken& token, void* closure = nullptr) throw (rfa::common::InvalidUsageException);

		const config_t& config_;

/* RFA context. */
		rfa_t& rfa_;

/* RFA asynchronous event queue. */
		rfa::common::EventQueue& event_queue_;

/* RFA session defines one or more connections for horizontal scaling. */
		rfa::sessionLayer::Session* session_;

/* RFA OMM provider interface. */
		rfa::sessionLayer::OMMProvider* provider_;

/* Reuters Wire Format versions. */
		uint8_t rwf_major_version_;
		uint8_t rwf_minor_version_;

/* RFA will return a CmdError message if the provider application submits data
 * before receiving a login success message.  Mute downstream publishing until
 * permission is granted to submit data.
 */
		bool is_muted_;

/* Container of all item streams keyed by symbol name. */
		stdext::hash_map<const std::string, item_stream_t*> directory_;
	};

} /* namespace nezumi */

#endif /* __PROVIDER_HH__ */

/* eof */
