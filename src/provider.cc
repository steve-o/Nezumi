/* RFA provider.
 *
 * One single provider, and hence wraps a RFA session for simplicity.
 * Connection events (7.4.7.4, 7.5.8.3) are ignored as they're completely
 * useless.
 */

/* RFA 7.2 headers */
#include <rfa.hh>

/* RFA 7.2 additional library */

#include <StarterCommon/AppUtil.h>
#include <StarterCommon/ExceptionHandler.h>

#include "provider.hh"
#include "error.hh"

using rfa::common::RFA_String;

/* 7.2.1 Configuring the Session Layer Package.
 * The config database is specified by the context name which must be "RFA".
 */
static const RFA_String kContextName ("RFA");

static const char* severity_string (const int severity_);
static const char* classification_string (const int classification_);

nezumi::provider_t::provider_t (
	const char* application_name_,
	nezumi::rfa_t& rfa_,
	Encoder& encoder_,
	const nezumi::config_t& config_
) :
	application_name (application_name_),
	rfa (rfa_),
	encoder (encoder_),
	config (config_),
	session (nullptr),
	provider (nullptr),
	is_muted (true)
{
}

nezumi::provider_t::~provider_t()
{
	if (nullptr != provider)
		provider->destroy();
}

bool
nezumi::provider_t::init (
	event_queue_t& event_queue_
	)
{
/* 7.2.1 Configuring the Session Layer Package.
 */
	session = rfa::sessionLayer::Session::acquire (*new RFA_String (rfa.getSessionName()));
	assert (nullptr != session);

/* 6.2.2.1 RFA Version Info.  The version is only available if an application
 * has acquired a Session (i.e., the Session Layer library is loaded).
 */
	AppUtil::log (
		0,
		AppUtil::INFO,
		"Rfa: { productVersion: \"%s\" }",
		rfa::common::Context::getRFAVersionInfo()->getProductVersion().c_str());

/* 7.5.6 Initializing an OMM Non-Interactive Provider. */
	provider = session->createOMMProvider (application_name, nullptr);
	if (nullptr == provider)
		return false;

/* 7.5.7 Registering for Events from an OMM Non-Interactive Provider. */
/* receive error events (OMMCmdErrorEvent) related to calls to submit(). */
	rfa::sessionLayer::OMMErrorIntSpec ommErrorIntSpec;
	rfa::common::Handle* handle = event_queue_.registerOMMIntSpecClient (*provider, ommErrorIntSpec, *this, nullptr /* closure */);
	if (nullptr == handle)
		return false;

	return sendLoginRequest (event_queue_);
}

/* 7.3.5.3 Making a Login Request
 * A Login request message is encoded and sent by OMM Consumer and OMM non-
 * interactive provider applications.
 */
bool
nezumi::provider_t::sendLoginRequest (
	event_queue_t& event_queue_
	)
{
	rfa::message::ReqMsg request;
	request.setMsgModelType (rfa::rdm::MMT_LOGIN);
	request.setInteractionType (rfa::message::ReqMsg::InitialImageFlag | rfa::message::ReqMsg::InterestAfterRefreshFlag);

	rfa::message::AttribInfo attribInfo;
	attribInfo.setNameType (rfa::rdm::USER_NAME);
	attribInfo.setName (config.user_name.c_str());

	if (!config.application_id.empty() ||
		!config.position.empty() ||
		!config.instance_id.empty())
	{
/* The request attributes ApplicationID and Position are encoded as an
 * ElementList (5.3.4).
 */
		rfa::data::ElementList elementList;
		rfa::data::ElementListWriteIterator it;
		it.start (elementList);

/* Application Id (optional), not detailed by Rfa documentation.
 * e.g. "256"
 */
		if (!config.application_id.empty())
		{
			rfa::data::ElementEntry element;
			element.setName (*new RFA_String (rfa::rdm::ENAME_APP_ID));
			rfa::data::DataBuffer elementData;
			elementData.setFromString (*new RFA_String (config.application_id.c_str()), rfa::data::DataBuffer::StringAsciiEnum);
			element.setData (elementData);
			it.bind (element);
		}

/* Position name (optional).
 * e.g. "localhost"
 */
		if (!config.position.empty())
		{
			rfa::data::ElementEntry element;
			element.setName (*new RFA_String (rfa::rdm::ENAME_POSITION));
			rfa::data::DataBuffer elementData;
			elementData.setFromString (*new RFA_String (config.position.c_str()), rfa::data::DataBuffer::StringAsciiEnum);
			element.setData (elementData);
			it.bind (element);
		}

/* Instance Id (optional).
 * e.g. "<Instance Id>"
 */
		if (!config.instance_id.empty())
		{
			rfa::data::ElementEntry element;
			element.setName (*new RFA_String (rfa::rdm::ENAME_INST_ID));
			rfa::data::DataBuffer elementData;
			elementData.setFromString (*new RFA_String (config.instance_id.c_str()), rfa::data::DataBuffer::StringAsciiEnum);
			element.setData (elementData);
			it.bind (element);
		}

		it.complete();
		attribInfo.setAttrib (elementList);
	}

	request.setAttribInfo (attribInfo);

/* 4.2.8 Message Validation.  RFA provides an interface to verify that
 * constructed messages of these types conform to the Reuters Domain
 * Models as specified in RFA API 7 RDM Usage Guide.
 */
	RFA_String warningText;
	uint8_t validation_status = request.validateMsg (&warningText);
	if (rfa::message::MsgValidationWarning == validation_status) {
		AppUtil::log (
			0,
			AppUtil::WARN,
			"MMT_LOGIN::validateMsg: { warningText: \"%s\" }",
			warningText.c_str());
	} else {
		assert (rfa::message::MsgValidationOk == validation_status);
	}

/* Not saving the returned handle as we will destroy the provider to logout,
 * reference:
 * 7.4.10.6 Other Cleanup
 * Note: The application may call destroy() on an Event Source without having
 * closed all Event Streams. RFA will internally unregister all open Event
 * Streams in this case.
 */
	rfa::sessionLayer::OMMItemIntSpec ommItemIntSpec;
	ommItemIntSpec.setMsg (&request);
	rfa::common::Handle* handle = event_queue_.registerOMMIntSpecClient (*provider, ommItemIntSpec, *this, nullptr /* closure */);
	if (nullptr == handle)
		return false;

/* Store negotiated Reuters Wire Format version information. */
	rfa::data::Map map;
	map.setAssociatedMetaInfo (*handle);
	rwf_major_version = map.getMajorVersion();
	rwf_minor_version = map.getMinorVersion();
	AppUtil::log (
		0,
		AppUtil::INFO,
		"RwfVersion: { MajorVersion: %u, MinorVersion: %u }",
		rwf_major_version,
		rwf_minor_version);

	return true;
}

rfa::sessionLayer::ItemToken&
nezumi::provider_t::generateItemToken ()
{
	return provider->generateItemToken();
}

/* 7.5.9.6 Create the OMMItemCmd object and populate it with the response
 * message.  The Cmd essentially acts as a wrapper around the response message.
 * The Cmd may be created on the heap or the stack.
 */
uint32_t
nezumi::provider_t::submit (
	rfa::common::Msg& msg,
	rfa::sessionLayer::ItemToken& token,
	void* closure
	)
{
	rfa::sessionLayer::OMMItemCmd itemCmd;
	itemCmd.setMsg (msg);
/* 7.5.9.7 Set the unique item identifier. */
	itemCmd.setItemToken (&token);
/* 7.5.9.8 Write the response message directly out to the network through the
 * connection.
 */
	return provider->submit (&itemCmd, closure);
}

void
nezumi::provider_t::processEvent (
	const rfa::common::Event& event_
	)
{
	switch (event_.getType()) {
	case rfa::sessionLayer::OMMItemEventEnum:
		processOMMItemEvent (static_cast<const rfa::sessionLayer::OMMItemEvent&>(event_));
		break;

        case rfa::sessionLayer::OMMCmdErrorEventEnum:
                processOMMCmdErrorEvent (static_cast<const rfa::sessionLayer::OMMCmdErrorEvent&>(event_));
                break;

        default:
                break;
        }
}

/* 7.5.8.1 Handling Item Events (Login Events).
 */
void
nezumi::provider_t::processOMMItemEvent (
	const rfa::sessionLayer::OMMItemEvent&	item_event
	)
{
	const rfa::common::Msg& msg = item_event.getMsg();

/* Verify event is a response event */
	if (rfa::message::RespMsgEnum != msg.getMsgType())
		return;

	processRespMsg (static_cast<const rfa::message::RespMsg&>(msg));
}

void
nezumi::provider_t::processRespMsg (
	const rfa::message::RespMsg&			reply_msg
	)
{
/* Verify event is a login response event */
	if (rfa::rdm::MMT_LOGIN != reply_msg.getMsgModelType()) {
		return;
	}

	const rfa::common::RespStatus& respStatus = reply_msg.getRespStatus();

	switch (respStatus.getStreamState()) {
	case rfa::common::RespStatus::OpenEnum:
		switch (respStatus.getDataState()) {
		case rfa::common::RespStatus::OkEnum:
			processLoginSuccess (reply_msg);
			break;

		case rfa::common::RespStatus::SuspectEnum:
			processLoginSuspect (reply_msg);
			break;

		default:
			break;
		}
		break;

	case rfa::common::RespStatus::ClosedEnum:
		processLoginClosed (reply_msg);
		break;

	default:
		break;
	}
}

/* 7.5.8.1.1 Login Success.
 * The stream state is OpenEnum one has received login permission from the
 * back-end infrastructure and the non-interactive provider can start to
 * publish data, including the service directory, dictionary, and other
 * response messages of different message model types.
 */
void
nezumi::provider_t::processLoginSuccess (
	const rfa::message::RespMsg&			login_msg
	)
{
	try {
		sendDirectoryResponse();
/* ignore any error */
	} catch (rfa::common::InvalidUsageException& e) {
		AppUtil::log (
			0,
			AppUtil::ERR,
			"MMT_DIRECTORY::validateMsg: { StatusText: \"%s\" }",
			e.getStatus().getStatusText());
/* cannot publish until directory is sent. */
		return;
	}
	is_muted = false;
}

/* 7.5.9 Sending Response Messages Using an OMM Non-Interactive Provider.
 * 10.4.3 Providing Service Directory.
 * Immediately after a successful login, and before publishing data, a non-
 * interactive provider must publish a service directory that indicates
 * services and capabilities associated with the non-interactive provider and
 * includes information about supported domain types, the service’s state, QoS,
 * and any item group information associated with the service.
 */
bool
nezumi::provider_t::sendDirectoryResponse()
{
/* 7.5.9.1 Create a response message (4.2.2) */
	rfa::message::RespMsg response;

/* 7.5.9.2 Set the message model type of the response. */
	response.setMsgModelType (rfa::rdm::MMT_DIRECTORY);
/* 7.5.9.3 Set response type. */
	response.setRespType (rfa::message::RespMsg::RefreshEnum);
/* 7.5.9.4 Set the response type enumation.
 * Note type is unsolicited despite being a mandatory requirement before
 * publishing.
 */
	response.setRespTypeNum (rfa::rdm::REFRESH_UNSOLICITED);

/* 7.5.9.5 Create or re-use a request attribute object (4.2.4) */
	rfa::message::AttribInfo attribInfo;

/* DataMask: required for refresh RespMsg
 *   SERVICE_INFO_FILTER  - Static information about service.
 *   SERVICE_STATE_FILTER - Refresh or update state.
 *   SERVICE_GROUP_FILTER - Transient groups within service.
 *   SERVICE_LOAD_FILTER  - Statistics about concurrent stream support.
 *   SERVICE_DATA_FILTER  - Broadcast data.
 *   SERVICE_LINK_FILTER  - Load balance grouping.
 */
	attribInfo.setDataMask (rfa::rdm::SERVICE_INFO_FILTER | rfa::rdm::SERVICE_STATE_FILTER);
/* Name:        Not used */
/* NameType:    Not used */
/* ServiceName: Not used */
/* ServiceId:   Not used */
/* Id:          Not used */
/* Attrib:      Not used */
	response.setAttribInfo (attribInfo);

/* 5.4.4 Versioning Support.  RFA Data and Msg interfaces provide versioning
 * functionality to allow application to encode data with a connection's
 * negotiated RWF version. Versioning support applies only to OMM connection
 * types and OMM message domain models.
 */
// not std::map :(  derived from rfa::common::Data
	rfa::data::Map map;
	map.setAssociatedMetaInfo (rwf_major_version, rwf_minor_version);
	encoder.encodeDirectoryDataBody (&map, *new RFA_String (config.service_name.c_str()), *new RFA_String (rfa.getVendorName()), nullptr);
	response.setPayload (map);

	rfa::common::RespStatus status;
/* Item interaction state: Open, Closed, ClosedRecover, Redirected, NonStreaming, or Unspecified. */
	status.setStreamState (rfa::common::RespStatus::OpenEnum);
/* Data quality state: Ok, Suspect, or Unspecified. */
	status.setDataState (rfa::common::RespStatus::OkEnum);
/* Error code, e.g. NotFound, InvalidArgument, ... */
	status.setStatusCode (rfa::common::RespStatus::NoneEnum);
	response.setRespStatus (status);

/* 4.2.8 Message Validation.  RFA provides an interface to verify that
 * constructed messages of these types conform to the Reuters Domain
 * Models as specified in RFA API 7 RDM Usage Guide.
 */
	RFA_String warningText;
	uint8_t validation_status = response.validateMsg (&warningText);
	if (rfa::message::MsgValidationWarning == validation_status) {
		AppUtil::log (
			0,
			AppUtil::WARN,
			"MMT_DIRECTORY::validateMsg: { warningText: \"%s\" }",
			warningText.c_str());
	} else {
		assert (rfa::message::MsgValidationOk == validation_status);
	}

	submit (static_cast<rfa::common::Msg&> (response), generateItemToken());
	return true;
}

/* 7.5.8.1.2 Other Login States.
 * All connections are down. The application should stop publishing; it may
 * resume once the data state becomes OkEnum.
 */
void
nezumi::provider_t::processLoginSuspect (
	const rfa::message::RespMsg&			suspect_msg
	)
{
	is_muted = true;
}

/* 7.5.8.1.2 Other Login States.
 * The login failed, and the provider application failed to get permission
 * from the back-end infrastructure. In this case, the provider application
 * cannot start to publish data.
 */
void
nezumi::provider_t::processLoginClosed (
	const rfa::message::RespMsg&			logout_msg
	)
{
	is_muted = true;
}

/* 7.5.8.2 Handling CmdError Events.
 * Represents an error Event that is generated during the submit() call on the
 * OMM non-interactive provider. This Event gives the provider application
 * access to the Cmd, CmdID, closure and OMMErrorStatus for the Cmd that
 * failed.
 */
void
nezumi::provider_t::processOMMCmdErrorEvent (
	const rfa::sessionLayer::OMMCmdErrorEvent& event_
	)
{
	AppUtil::log (
		0,
		AppUtil::ERR,
		"OMMCmdErrorEvent: { CmdId: %u, State: %d, StatusCode: %d, StatusText: \"%s\" }",
		event_.getCmdID(),
		event_.getStatus().getState(),
		event_.getStatus().getStatusCode(),
		event_.getStatus().getStatusText().c_str());
}

/* eof */
