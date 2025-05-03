#ifndef LYNX_TIMER_TIMER_ID_H
#define LYNX_TIMER_TIMER_ID_H

#include <functional>

namespace lynx {

class Timer;

using TimerCallback = std::function<void()>;

/**
 * @class TimerId
 * @brief Represents a unique identifier for a Timer object.
 *
 * The TimerId class is used to uniquely identify a Timer object in the
 * TimerQueue. It is composed of a pointer to the Timer object and a sequence
 * number. The TimerQueue uses these identifiers to manage and schedule timers.
 */
class TimerId {
public:
  /**
   * @brief Constructs a TimerId object with a null timer pointer and a sequence
   * number of 0.
   */
  TimerId() : timer_(nullptr), sequence_(0) {}

  /**
   * @brief Constructs a TimerId object with a pointer to a Timer object and a
   * sequence number.
   *
   * @param timer Pointer to the Timer object.
   * @param seq Sequence number.
   */
  TimerId(Timer *timer, int64_t seq) : timer_(timer), sequence_(seq) {}

  /**
   * @brief Friend declaration for TimerQueue class.
   *
   * The TimerQueue class is allowed to access the private members of the
   * TimerId class.
   */
  friend class TimerQueue;

private:
  Timer *timer_;     /// Pointer to the Timer object.
  int64_t sequence_; /// Sequence number of the TimerId object.
};

} // namespace lynx

#endif
