#ifndef LYNX_BASE_TIMESTAMP_H
#define LYNX_BASE_TIMESTAMP_H

#include <cstdio>
#include <ctime>
#include <string>
#include <sys/time.h>

namespace lynx {

class Timestamp {
public:
  Timestamp() : micro_seconds_since_epoch_(0) {}

  explicit Timestamp(int64_t microSecondsSinceEpoch)
      : micro_seconds_since_epoch_(microSecondsSinceEpoch) {}

  void swap(Timestamp &other) {
    std::swap(micro_seconds_since_epoch_, other.micro_seconds_since_epoch_);
  }

  std::string toString() const {
    char buf[32] = {0};
    int64_t seconds = micro_seconds_since_epoch_ / K_MICRO_SECONDS_PER_SECOND;
    int64_t microseconds =
        micro_seconds_since_epoch_ % K_MICRO_SECONDS_PER_SECOND;
    snprintf(buf, sizeof(buf), "%ld.%06ld", seconds, microseconds);
    return buf;
  }

  std::string toFormattedString(bool showMicroseconds = true) const {
    char buf[32] = {0};
    int64_t seconds = micro_seconds_since_epoch_ / K_MICRO_SECONDS_PER_SECOND;
    std::tm tm_time;
    gmtime_r(&seconds, &tm_time);

    if (showMicroseconds) {
      auto microseconds = static_cast<int>(micro_seconds_since_epoch_ %
                                           K_MICRO_SECONDS_PER_SECOND);
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

  bool valid() const { return micro_seconds_since_epoch_ > 0; }

  int64_t microSecondsSinceEpoch() const { return micro_seconds_since_epoch_; }
  time_t secondsSinceEpoch() const {
    return static_cast<time_t>(micro_seconds_since_epoch_ /
                               K_MICRO_SECONDS_PER_SECOND);
  }

  static Timestamp now() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    int64_t seconds = tv.tv_sec;
    // struct tm *tm_time = localtime(&seconds);
    // seconds += tm_time->tm_gmtoff;
    return Timestamp(seconds * K_MICRO_SECONDS_PER_SECOND + tv.tv_usec);
  }

  static Timestamp invalid() { return {}; }

  static Timestamp fromUnixTime(time_t t) { return fromUnixTime(t, 0); }
  static Timestamp fromUnixTime(time_t t, int microseconds) {
    return Timestamp(static_cast<int64_t>(t) * K_MICRO_SECONDS_PER_SECOND +
                     microseconds);
  }

  static const int K_MICRO_SECONDS_PER_SECOND = 1000 * 1000;

private:
  int64_t micro_seconds_since_epoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
  return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
  return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline double timeDifference(Timestamp high, Timestamp low) {
  int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
  return static_cast<double>(diff) / Timestamp::K_MICRO_SECONDS_PER_SECOND;
}

inline Timestamp addTime(Timestamp timestamp, double seconds) {
  auto delta =
      static_cast<int64_t>(seconds * Timestamp::K_MICRO_SECONDS_PER_SECOND);
  return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

} // namespace lynx

#endif
