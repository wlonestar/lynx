#ifndef LYNX_BASE_TIMESTAMP_H
#define LYNX_BASE_TIMESTAMP_H

#include <cstdio>
#include <ctime>
#include <string>
#include <sys/time.h>

#include <fmt/format.h>

namespace lynx {

/**
 * @class Timestamp
 * @brief Class represents a point in time as microseconds since Unix epoch.
 *
 * It provides methods to access the timestamp in different formats and to
 * compare or manipulate timestamps.
 */
class Timestamp {
public:
  /// Constructs a new timestamp initialized to 0.
  Timestamp() : MicrosecsSinceEpoch(0) {}

  /**
   * @brief Constructs a new timestamp from the number of microseconds since
   * Unix epoch.
   *
   * @param microsecsSinceEpoch The number of microseconds since Unix epoch.
   */
  explicit Timestamp(int64_t MicrosecsSinceEpoch)
      : MicrosecsSinceEpoch(MicrosecsSinceEpoch) {}

  /// Gets the number of microseconds since Unix epoch.
  int64_t microsecsSinceEpoch() const { return MicrosecsSinceEpoch; }

  /// Gets the number of seconds since Unix epoch.
  time_t secondsSinceEpoch() const {
    return static_cast<time_t>(MicrosecsSinceEpoch / KMicroSecsPerSec);
  }

  /// Checks if the timestamp is valid.
  bool valid() const { return MicrosecsSinceEpoch > 0; }

  /// Swaps the timestamp with another one.
  void swap(Timestamp &Other) {
    std::swap(MicrosecsSinceEpoch, Other.MicrosecsSinceEpoch);
  }

  /**
   * @brief Converts the timestamp to a string in the format:
   * "[millsecond].[microsecond]".
   *
   * @return The string representation of the timestamp.
   */
  std::string toString() const {
    int64_t Seconds = MicrosecsSinceEpoch / KMicroSecsPerSec;
    int64_t Microseconds = MicrosecsSinceEpoch % KMicroSecsPerSec;
    return fmt::format("{:d}.{:06d}", Seconds, Microseconds);
  }

  /**
   * @brief Converts the timestamp to a formatted string in the format:
   * "YYYYMMDD HH:mm:ss.SSSSSS".
   *
   * @param showMicrosecs Whether to include microseconds in the formatted
   * string.
   *
   * @return The formatted string representation of the timestamp.
   */
  std::string toFormattedString(bool ShowMicrosecs = true) const {
    int64_t Seconds = MicrosecsSinceEpoch / KMicroSecsPerSec;
    std::tm TmTime;
    localtime_r(&Seconds, &TmTime);

    if (ShowMicrosecs) {
      auto Microseconds =
          static_cast<int>(MicrosecsSinceEpoch % KMicroSecsPerSec);
      return fmt::format("{:4d}{:02d}{:02d} {:02d}:{:02d}:{:02d}.{:06d}",
                         TmTime.tm_year + 1900, TmTime.tm_mon + 1,
                         TmTime.tm_mday, TmTime.tm_hour, TmTime.tm_min,
                         TmTime.tm_sec, Microseconds);
    }

    return fmt::format("{:4d}{:02d}{:02d} {:02d}:{:02d}:{:02d}",
                       TmTime.tm_year + 1900, TmTime.tm_mon + 1, TmTime.tm_mday,
                       TmTime.tm_hour, TmTime.tm_min, TmTime.tm_sec);
  }

  /// Creates a new timestamp representing the current time.
  static Timestamp now() {
    struct timeval Tv;
    gettimeofday(&Tv, nullptr);
    int64_t Seconds = Tv.tv_sec;
    return Timestamp(Seconds * KMicroSecsPerSec + Tv.tv_usec);
  }

  /// Creates an invalid timestamp.
  static Timestamp invalid() { return {}; }

  /// The number of microseconds per second.
  static const int KMicroSecsPerSec = 1000 * 1000;

private:
  int64_t MicrosecsSinceEpoch;
};

/**
 * @brief Compares two timestamps for ordering.
 *
 * @param lhs The first timestamp.
 * @param rhs The second timestamp.
 *
 * @return True if the first timestamp is less than the second timestamp, and
 * false otherwise.
 */
inline bool operator<(Timestamp Lhs, Timestamp Rhs) {
  return Lhs.microsecsSinceEpoch() < Rhs.microsecsSinceEpoch();
}

/**
 * @brief Compares two timestamps for equality.
 *
 * @param lhs The first timestamp.
 * @param rhs The second timestamp.
 *
 * @return True if the two timestamps are equal, and false otherwise.
 */
inline bool operator==(Timestamp Lhs, Timestamp Rhs) {
  return Lhs.microsecsSinceEpoch() == Rhs.microsecsSinceEpoch();
}

/**
 * @brief Calculates the time difference between two timestamps.
 *
 * @param high The later timestamp.
 * @param low The earlier timestamp.
 *
 * @return The time difference between the two timestamps in seconds as a
 * double.
 */
inline double timeDiff(Timestamp High, Timestamp Low) {
  int64_t Diff = High.microsecsSinceEpoch() - Low.microsecsSinceEpoch();
  return static_cast<double>(Diff) / Timestamp::KMicroSecsPerSec;
}

/**
 * @brief Adds a time interval to a timestamp.
 *
 * @param timestamp The timestamp to which the interval is added.
 * @param seconds The time interval in seconds to add.
 *
 * @return A new timestamp that is the result of adding the time interval to the
 * original timestamp.
 */
inline Timestamp addTime(Timestamp TS, double Seconds) {
  auto Delta = static_cast<int64_t>(Seconds * Timestamp::KMicroSecsPerSec);
  return Timestamp(TS.microsecsSinceEpoch() + Delta);
}

} // namespace lynx

#endif
