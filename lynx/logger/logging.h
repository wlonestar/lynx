#ifndef LYNX_LOGGER_LOGGING_H
#define LYNX_LOGGER_LOGGING_H

#include "lynx/base/timestamp.h"
#include "lynx/logger/log_stream.h"

#include <cstring>

namespace lynx {

class Logger {
public:
  enum LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS,
  };

  using LogLevel = Logger::LogLevel;

  class SourceFile {
  public:
    template <int N>
    SourceFile(const char (&arr)[N]) : data_(arr), size_(N - 1) {
      const char *slash = strrchr(data_, '/');
      if (slash != nullptr) {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
      }
    }

    explicit SourceFile(const char *filename) : data_(filename) {
      const char *slash = strrchr(filename, '/');
      if (slash != nullptr) {
        data_ = slash + 1;
      }
      size_ = static_cast<int>(strlen(data_));
    }

    const char *data_;
    int size_;
  };

  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char *func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();

  LogStream &stream() { return impl_.stream_; }

  static LogLevel logLevel();
  static void setLogLevel(LogLevel level);

  using OutputFunc = void (*)(const char *, int);
  using FlushFunc = void (*)();
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);

private:
  class Impl {
  public:
    Impl(LogLevel level, int oldErrno, const SourceFile &file, int line);

    void formatTime();
    void finish();

    Timestamp time_;
    LogStream stream_;
    LogLevel level_;
    int line_;
    SourceFile basename_;
  };

  Impl impl_;
};

extern Logger::LogLevel g_log_level;
inline Logger::LogLevel Logger::logLevel() { return g_log_level; }

#define LOG_TRACE                                                              \
  if (lynx::Logger::logLevel() <= lynx::Logger::TRACE)                         \
  lynx::Logger(__FILE__, __LINE__, lynx::Logger::TRACE, __func__).stream()
#define LOG_DEBUG                                                              \
  if (lynx::Logger::logLevel() <= lynx::Logger::DEBUG)                         \
  lynx::Logger(__FILE__, __LINE__, lynx::Logger::DEBUG, __func__).stream()
#define LOG_INFO                                                               \
  if (lynx::Logger::logLevel() <= lynx::Logger::INFO)                          \
  lynx::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN lynx::Logger(__FILE__, __LINE__, lynx::Logger::WARN).stream()
#define LOG_ERROR lynx::Logger(__FILE__, __LINE__, lynx::Logger::ERROR).stream()
#define LOG_FATAL lynx::Logger(__FILE__, __LINE__, lynx::Logger::FATAL).stream()
#define LOG_SYSERR lynx::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL lynx::Logger(__FILE__, __LINE__, true).stream()

#define CHECK_NOTNULL(val)                                                     \
  lynx::checkNotNull(__FILE__, __LINE__, "'" #val "' Must be non null", (val))

template <typename T>
T *checkNotNull(Logger::SourceFile file, int line, const char *names, T *ptr) {
  if (ptr == NULL) {
    Logger(file, line, Logger::FATAL).stream() << names;
  }
  return ptr;
}

} // namespace lynx

#endif
