/* RFA 7.2 broadcast nee non-interactive provider.
 */

#include "nezumi.hh"

#include <cstdlib>

#include "chromium/command_line.hh"
#include "chromium/logging.hh"

int
main (
	int		argc,
	const char*	argv[]
	)
{
#ifdef _MSC_VER
/* Suppress abort message. */
	_set_abort_behavior (0, ~0);
#endif

	CommandLine::Init (argc, argv);
	logging::InitLogging();

	nezumi::nezumi_t nezumi;
	return nezumi.run();
}

/* eof */
