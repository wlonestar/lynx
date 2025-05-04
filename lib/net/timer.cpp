#include "lynx/net/timer.h"

namespace lynx {

std::atomic_int64_t Timer::NumCreated;

void Timer::restart(Timestamp Now) {
  if (Repeat) {
    Expiration = addTime(Now, Interval);
  } else {
    Expiration = Timestamp::invalid();
  }
}

} // namespace lynx
