#ifndef LYNX_TIMER_TIMER_ID_H
#define LYNX_TIMER_TIMER_ID_H

#include "lynx/base/copyable.h"

#include <cstdint>

namespace lynx {

class Timer;

class TimerId : public Copyable {
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
