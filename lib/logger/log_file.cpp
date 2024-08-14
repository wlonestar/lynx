#include "lynx/logger/log_file.h"

#include <cassert>
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

  /// If the current time is greater than the last roll time,
  /// update the last roll time, last flush time, and start of period time.
  if (now > last_roll_) {
    last_roll_ = now;
    last_flush_ = now;
    start_of_period_ = start;
    /// Create a new AppendFile object with the generated filename.
    file_ = std::make_unique<util::AppendFile>(filename);
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
  filename = basename; /// (1). Copy the base name of the log file.

  char timebuf[32];
  std::tm tm;
  *now = time(nullptr);
  localtime_r(now, &tm); /// Convert the current time to local time.
  strftime(timebuf, sizeof(timebuf), ".%Y%m%d-%H%M%S.", &tm);
  filename += timebuf; /// (2). Format the current time as a string.

  char buf[256];

  if (::gethostname(buf, sizeof(buf)) == 0) {
    filename += buf; /// (3). Get the host name and append it to the filename.
  } else {
    filename += "unknownhost";
  }

  char pidbuf[32];

  snprintf(pidbuf, sizeof(pidbuf), ".%d", ::getpid());
  filename += pidbuf; /// (4). Format the process ID as a string.

  filename += ".log"; /// (5). Append the file extension to the filename.

  return filename;
}

} // namespace lynx
