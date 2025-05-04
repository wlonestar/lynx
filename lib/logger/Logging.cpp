#include "lynx/logger/Logging.h"
#include "lynx/base/CurrentThread.h"
#include "lynx/base/Timestamp.h"

namespace lynx {

namespace detail {

/// Thread-local variables for logging time and last second.
thread_local char TTime[64];
thread_local time_t TLastSecond;

Logger::LogLevel initLogLevel() {
  char *LogLevel = ::getenv("LYNX_LOG");
  if (LogLevel != nullptr) {
    if (strncmp(LogLevel, "TRACE", 5) == 0) {
      return Logger::LogLevel::TRACE;
    }
    if (strncmp(LogLevel, "DEBUG", 5) == 0) {
      return Logger::LogLevel::DEBUG;
    }
  }
  return Logger::LogLevel::INFO;
}

const static char *LogLevelName[Logger::LogLevel::NUM_LOG_LEVELS] = {
    "TRACE ", "DEBUG ", " INFO ", " WARN ", "ERROR ", "FATAL ",
};

const static char *LogLevelColor[Logger::LogLevel::NUM_LOG_LEVELS] = {
    "\033[36m", "\033[34m", "\033[32m", "\033[33m", "\033[31m", "\033[1;31m",
};

void defaultOutput(const char *Msg, int Len) {
  size_t N = ::fwrite(Msg, 1, Len, stdout);
  (void)N;
}

void defaultFlush() { ::fflush(stdout); }

} // namespace detail

inline LogStream &operator<<(LogStream &S, const Logger::SourceFile &V) {
  S.append(V.Data, V.Size);
  return S;
}

Logger::LogLevel GLogLevel = detail::initLogLevel();
Logger::OutputFunc GOutput = detail::defaultOutput;
Logger::FlushFunc GFlush = detail::defaultFlush;

Logger::Impl::Impl(LogLevel Level, int OldErrno, const SourceFile &File,
                   int Line)
    : Time(Timestamp::now()), Stream(), Level(Level), Line(Line),
      Basename(File) {
  Stream << detail::LogLevelColor[Level]; /// (1). Append log level color.
  formatTime();                           /// (2).Format the log timestamp.
  current_thread::tid();
  Stream << current_thread::tidString(); /// (3). Append the thread id.
  Stream << detail::LogLevelName[Level]; /// (4). Append the log level name.

  /// If there is an error number, append the error message to the log stream.
  if (OldErrno != 0) {
    Stream << current_thread::strError(OldErrno) << "( errno=" << OldErrno
           << ") ";
  }
}

void Logger::Impl::formatTime() {
  /// Calculate the number of seconds and microseconds since the epoch.
  int64_t MicroSecondsSinceEpoch = Time.microsecsSinceEpoch();
  auto Seconds =
      static_cast<time_t>(MicroSecondsSinceEpoch / Timestamp::KMicroSecsPerSec);
  auto MicroSeconds =
      static_cast<int>(MicroSecondsSinceEpoch % Timestamp::KMicroSecsPerSec);

  /// Format datetime if second changed.
  if (Seconds != detail::TLastSecond) {
    detail::TLastSecond = Seconds; /// Update the last second seen.

    std::tm Tm;
    localtime_r(&Seconds, &Tm);
    // Format the date and time into a thread-local buffer.
    snprintf(detail::TTime, sizeof(detail::TTime), "%4d%02d%02d %02d:%02d:%02d",
             Tm.tm_year + 1900, Tm.tm_mon + 1, Tm.tm_mday, Tm.tm_hour,
             Tm.tm_min, Tm.tm_sec);
  }

  /// Format the microseconds with leading zeros and append it to log message.
  char Us[16];
  snprintf(Us, sizeof(Us), ".%06d ", MicroSeconds);
  Stream << detail::TTime << Us;
}

void Logger::Impl::finish() {
  /// Append the log level reset escape sequence and a newline character to log
  /// message.
  Stream << " - " << Basename << ':' << Line << "\033[0m" << '\n';
}

Logger::Logger(SourceFile File, int Line) : Impl(INFO, 0, File, Line) {}

Logger::Logger(SourceFile File, int Line, LogLevel Level)
    : Impl(Level, 0, File, Line) {}

Logger::Logger(SourceFile File, int Line, LogLevel Level, const char *Func)
    : Impl(Level, 0, File, Line) {
  Impl.Stream << Func << ' ';
}

Logger::Logger(SourceFile File, int Line, bool ToAbort)
    : Impl(ToAbort ? FATAL : ERROR, errno, File, Line) {}

Logger::~Logger() {
  Impl.finish();
  const LogStream::BufferTy &Buf(stream().buffer());
  GOutput(Buf.data(), Buf.length());
  if (Impl.Level == FATAL) {
    GFlush();
    abort();
  }
}

void Logger::setLogLevel(LogLevel Level) { GLogLevel = Level; }
void Logger::setOutput(OutputFunc Out) { GOutput = Out; }
void Logger::setFlush(FlushFunc Flush) { GFlush = Flush; }

} // namespace lynx
