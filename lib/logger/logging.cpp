#include "lynx/logger/logging.h"
#include "lynx/base/current_thread.h"
#include "lynx/base/timestamp.h"

namespace lynx {

namespace detail {

/// Thread-local variables for logging time and last second.
thread_local char t_time[64];
thread_local time_t t_last_second;

Logger::LogLevel initLogLevel() {
  char *log_level = ::getenv("LYNX_LOG");
  if (log_level != nullptr) {
    if (strncmp(log_level, "TRACE", 5) == 0) {
      return Logger::LogLevel::TRACE;
    }
    if (strncmp(log_level, "DEBUG", 5) == 0) {
      return Logger::LogLevel::DEBUG;
    }
  }
  return Logger::LogLevel::INFO;
}

const static char *log_level_name[Logger::LogLevel::NUM_LOG_LEVELS] = {
    "TRACE ", "DEBUG ", " INFO ", " WARN ", "ERROR ", "FATAL ",
};

const static char *log_level_color[Logger::LogLevel::NUM_LOG_LEVELS] = {
    "\033[36m", "\033[34m", "\033[32m", "\033[33m", "\033[31m", "\033[1;31m",
};

void defaultOutput(const char *msg, int len) {
  size_t n = ::fwrite(msg, 1, len, stdout);
  (void)n;
}

void defaultFlush() { ::fflush(stdout); }

} // namespace detail

inline LogStream &operator<<(LogStream &s, const Logger::SourceFile &v) {
  s.append(v.data_, v.size_);
  return s;
}

Logger::LogLevel g_log_level = detail::initLogLevel();
Logger::OutputFunc g_output = detail::defaultOutput;
Logger::FlushFunc g_flush = detail::defaultFlush;

Logger::Impl::Impl(LogLevel level, int oldErrno, const SourceFile &file,
                   int line)
    : time_(Timestamp::now()), stream_(), level_(level), line_(line),
      basename_(file) {
  stream_ << detail::log_level_color[level]; /// (1). Append log level color.
  formatTime();                              /// (2).Format the log timestamp.
  current_thread::tid();
  stream_ << current_thread::tidString();   /// (3). Append the thread id.
  stream_ << detail::log_level_name[level]; /// (4). Append the log level name.

  /// If there is an error number, append the error message to the log stream.
  if (oldErrno != 0) {
    stream_ << current_thread::strError(oldErrno) << "( errno=" << oldErrno
            << ") ";
  }
}

void Logger::Impl::formatTime() {
  /// Calculate the number of seconds and microseconds since the epoch.
  int64_t micro_seconds_since_epoch = time_.microsecsSinceEpoch();
  auto seconds = static_cast<time_t>(micro_seconds_since_epoch /
                                     Timestamp::K_MICRO_SECS_PER_SEC);
  auto micro_seconds = static_cast<int>(micro_seconds_since_epoch %
                                        Timestamp::K_MICRO_SECS_PER_SEC);

  /// Format datetime if second changed.
  if (seconds != detail::t_last_second) {
    detail::t_last_second = seconds; /// Update the last second seen.

    std::tm tm;
    localtime_r(&seconds, &tm);
    // Format the date and time into a thread-local buffer.
    snprintf(detail::t_time, sizeof(detail::t_time),
             "%4d%02d%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1,
             tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  }

  /// Format the microseconds with leading zeros and append it to log message.
  char us[16];
  snprintf(us, sizeof(us), ".%06d ", micro_seconds);
  stream_ << detail::t_time << us;
}

void Logger::Impl::finish() {
  /// Append the log level reset escape sequence and a newline character to log
  /// message.
  stream_ << " - " << basename_ << ':' << line_ << "\033[0m" << '\n';
}

Logger::Logger(SourceFile file, int line) : impl_(INFO, 0, file, line) {}

Logger::Logger(SourceFile file, int line, LogLevel level)
    : impl_(level, 0, file, line) {}

Logger::Logger(SourceFile file, int line, LogLevel level, const char *func)
    : impl_(level, 0, file, line) {
  impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, bool toAbort)
    : impl_(toAbort ? FATAL : ERROR, errno, file, line) {}

Logger::~Logger() {
  impl_.finish();
  const LogStream::Buffer &buf(stream().buffer());
  g_output(buf.data(), buf.length());
  if (impl_.level_ == FATAL) {
    g_flush();
    abort();
  }
}

void Logger::setLogLevel(LogLevel level) { g_log_level = level; }
void Logger::setOutput(OutputFunc out) { g_output = out; }
void Logger::setFlush(FlushFunc flush) { g_flush = flush; }

} // namespace lynx
