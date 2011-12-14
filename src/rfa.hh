/* RFA context.
 */

#ifndef __RFA_HH__
#define __RFA_HH__

#pragma once

/* Boost noncopyable base class */
#include <boost/utility.hpp>

/* RFA 7.2 */
#include <rfa.hh>

#include "config.hh"

namespace nezumi
{

	class rfa_t :
		boost::noncopyable
	{
	public:
		rfa_t (const config_t& config);
		~rfa_t();

		bool init() throw (rfa::common::InvalidUsageException);

	private:

		const config_t& config_;		

/* Live config database */
		rfa::config::ConfigDatabase* rfa_config_;
	};

} /* namespace nezumi */

namespace rfa {
namespace common {

inline
std::ostream& operator<< (std::ostream& o, const rfa::common::RFA_String& rfa_string) {
	o << rfa_string.c_str();
	return o;
}

inline
std::ostream& operator<< (std::ostream& o, const rfa::common::Event& event_) {
	const char* type_name;
	switch (event_.getType()) {
	case common::EventQueueStatusEventEnum:	type_name = "EventQueueStatusEventEnum"; break;
	case common::ComplEventEnum:		type_name = "ComplEventEnum"; break;

	case sessionLayer::MarketDataItemEventEnum:				type_name = "MarketDataItemEventEnum"; break;
	case sessionLayer::MarketDataDictEventEnum:				type_name = "MarketDataDictEventEnum"; break; 
	case sessionLayer::MarketDataSvcEventEnum:				type_name = "MarketDataSvcEventEnum"; break;
	case sessionLayer::MarketDataManagedPubChannelReqEventEnum:		type_name = "MarketDataManagedPubChannelReqEventEnum"; break;
	case sessionLayer::MarketDataManagedPubItemReqEventEnum:		type_name = "MarketDataManagedPubItemReqEventEnum"; break;
	case sessionLayer::PubErrorEventEnum:					type_name = "PubErrorEventEnum"; break;  
	case sessionLayer::MarketDataItemContReplyEventEnum:			type_name = "MarketDataItemContReplyEventEnum"; break;
	case sessionLayer::SessionEventEnum:					type_name = "SessionEventEnum"; break;
	case sessionLayer::MarketDataCBR_NameEventEnum:				type_name = "MarketDataCBR_NameEventEnum"; break;
	case sessionLayer::EntitlementsAuthenticationEventEnum:			type_name = "EntitlementsAuthenticationEventEnum"; break;
	case sessionLayer::ConnectionEventEnum:					type_name = "ConnectionEventEnum"; break;
	case sessionLayer::MarketDataManagedActiveItemsPubEventEnum:		type_name = "MarketDataManagedActiveItemsPubEventEnum"; break;
	case sessionLayer::MarketDataManagedInactiveItemsPubEventEnum:		type_name = "MarketDataManagedInactiveItemsPubEventEnum"; break;
	case sessionLayer::MarketDataManagedPubMirroringModeEventEnum:		type_name = "MarketDataManagedPubMirroringModeEventEnum"; break; 
	case sessionLayer::MarketDataManagedItemContReqPubEventEnum:		type_name = "MarketDataManagedItemContReqPubEventEnum"; break;
	case sessionLayer::MarketDataManagedActiveClientSessionPubEventEnum:	type_name = "MarketDataManagedActiveClientSessionPubEventEnum"; break;
	case sessionLayer::MarketDataManagedInactiveClientSessionPubEventEnum:	type_name = "MarketDataManagedInactiveClientSessionPubEventEnum"; break;
	case sessionLayer::OMMItemEventEnum:					type_name = "OMMItemEventEnum"; break;
	case sessionLayer::OMMActiveClientSessionEventEnum:			type_name = "OMMActiveClientSessionEventEnum"; break;
	case sessionLayer::OMMInactiveClientSessionEventEnum:			type_name = "OMMInactiveClientSessionEventEnum"; break;
	case sessionLayer::OMMSolicitedItemEventEnum:				type_name = "OMMSolicitedItemEventEnum"; break;
	case sessionLayer::OMMCmdErrorEventEnum:				type_name = "OMMCmdErrorEventEnum"; break;

	case logger::LoggerNotifyEventEnum:	type_name = "LoggerNotifyEventEnum"; break;
	}
	o << "Event: { Type: \"" << type_name << "\""
		", isEventStreamClosed: " << event_.isEventStreamClosed() << "}";
	return o;
}

inline
std::ostream& operator<< (std::ostream& o, const rfa::common::Msg& msg_) {
	const char *hint_mask = "TODO",
		*indication_mask = "TODO",
		*model_type = "TODO",
		*msg_type;
	switch (msg_.getMsgType()) {
	case message::RespMsgEnum:	msg_type = "RespMsg"; break;
	case message::ReqMsgEnum:	msg_type = "ReqMsg"; break;
	case message::GenericMsgEnum:	msg_type = "GenericMsg"; break;
	case message::PostMsgEnum:	msg_type = "PostMsg"; break;
	case message::AckMsgEnum:	msg_type = "AckMsg"; break;
	}
	o << "Msg: { HintMask: \"" << hint_mask << "\""
		", IndicationMask: \"" << indication_mask << "\""
		", MsgModelType, \"" << model_type << "\""
		", MsgType: \"" << msg_type << "\"}";
	return o;
}

inline
std::ostream& operator<< (std::ostream& o, const rfa::common::RespStatus& status) {
	const char *data_state,
		*status_code,
		*stream_state;
	switch (status.getDataState()) {
	case RespStatus::UnspecifiedEnum:	data_state = "UnspecifiedEnum"; break;
	case RespStatus::OkEnum:		data_state = "OkEnum"; break;
	case RespStatus::SuspectEnum:		data_state = "SuspectEnum"; break;
	}
	switch (status.getStatusCode()) {
	case RespStatus::NoneEnum:		status_code = "NoneEnum"; break;
	case RespStatus::NotFoundEnum:		status_code = "NotFoundEnum"; break;
	case RespStatus::TimeoutEnum:		status_code = "TimeoutEnum"; break;
	case RespStatus::NotAuthorizedEnum:	status_code = "NotAuthorizedEnum"; break;
	case RespStatus::InvalidArgumentEnum:	status_code = "InvalidArgumentEnum"; break;
	case RespStatus::UsageErrorEnum:	status_code = "UsageErrorEnum"; break;
	case RespStatus::PreemptedEnum:		status_code = "PreemptedEnum"; break;
	case RespStatus::JustInTimeFilteringStartedEnum: status_code = "JustInTimeFilteringStartedEnum"; break;
	case RespStatus::TickByTickResumedEnum: status_code = "TickByTickResumedEnum"; break;
	case RespStatus::FailoverStartedEnum:	status_code = "FailoverStartedEnum"; break;
	case RespStatus::FailoverCompletedEnum: status_code = "FailoverCompletedEnum"; break;
	case RespStatus::GapDetectedEnum:	status_code = "GapDetectedEnum"; break;
	case RespStatus::NoResourcesEnum:	status_code = "NoResourcesEnum"; break;
	case RespStatus::TooManyItemsEnum:	status_code = "TooManyItemsEnum"; break;
	case RespStatus::AlreadyOpenEnum:	status_code = "AlreadyOpenEnum"; break;
	case RespStatus::SourceUnknownEnum:	status_code = "SourceUnknownEnum"; break;
	case RespStatus::NotOpenEnum:		status_code = "NotOpenEnum"; break;
	case RespStatus::NonUpdatingItemEnum:	status_code = "NonUpdatingItemEnum"; break;
	case RespStatus::UnsupportedViewTypeEnum: status_code = "UnsupportedViewTypeEnum"; break;
	case RespStatus::InvalidViewEnum:	status_code = "InvalidViewEnum"; break;
	case RespStatus::FullViewProvidedEnum:	status_code = "FullViewProvidedEnum"; break;
	case RespStatus::UnableToRequestAsBatchEnum: status_code = "UnableToRequestAsBatchEnum"; break;
	}
	switch (status.getStreamState()) {
	case RespStatus::UnspecifiedStreamStateEnum: stream_state = "UnspecifiedStreamStateEnum"; break;
	case RespStatus::OpenEnum:		stream_state = "OpenEnum"; break;
	case RespStatus::NonStreamingEnum:	stream_state = "NonStreamingEnum"; break;
	case RespStatus::ClosedRecoverEnum:	stream_state = "ClosedRecoverEnum"; break;
	case RespStatus::ClosedEnum:		stream_state = "ClosedEnum"; break;
	case RespStatus::RedirectedEnum:	stream_state = "RedirectedEnum"; break;
	}
	o << "RespStatus: { DataState: \"" << data_state << "\""
		", StatusCode: \"" << status_code << "\""
		", StreamState: " << stream_state << "}";
	return o;
}

inline
std::ostream& operator<< (std::ostream& o, const rfa::message::RespMsg& msg) {
	const char* model_type = "TODO";
	o << "RespMsg: { MsgModelType: \"" << model_type << "\""
		", RespStatus: " << msg.getRespStatus() << "}";
	return o;
}

} /* namespace common */
} /* namespace rfa */

#endif /* __RFA_HH__ */

/* eof */
