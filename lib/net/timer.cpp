#include "lynx/net/timer.h"
#include "lynx/base/timestamp.h"

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
