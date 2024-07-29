#include "lynx/logger/logging.h"
#include "lynx/base/current_thread.h"
#include "lynx/base/timestamp.h"

namespace lynx {

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
    if (strncmp(log_level, "INFO", 4) == 0) {
      return Logger::LogLevel::INFO;
    }
    if (strncmp(log_level, "WARN", 4) == 0) {
      return Logger::LogLevel::WARN;
    }
    if (strncmp(log_level, "ERROR", 5) == 0) {
      return Logger::LogLevel::ERROR;
    }
  }
  return Logger::LogLevel::INFO;
}

Logger::LogLevel g_log_level = initLogLevel();

const static char *log_level_name[Logger::LogLevel::NUM_LOG_LEVELS] = {
    "TRACE ", "DEBUG ", " INFO ", " WARN ", "ERROR ", "FATAL ",
};

const static char *log_level_color[Logger::LogLevel::NUM_LOG_LEVELS] = {
    "\033[36m", "\033[34m", "\033[32m", "\033[33m", "\033[31m", "\033[1;31m",
};

inline LogStream &operator<<(LogStream &s, const Logger::SourceFile &v) {
  s.append(v.data_, v.size_);
  return s;
}

void defaultOutput(const char *msg, int len) {
  size_t n = ::fwrite(msg, 1, len, stdout);
  (void)n;
}

void defaultFlush() { ::fflush(stdout); }

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

Logger::Impl::Impl(LogLevel level, int oldErrno, const SourceFile &file,
                   int line)
    : time_(Timestamp::now()), stream_(), level_(level), line_(line),
      basename_(file) {
  stream_ << log_level_color[level];
  formatTime();
  current_thread::tid();
  stream_ << current_thread::tidString();
  stream_ << log_level_name[level];
  if (oldErrno != 0) {
    stream_ << current_thread::strError(oldErrno) << "( errno=" << oldErrno
            << ") ";
  }
}

void Logger::Impl::formatTime() {
  int64_t micro_seconds_since_epoch = time_.microsecsSinceEpoch();
  auto seconds = static_cast<time_t>(micro_seconds_since_epoch /
                                     Timestamp::K_MICRO_SECS_PER_SEC);
  auto micro_seconds = static_cast<int>(micro_seconds_since_epoch %
                                        Timestamp ::K_MICRO_SECS_PER_SEC);

  if (seconds != t_last_second) {
    t_last_second = seconds;

    std::tm tm;
    localtime_r(&seconds, &tm);
    snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
             tm.tm_min, tm.tm_sec);
  }

  char us[16];
  snprintf(us, sizeof(us), ".%06d ", micro_seconds);
  stream_ << t_time << us;
}

void Logger::Impl::finish() {
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
