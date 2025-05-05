#ifndef LYNX_NET_EVENT_LOOP_H
#define LYNX_NET_EVENT_LOOP_H

#include "lynx/base/CurrentThread.h"
#include "lynx/base/Timestamp.h"
#include "lynx/net/TimerId.h"

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
class EventLoop {
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
   *
   * @return The timestamp of the last poll() return.
   */
  Timestamp pollReturnTime() const { return PollReturnTime; }

  /**
   * @brief Runs a callback immediately in the event loop.
   *
   * @param cb The callback to run.
   */
  void runInLoop(Functor Cb);

  /**
   * @brief Queues a callback to be run in the event loop.
   *
   * @param cb The callback to queue.
   */
  void queueInLoop(Functor Cb);

  /**
   * @brief Gets the size of the pending functor queue.
   *
   * @return The size of the pending functor queue.
   */
  size_t queueSize() const;

  /**
   * @brief Runs a callback at a specific time.
   *
   * @param time The time to run the callback.
   * @param cb The callback to run.
   *
   * @return The ID of the timer.
   */
  TimerId runAt(Timestamp Time, TimerCallback Cb);

  /**
   * @brief Runs a callback after a delay.
   *
   * @param delay The delay in seconds.
   * @param cb The callback to run.
   *
   * @return The ID of the timer.
   */
  TimerId runAfter(double Delay, TimerCallback Cb);

  /**
   * @brief Runs a callback at regular intervals.
   *
   * @param interval The interval in seconds.
   * @param cb The callback to run.
   *
   * @return The ID of the timer.
   */
  TimerId runEvery(double Interval, TimerCallback Cb);

  /**
   * @brief Cancels a timer.
   *
   * @param timerId The ID of the timer to cancel.
   */
  void cancel(TimerId TimerId);

  /// Wakes up the event loop.
  void wakeup();

  /// Updates a channel.
  void updateChannel(Channel *Channel);
  /// Removes a channel.
  void removeChannel(Channel *Channel);

  /**
   * @brief Checks if the event loop has a specific channel.
   *
   * @param channel The channel to check.
   *
   * @return True if the channel exists, false otherwise.
   */
  bool hasChannel(Channel *Channel);

  void assertInLoopThread() {
    if (!isInLoopThread())
      abortNotInLoopThread();
  }
  bool isInLoopThread() const { return ThreadId == current_thread::tid(); }
  bool eventHandling() const { return EventHandling; }

  static EventLoop *getEventLoopOfCurrentThread();

private:
  void abortNotInLoopThread();

  /// Handles the read event on the wakeup fd.
  void handleRead();
  void doPendingFunctors();

  void printActiveChannels() const;

  using ChannelList = std::vector<Channel *>;

  bool Looping;
  std::atomic_bool Quit;
  bool EventHandling;
  bool CallingPendingFunctors;
  const pid_t ThreadId;
  Timestamp PollReturnTime;
  std::unique_ptr<Epoller> Poller;
  std::unique_ptr<TimerQueue> Queue;
  int WakeupFd;
  std::unique_ptr<Channel> WakeupChannel;

  ChannelList ActiveChannels;
  Channel *CurrentActiveChannel;

  mutable std::mutex Mutex;
  std::vector<Functor> PendingFunctors;
};

} // namespace lynx

#endif
