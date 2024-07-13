#include "lynx/logger/logging.h"
#include "lynx/base/current_thread.h"
#include "lynx/base/timestamp.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace lynx {

thread_local char t_errnobuf[512];
thread_local char t_time[64];
thread_local time_t t_last_second;

const char *strErrorTl(int savedErrno) {
  return strerror_r(savedErrno, t_errnobuf, sizeof(t_errnobuf));
}

LogLevel initLogLevel() {
  if (::getenv("LYNX_LOG_TRACE") != nullptr) {
    return LogLevel::TRACE;
  }
  if (::getenv("LYNX_LOG_DEBUG") != nullptr) {
    return LogLevel::DEBUG;
  }
  return LogLevel::INFO;
}

LogLevel g_log_level = initLogLevel();

const char *log_level_name[LogLevel::NUM_LOG_LEVELS] = {
    "TRACE ", "DEBUG ", "INFO  ", "WARN  ", "ERROR ", "FATAL ",
};

class T {
public:
  T(const char *str, unsigned len) : str_(str), len_(len) {
    assert(strlen(str) == len);
  }

  const char *str_;
  const unsigned len_;
};

inline LogStream &operator<<(LogStream &s, T v) {
  s.append(v.str_, v.len_);
  return s;
}

inline LogStream &operator<<(LogStream &s, const Logger::SourceFile &v) {
  s.append(v.data_, v.size_);
  return s;
}

void defaultOutput(const char *msg, int len) {
  size_t n = ::fwrite(msg, 1, len, stderr);
  (void)n;
}

void defaultFlush() { ::fflush(stderr); }

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

Logger::Impl::Impl(LogLevel level, int oldErrno, const SourceFile &file,
                   int line)
    : time_(Timestamp::now()), stream_(), level_(level), line_(line),
      basename_(file) {
  formatTime();
  current_thread::tid();
  stream_ << T(current_thread::tidString(), current_thread::tidStringLength());
  stream_ << T(log_level_name[level], 6);
  if (oldErrno != 0) {
    stream_ << strErrorTl(oldErrno) << "( errno=" << oldErrno << ") ";
  }
}

void Logger::Impl::formatTime() {
  int64_t micro_seconds_since_epoch = time_.microSecondsSinceEpoch();
  auto seconds = static_cast<time_t>(micro_seconds_since_epoch /
                                     Timestamp::K_MICRO_SECONDS_PER_SECOND);
  auto micro_seconds = static_cast<int>(micro_seconds_since_epoch %
                                        Timestamp ::K_MICRO_SECONDS_PER_SECOND);

  if (seconds != t_last_second) {
    t_last_second = seconds;

    std::tm tm;
    gmtime_r(&seconds, &tm);
    snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
             tm.tm_min, tm.tm_sec);
  }

  char us[16];
  snprintf(us, sizeof(us), ".%06d ", micro_seconds);

  stream_ << T(t_time, 17) << T(us, 8);
}

void Logger::Impl::finish() {
  stream_ << " - " << basename_ << ':' << line_ << '\n';
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
