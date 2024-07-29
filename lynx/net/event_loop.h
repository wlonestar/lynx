#ifndef LYNX_NET_EVENT_LOOP_H
#define LYNX_NET_EVENT_LOOP_H

#include "lynx/base/current_thread.h"
#include "lynx/base/noncopyable.h"
#include "lynx/base/timestamp.h"
#include "lynx/net/timer_id.h"

#include <atomic>
#include <cassert>
#include <functional>
#include <memory>

namespace lynx {

class Channel;
class Epoller;
class TimerQueue;

class EventLoop : Noncopyable {
public:
  using Functor = std::function<void()>;

  EventLoop();
  ~EventLoop();

  void loop();
  void quit();

  Timestamp pollReturnTime() const { return poll_return_time_; }
  int64_t iteration() const { return iteration_; }

  void runInLoop(Functor cb);
  void queueInLoop(Functor cb);

  size_t queueSize() const;

  TimerId runAt(Timestamp time, TimerCallback cb);
  TimerId runAfter(double delay, TimerCallback cb);
  TimerId runEvery(double interval, TimerCallback cb);
  void cancel(TimerId timerId);

  void wakeup();
  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);
  bool hasChannel(Channel *channel);

  void assertInLoopThread() {
    if (!isInLoopThread()) {
      abortNotInLoopThread();
    }
  }
  bool isInLoopThread() const { return thread_id_ == current_thread::tid(); }
  bool eventHandling() const { return event_handling_; }

  static EventLoop *getEventLoopOfCurrentThread();

private:
  void abortNotInLoopThread();
  void handleRead();
  void doPendingFunctors();

  void printActiveChannels() const;

  using ChannelList = std::vector<Channel *>;

  bool looping_;
  std::atomic_bool quit_;
  bool event_handling_;
  bool calling_pending_functors_;
  int64_t iteration_;
  const pid_t thread_id_;
  Timestamp poll_return_time_;
  std::unique_ptr<Epoller> poller_;
  std::unique_ptr<TimerQueue> timer_queue_;
  int wakeup_fd_;
  std::unique_ptr<Channel> wakeup_channel_;

  ChannelList active_channels_;
  Channel *current_active_channel_;

  mutable std::mutex mutex_;
  std::vector<Functor> pending_functors_;
};

} // namespace lynx

#endif
