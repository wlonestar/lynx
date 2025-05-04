#ifndef LYNX_TIMER_TIMER_H
#define LYNX_TIMER_TIMER_H

#include "lynx/base/Timestamp.h"
#include "lynx/net/timer_id.h"

#include <atomic>
#include <functional>

namespace lynx {

/**
 * @class Timer
 * @brief Represents a timer that can be scheduled to run at a specific time.
 *
 * The Timer class is used to schedule the execution of a callback function at a
 * specific time. It allows the user to specify the callback function to be
 * executed, the time at which it should be executed, and the interval at which
 * it should be repeated.
 */
class Timer {
public:
  /**
   * @brief Constructs a Timer object.
   *
   * @param cb The callback function to be executed.
   * @param when The time at which the callback function should be executed.
   * @param interval The interval at which the callback function should be
   * repeated. If the interval is non-positive, the timer will be a one-shot
   * timer.
   */
  Timer(TimerCallback Cb, Timestamp When, double Interval)
      : Callback(std::move(Cb)), Expiration(When), Interval(Interval),
        Repeat(Interval > 0.0), Sequence(NumCreated.fetch_add(1)) {}

  /// Executes the callback function associated with this timer.
  void run() const { Callback(); }

  /// Returns the time at which this timer is scheduled to execute.
  Timestamp expiration() const { return Expiration; }

  /// Returns whether this timer is a repeating timer.
  bool repeat() const { return Repeat; }

  /// Returns the sequence number of this timer.
  int64_t sequence() const { return Sequence; }

  /// Restarts the timer with a new expiration time.
  void restart(Timestamp Now);

  /// Returns the total number of Timer objects created.
  static int64_t numCreated() { return NumCreated; }

private:
  const TimerCallback Callback; /// The callback function to be executed when
                                /// the timer expires.
  Timestamp Expiration;         /// The time at which the timer should expire.
  const double Interval; /// The interval at which the timer should be repeated.
  const bool Repeat;     /// Whether this timer is a repeating timer.
  const int64_t Sequence; /// The sequence number of the timer.

  // The atomic counter for generating unique sequence numbers for each timer.
  static std::atomic_int64_t NumCreated;
};

} // namespace lynx

#endif
