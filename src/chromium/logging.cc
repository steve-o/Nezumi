/* logging.cc
 *
 * C++ stream based logging, forwarding onto Velocity Analytics Engine own logging API.
 *
 * Copyright (c) 2011 The Chromium Authors. All rights reserved.
 */

#include "logging.hh"

#define NOMINMAX
#include <winsock2.h>

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <iomanip>

#include "chromium_switches.hh"
#include "command_line.hh"
#include "debug/stack_trace.hh"
#include "synchronization/lock_impl.hh"
#include "vlog.hh"

namespace logging {

namespace {

VlogInfo* g_vlog_info = nullptr;

const char* const log_severity_names[LOG_NUM_SEVERITIES] = {
	"INFO", "WARNING", "ERROR", "FATAL" };

int min_log_level = 0;

// what should be prepended to each message?
bool log_process_id = false;
bool log_thread_id = false;
bool log_timestamp = false;
bool log_tickcount = false;

// Helper functions to wrap platform differences.

int32_t CurrentProcessId() {
	return GetCurrentProcessId();
}

int32_t CurrentThreadId() {
	return GetCurrentThreadId();
}

uint64_t TickCount() {
	return GetTickCount();
}

// This class acts as a wrapper for locking the logging files.
// LoggingLock::Init() should be called from the main thread before any logging
// is done. Then whenever logging, be sure to have a local LoggingLock
// instance on the stack. This will ensure that the lock is unlocked upon
// exiting the frame.
// LoggingLocks can not be nested.
class LoggingLock {
 public:
  LoggingLock() {
    LockLogging();
  }

  ~LoggingLock() {
    UnlockLogging();
  }

  static void Init() {
    if (is_initialized)
      return;
    log_lock = new chromium::internal::LockImpl();
    is_initialized = true;
  }

 private:
  static void LockLogging() {
      // use the lock
      log_lock->Lock();
  }

  static void UnlockLogging() {
      log_lock->Unlock();
  }

  // The lock is used if log file locking is false. It helps us avoid problems
  // with multiple threads writing to the log file at the same time.  Use
  // LockImpl directly instead of using Lock, because Lock makes logging calls.
  static chromium::internal::LockImpl* log_lock;

  static bool is_initialized;
};

// static
bool LoggingLock::is_initialized = false;
// static
chromium::internal::LockImpl* LoggingLock::log_lock = NULL;

}  /* anonymous namespace */

bool ChromiumInitLoggingImpl() {

  CommandLine* command_line = CommandLine::ForCurrentProcess();

  // Don't bother initializing g_vlog_info unless we use one of the
  // vlog switches.
  if (command_line->HasSwitch(switches::kV) ||
      command_line->HasSwitch(switches::kVModule)) {
    g_vlog_info =
        new VlogInfo(command_line->GetSwitchValueASCII(switches::kV),
                     command_line->GetSwitchValueASCII(switches::kVModule),
                     &min_log_level);
  }

  LoggingLock::Init();

  LoggingLock logging_lock;

    return true;
}

void SetMinLogLevel(int level) {
  min_log_level = std::min(LOG_ERROR, level);
}

int GetMinLogLevel() {
  return min_log_level;
}

int GetVlogVerbosity() {
  return std::max(-1, LOG_INFO - GetMinLogLevel());
}

int GetVlogLevelHelper(const char*file, size_t N) {
  DCHECK_GT(N, 0U);
    // Note: g_vlog_info may change on a different thread during startup
  // (but will always be valid or NULL).
  VlogInfo* vlog_info = g_vlog_info;
  return vlog_info ?
      vlog_info->GetVlogLevel(chromium::StringPiece(file, N - 1)) :
      GetVlogVerbosity();
}

void SetLogItems(bool enable_process_id, bool enable_thread_id,
                 bool enable_timestamp, bool enable_tickcount) {
  log_process_id = enable_process_id;
  log_thread_id = enable_thread_id;
  log_timestamp = enable_timestamp;
  log_tickcount = enable_tickcount;
}

// MSVC doesn't like complex extern templates and DLLs.
#if !defined(_MSC_VER)
// Explicit instantiations for commonly used comparisons.
template std::string* MakeCheckOpString<int, int>(const int&, const int&, const char* names);
template std::string* MakeCheckOpString<unsigned long, unsigned long>(const unsigned long&, const unsigned long&, const char* names);
template std::string* MakeCheckOpString<unsigned long, unsigned int>(const unsigned long&, const unsigned int&, const char* names);
template std::string* MakeCheckOpString<unsigned int, unsigned long>(const unsigned int&, const unsigned long&, const char* names);
template std::string* MakeCheckOpString<std::string, std::string>(const std::string&, const std::string&, const char* name);
#endif

LogMessage::LogMessage(const char* file, int line, LogSeverity severity, int ctr)
    : severity_(severity), file_(file), line_(line) {
  Init(file, line);
}

LogMessage::LogMessage(const char* file, int line)
    : severity_(LOG_INFO), file_(file), line_(line) {
  Init(file, line);
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity)
    : severity_(severity), file_(file), line_(line) {
  Init(file, line);
}

LogMessage::LogMessage(const char* file, int line, std::string* result)
    : severity_(LOG_FATAL), file_(file), line_(line) {
  Init(file, line);
  stream_ << "Check failed: " << *result;
  delete result;
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity, std::string* result)
    : severity_(severity), file_(file), line_(line) {
  Init(file, line);
  stream_ << "Check failed: " << *result;
  delete result;
}

LogMessage::~LogMessage() {
#ifndef NDEBUG
	if (severity_ == LOG_FATAL) {
		// Include a stack trace on a fatal.
		chromium::debug::StackTrace trace;
		stream_ << std::endl;  // Newline to separate from log message
		trace.OutputToStream(&stream_);
	}
#endif
	stream_ << std::endl;
	std::string str_newline(stream_.str());

	fprintf(stderr, "%s", str_newline.c_str());
	fflush(stderr);
}

// writes the common header info to the stream
void LogMessage::Init(const char* file, int line) {
  std::string filename(file);
  size_t last_slash_pos = filename.find_last_of("\\/");
  if (last_slash_pos != std::string::npos)
	  filename.erase (0, last_slash_pos + 1);

  stream_ <<  '[';
  if (log_process_id)
    stream_ << CurrentProcessId() << ':';
  if (log_thread_id)
    stream_ << CurrentThreadId() << ':';
  if (log_timestamp) {
    time_t t = time(NULL);
    struct tm local_time = {0};
#if _MSC_VER >= 1400
    localtime_s(&local_time, &t);
#else
    localtime_r(&t, &local_time);
#endif
    struct tm* tm_time = &local_time;
    stream_ << std::setfill('0')
            << std::setw(2) << 1 + tm_time->tm_mon
            << std::setw(2) << tm_time->tm_mday
            << '/'
            << std::setw(2) << tm_time->tm_hour
            << std::setw(2) << tm_time->tm_min
            << std::setw(2) << tm_time->tm_sec
            << ':';
  }
  if (log_tickcount)
    stream_ << TickCount() << ':';
  if (severity_ >= 0)
    stream_ << log_severity_names[severity_];
  else
    stream_ << "VERBOSE" << -severity_;

  stream_ << ":" << filename << "(" << line << ")] ";
}

}  /* namespace logging */

/* eof */
