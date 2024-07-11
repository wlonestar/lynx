#include "lynx/logger/log_file.h"
#include "lynx/base/file_util.h"
#include "lynx/base/process_info.h"

#include <cassert>
#include <memory>
#include <mutex>

namespace lynx {

LogFile::LogFile(const std::string &basename, off_t rollSize, bool threadSafe,
                 int flushInterval, int checkEveryN)
    : basename_(basename), roll_size_(rollSize), flush_interval_(flushInterval),
      check_every_n_(checkEveryN), count_(0),
      mutex_(threadSafe ? new std::mutex : nullptr), start_of_period_(0),
      last_roll_(0), last_flush_(0) {
  assert(basename.find('/') == std::string::npos);
  rollFile();
}

LogFile::~LogFile() = default;

void LogFile::append(const char *logline, int len) {
  if (mutex_) {
    std::lock_guard<std::mutex> lock(*mutex_);
    appendUnlocked(logline, len);
  } else {
    appendUnlocked(logline, len);
  }
}

void LogFile::flush() {
  if (mutex_) {
    std::lock_guard<std::mutex> lock(*mutex_);
    file_->flush();
  } else {
    file_->flush();
  }
}

bool LogFile::rollFile() {
  time_t now = 0;
  std::string filename = getLogFileName(basename_, &now);
  time_t start = now / K_ROLL_PER_SECONDS * K_ROLL_PER_SECONDS;

  if (now > last_roll_) {
    last_roll_ = now;
    last_flush_ = now;
    start_of_period_ = start;
    file_ = std::make_unique<fs::AppendFile>(filename);
    return true;
  }
  return false;
}

void LogFile::appendUnlocked(const char *logline, int len) {
  file_->append(logline, len);

  if (file_->writtenBytes() > roll_size_) {
    rollFile();
  } else {
    ++count_;
    if (count_ >= check_every_n_) {
      count_ = 0;
      time_t now = ::time(nullptr);
      time_t this_period = now / K_ROLL_PER_SECONDS * K_ROLL_PER_SECONDS;
      if (this_period != start_of_period_) {
        rollFile();
      } else if (now - last_flush_ > flush_interval_) {
        last_flush_ = now;
        file_->flush();
      }
    }
  }
}

std::string LogFile::getLogFileName(const std::string &basename, time_t *now) {
  std::string filename;
  filename.reserve(basename.size() + 64);
  filename = basename;

  char timebuf[32];
  struct tm tm;
  *now = time(nullptr);
  gmtime_r(now, &tm);
  strftime(timebuf, sizeof(timebuf), ".%Y%m%d-%H%M%S.", &tm);
  filename += timebuf;

  filename += process_info::hostname();

  char pidbuf[32];
  snprintf(pidbuf, sizeof(pidbuf), ".%d", process_info::pid());
  filename += pidbuf;

  filename += ".log";

  return filename;
}

} // namespace lynx
