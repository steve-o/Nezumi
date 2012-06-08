/* RFA 7.2 broadcast nee non-interactive provider.
 */

#include "nezumi.hh"

#include <cstdlib>

#include <windows.h>
#include <mmsystem.h>

#pragma comment (lib, "winmm")

#include "chromium/command_line.hh"
#include "chromium/logging.hh"

class env_t
{
public:
	env_t (int argc, const char* argv[])
	{
/* startup from clean string */
		CommandLine::Init (argc, argv);
/* forward onto logging */
		logging::InitLogging(
			"/psych.log",
#	if 0
			logging::LOG_ONLY_TO_FILE,
#	else
			logging::LOG_NONE,
#	endif
			logging::DONT_LOCK_LOG_FILE,
			logging::APPEND_TO_OLD_LOG_FILE,
			logging::ENABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS
			);
		logging::SetLogMessageHandler (log_handler);
	}

protected:
	static bool log_handler (int severity, const char* file, int line, size_t message_start, const std::string& str)
	{
		fprintf (stdout, "%s", str.c_str());
		fflush (stdout);
		return true;
	}
};

class timecaps_t
{
	UINT wTimerRes;
public:
	timecaps_t (unsigned resolution_ms) :
		wTimerRes (0)
	{
		TIMECAPS tc;
		if (MMSYSERR_NOERROR == timeGetDevCaps (&tc, sizeof (TIMECAPS))) {
			wTimerRes = min (max (tc.wPeriodMin, resolution_ms), tc.wPeriodMax);
			if (TIMERR_NOCANDO == timeBeginPeriod (wTimerRes)) {
				LOG(WARNING) << "Minimum timer resolution " << wTimerRes << "ms is out of range.";
				wTimerRes = 0;
			}
		} else {
			LOG(WARNING) << "Failed to query timer device resolution.";
		}
	}

	~timecaps_t()
	{
		if (wTimerRes > 0)
			timeEndPeriod (wTimerRes);
	}
};

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

	env_t env (argc, argv);
	timecaps_t timecaps (1 /* ms */);

	nezumi::nezumi_t nezumi;
	return nezumi.run();
}

/* eof */
