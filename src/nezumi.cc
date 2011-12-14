/* Velocity Analytics RDM publisher plugin.
 */

#include "nezumi.hh"

/* Windows SDK */
#include <windows.h>

/* RFA 7.2 headers */
#include <rfa.hh>

/* RFA 7.2 additional library */

#include <StarterCommon/Encoder.h>

#include "error.hh"

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

static bool isShutdown = false;

nezumi::nezumi_t::nezumi_t() :
	event_queue_ (nullptr),
	provider_ (nullptr),
	timer_ (nullptr)
{
}

nezumi::nezumi_t::~nezumi_t()
{
}

int
nezumi::nezumi_t::run (
	int		argc,
	const char*	argv[]
	)
{
	rfa_t* rfa = nullptr;
	logging::LogEventProvider* log = nullptr;

	LOG(INFO) << config_;

	try {
/* RFA context. */
		rfa = new rfa_t (config_);
		if (nullptr == rfa || !rfa->init())
			goto cleanup;

/* RFA asynchronous event queue. */
		const RFA_String eventQueueName (config_.event_queue_name.c_str(), 0, false);
		event_queue_ = rfa::common::EventQueue::create (eventQueueName);
		if (nullptr == event_queue_)
			goto cleanup;

/* RFA logging. */
		log = new logging::LogEventProvider (config_, *event_queue_);
		if (nullptr == log || !log->Register())
			goto cleanup;

/* RFA provider. */
		provider_ = new provider_t (config_, *rfa, *event_queue_);
		if (nullptr == provider_ || !provider_->init())
			goto cleanup;

		{
			char buffer[1024];
			sprintf (buffer, "1/token address is %p", &msft_stream_.token);
			LOG(INFO) << buffer;
		}

/* Create state for published RIC. */
		static const char* msft = "MSFT.O";
		if (!provider_->createItemStream (msft, msft_stream_))
			goto cleanup;

/* RFA example timer queue. */
		timer_ = new Timer();
		if (nullptr == timer_)
			goto cleanup;

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
	timer_->addTimerClient (*this, 1000 /* ms */, true);

	mainLoop ();

	event_queue_->deactivate();
	event_queue_->destroy();
	log->Unregister();

	return EXIT_SUCCESS;
cleanup:
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
	::isShutdown = true;
	LOG(INFO) << message;
	return TRUE;
}

void
nezumi::nezumi_t::mainLoop()
{
	LOG(INFO) << "Entering mainloop ...";
/* Add shutdown handler. */
	::SetConsoleCtrlHandler ((PHANDLER_ROUTINE)::CtrlHandler, TRUE);
	while (!::isShutdown) {
		const long nextTimerVal = timer_->nextTimer();
		long timeOut = rfa::common::Dispatchable::NoWait;
		if (INFINITE != nextTimerVal) {
			if (nextTimerVal <= Timer_MinimumInterval)
				timer_->processExpiredTimers();
			timeOut = nextTimerVal;
		}
		while (event_queue_->dispatch (timeOut) > rfa::common::Dispatchable::NothingDispatched);
	}
/* Remove shutdown handler. */
	::SetConsoleCtrlHandler ((PHANDLER_ROUTINE)::CtrlHandler, FALSE);
	LOG(INFO) << "Mainloop terminated.";
}

void
nezumi::nezumi_t::processTimer (
	void*	pClosure
	)
{
	try {
		sendRefresh();
	} catch (rfa::common::InvalidUsageException& e) {
		LOG(ERROR) << "InvalidUsageException: { "
			"Severity: \"" << severity_string (e.getSeverity()) << "\""
			", Classification: \"" << classification_string (e.getClassification()) << "\""
			", StatusText: \"" << e.getStatus().getStatusText() << "\" }";
	}
}

bool
nezumi::nezumi_t::sendRefresh()
{
/* 7.5.9.1 Create a response message (4.2.2) */
	rfa::message::RespMsg response;

/* 7.5.9.2 Set the message model type of the response. */
	response.setMsgModelType (rfa::rdm::MMT_MARKET_PRICE);
/* 7.5.9.3 Set response type. */
	response.setRespType (rfa::message::RespMsg::RefreshEnum);
	response.setIndicationMask (response.getIndicationMask() | rfa::message::RespMsg::RefreshCompleteFlag);
/* 7.5.9.4 Set the response type enumation. */
	response.setRespTypeNum (rfa::rdm::REFRESH_UNSOLICITED);

/* 7.5.9.5 Create or re-use a request attribute object (4.2.4) */
	rfa::message::AttribInfo attribInfo;
	attribInfo.setNameType (rfa::rdm::INSTRUMENT_NAME_RIC);
	RFA_String service_name (config_.service_name.c_str(), 0, false);
	attribInfo.setName (msft_stream_.name);
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
		real64.setValue (++msft_stream_.count);
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
		LOG(WARNING) << "respMsg::validateMsg: { warningText: \"" << warningText << "\" }";
	} else {
		assert (rfa::message::MsgValidationOk == validation_status);
	}
#endif

	provider_->send (msft_stream_, static_cast<rfa::common::Msg&> (response));
	LOG(INFO) << "sent";
	return true;
}

/* eof */
