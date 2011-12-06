/* User-configurable settings.
 */

#include "config.hh"

nezumi::config_t::config_t() :
	field_dictionary_path ("c:/rfa7.2.0.L1.win-shared.rrg/etc/RDM/RDMFieldDictionary"),
	enumtype_dictionary_path ("c:/rfa7.2.0.L1.win-shared.rrg/etc/RDM/enumtype.def"),
	adh_address ("nylabadh2"),
	adh_port ("14003"),
	service_name ("NI_VTA"),
	instance_id (/* 1 */ "<instance id>"),
	application_id (/* 256 */ "256"),
	user_name ("user1"),
	position ("")
{
}

/* eof */
