#ifndef LYNX_LOGGER_LOG_FILE_H
#define LYNX_LOGGER_LOG_FILE_H

#include "lynx/logger/file_util.h"

#include <memory>

namespace lynx {

/// @class LogFile
/// @brief A class that manages the appending of log lines, file rolling, and
/// flushing to disk.
///
/// It uses a unique file name for each rolled log file based on a base name
/// and a timestamp.
class LogFile : Noncopyable {
public:
  LogFile(const std::string &basename, off_t rollSize, bool threadSafe = true,
          int flushInterval = 3, int checkEveryN = 1024);
  ~LogFile();

  // Appends a log line to the current log file.
  void append(const char *logline, int len);
  void flush();

  /// Rolls the current log file if it has reached the roll size or if the
  /// specified time period has elapsed since the last roll. Returns true if the
  /// file was rolled, false otherwise.
  bool rollFile();

private:
  /// Appends a log line to the current log file without any thread-safety
  /// checks. This function is intended to be called only from within the append
  /// function when thread-safety is enabled.
  void appendUnlocked(const char *logline, int len);

  /// Generates the log file name based on the basename and the current time.
  static std::string getLogFileName(const std::string &basename, time_t *now);

  const std::string basename_;
  const off_t roll_size_;    /// The maximum size of a log file before rolling.
  const int flush_interval_; /// The interval (in seconds) at which to flush the
                             /// log file.
  const int check_every_n_; /// The number of log lines after which to check for
                            /// rolling/flushing.

  int count_;
  std::unique_ptr<std::mutex> mutex_;
  time_t start_of_period_;
  time_t last_roll_;
  time_t last_flush_;
  std::unique_ptr<util::AppendFile> file_;

  const static int K_ROLL_PER_SECONDS = 60 * 60 * 24;
};

} // namespace lynx

#endif
