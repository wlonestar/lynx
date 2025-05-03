#ifndef LYNX_LOGGER_LOGGING_H
#define LYNX_LOGGER_LOGGING_H

#include "lynx/base/timestamp.h"
#include "lynx/logger/log_stream.h"

#include <functional>

namespace lynx {

/**
 * @class Logger
 * @brief A class that provides a flexible logging mechanism for applications.
 *
 * It allows for different levels of logging, supports automatic filename and
 * line number capture, and allows customization of output and flushing
 * behavior.
 */
class Logger {
public:
  /**
   * @enum LogLevel
   * @brief Defines the different levels of logging messages.
   *
   * Each level represents a different severity or purpose for the log message.
   * NUM_LOG_LEVELS is a sentinel value used for iteration over log levels but
   * should not be used for actual logging.
   */
  enum LogLevel {
    TRACE,          /// Trace-level message
    DEBUG,          /// Debug-level message
    INFO,           /// Informational message
    WARN,           /// Warning message
    ERROR,          /// Error message
    FATAL,          /// Fatal error message
    NUM_LOG_LEVELS, /// Used for iteration
  };

  using LogLevel = Logger::LogLevel;

  /**
   * @class SourceFile
   * @brief A nested class encapsulates information about the source file where
   * a log message originates.
   *
   * It strips the path prefix from the filename if present, making it easier
   * to read in log output.
   */
  class SourceFile {
  public:
    /**
     * @brief Constructor that takes a C-style string array (for automatic size
     * deduction).
     *
     * @param arr C-style string array
     */
    template <int N>
    SourceFile(const char (&arr)[N]) : data_(arr), size_(N - 1) {
      const char *slash = strrchr(data_, '/');
      if (slash != nullptr) {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
      }
    }

    /**
     * @brief Constructor that takes a raw C-style string pointer.
     *
     * @param filename Raw C-style string pointer
     */
    explicit SourceFile(const char *filename) : data_(filename) {
      const char *slash = strrchr(filename, '/');
      if (slash != nullptr) {
        data_ = slash + 1;
      }
      size_ = static_cast<int>(strlen(data_));
    }

    const char *data_; /// Pointer to the file name data
    int size_;         /// Size of the file name data
  };

  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char *func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();

  LogStream &stream() { return impl_.stream_; }

  static LogLevel logLevel();
  static void setLogLevel(LogLevel level);

  using OutputFunc = std::function<void(const char *, int)>;
  using FlushFunc = std::function<void()>;

  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);

private:
  /**
   * @class Impl
   * @brief A private nested class that handles the actual logging logic.
   *
   * It stores the timestamp, log stream, log level, line number, and source
   * file basename of the log message. It provides methods for formatting the
   * timestamp and finishing the log message.
   */
  class Impl {
  public:
    /**
     * Constructor
     *
     * @param level Log level
     * @param oldErrno Old error number
     * @param file Source file
     * @param line Line number
     */
    Impl(LogLevel level, int oldErrno, const SourceFile &file, int line);

    /**
     * @brief Formats the timestamp of the log message.
     *
     * This function formats the timestamp of the log message. It first
     * calculates the number of seconds and microseconds since the epoch. If the
     * second has changed since the last log message, it formats the date and
     * time using `localtime_r` and stores it in a thread-local buffer. It then
     * appends the formatted timestamp and microseconds to the log message
     * stream.
     */
    void formatTime();

    /**
     * @brief Finish the log message by appending the log level reset escape
     * sequence and a newline character.
     *
     * The log level reset escape sequence "\033[0m" is appended to reset the
     * terminal color to the default color. A newline character ("\n") is also
     * appended to separate the log messages.
     */
    void finish();

    Timestamp time_;      /// Timestamp
    LogStream stream_;    /// Log stream
    LogLevel level_;      /// Log level
    int line_;            /// Line number
    SourceFile basename_; /// Source file basename
  };

  Impl impl_; /// Logging implementation
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

template <typename T>
T *checkNotNull(Logger::SourceFile file, int line, const char *names, T *ptr) {
  if (ptr == NULL) {
    Logger(file, line, Logger::FATAL).stream() << names;
  }
  return ptr;
}

#define CHECK_NOTNULL(val)                                                     \
  lynx::checkNotNull(__FILE__, __LINE__, "'" #val "' Must be non null", (val))

} // namespace lynx

#endif
