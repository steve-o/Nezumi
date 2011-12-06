/* RFA context.
 *
 */

#include <cassert>

/* RFA 7.2 headers */
#include <rfa.hh>

#include "rfa.hh"

using rfa::common::RFA_String;

static const RFA_String kContextName ("RFA");
static const RFA_String kSessionName ("Session");
static const RFA_String kConnectionName ("Connection");
static const RFA_String kConnectionType ("RSSL_NIPROV");
static const RFA_String kVendorName ("Vendor");

nezumi::rfa_t::rfa_t (const config_t& config_) :
	config (config_),
	rfa_config (nullptr)
{
}

nezumi::rfa_t::~rfa_t()
{
	if (nullptr != rfa_config)
		rfa_config->release();
	rfa::common::Context::uninitialize();
}

bool
nezumi::rfa_t::init()
{
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
	name = "\\Sessions\\" + kSessionName + "\\connectionList";
	staging->setString (name, kConnectionName);
/* Connection list */
	name = "\\Connections\\" + kConnectionName + "\\connectionType";
	staging->setString (name, kConnectionType);
	name = "\\Connections\\" + kConnectionName + "\\hostname";
	value.set (config.adh_address.c_str());
	staging->setString (name, value);
	name = "\\Connections\\" + kConnectionName + "\\rsslPort";
	value.set (config.adh_port.c_str());
	staging->setString (name, value);

	rfa_config = rfa::config::ConfigDatabase::acquire (kContextName);
	assert (nullptr != rfa_config);

	const bool is_config_merged = rfa_config->merge (*staging);
	staging->destroy();
	if (!is_config_merged)
		return false;

	return true;
}

const char*
nezumi::rfa_t::getSessionName()
{
	return kSessionName.c_str();
}

const char*
nezumi::rfa_t::getVendorName()
{
	return kVendorName.c_str();
}

/* eof */
