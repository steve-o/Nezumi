/* User-configurable settings.
 */

#ifndef __CONFIG_HH__
#define __CONFIG_HH__

#pragma once

#include <string>

namespace nezumi
{

	struct config_t
	{
		config_t();

//  File path for TREP-RT RDM field or fid data dictionary.
		std::string field_dictionary_path;

//  File path for TREP-RT enumerated type dictionary.
		std::string enumtype_dictionary_path;

//  RFA session name.
		std::string session_name;

//  RFA application logger monitor name.
		std::string monitor_name;

//  RFA event queue name.
		std::string event_queue_name;

//  RFA connection name.
		std::string connection_name;

//  RFA publisher name.
		std::string publisher_name;

//  RFA vendor name.
		std::string vendor_name;

//  TREP-RT ADH hostname.
		std::string adh_address;

//  TREP-RT ADH port, e.g. 14002.
		std::string adh_port;

//  TREP-RT service name, e.g. IDN_RDF.
		std::string service_name;

/* InstanceId is used to differentiate applications running on the same host.
 * If there is more than one noninteractive provider instance running on the
 * same host, they must be set as a different value by the provider
 * application. Otherwise, the infrastructure component which the providers
 * connect to will reject a login request that has the same InstanceId value
 * and cut the connection.
 * Range: "" (None) or any Ascii string, presumably to maximum RFA_String length.
 */
		std::string instance_id;

/* DACS application Id.  If the server authenticates with DACS, the consumer
 * application may be required to pass in a valid ApplicationId.
 * Range: "" (None) or 1-511 as an Ascii string.
 */
		std::string application_id;

/* DACS username, frequently non-checked and set to similar: user1.
 */
		std::string user_name;

/* DACS position, the station which the user is using.
 * Range: "" (None) or "<IPv4 address>/hostname" or "<IPv4 address>/net"
 */
		std::string position;
	};

	inline
	std::ostream& operator<< (std::ostream& o, const config_t& config) {
		o << "config_t: { field_dictionary_path: \"" << config.field_dictionary_path << "\""
			", enumtype_dictionary_path: \"" << config.enumtype_dictionary_path << "\""
			", session_name: \"" << config.session_name << "\""
			", monitor_name: \"" << config.monitor_name << "\""
			", event_queue_name: \"" << config.event_queue_name << "\""
			", connection_name: \"" << config.connection_name << "\""
			", publisher_name: \"" << config.publisher_name << "\""
			", vendor_name: \"" << config.vendor_name << "\""
			", adh_address: \"" << config.adh_address << "\""
			", adh_port: \"" << config.adh_port << "\""
			", service_name: \"" << config.service_name << "\""
			", instance_id: \"" << config.instance_id << "\""
			", application_id: \"" << config.application_id << "\""
			", user_name: \"" << config.user_name << "\""
			", position: \"" << config.position << "\" }";
		return o;
	}

} /* namespace nezumi */

#endif /* __CONFIG_HH__ */

/* eof */
