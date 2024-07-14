#ifndef LYNX_TIMER_TIMER_ID_H
#define LYNX_TIMER_TIMER_ID_H

#include <cstdint>

namespace lynx {

class Timer;

class TimerId {
public:
  TimerId() : timer_(nullptr), sequence_(0) {}
  TimerId(Timer *timer, int64_t seq) : timer_(timer), sequence_(seq) {}

  friend class TimerQueue;

private:
  Timer *timer_;
  int64_t sequence_;
};

} // namespace lynx

#endif
