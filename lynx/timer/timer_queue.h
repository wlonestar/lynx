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

class TimerQueue : Noncopyable {
public:
  explicit TimerQueue(EventLoop *loop);
  ~TimerQueue();

  TimerId addTimer(TimerCallback cb, Timestamp when, double interval);

  void cancel(TimerId timerId);

private:
  using Entry = std::pair<Timestamp, Timer *>;
  using TimerList = std::set<Entry>;
  using ActiveTimer = std::pair<Timer *, int64_t>;
  using ActiveTimerSet = std::set<ActiveTimer>;

  void addTimerInLoop(Timer *timer);
  void cancelInLoop(TimerId timerId);
  void handleRead();
  std::vector<Entry> getExpired(Timestamp now);
  void reset(const std::vector<Entry> &expired, Timestamp now);
  bool insert(Timer *timer);

  EventLoop *loop_;
  const int timerfd_;
  Channel timerfd_channel_;
  TimerList timers_;
  ActiveTimerSet active_timers_;
  bool calling_expired_timers_;
  ActiveTimerSet canceling_timers_;
};

} // namespace lynx

#endif
