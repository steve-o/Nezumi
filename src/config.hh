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

//  TREP-RT ADH hostname
		std::string adh_address;

//  TREP-RT ADH port, e.g. 14002
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

} /* namespace nezumi */

#endif /* __CONFIG_HH__ */

/* eof */
