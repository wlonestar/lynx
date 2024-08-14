#include "lynx/timer/timer_queue.h"
#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"
#include "lynx/timer/timer.h"

#include <cassert>
#include <sys/timerfd.h>

namespace lynx {

namespace detail {

/**
 * @brief Creates a timer file descriptor using the timerfd_create system call.
 *
 * @return The file descriptor of the timer.
 */
int createTimerfd() {
  /// Create a timer file descriptor.
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  /// Check if the timerfd_create system call was successful.
  if (timerfd < 0) {
    /// Log a fatal error and terminate the program.
    LOG_SYSFATAL << "Failed in timerfd_create";
  }
  return timerfd;
}

/**
 * @brief Calculates the time difference between the current time and the
 * specified time.
 *
 * This function calculates the time difference between the current time and the
 * specified time. The time difference is returned as a timespec structure. The
 * function ensures that the time difference is at least 100 microseconds.
 *
 * @param when The specified time.
 *
 * @return The time difference between the current time and the specified time.
 */
struct timespec howMuchTimeFromNow(Timestamp when) {
  int64_t microseconds =
      when.microsecsSinceEpoch() - Timestamp::now().microsecsSinceEpoch();
  /// Ensure that the time difference is at least 100 microseconds.
  if (microseconds < 100) {
    microseconds = 100;
  }
  struct timespec ts;
  ts.tv_sec =
      static_cast<time_t>(microseconds / Timestamp::K_MICRO_SECS_PER_SEC);
  ts.tv_nsec = static_cast<long>(
      (microseconds % Timestamp::K_MICRO_SECS_PER_SEC) * 1000);
  return ts;
}

/**
 * @brief Reads the timerfd and logs the number of events that have occurred and
 * the current time.
 *
 * This function reads the timerfd and logs the number of events that have
 * occurred and the current time. The function reads the timerfd using the
 * read() system call and logs the result using the LOG_TRACE macro. If the
 * number of bytes read is not equal to the size of the uint64_t type, the
 * function logs an error using the LOG_ERROR macro.
 *
 * @param timerfd The file descriptor of the timerfd.
 * @param now The current time.
 */
void readTimerfd(int timerfd, Timestamp now) {
  uint64_t howmany;
  ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
  LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at "
            << now.toString();
  if (n != sizeof(howmany)) {
    LOG_ERROR << "TimerQueue::handleRead() reads " << n
              << " bytes instead of 8";
  }
}

/**
 * @brief Resets the timerfd to the specified expiration time.
 *
 * This function resets the timerfd to the specified expiration time. It
 * calculates the time difference between the expiration time and the current
 * time and sets the new interval and value of the timerfd using the
 * timerfd_settime() system call. If the timerfd_settime() call fails, an error
 * message is logged using the LOG_SYSERR macro.
 *
 * @param timerfd The file descriptor of the timerfd.
 * @param expiration The desired expiration time.
 */
void resetTimerfd(int timerfd, Timestamp expiration) {
  struct itimerspec new_value;
  struct itimerspec old_value;
  memset(&new_value, 0, sizeof(new_value));
  memset(&old_value, 0, sizeof(old_value));
  new_value.it_value = howMuchTimeFromNow(expiration);
  /// Set the new interval and value of the timerfd.
  int ret = ::timerfd_settime(timerfd, 0, &new_value, &old_value);
  if (ret != 0) {
    LOG_SYSERR << "timerfd_settime()";
  }
}

} // namespace detail

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop), timerfd_(detail::createTimerfd()),
      timerfd_channel_(loop, timerfd_), calling_expired_timers_(false) {
  timerfd_channel_.setReadCallback([this](auto && /*PH1*/) { handleRead(); });
  timerfd_channel_.enableReading();
}

TimerQueue::~TimerQueue() {
  timerfd_channel_.disableAll();
  timerfd_channel_.remove();
  ::close(timerfd_);
  for (const Entry &timer : timers_) {
    delete timer.second;
  }
}

TimerId TimerQueue::addTimer(TimerCallback cb, Timestamp when,
                             double interval) {
  auto *timer = new Timer(std::move(cb), when, interval);
  loop_->runInLoop([this, timer] { addTimerInLoop(timer); });
  return {timer, timer->sequence()};
}

void TimerQueue::cancel(TimerId timerId) {
  loop_->runInLoop([this, timerId] { cancelInLoop(timerId); });
}

void TimerQueue::addTimerInLoop(Timer *timer) {
  loop_->assertInLoopThread();
  bool earliest_changed = insert(timer);

  if (earliest_changed) {
    detail::resetTimerfd(timerfd_, timer->expiration());
  }
}

void TimerQueue::cancelInLoop(TimerId timerId) {
  loop_->assertInLoopThread();
  assert(timers_.size() == active_timers_.size());
  ActiveTimer timer(timerId.timer_, timerId.sequence_);
  auto it = active_timers_.find(timer);
  if (it != active_timers_.end()) {
    size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
    assert(n == 1);
    (void)n;
    delete it->first;
    active_timers_.erase(it);
  } else if (calling_expired_timers_) {
    canceling_timers_.insert(timer);
  }
  assert(timers_.size() == active_timers_.size());
}

void TimerQueue::handleRead() {
  loop_->assertInLoopThread();
  Timestamp now(Timestamp::now());
  detail::readTimerfd(timerfd_, now);

  std::vector<Entry> expired = getExpired(now);

  calling_expired_timers_ = true;
  canceling_timers_.clear();
  for (const Entry &it : expired) {
    it.second->run();
  }
  calling_expired_timers_ = false;

  reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now) {
  assert(timers_.size() == active_timers_.size());
  std::vector<Entry> expired;
  Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
  auto end = timers_.lower_bound(sentry);
  assert(end == timers_.end() || now < end->first);
  std::copy(timers_.begin(), end, back_inserter(expired));
  timers_.erase(timers_.begin(), end);

  for (const Entry &it : expired) {
    ActiveTimer timer(it.second, it.second->sequence());
    size_t n = active_timers_.erase(timer);
    assert(n == 1);
    (void)n;
  }

  assert(timers_.size() == active_timers_.size());
  return expired;
}

void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now) {
  Timestamp next_expire;

  for (const Entry &it : expired) {
    ActiveTimer timer(it.second, it.second->sequence());
    if (it.second->repeat() &&
        canceling_timers_.find(timer) == canceling_timers_.end()) {
      it.second->restart(now);
      insert(it.second);
    } else {
      delete it.second;
    }
  }

  if (!timers_.empty()) {
    next_expire = timers_.begin()->second->expiration();
  }

  if (next_expire.valid()) {
    detail::resetTimerfd(timerfd_, next_expire);
  }
}

bool TimerQueue::insert(Timer *timer) {
  loop_->assertInLoopThread();
  assert(timers_.size() == active_timers_.size());
  bool earliest_changed = false;
  Timestamp when = timer->expiration();
  auto it = timers_.begin();
  if (it == timers_.end() || when < it->first) {
    earliest_changed = true;
  }
  {
    std::pair<TimerList::iterator, bool> result =
        timers_.insert(Entry(when, timer));
    assert(result.second);
    (void)result;
  }
  {
    std::pair<ActiveTimerSet::iterator, bool> result =
        active_timers_.insert(ActiveTimer(timer, timer->sequence()));
    assert(result.second);
    (void)result;
  }

  assert(timers_.size() == active_timers_.size());
  return earliest_changed;
}

} // namespace lynx
