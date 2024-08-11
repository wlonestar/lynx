#ifndef LYNX_NET_EVENT_LOOP_THREAD_POOL_H
#define LYNX_NET_EVENT_LOOP_THREAD_POOL_H

#include "lynx/base/noncopyable.h"

#include <functional>
#include <memory>

namespace lynx {

class EventLoop;
class EventLoopThread;

/**
 * @class EventLoopThreadPool
 * @brief Manages a pool of EventLoop threads.
 *
 * The EventLoopThreadPool class is responsible for creating and managing
 * a pool of EventLoop threads. This allows for handling events in multiple
 * threads, distributing the load and improving performance.
 */
class EventLoopThreadPool : Noncopyable {
public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  /**
   * @brief Constructs an EventLoopThreadPool with a base EventLoop and a name.
   * @param baseLoop The base EventLoop used to initialize the pool.
   * @param name The name of the thread pool.
   */
  EventLoopThreadPool(EventLoop *baseLoop, const std::string &name);
  ~EventLoopThreadPool();

  /**
   * @brief Sets the number of threads in the pool.
   * @param numThreads The number of threads to create in the pool.
   */
  void setThreadNum(int numThreads) { num_threads_ = numThreads; }

  /**
   * @brief Starts the thread pool with an optional initialization callback.
   * @param cb The callback to run when each thread starts. Defaults to an empty
   * callback.
   */
  void start(const ThreadInitCallback &cb = ThreadInitCallback());

  /**
   * @brief Gets the next EventLoop in a round-robin fashion.
   * @return A pointer to the next EventLoop.
   */
  EventLoop *getNextLoop();

  /**
   * @brief Gets all the EventLoops in the pool.
   * @return A vector of pointers to all the EventLoops.
   */
  std::vector<EventLoop *> getAllLoops();

  bool started() const { return started_; }
  const std::string &name() const { return name_; }

private:
  EventLoop *base_loop_;
  std::string name_;
  bool started_;
  int num_threads_;
  int next_;
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop *> loops_;
};

} // namespace lynx

#endif
