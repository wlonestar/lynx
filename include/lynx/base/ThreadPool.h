#ifndef LYNX_BASE_THREAD_POOL_H
#define LYNX_BASE_THREAD_POOL_H

#include "lynx/base/Thread.h"

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
class ThreadPool {
public:
  using Task = std::function<void()>;

  explicit ThreadPool(const std::string &Name = "ThreadPool");
  ~ThreadPool();

  /**
   * @brief Starts the thread pool with the specified number of threads.
   *
   * This function initializes the thread pool with the specified number of
   * threads. If the number of threads is zero, the thread_init_callback
   * function is called immediately.
   *
   * @param numThreads The number of threads to create in the pool.
   */
  void start(int NumThreads);

  /// Stops the thread pool.
  void stop();

  /**
   * @brief Sets the maximum size of the task queue.
   *
   * @param maxSize The maximum size of the task queue.
   */
  void setMaxQueueSize(int MaxSize) { MaxQueueSize = MaxSize; }

  /// Sets the callback to be executed by each thread before it starts
  /// processing tasks.
  void setThreadInitCallback(const Task &Cb) { ThreadInitCallback = Cb; }

  /// Returns the name of the thread pool.
  const std::string &name() const { return Name; }

  /// Returns the current size of the task queue.
  size_t queueSize() const;

  /**
   * @brief Run a task in the thread pool.
   *
   * If the thread pool has no threads, the task is run directly. Otherwise, the
   * task is added to the task queue and the first available thread will run it.
   *
   * If the task queue is full and the thread pool is still running, the caller
   * will block until the task can be added to the queue.
   *
   * If the thread pool is not running, the task is discarded.
   *
   * @param task The task to be run in the thread pool.
   */
  void run(Task Task);

private:
  /// Checks if the task queue is full.
  bool isFull() const;

  /**
   * @brief This function is the main function of each thread in the thread
   * pool. It continuously takes tasks from the task queue and executes them.
   *
   * It acquires the lock to access the task queue and waits until a task is
   * available or the thread pool has stopped running. Once a task is available,
   * it executes the task and releases the lock. If an exception is caught
   * during the execution of the task, it prints the exception message to stderr
   * and aborts the program.
   */
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

  mutable std::mutex Mutex;
  std::condition_variable NotEmpty;
  std::condition_variable NotFull;

  std::string Name; /// The name of the thread pool.

  /// Callback that will be executed by each thread before it starts processing
  /// tasks.
  Task ThreadInitCallback;

  std::vector<std::unique_ptr<Thread>> Threads; /// The threads in the pool.
  std::deque<Task> Queue;                       /// The task queue.
  size_t MaxQueueSize; /// The maximum size of the task queue.
  bool Running;        /// Flag to indicate if the thread pool is running.
};

} // namespace lynx

#endif
