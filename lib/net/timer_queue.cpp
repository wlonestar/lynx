#include "lynx/net/timer_queue.h"
#include "lynx/logger/Logging.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/timer.h"

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
  int Timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  /// Check if the timerfd_create system call was successful.
  if (Timerfd < 0)
    /// Log a fatal error and terminate the program.
    LOG_SYSFATAL << "Failed in timerfd_create";

  return Timerfd;
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
struct timespec howMuchTimeFromNow(Timestamp When) {
  int64_t Microseconds =
      When.microsecsSinceEpoch() - Timestamp::now().microsecsSinceEpoch();
  /// Ensure that the time difference is at least 100 microseconds.
  if (Microseconds < 100) {
    Microseconds = 100;
  }
  struct timespec Ts;
  Ts.tv_sec = static_cast<time_t>(Microseconds / Timestamp::KMicroSecsPerSec);
  Ts.tv_nsec =
      static_cast<long>((Microseconds % Timestamp::KMicroSecsPerSec) * 1000);
  return Ts;
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
void readTimerfd(int Timerfd, Timestamp Now) {
  uint64_t Howmany;
  ssize_t N = ::read(Timerfd, &Howmany, sizeof(Howmany));
  LOG_TRACE << "TimerQueue::handleRead() " << Howmany << " at "
            << Now.toString();
  if (N != sizeof(Howmany))
    LOG_ERROR << "TimerQueue::handleRead() reads " << N
              << " bytes instead of 8";
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
void resetTimerfd(int Timerfd, Timestamp Expiration) {
  struct itimerspec NewValue;
  struct itimerspec OldValue;
  memset(&NewValue, 0, sizeof(NewValue));
  memset(&OldValue, 0, sizeof(OldValue));
  NewValue.it_value = howMuchTimeFromNow(Expiration);
  /// Set the new interval and value of the timerfd.
  int Ret = ::timerfd_settime(Timerfd, 0, &NewValue, &OldValue);
  if (Ret != 0)
    LOG_SYSERR << "timerfd_settime()";
}

} // namespace detail

TimerQueue::TimerQueue(EventLoop *Loop)
    : Loop(Loop), Timerfd(detail::createTimerfd()),
      TimerfdChannel(Loop, Timerfd), CallingExpiredTimers(false) {
  TimerfdChannel.setReadCallback([this](auto && /*PH1*/) { handleRead(); });
  TimerfdChannel.enableReading();
}

TimerQueue::~TimerQueue() {
  TimerfdChannel.disableAll();
  TimerfdChannel.remove();
  ::close(Timerfd);
  for (const Entry &Timer : Timers) {
    delete Timer.second;
  }
}

TimerId TimerQueue::addTimer(TimerCallback Cb, Timestamp When,
                             double Interval) {
  auto *Timr = new Timer(std::move(Cb), When, Interval);
  Loop->runInLoop([this, Timr] { addTimerInLoop(Timr); });
  return {Timr, Timr->sequence()};
}

void TimerQueue::cancel(TimerId TimerId) {
  Loop->runInLoop([this, TimerId] { cancelInLoop(TimerId); });
}

void TimerQueue::addTimerInLoop(Timer *Timer) {
  Loop->assertInLoopThread();
  bool EarliestChanged = insert(Timer);

  if (EarliestChanged) {
    detail::resetTimerfd(Timerfd, Timer->expiration());
  }
}

void TimerQueue::cancelInLoop(TimerId TimerId) {
  Loop->assertInLoopThread();
  assert(Timers.size() == ActiveTimers.size());
  ActiveTimer Timer(TimerId.Timer, TimerId.Sequence);
  auto It = ActiveTimers.find(Timer);
  if (It != ActiveTimers.end()) {
    size_t N = Timers.erase(Entry(It->first->expiration(), It->first));
    assert(N == 1);
    (void)N;
    delete It->first;
    ActiveTimers.erase(It);
  } else if (CallingExpiredTimers) {
    CancelingTimers.insert(Timer);
  }
  assert(Timers.size() == ActiveTimers.size());
}

void TimerQueue::handleRead() {
  Loop->assertInLoopThread();
  Timestamp Now(Timestamp::now());
  detail::readTimerfd(Timerfd, Now);

  std::vector<Entry> Expired = getExpired(Now);

  CallingExpiredTimers = true;
  CancelingTimers.clear();
  for (const Entry &It : Expired) {
    It.second->run();
  }
  CallingExpiredTimers = false;

  reset(Expired, Now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp Now) {
  assert(Timers.size() == ActiveTimers.size());
  std::vector<Entry> Expired;
  Entry Sentry(Now, reinterpret_cast<Timer *>(UINTPTR_MAX));
  auto End = Timers.lower_bound(Sentry);
  assert(End == Timers.end() || Now < End->first);
  std::copy(Timers.begin(), End, back_inserter(Expired));
  Timers.erase(Timers.begin(), End);

  for (const Entry &It : Expired) {
    ActiveTimer Timer(It.second, It.second->sequence());
    size_t N = ActiveTimers.erase(Timer);
    assert(N == 1);
    (void)N;
  }

  assert(Timers.size() == ActiveTimers.size());
  return Expired;
}

void TimerQueue::reset(const std::vector<Entry> &Expired, Timestamp Now) {
  Timestamp NextExpire;

  for (const Entry &It : Expired) {
    ActiveTimer Timer(It.second, It.second->sequence());
    if (It.second->repeat() &&
        CancelingTimers.find(Timer) == CancelingTimers.end()) {
      It.second->restart(Now);
      insert(It.second);
    } else {
      delete It.second;
    }
  }

  if (!Timers.empty()) {
    NextExpire = Timers.begin()->second->expiration();
  }

  if (NextExpire.valid()) {
    detail::resetTimerfd(Timerfd, NextExpire);
  }
}

bool TimerQueue::insert(Timer *Timer) {
  Loop->assertInLoopThread();
  assert(Timers.size() == ActiveTimers.size());
  bool EarliestChanged = false;
  Timestamp When = Timer->expiration();
  auto It = Timers.begin();
  if (It == Timers.end() || When < It->first)
    EarliestChanged = true;

  {
    std::pair<TimerList::iterator, bool> Result =
        Timers.insert(Entry(When, Timer));
    assert(Result.second);
    (void)Result;
  }

  {
    std::pair<ActiveTimerSet::iterator, bool> Result =
        ActiveTimers.insert(ActiveTimer(Timer, Timer->sequence()));
    assert(Result.second);
    (void)Result;
  }

  assert(Timers.size() == ActiveTimers.size());
  return EarliestChanged;
}

} // namespace lynx
