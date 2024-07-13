#ifndef LYNX_LOGGER_LOG_FILE_H
#define LYNX_LOGGER_LOG_FILE_H

#include "lynx/base/noncopyable.h"

#include <memory>
#include <string>

namespace lynx {

namespace fs {
class AppendFile;
}

class LogFile : Noncopyable {
public:
  LogFile(const std::string &basename, off_t rollSize, bool threadSafe = true,
          int flushInterval = 3, int checkEveryN = 1024);
  ~LogFile();

  void append(const char *logline, int len);
  void flush();
  bool rollFile();

private:
  void appendUnlocked(const char *logline, int len);

  static std::string getLogFileName(const std::string &basename, time_t *now);

  const std::string basename_;
  const off_t roll_size_;
  const int flush_interval_;
  const int check_every_n_;

  int count_;
  std::unique_ptr<std::mutex> mutex_;
  time_t start_of_period_;
  time_t last_roll_;
  time_t last_flush_;
  std::unique_ptr<fs::AppendFile> file_;

  const static int K_ROLL_PER_SECONDS = 60 * 60 * 24;
};

} // namespace lynx

#endif
