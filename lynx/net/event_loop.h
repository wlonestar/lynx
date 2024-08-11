#ifndef LYNX_NET_EVENT_LOOP_H
#define LYNX_NET_EVENT_LOOP_H

#include "lynx/base/current_thread.h"
#include "lynx/base/noncopyable.h"
#include "lynx/base/timestamp.h"
#include "lynx/timer/timer_id.h"

#include <atomic>
#include <memory>

namespace lynx {

class Channel;
class Epoller;
class TimerQueue;

/**
 * @class EventLoop
 * @brief The core of the Reactor pattern, managing the event loop and handling
 * I/O events.
 *
 * The EventLoop class is responsible for handling I/O events, timers, and other
 * callbacks in a single-threaded event-driven programming model.
 */
class EventLoop : Noncopyable {
public:
  using Functor = std::function<void()>;

  EventLoop();
  ~EventLoop();

  /**
   * @brief Starts the event loop.
   *
   * This function will block and loop until quit() is called.
   */
  void loop();

  /**
   * @brief Quits the event loop.
   *
   * This function will cause loop() to return, ending the event loop.
   */
  void quit();

  /**
   * @brief Gets the time when poll() returned.
   * @return The timestamp of the last poll() return.
   */
  Timestamp pollReturnTime() const { return poll_return_time_; }

  /**
   * @brief Runs a callback immediately in the event loop.
   * @param cb The callback to run.
   */
  void runInLoop(Functor cb);

  /**
   * @brief Queues a callback to be run in the event loop.
   * @param cb The callback to queue.
   */
  void queueInLoop(Functor cb);

  /**
   * @brief Gets the size of the pending functor queue.
   * @return The size of the pending functor queue.
   */
  size_t queueSize() const;

  /**
   * @brief Runs a callback at a specific time.
   * @param time The time to run the callback.
   * @param cb The callback to run.
   * @return The ID of the timer.
   */
  TimerId runAt(Timestamp time, TimerCallback cb);

  /**
   * @brief Runs a callback after a delay.
   * @param delay The delay in seconds.
   * @param cb The callback to run.
   * @return The ID of the timer.
   */
  TimerId runAfter(double delay, TimerCallback cb);

  /**
   * @brief Runs a callback at regular intervals.
   * @param interval The interval in seconds.
   * @param cb The callback to run.
   * @return The ID of the timer.
   */
  TimerId runEvery(double interval, TimerCallback cb);

  /**
   * @brief Cancels a timer.
   * @param timerId The ID of the timer to cancel.
   */
  void cancel(TimerId timerId);

  /// Wakes up the event loop.
  void wakeup();

  /// Updates a channel.
  void updateChannel(Channel *channel);
  /// Removes a channel.
  void removeChannel(Channel *channel);

  /**
   * @brief Checks if the event loop has a specific channel.
   * @param channel The channel to check.
   * @return True if the channel exists, false otherwise.
   */
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

  /// Handles the read event on the wakeup fd.
  void handleRead();
  void doPendingFunctors();

  void printActiveChannels() const;

  using ChannelList = std::vector<Channel *>;

  bool looping_;
  std::atomic_bool quit_;
  bool event_handling_;
  bool calling_pending_functors_;
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
