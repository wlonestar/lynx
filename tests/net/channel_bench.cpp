#include "lynx/logger/logging.h"
#include "lynx/net/channel.h"
#include "lynx/net/event_loop.h"

#include <cstdio>
#include <functional>
#include <map>
#include <sys/timerfd.h>
#include <unistd.h>

void print(const char *msg) {
  static std::map<const char *, lynx::Timestamp> lasts;
  lynx::Timestamp &last = lasts[msg];
  lynx::Timestamp curr = lynx::Timestamp::now();
  printf("%s tid %d %s delay %f\n", curr.toString().c_str(),
         lynx::current_thread::tid(), msg, timeDifference(curr, last));
  last = curr;
}

namespace lynx::detail {
int createTimerfd();
void readTimerfd(int timerfd, Timestamp now);
} // namespace lynx::detail

// Use relative time, immunized to wall clock changes.
class PeriodicTimer {
public:
  PeriodicTimer(lynx::EventLoop *loop, double interval,
                const lynx::TimerCallback &cb)
      : loop_(loop), timerfd_(lynx::detail::createTimerfd()),
        timerfd_channel_(loop, timerfd_), interval_(interval), cb_(cb) {
    timerfd_channel_.setReadCallback([this](auto && /*PH1*/) { handleRead(); });
    timerfd_channel_.enableReading();
  }

  void start() {
    struct itimerspec spec;
    bzero(&spec, sizeof spec);
    spec.it_interval = toTimeSpec(interval_);
    spec.it_value = spec.it_interval;
    int ret =
        ::timerfd_settime(timerfd_, 0 /* relative timer */, &spec, nullptr);
    if (ret != 0) {
      LOG_SYSERR << "timerfd_settime()";
    }
  }

  ~PeriodicTimer() {
    timerfd_channel_.disableAll();
    timerfd_channel_.remove();
    ::close(timerfd_);
  }

private:
  void handleRead() {
    loop_->assertInLoopThread();
    lynx::detail::readTimerfd(timerfd_, lynx::Timestamp::now());
    if (cb_) {
      cb_();
    }
  }

  static struct timespec toTimeSpec(double seconds) {
    struct timespec ts;
    bzero(&ts, sizeof ts);
    const int64_t k_nano_seconds_per_second = 1000000000;
    const int k_min_interval = 100000;
    auto nanoseconds =
        static_cast<int64_t>(seconds * k_nano_seconds_per_second);
    if (nanoseconds < k_min_interval) {
      nanoseconds = k_min_interval;
    }
    ts.tv_sec = static_cast<time_t>(nanoseconds / k_nano_seconds_per_second);
    ts.tv_nsec = static_cast<long>(nanoseconds % k_nano_seconds_per_second);
    return ts;
  }

  lynx::EventLoop *loop_;
  const int timerfd_;
  lynx::Channel timerfd_channel_;
  const double interval_; // in seconds
  lynx::TimerCallback cb_;
};

int main(int argc, char *argv[]) {
  LOG_INFO << "pid = " << getpid() << ", tid = " << lynx::current_thread::tid()
           << " Try adjusting the wall clock, see what happens.";
  lynx::EventLoop loop;
  PeriodicTimer timer(&loop, 1, [] { return print("PeriodicTimer"); });
  timer.start();
  loop.runEvery(1, [] { return print("EventLoop::runEvery"); });
  loop.loop();
}
