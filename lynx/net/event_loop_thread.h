#ifndef LYNX_NET_EVENT_LOOP_THREAD_H
#define LYNX_NET_EVENT_LOOP_THREAD_H

#include "lynx/base/thread.h"

#include <condition_variable>

namespace lynx {

class EventLoop;

/**
 * @class EventLoopThread
 * @brief A class that manages a thread running an EventLoop.
 *
 * The EventLoopThread class is responsible for creating and managing a thread
 * that runs an EventLoop. This allows for handling events in a separate thread.
 */
class EventLoopThread : Noncopyable {
public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  /**
   * @brief Constructs an EventLoopThread with an optional initialization
   * callback and thread name.
   *
   * @param cb The callback to run when the thread starts. Defaults to an empty
   * callback.
   * @param name The name of the thread. Defaults to an empty string.
   */
  EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                  const std::string &name = std::string());

  /**
   * @brief Destructs the EventLoopThread.
   *
   * The destructor ensures that the thread is properly exited and joined.
   */
  ~EventLoopThread();

  /**
   * @brief Starts the thread and returns the EventLoop running in the thread.
   *
   * @return A pointer to the EventLoop running in the thread.
   */
  EventLoop *startLoop();

private:
  /**
   * @brief The function that runs in the thread.
   *
   * This function creates the EventLoop and runs it, calling the initialization
   * callback if provided.
   */
  void threadFunc();

  EventLoop *loop_;
  bool exiting_;
  Thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  ThreadInitCallback callback_;
};

} // namespace lynx

#endif
