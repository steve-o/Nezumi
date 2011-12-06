/* Velocity Analytics RDM publisher plugin.
 */

/* RFA 7.2 headers */
#include <rfa.hh>

/* RFA 7.2 additional library */

#include <StarterCommon/AppUtil.h>
#include <StarterCommon/CtrlBreakHandler.h>
#include <StarterCommon/Encoder.h>

#include "nezumi.hh"
#include "error.hh"

static const char* kApplicationName = "Nezumi";

using rfa::common::RFA_String;

nezumi::nezumi_t::nezumi_t() :
	rfa (config),
	provider (kApplicationName, rfa, encoder, config)
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
/* RFA StarterCommon based logging. */
	if (!log.init (kApplicationName))
		return EXIT_FAILURE;

/* Windows termination notifier. */
	if (!CtrlBreakHandler::init())
		return EXIT_FAILURE;

	try {
/* RFA context. */
		if (!rfa.init())
			goto cleanup;

/* RFA asynchronous event queue. */
		if (!event_queue.init())
			goto cleanup;

/* RFA logging. */
		if (!log.registerLoggerClient (event_queue))
			goto cleanup;

/* RFA provider. */
		if (!provider.init (event_queue))
			goto cleanup;

	} catch (rfa::common::InvalidUsageException& e) {
		AppUtil::log (
			0,
			AppUtil::ERR,
			"InvalidUsageException: { Severity: \"%s\", Classification: \"%s\", StatusText: \"%s\" }",
			severity_string (e.getSeverity()),
			classification_string (e.getClassification()),
			e.getStatus().getStatusText().c_str());
		goto cleanup;
	} catch (rfa::common::InvalidConfigurationException& e) {
		AppUtil::log (
			0,
			AppUtil::ERR,
			"InvalidConfigurationException: { Severity: \"%s\", Classification: \"%s\", StatusText: \"%s\", ParameterName: \"%s\", ParameterValue: \"%s\" }",
			severity_string (e.getSeverity()),
			classification_string (e.getClassification()),
			e.getStatus().getStatusText().c_str(),
			e.getParameterName().c_str(),
			e.getParameterValue().c_str());
		goto cleanup;
	}

/* Timer for demo periodic publishing of items.
 */
	timer.addTimerClient (*this, 1000 /* ms */, true);

	while (!CtrlBreakHandler::isTerminated())
	{
		const long nextTimerVal = timer.nextTimer();
		long timeOut = rfa::common::Dispatchable::NoWait;
		if (INFINITE != nextTimerVal) {
			if (nextTimerVal <= Timer_MinimumInterval)
				timer.processExpiredTimers();
			timeOut = nextTimerVal;
		}
		while (event_queue.dispatch (timeOut) > rfa::common::Dispatchable::NothingDispatched);
	}

	return EXIT_SUCCESS;
cleanup:
	CtrlBreakHandler::exit();
	return EXIT_FAILURE;
}

void
nezumi::nezumi_t::processTimer (
	void*	pClosure
	)
{
	try {
		sendRefresh();
	} catch (rfa::common::InvalidUsageException& e) {
		AppUtil::log (
			0,
			AppUtil::ERR,
			"InvalidUsageException: { Severity: \"%s\", Classification: \"%s\", StatusText: \"%s\" }",
			severity_string (e.getSeverity()),
			classification_string (e.getClassification()),
			e.getStatus().getStatusText().c_str());
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
	RFA_String ric ("MSFT.O"), service_name (config.service_name.c_str());
	attribInfo.setName (ric);
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

// not std::map :(  derived from rfa::common::Data
	rfa::data::FieldList fields;
	encoder.encodeMarketPriceDataBody (&fields, true /* RefreshEnum */);
	response.setPayload (fields);

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
	uint8_t validation_status = response.validateMsg (warningText);
	if (MsgValidationWarning == validation_status) {
		AppUtil::log (
			0,
			AppUtil::WARN,
			"respMsg::validateMsg: { warningText: \"%s\" }",
			warningText.c_str());
	} else {
		assert (MsgValidationOk == validation_status);
	}
#endif
	
/* Create a single token for this stream. */
	static rfa::sessionLayer::ItemToken& token = provider.generateItemToken();
	provider.submit (static_cast<rfa::common::Msg&> (response), token);
	return true;
}

/* eof */
