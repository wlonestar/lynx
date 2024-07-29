#ifndef LYNX_BASE_TIMESTAMP_H
#define LYNX_BASE_TIMESTAMP_H

#include <cstdio>
#include <ctime>
#include <string>
#include <sys/time.h>

namespace lynx {

class Timestamp {
public:
  Timestamp() : microsecs_since_epoch_(0) {}

  explicit Timestamp(int64_t microsecsSinceEpoch)
      : microsecs_since_epoch_(microsecsSinceEpoch) {}

  int64_t microsecsSinceEpoch() const { return microsecs_since_epoch_; }
  time_t secondsSinceEpoch() const {
    return static_cast<time_t>(microsecs_since_epoch_ / K_MICRO_SECS_PER_SEC);
  }

  bool valid() const { return microsecs_since_epoch_ > 0; }

  void swap(Timestamp &other) {
    std::swap(microsecs_since_epoch_, other.microsecs_since_epoch_);
  }

  /// format: [millsecond].[microsecond]
  std::string toString() const {
    char buf[32] = {0};
    int64_t seconds = microsecs_since_epoch_ / K_MICRO_SECS_PER_SEC;
    int64_t microseconds = microsecs_since_epoch_ % K_MICRO_SECS_PER_SEC;
    snprintf(buf, sizeof(buf), "%ld.%06ld", seconds, microseconds);
    return buf;
  }

  /// format: YYYYMMDD HH:mm:ss.SSSSSS
  std::string toFormattedString(bool showMicrosecs = true) const {
    char buf[32] = {0};
    int64_t seconds = microsecs_since_epoch_ / K_MICRO_SECS_PER_SEC;
    std::tm tm_time;
    localtime_r(&seconds, &tm_time);

    if (showMicrosecs) {
      auto microseconds =
          static_cast<int>(microsecs_since_epoch_ % K_MICRO_SECS_PER_SEC);
      snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
               tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
               tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);
    } else {
      snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
               tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
               tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    return buf;
  }

  /// use `gettimeofday` to get current time, not a syscall, costless
  static Timestamp now() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * K_MICRO_SECS_PER_SEC + tv.tv_usec);
  }

  static Timestamp invalid() { return {}; }

  static const int K_MICRO_SECS_PER_SEC = 1000 * 1000;

private:
  int64_t microsecs_since_epoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
  return lhs.microsecsSinceEpoch() < rhs.microsecsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
  return lhs.microsecsSinceEpoch() == rhs.microsecsSinceEpoch();
}

inline double timeDiff(Timestamp high, Timestamp low) {
  int64_t diff = high.microsecsSinceEpoch() - low.microsecsSinceEpoch();
  return static_cast<double>(diff) / Timestamp::K_MICRO_SECS_PER_SEC;
}

inline Timestamp addTime(Timestamp timestamp, double seconds) {
  auto delta = static_cast<int64_t>(seconds * Timestamp::K_MICRO_SECS_PER_SEC);
  return Timestamp(timestamp.microsecsSinceEpoch() + delta);
}

} // namespace lynx

#endif
