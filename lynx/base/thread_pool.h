#ifndef LYNX_BASE_THREAD_POOL_H
#define LYNX_BASE_THREAD_POOL_H

#include "lynx/base/thread.h"

#include <condition_variable>
#include <deque>

namespace lynx {

/// \brief A fixed size pool of  threads
///
/// ThreadPool class manages a fixed size pool of threads to execute tasks in
/// parallel. It provides methods to start and stop the pool, submit tasks, and
/// configure pool properties.
class ThreadPool : Noncopyable {
public:
  using Task = std::function<void()>;

  explicit ThreadPool(const std::string &name = std::string("ThreadPool"));
  ~ThreadPool();

  void start(int numThreads);
  void stop();

  void setMaxQueueSize(int maxSize) { max_queue_size_ = maxSize; }
  void setThreadInitCallback(const Task &cb) { thread_init_callback_ = cb; }
  const std::string &name() const { return name_; }

  /// Returns the current size of the task queue.
  size_t queueSize() const;

  /// Submits a task to the ThreadPool for execution.
  void run(Task task);

private:
  bool isFull() const;

  /// The main function executed by each thread in the pool.
  void runInThread();

  /// Removes and returns the next task from the queue. Blocks if the queue is
  /// empty.
  Task take();

  mutable std::mutex mutex_;
  std::condition_variable not_empty_;
  std::condition_variable not_full_;

  std::string name_;

  // Callback that will be executed by each thread before it starts processing
  // tasks.
  Task thread_init_callback_;

  std::vector<std::unique_ptr<Thread>> threads_;
  std::deque<Task> queue_;
  size_t max_queue_size_;
  bool running_;
};

} // namespace lynx

#endif
