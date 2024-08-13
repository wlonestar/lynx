#ifndef LYNX_BASE_THREAD_POOL_H
#define LYNX_BASE_THREAD_POOL_H

#include "lynx/base/thread.h"

#include <condition_variable>
#include <deque>

namespace lynx {

/**
 * @class ThreadPool
 * @brief A fixed size pool of threads to execute tasks in parallel.
 *
 * The ThreadPool class manages a fixed size pool of threads to execute tasks in
 * parallel. It provides methods to start and stop the pool, submit tasks, and
 * configure pool properties.
 */
class ThreadPool : Noncopyable {
public:
  /// Alias for the type of the tasks to be executed.
  using Task = std::function<void()>;

  explicit ThreadPool(const std::string &name = std::string("ThreadPool"));

  ~ThreadPool();

  /**
   * @brief Starts the thread pool with the given number of threads.
   *
   * @param numThreads The number of threads to create in the pool.
   */
  void start(int numThreads);

  /// Stops the thread pool.
  void stop();

  /**
   * @brief Sets the maximum size of the task queue.
   *
   * @param maxSize The maximum size of the task queue.
   */
  void setMaxQueueSize(int maxSize) { max_queue_size_ = maxSize; }

  /// Sets the callback to be executed by each thread before it starts
  /// processing tasks.
  void setThreadInitCallback(const Task &cb) { thread_init_callback_ = cb; }

  /// Returns the name of the thread pool.
  const std::string &name() const { return name_; }

  /// Returns the current size of the task queue.
  size_t queueSize() const;

  /**
   * @brief Submits a task to the ThreadPool for execution.
   *
   * @param task The task to be executed.
   */
  void run(Task task);

private:
  /// Checks if the task queue is full.
  bool isFull() const;

  /// The main function executed by each thread in the pool.
  void runInThread();

  /**
   * @brief Retrieves and removes the next task from the task queue.
   *
   * If the task queue is empty and the thread pool is running, it waits until a
   * new task is available. Once a task is available, it is retrieved and
   * removed from the queue. If the maximum queue size is set, it notifies the
   * not_full condition variable to signal that the queue is no longer full.
   *
   * @return The next task to be executed or an empty task if the thread pool is
   *         no longer running.
   */
  Task take();

  mutable std::mutex mutex_;
  std::condition_variable not_empty_;
  std::condition_variable not_full_;

  std::string name_; /// The name of the thread pool.

  /// Callback that will be executed by each thread before it starts processing
  /// tasks.
  Task thread_init_callback_;

  std::vector<std::unique_ptr<Thread>> threads_; /// The threads in the pool.
  std::deque<Task> queue_;                       /// The task queue.
  size_t max_queue_size_; /// The maximum size of the task queue.
  bool running_;          /// Flag to indicate if the thread pool is running.
};

} // namespace lynx

#endif
