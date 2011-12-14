/* RFA context.
 *
 */

#include <cassert>

/* RFA 7.2 headers */
#include <rfa.hh>

#include "rfa.hh"

using rfa::common::RFA_String;

static const RFA_String kContextName ("RFA");
static const RFA_String kConnectionType ("RSSL_NIPROV");

nezumi::rfa_t::rfa_t (const config_t& config) :
	config_ (config),
	rfa_config_ (nullptr)
{
}

nezumi::rfa_t::~rfa_t()
{
	if (nullptr != rfa_config_)
		rfa_config_->release();
	rfa::common::Context::uninitialize();
}

bool
nezumi::rfa_t::init()
{
	const RFA_String sessionName (config_.session_name.c_str(), 0, false),
		connectionName (config_.connection_name.c_str(), 0, false);

	rfa::common::Context::initialize();

/* 8.2.3 Populate Config Database.
 */
	rfa::config::StagingConfigDatabase* staging = rfa::config::StagingConfigDatabase::create();
	assert (nullptr != staging);

/* Disable Windows Event Logger. */
	RFA_String name, value;

	name.set ("\\Logger\\AppLogger\\windowsLoggerEnabled");
	staging->setBool (name, false);
/* Session list */
	name = "\\Sessions\\" + sessionName + "\\connectionList";
	staging->setString (name, connectionName);
/* Connection list */
	name = "\\Connections\\" + connectionName + "\\connectionType";
	staging->setString (name, kConnectionType);
	name = "\\Connections\\" + connectionName + "\\hostname";
	value.set (config_.adh_address.c_str());
	staging->setString (name, value);
	name = "\\Connections\\" + connectionName + "\\rsslPort";
	value.set (config_.adh_port.c_str());
	staging->setString (name, value);

	rfa_config_ = rfa::config::ConfigDatabase::acquire (kContextName);
	assert (nullptr != rfa_config_);

	const bool is_config_merged = rfa_config_->merge (*staging);
	staging->destroy();
	if (!is_config_merged)
		return false;

	return true;
}

/* eof */
