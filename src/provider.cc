/* RFA provider.
 *
 * One single provider, and hence wraps a RFA session for simplicity.
 * Connection events (7.4.7.4, 7.5.8.3) are ignored as they're completely
 * useless.
 */

#include "provider.hh"

#include <utility>

/* RFA 7.2 headers */
#include <rfa.hh>

#include "error.hh"
#include "logging.hh"

using rfa::common::RFA_String;

/* 7.2.1 Configuring the Session Layer Package.
 * The config database is specified by the context name which must be "RFA".
 */
static const RFA_String kContextName ("RFA");

/* Reuters Wire Format nomenclature for dictionary names. */
static const RFA_String kRdmFieldDictionaryName ("RWFFld");
static const RFA_String kEnumTypeDictionaryName ("RWFEnum");

nezumi::provider_t::provider_t (
	const nezumi::config_t& config,
	nezumi::rfa_t& rfa,
	rfa::common::EventQueue& event_queue
	) :
	config_ (config),
	rfa_ (rfa),
	event_queue_ (event_queue),
	session_ (nullptr),
	provider_ (nullptr),
	rwf_major_version_ (0),
	rwf_minor_version_ (0),
	is_muted_ (true)
{
}

nezumi::provider_t::~provider_t()
{
	if (nullptr != provider_)
		provider_->destroy();
}

bool
nezumi::provider_t::init()
{
/* 7.2.1 Configuring the Session Layer Package.
 */
	const RFA_String sessionName (config_.session_name.c_str(), 0, false);
	session_ = rfa::sessionLayer::Session::acquire (sessionName);
	assert (nullptr != session_);

/* 6.2.2.1 RFA Version Info.  The version is only available if an application
 * has acquired a Session (i.e., the Session Layer library is loaded).
 */
	LOG(INFO) << "RFA: { productVersion: \"" << rfa::common::Context::getRFAVersionInfo()->getProductVersion() << "\" }";

/* 7.5.6 Initializing an OMM Non-Interactive Provider. */
	const RFA_String publisherName (config_.publisher_name.c_str(), 0, false);
	provider_ = session_->createOMMProvider (publisherName, nullptr);
	if (nullptr == provider_)
		return false;

/* 7.5.7 Registering for Events from an OMM Non-Interactive Provider. */
/* receive error events (OMMCmdErrorEvent) related to calls to submit(). */
	rfa::sessionLayer::OMMErrorIntSpec ommErrorIntSpec;
	rfa::common::Handle* handle = provider_->registerClient (&event_queue_, &ommErrorIntSpec, *this, nullptr /* closure */);
	if (nullptr == handle)
		return false;

	return sendLoginRequest();
}

/* 7.3.5.3 Making a Login Request	
 * A Login request message is encoded and sent by OMM Consumer and OMM non-
 * interactive provider applications.
 */
bool
nezumi::provider_t::sendLoginRequest()
{
	rfa::message::ReqMsg request;
	request.setMsgModelType (rfa::rdm::MMT_LOGIN);
	request.setInteractionType (rfa::message::ReqMsg::InitialImageFlag | rfa::message::ReqMsg::InterestAfterRefreshFlag);

	rfa::message::AttribInfo attribInfo;
	{
		attribInfo.setNameType (rfa::rdm::USER_NAME);
		const RFA_String userName (config_.user_name.c_str(), 0, false);
		attribInfo.setName (userName);
	}

/* The request attributes ApplicationID and Position are encoded as an
 * ElementList (5.3.4).
 */
	rfa::data::ElementList elementList;
	rfa::data::ElementListWriteIterator it;
	it.start (elementList);

/* DACS Application Id.
 * e.g. "256"
 */
	rfa::data::ElementEntry element;
	element.setName (rfa::rdm::ENAME_APP_ID);
	rfa::data::DataBuffer elementData;
	{
		const RFA_String applicationId (config_.application_id.c_str(), 0, false);
		elementData.setFromString (applicationId, rfa::data::DataBuffer::StringAsciiEnum);
	}
	element.setData (elementData);
	it.bind (element);

/* DACS Position name.
 * e.g. "localhost"
 */
	element.setName (rfa::rdm::ENAME_POSITION);
	{
		const RFA_String position (config_.position.c_str(), 0, false);
		elementData.setFromString (position, rfa::data::DataBuffer::StringAsciiEnum);
	}
	element.setData (elementData);
	it.bind (element);

/* Instance Id (optional).
 * e.g. "<Instance Id>"
 */
	if (!config_.instance_id.empty())
	{
		element.setName (rfa::rdm::ENAME_INST_ID);
		const RFA_String instanceId (config_.instance_id.c_str(), 0, false);
		elementData.setFromString (instanceId, rfa::data::DataBuffer::StringAsciiEnum);
		element.setData (elementData);
		it.bind (element);
	}

	it.complete();
	attribInfo.setAttrib (elementList);
	request.setAttribInfo (attribInfo);

/* 4.2.8 Message Validation.  RFA provides an interface to verify that
 * constructed messages of these types conform to the Reuters Domain
 * Models as specified in RFA API 7 RDM Usage Guide.
 */
	RFA_String warningText;
	uint8_t validation_status = request.validateMsg (&warningText);
	if (rfa::message::MsgValidationWarning == validation_status) {
		LOG(WARNING) << "MMT_LOGIN::validateMsg: { warningText: \"" << warningText << "\" }";
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
	rfa::common::Handle* handle = provider_->registerClient (&event_queue_, &ommItemIntSpec, *this, nullptr /* closure */);
	if (nullptr == handle)
		return false;

/* Store negotiated Reuters Wire Format version information. */
	rfa::data::Map map;
	map.setAssociatedMetaInfo (*handle);
	rwf_major_version_ = map.getMajorVersion();
	rwf_minor_version_ = map.getMinorVersion();
	LOG(INFO) << "RWF: { MajorVersion: " << (unsigned)rwf_major_version_
		<< ", MinorVersion: " << (unsigned)rwf_minor_version_ << " }";
	return true;
}

/* Create an item stream for a given symbol name.  The Item Stream maintains
 * the provider state on behalf of the application.
 */
bool
nezumi::provider_t::createItemStream (
	const char* name,
	item_stream_t& item_stream
	)
{
	item_stream.name.set (name, 0, true);
	if (!is_muted_) {
		LOG(INFO) << "Generating token for " << name;
		item_stream.token = &( provider_->generateItemToken() );
		assert (nullptr != item_stream.token);
	} else {
		LOG(INFO) << "Not generating token for " << name << " as provider is muted.";
		assert (nullptr == item_stream.token);
	}
	const std::string key (name);
	auto status = directory_.insert (std::make_pair (key, &item_stream));
	assert (true == status.second);
	assert (directory_.end() != directory_.find (key));
	LOG(INFO) << "Directory size: " << directory_.size();
	return true;
}

/* Send an Rfa message through the pre-created item stream.
 */

bool
nezumi::provider_t::send (
	item_stream_t& item_stream,
	rfa::common::Msg& msg
)
{
	if (is_muted_)
		return false;
	assert (nullptr != item_stream.token);
	submit (msg, *item_stream.token);
	return true;
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
	assert (nullptr != provider_);
	return provider_->submit (&itemCmd, closure);
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
		LOG(INFO) << "Uncaught: " << event_;
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
	if (rfa::message::RespMsgEnum != msg.getMsgType()) {
		LOG(INFO) << "Uncaught: " << msg;
		return;
	}

	processRespMsg (static_cast<const rfa::message::RespMsg&>(msg));
}

void
nezumi::provider_t::processRespMsg (
	const rfa::message::RespMsg&	reply_msg
	)
{
/* Verify event is a login response event */
	if (rfa::rdm::MMT_LOGIN != reply_msg.getMsgModelType()) {
		LOG(INFO) << "Uncaught: " << reply_msg;
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
			LOG(INFO) << "Uncaught: " << reply_msg;
			break;
		}
		break;

	case rfa::common::RespStatus::ClosedEnum:
		processLoginClosed (reply_msg);
		break;

	default:
		LOG(INFO) << "Uncaught: " << reply_msg;
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
		resetTokens();
		LOG(INFO) << "Unmuting provider.";
		is_muted_ = false;

/* ignore any error */
	} catch (rfa::common::InvalidUsageException& e) {
		LOG(ERROR) << "MMT_DIRECTORY::validateMsg: { StatusText: \"" << e.getStatus().getStatusText() << "\" }";
/* cannot publish until directory is sent. */
		return;
	}
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
	getServiceDirectory (map);
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
		LOG(WARNING) << "MMT_DIRECTORY::validateMsg: { warningText: \"" << warningText << "\" }";
	} else {
		assert (rfa::message::MsgValidationOk == validation_status);
	}

/* Create and throw away first token for MMT_DIRECTORY. */
	submit (static_cast<rfa::common::Msg&> (response), provider_->generateItemToken());
	return true;
}

void
nezumi::provider_t::getServiceDirectory (
	rfa::data::Map& map
	)
{
	rfa::data::MapWriteIterator it;
	rfa::data::MapEntry mapEntry;
	rfa::data::DataBuffer dataBuffer;
	rfa::data::FilterList filterList;
	const RFA_String serviceName (config_.service_name.c_str(), 0, false);

	map.setAssociatedMetaInfo (rwf_major_version_, rwf_minor_version_);
	it.start (map);

/* No idea ... */
	map.setKeyDataType (rfa::data::DataBuffer::StringAsciiEnum);
/* One service. */
	map.setTotalCountHint (1);

/* Service name -> service filter list */
	mapEntry.setAction (rfa::data::MapEntry::Add);
	dataBuffer.setFromString (serviceName, rfa::data::DataBuffer::StringAsciiEnum);
	mapEntry.setKeyData (dataBuffer);
	getServiceFilterList (filterList);
	mapEntry.setData (static_cast<rfa::common::Data&>(filterList));
	it.bind (mapEntry);

	it.complete();
}

void
nezumi::provider_t::getServiceFilterList (
	rfa::data::FilterList& filterList
	)
{
	rfa::data::FilterListWriteIterator it;
	rfa::data::FilterEntry filterEntry;
	rfa::data::ElementList elementList;

	filterList.setAssociatedMetaInfo (rwf_major_version_, rwf_minor_version_);
	it.start (filterList);  

/* SERVICE_INFO_ID and SERVICE_STATE_ID */
	filterList.setTotalCountHint (2);

/* SERVICE_INFO_ID */
	filterEntry.setFilterId (rfa::rdm::SERVICE_INFO_ID);
	filterEntry.setAction (rfa::data::FilterEntry::Set);
	getServiceInformation (elementList);
	filterEntry.setData (static_cast<const rfa::common::Data&>(elementList));
	it.bind (filterEntry);

/* SERVICE_STATE_ID */
	filterEntry.setFilterId (rfa::rdm::SERVICE_STATE_ID);
	filterEntry.setAction (rfa::data::FilterEntry::Set);
	getServiceState (elementList);
	filterEntry.setData (static_cast<const rfa::common::Data&>(elementList));
	it.bind (filterEntry);

	it.complete();
}

/* SERVICE_INFO_ID
 * Information about a service that does not update very often.
 */
void
nezumi::provider_t::getServiceInformation (
	rfa::data::ElementList& elementList
	)
{
	rfa::data::ElementListWriteIterator it;
	rfa::data::ElementEntry element;
	rfa::data::DataBuffer dataBuffer;
	rfa::data::Array array_;
	const RFA_String serviceName (config_.service_name.c_str(), 0, false);

	elementList.setAssociatedMetaInfo (rwf_major_version_, rwf_minor_version_);
	it.start (elementList);

/* Name<AsciiString>
 * Service name. This will match the concrete service name or the service group
 * name that is in the Map.Key.
 */
	element.setName (rfa::rdm::ENAME_NAME);
	dataBuffer.setFromString (serviceName, rfa::data::DataBuffer::StringAsciiEnum);
	element.setData (dataBuffer);
	it.bind (element);
	
/* Capabilities<Array of UInt>
 * Array of valid MessageModelTypes that the service can provide. The UInt
 * MesageModelType is extensible, using values defined in the RDM Usage Guide
 * (1-255). Login and Service Directory are omitted from this list. This
 * element must be set correctly because RFA will only request an item from a
 * service if the MessageModelType of the request is listed in this element.
 */
	element.setName (rfa::rdm::ENAME_CAPABILITIES);
	getServiceCapabilities (array_);
	element.setData (static_cast<const rfa::common::Data&>(array_));
	it.bind (element);

/* DictionariesUsed<Array of AsciiString>
 * List of Dictionary names that may be required to process all of the data 
 * from this service. Whether or not the dictionary is required depends on 
 * the needs of the consumer (e.g. display application, caching application)
 */
	element.setName (rfa::rdm::ENAME_DICTIONARYS_USED);
	getServiceDictionaries (array_);
	element.setData (static_cast<const rfa::common::Data&>(array_));
	it.bind (element);

	it.complete();
}

/* Array of valid MessageModelTypes that the service can provide.
 * rfa::data::Array does not require version tagging according to examples.
 */
void
nezumi::provider_t::getServiceCapabilities (
	rfa::data::Array& capabilities
	)
{
	rfa::data::ArrayWriteIterator it;
	rfa::data::ArrayEntry arrayEntry;
	rfa::data::DataBuffer dataBuffer;

	it.start (capabilities);

/* MarketPrice = 6 */
	dataBuffer.setUInt32 (rfa::rdm::MMT_MARKET_PRICE);
	arrayEntry.setData (dataBuffer);
	it.bind (arrayEntry);

	it.complete();
}

void
nezumi::provider_t::getServiceDictionaries (
	rfa::data::Array& dictionaries
	)
{
	rfa::data::ArrayWriteIterator it;
	rfa::data::ArrayEntry arrayEntry;
	rfa::data::DataBuffer dataBuffer;

	it.start (dictionaries);

/* RDM Field Dictionary */
	dataBuffer.setFromString (kRdmFieldDictionaryName, rfa::data::DataBuffer::StringAsciiEnum);
	arrayEntry.setData (dataBuffer);
	it.bind (arrayEntry);

/* Enumerated Type Dictionary */
	dataBuffer.setFromString (kEnumTypeDictionaryName, rfa::data::DataBuffer::StringAsciiEnum);
	arrayEntry.setData (dataBuffer);
	it.bind (arrayEntry);

	it.complete();
}

/* SERVICE_STATE_ID
 * State of a service.
 */
void
nezumi::provider_t::getServiceState (
	rfa::data::ElementList& elementList
	)
{
	rfa::data::ElementListWriteIterator it;
	rfa::data::ElementEntry element;
	rfa::data::DataBuffer dataBuffer;

	elementList.setAssociatedMetaInfo (rwf_major_version_, rwf_minor_version_);
	it.start (elementList);

/* ServiceState<UInt>
 * 1: Up/Yes
 * 0: Down/No
 * Is the original provider of the data responding to new requests. All
 * existing streams are left unchanged.
 */
	element.setName (rfa::rdm::ENAME_SVC_STATE);
	dataBuffer.setUInt32 (1);
	element.setData (dataBuffer);
	it.bind (element);

/* AcceptingRequests<UInt>
 * 1: Yes
 * 0: No
 * If the value is 0, then consuming applications should not send any new
 * requests to the service provider. (Reissues may still be sent.) If an RFA
 * application makes new requests to the service, they will be queued. All
 * existing streams are left unchanged.
 */
	element.setName (rfa::rdm::ENAME_ACCEPTING_REQS);
	dataBuffer.setUInt32 (1);
	element.setData (dataBuffer);
	it.bind (element);

	it.complete();
}

/* Iterate through entire item dictionary and re-generate tokens.
 */
bool
nezumi::provider_t::resetTokens()
{
	LOG(INFO) << "Resetting " << directory_.size() << " provider tokens";
	unsigned i = 0;
	assert (nullptr != provider_);
	for (auto cit = directory_.begin();
		cit != directory_.end();
		++cit)
	{
		LOG(INFO) << "Token #" << ++i << ": " << cit->first;
		assert (nullptr != cit->second);
		cit->second->token = &( provider_->generateItemToken() );
		assert (nullptr != cit->second->token);
	}
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
	is_muted_ = true;
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
	is_muted_ = true;
}

/* 7.5.8.2 Handling CmdError Events.
 * Represents an error Event that is generated during the submit() call on the
 * OMM non-interactive provider. This Event gives the provider application
 * access to the Cmd, CmdID, closure and OMMErrorStatus for the Cmd that
 * failed.
 */
void
nezumi::provider_t::processOMMCmdErrorEvent (
	const rfa::sessionLayer::OMMCmdErrorEvent& error
	)
{
	LOG(ERROR) << "OMMCmdErrorEvent: { "
		"CmdId: " << error.getCmdID() <<
		", State: " << error.getStatus().getState() <<
		", StatusCode: " << error.getStatus().getStatusCode() <<
		", StatusText: \"" << error.getStatus().getStatusText() << "\" }";
}

/* eof */
