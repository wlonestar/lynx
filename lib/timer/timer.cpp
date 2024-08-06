#include "lynx/timer/timer.h"

namespace lynx {

std::atomic_int64_t Timer::num_created;

void Timer::restart(Timestamp now) {
  if (repeat_) {
    expiration_ = addTime(now, interval_);
  } else {
    expiration_ = Timestamp::invalid();
  }
}

} // namespace lynx
