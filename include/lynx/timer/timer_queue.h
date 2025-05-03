#ifndef LYNX_TIMER_TIMER_QUEUE_H
#define LYNX_TIMER_TIMER_QUEUE_H

#include "lynx/base/timestamp.h"
#include "lynx/net/channel.h"
#include "lynx/timer/timer_id.h"

#include <set>

namespace lynx {

class EventLoop;
class Timer;
class TimerId;

/**
 * @class TimerQueue
 * @brief A timer queue that manages a set of timers and their expiration times.
 *
 * The TimerQueue class is responsible for scheduling and executing timers.
 * It uses a timerfd to handle the underlying timer events and dispatches
 * callbacks to the appropriate timers.
 */
class TimerQueue : Noncopyable {
public:
  /**
   * @brief Constructs a TimerQueue object.
   *
   * @param loop The event loop associated with this timer queue.
   */
  explicit TimerQueue(EventLoop *loop);
  ~TimerQueue();

  /**
   * @brief Adds a timer to the timer queue.
   *
   * @param cb The callback function to be executed when the timer expires.
   * @param when The time at which the timer should expire.
   * @param interval The interval at which the timer should be repeated.
   *
   * @return The TimerId of the added timer.
   */
  TimerId addTimer(TimerCallback cb, Timestamp when, double interval);

  /**
   * @brief Cancels a timer.
   *
   * @param timerId The TimerId of the timer to be cancelled.
   */
  void cancel(TimerId timerId);

private:
  using Entry = std::pair<Timestamp, Timer *>;
  using TimerList = std::set<Entry>;
  using ActiveTimer = std::pair<Timer *, int64_t>;
  using ActiveTimerSet = std::set<ActiveTimer>;

  /**
   * @brief Adds a timer to the timer queue.
   *
   * @param timer The timer to be added.
   */
  void addTimerInLoop(Timer *timer);

  /**
   * @brief Cancels a timer.
   *
   * @param timerId The TimerId of the timer to be cancelled.
   */
  void cancelInLoop(TimerId timerId);

  /// Handles the read event of the timerfd.
  void handleRead();

  /**
   * @brief Gets the expired timers.
   *
   * @param now The current time.
   *
   * @return The vector of expired timers.
   */
  std::vector<Entry> getExpired(Timestamp now);

  /**
   * @brief Resets the expired timers.
   *
   * @param expired The vector of expired timers.
   * @param now The current time.
   */
  void reset(const std::vector<Entry> &expired, Timestamp now);

  /**
   * @brief Inserts a timer into the timer list.
   *
   * @param timer The timer to be inserted.
   *
   * @return True if the insertion is successful, false otherwise.
   */
  bool insert(Timer *timer);

  EventLoop *loop_;         /// The event loop associated with this timer queue.
  const int timerfd_;       /// The file descriptor of the timerfd.
  Channel timerfd_channel_; /// The channel associated with the timerfd.
  TimerList timers_;        /// The timer list.
  ActiveTimerSet active_timers_; /// The set of active timers.
  bool calling_expired_timers_;  /// Indicates whether expired timers are being
                                 /// called.
  ActiveTimerSet canceling_timers_; /// The set of cancelling timers.
};

} // namespace lynx

#endif
