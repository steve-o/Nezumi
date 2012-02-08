/* User-configurable settings.
 */

#include "config.hh"

static const char* kDefaultAdhPort = "14003";

nezumi::config_t::config_t() :
/* default values */
	service_name ("NI_VTA"),
	rssl_default_port (kDefaultAdhPort),
	application_id ("256"),
	instance_id ("Instance1"),
	user_name ("user1"),
	position (""),
	session_name ("SessionName"),
	monitor_name ("ApplicationLoggerMonitorName"),
	event_queue_name ("EventQueueName"),
	connection_name ("ConnectionName"),
	publisher_name ("PublisherName"),
	vendor_name ("VendorName")
{
/* C++11 initializer lists not supported in MSVC2010 */
	rssl_servers.push_back ("nylabadh2");
}

/* eof */
