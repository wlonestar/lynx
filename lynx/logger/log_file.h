#ifndef LYNX_LOGGER_LOG_FILE_H
#define LYNX_LOGGER_LOG_FILE_H

#include "lynx/logger/file_util.h"

#include <memory>

namespace lynx {

/**
 * @class LogFile
 * @brief A class that manages the appending of log lines, file rolling, and
 * flushing to disk.
 *
 * It uses a unique file name for each rolled log file based on a base name
 * and a timestamp.
 */
class LogFile : Noncopyable {
public:
  /**
   * @brief Construct a LogFile object.
   *
   * @param basename The base name of the log file.
   * @param rollSize The maximum size of a log file before rolling.
   * @param threadSafe Enable or disable thread safety.
   * @param flushInterval The interval (in seconds) at which to flush the log
   * file.
   * @param checkEveryN The number of log lines after which to check for
   * rolling/flushing.
   */
  LogFile(const std::string &basename, off_t rollSize, bool threadSafe = true,
          int flushInterval = 3, int checkEveryN = 1024);
  ~LogFile();

  /**
   * @brief Append a log line to the current log file.
   *
   * @param logline The log line to append.
   * @param len The length of the log line.
   */
  void append(const char *logline, int len);

  /**
   * @brief Flush the log file to disk.
   */
  void flush();

  /**
   * @brief Roll the current log file if it has reached the roll size or if the
   * specified time period has elapsed since the last roll.
   *
   * @return True if the file was rolled, false otherwise.
   */
  bool rollFile();

private:
  /**
   * @brief Append a log line to the current log file without any thread-safety
   * checks. This function is intended to be called only from within the append
   * function when thread-safety is enabled.
   *
   * @param logline The log line to append.
   * @param len The length of the log line.
   */
  void appendUnlocked(const char *logline, int len);

  /**
   * @brief Generate the log file name based on the base name and the current
   * time.
   *
   * @param basename The base name of the log file.
   * @param now A pointer to the current time.
   * @return The generated log file name.
   */
  static std::string getLogFileName(const std::string &basename, time_t *now);

  const std::string basename_; /// The base name of the log file.
  const off_t roll_size_;    /// The maximum size of a log file before rolling.
  const int flush_interval_; /// The interval (in seconds) at which to flush the
                             /// log file.
  const int check_every_n_; /// The number of log lines after which to check for
                            /// rolling/flushing.

  int count_;                         /// The number of log lines appended.
  std::unique_ptr<std::mutex> mutex_; /// The mutex for thread safety.
  time_t start_of_period_; /// The start time of the current time period.
  time_t last_roll_;       /// The time of the last file roll.
  time_t last_flush_;      /// The time of the last log file flush.
  std::unique_ptr<util::AppendFile> file_; /// The appended file.

  const static int K_ROLL_PER_SECONDS =
      60 * 60 * 24; /// The number of seconds in a day.
};

} // namespace lynx

#endif
