/* Non-interactive RDM publisher application.
 */

#include "nezumi.hh"

#define __STDC_FORMAT_MACROS
#include <cstdint>
#include <inttypes.h>

#include <windows.h>

#include "chromium/logging.hh"
#include "error.hh"
#include "rfa_logging.hh"
#include "rfaostream.hh"

/* RDM Usage Guide: Section 6.5: Enterprise Platform
 * For future compatibility, the DictionaryId should be set to 1 by providers.
 * The DictionaryId for the RDMFieldDictionary is 1.
 */
static const int kDictionaryId = 1;

/* RDM: Absolutely no idea. */
static const int kFieldListId = 3;

/* RDM Field Identifiers. */
static const int kRdmRdnDisplayId = 2;		/* RDNDISPLAY */
static const int kRdmTradePriceId = 6;		/* TRDPRC_1 */

using rfa::common::RFA_String;

static std::weak_ptr<rfa::common::EventQueue> g_event_queue;

nezumi::nezumi_t::~nezumi_t()
{
	LOG(INFO) << "fin.";
}

int
nezumi::nezumi_t::run ()
{
	LOG(INFO) << config_;

	try {
/* RFA context. */
		rfa_.reset (new rfa_t (config_));
		if (!(bool)rfa_ || !rfa_->init())
			goto cleanup;

/* RFA asynchronous event queue. */
		const RFA_String eventQueueName (config_.event_queue_name.c_str(), 0, false);
		event_queue_.reset (rfa::common::EventQueue::create (eventQueueName), std::mem_fun (&rfa::common::EventQueue::destroy));
		if (!(bool)event_queue_)
			goto cleanup;
/* Create weak pointer to handle application shutdown. */
		g_event_queue = event_queue_;

/* RFA logging. */
		log_.reset (new logging::LogEventProvider (config_, event_queue_));
		if (!(bool)log_ || !log_->Register())
			goto cleanup;

/* RFA provider. */
		provider_.reset (new provider_t (config_, rfa_, event_queue_));
		if (!(bool)provider_ || !provider_->init())
			goto cleanup;

/* Create state for published RIC. */
		static const std::string msft ("MSFT.O");
		auto stream = std::make_shared<broadcast_stream_t> ();
		if (!(bool)stream)
			goto cleanup;
		if (!provider_->createItemStream (msft.c_str(), stream))
			goto cleanup;
		msft_stream_ = std::move (stream);

	} catch (rfa::common::InvalidUsageException& e) {
		LOG(ERROR) << "InvalidUsageException: { "
			"Severity: \"" << severity_string (e.getSeverity()) << "\""
			", Classification: \"" << classification_string (e.getClassification()) << "\""
			", StatusText: \"" << e.getStatus().getStatusText() << "\" }";
		goto cleanup;
	} catch (rfa::common::InvalidConfigurationException& e) {
		LOG(ERROR) << "InvalidConfigurationException: { "
			"Severity: \"" << severity_string (e.getSeverity()) << "\""
			", Classification: \"" << classification_string (e.getClassification()) << "\""
			", StatusText: \"" << e.getStatus().getStatusText() << "\""
			", ParameterName: \"" << e.getParameterName() << "\""
			", ParameterValue: \"" << e.getParameterValue() << "\" }";
		goto cleanup;
	}

/* Timer for demo periodic publishing of items.
 */
	timer_.reset (new time_pump_t<boost::chrono::system_clock> (boost::chrono::system_clock::now(), boost::chrono::seconds (1), this));
	if (!(bool)timer_)
		goto cleanup;
	timer_thread_.reset (new boost::thread (*timer_.get()));
	if (!(bool)timer_thread_)
		goto cleanup;
	LOG(INFO) << "Added periodic timer, interval " << boost::chrono::seconds (1).count() << " seconds";

	LOG(INFO) << "Init complete, entering main loop.";
	mainLoop ();
	LOG(INFO) << "Main loop terminated, cleaning up.";
	clear();
	return EXIT_SUCCESS;
cleanup:
	LOG(INFO) << "Init failed, cleaning up.";
	clear();
	return EXIT_FAILURE;
}

/* On a shutdown event set a global flag and force the event queue
 * to catch the event by submitting a log event.
 */
static
BOOL
CtrlHandler (
	DWORD	fdwCtrlType
	)
{
	const char* message;
	switch (fdwCtrlType) {
	case CTRL_C_EVENT:
		message = "Caught ctrl-c event, shutting down";
		break;
	case CTRL_CLOSE_EVENT:
		message = "Caught close event, shutting down";
		break;
	case CTRL_BREAK_EVENT:
		message = "Caught ctrl-break event, shutting down";
		break;
	case CTRL_LOGOFF_EVENT:
		message = "Caught logoff event, shutting down";
		break;
	case CTRL_SHUTDOWN_EVENT:
	default:
		message = "Caught shutdown event, shutting down";
		break;
	}
/* if available, deactivate global event queue pointer to break running loop. */
	if (!g_event_queue.expired()) {
		auto sp = g_event_queue.lock();
		sp->deactivate();
	}
	LOG(INFO) << message;
	return TRUE;
}

void
nezumi::nezumi_t::mainLoop()
{
/* Add shutdown handler. */
	::SetConsoleCtrlHandler ((PHANDLER_ROUTINE)::CtrlHandler, TRUE);
	while (event_queue_->isActive()) {
		event_queue_->dispatch (rfa::common::Dispatchable::InfiniteWait);
	}
/* Remove shutdown handler. */
	::SetConsoleCtrlHandler ((PHANDLER_ROUTINE)::CtrlHandler, FALSE);
}

void
nezumi::nezumi_t::clear()
{
/* Stop generating new events. */
	if (timer_thread_) {
		timer_thread_->interrupt();
		timer_thread_->join();
	}	
	timer_thread_.reset();
	timer_.reset();

/* Signal message pump thread to exit. */
	if ((bool)event_queue_)
		event_queue_->deactivate();

	msft_stream_.reset();

/* Release everything with an RFA dependency. */
	assert (provider_.use_count() <= 1);
	provider_.reset();
	assert (log_.use_count() <= 1);
	log_.reset();
	assert (event_queue_.use_count() <= 1);
	event_queue_.reset();
	assert (rfa_.use_count() <= 1);
	rfa_.reset();
}

bool
nezumi::nezumi_t::processTimer (
	const boost::chrono::time_point<boost::chrono::system_clock>& t
	)
{
/* calculate timer accuracy, typically 15-1ms with default timer resolution.
 */
	if (DLOG_IS_ON(INFO)) {
		using namespace boost::chrono;
		auto now = system_clock::now();
		auto ms = duration_cast<milliseconds> (now - t);
		if (0 == ms.count()) {
			LOG(INFO) << "delta " << duration_cast<microseconds> (now - t).count() << "us";
		} else {
			LOG(INFO) << "delta " << ms.count() << "ms";
		}
	}

	try {
		sendRefresh();
	} catch (rfa::common::InvalidUsageException& e) {
		LOG(ERROR) << "InvalidUsageException: { "
			"Severity: \"" << severity_string (e.getSeverity()) << "\""
			", Classification: \"" << classification_string (e.getClassification()) << "\""
			", StatusText: \"" << e.getStatus().getStatusText() << "\" }";
	}
/* continue raising timer events */
	return true;
}

bool
nezumi::nezumi_t::sendRefresh()
{
/* 7.5.9.1 Create a response message (4.2.2) */
	rfa::message::RespMsg response (false);	/* reference */

/* 7.5.9.2 Set the message model type of the response. */
	response.setMsgModelType (rfa::rdm::MMT_MARKET_PRICE);
/* 7.5.9.3 Set response type. */
	response.setRespType (rfa::message::RespMsg::RefreshEnum);
	response.setIndicationMask (rfa::message::RespMsg::RefreshCompleteFlag);
/* 7.5.9.4 Set the response type enumation. */
	response.setRespTypeNum (rfa::rdm::REFRESH_UNSOLICITED);

/* 7.5.9.5 Create or re-use a request attribute object (4.2.4) */
	rfa::message::AttribInfo attribInfo (false);	/* reference */
	attribInfo.setNameType (rfa::rdm::INSTRUMENT_NAME_RIC);
	RFA_String service_name (config_.service_name.c_str(), 0, false);	/* reference */
	attribInfo.setName (msft_stream_->rfa_name);
	attribInfo.setServiceName (service_name);
	response.setAttribInfo (attribInfo);

/* 4.3.1 RespMsg.Payload */
/* 6.2.8 Quality of Service. */
	rfa::common::QualityOfService QoS;
/* Timeliness: age of data, either real-time, unspecified delayed timeliness,
 * unspecified timeliness, or any positive number representing the actual
 * delay in seconds.
 */
	QoS.setTimeliness (rfa::common::QualityOfService::realTime);
/* Rate: minimum period of change in data, either tick-by-tick, just-in-time
 * filtered rate, unspecified rate, or any positive number representing the
 * actual rate in milliseconds.
 */
	QoS.setRate (rfa::common::QualityOfService::tickByTick);
	response.setQualityOfService (QoS);

	{
// not std::map :(  derived from rfa::common::Data
		fields_.setAssociatedMetaInfo (provider_->getRwfMajorVersion(), provider_->getRwfMinorVersion());
		fields_.setInfo (kDictionaryId, kFieldListId);

		rfa::data::FieldListWriteIterator it;
		it.start (fields_);

		rfa::data::FieldEntry field;
		rfa::data::DataBuffer dataBuffer;
		rfa::data::Real64 real64;

		field.setFieldID (kRdmRdnDisplayId);
		dataBuffer.setUInt32 (100);
		field.setData (dataBuffer);
		it.bind (field);

		field.setFieldID (kRdmTradePriceId);
		real64.setValue (++msft_stream_->count);
		real64.setMagnitudeType (rfa::data::ExponentNeg2);
		dataBuffer.setReal64 (real64);
		it.bind (field);

		it.complete();
/* Set a reference to field list, not a copy */
		response.setPayload (fields_);
	}

	rfa::common::RespStatus status;
/* Item interaction state: Open, Closed, ClosedRecover, Redirected, NonStreaming, or Unspecified. */
	status.setStreamState (rfa::common::RespStatus::OpenEnum);
/* Data quality state: Ok, Suspect, or Unspecified. */
	status.setDataState (rfa::common::RespStatus::OkEnum);
/* Error code, e.g. NotFound, InvalidArgument, ... */
	status.setStatusCode (rfa::common::RespStatus::NoneEnum);
	response.setRespStatus (status);

#ifdef DEBUG
/* 4.2.8 Message Validation.  RFA provides an interface to verify that
 * constructed messages of these types conform to the Reuters Domain
 * Models as specified in RFA API 7 RDM Usage Guide.
 */
	RFA_String warningText;
	const uint8_t validation_status = response.validateMsg (&warningText);
	if (rfa::message::MsgValidationWarning == validation_status) {
		LOG(ERROR) << "respMsg::validateMsg: { warningText: \"" << warningText << "\" }";
	} else {
		assert (rfa::message::MsgValidationOk == validation_status);
	}
#endif

	provider_->send (*msft_stream_.get(), static_cast<rfa::common::Msg&> (response));
	LOG(INFO) << "Sent refresh.";
	return true;
}

/* eof */
