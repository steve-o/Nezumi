/* RFA 7.2 broadcast nee non-interactive provider.
 */

#include <cstdlib>
#include "nezumi.hh"

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

	nezumi::nezumi_t nezumi;
	return nezumi.run (argc, argv);
}

/* eof */
