#ifndef LYNX_TIMER_TIMER_H
#define LYNX_TIMER_TIMER_H

#include "lynx/base/noncopyable.h"
#include "lynx/base/timestamp.h"
#include "lynx/net/callbacks.h"

#include <atomic>
#include <functional>

namespace lynx {

class Timer : Noncopyable {
public:
  Timer(TimerCallback cb, Timestamp when, double interval)
      : callback_(std::move(cb)), expiration_(when), interval_(interval),
        repeat_(interval > 0.0), sequence_(num_created.fetch_add(1)) {}

  void run() const { callback_(); }

  Timestamp expiration() const { return expiration_; }
  bool repeat() const { return repeat_; }
  int64_t sequence() const { return sequence_; }

  void restart(Timestamp now);

  static int64_t numCreated() { return num_created; }

private:
  const TimerCallback callback_;
  Timestamp expiration_;
  const double interval_;
  const bool repeat_;
  const int64_t sequence_;

  static std::atomic_int64_t num_created;
};

} // namespace lynx

#endif
