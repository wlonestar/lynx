#ifndef LYNX_BASE_THREAD_POOL_H
#define LYNX_BASE_THREAD_POOL_H

#include "lynx/base/thread.h"

#include <condition_variable>
#include <deque>

namespace lynx {

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

  size_t queueSize() const;

  void run(Task task);

private:
  bool isFull() const;
  void runInThread();
  Task take();

  mutable std::mutex mutex_;
  std::condition_variable not_empty_;
  std::condition_variable not_full_;
  std::string name_;
  Task thread_init_callback_;
  std::vector<std::unique_ptr<Thread>> threads_;
  std::deque<Task> queue_;
  size_t max_queue_size_;
  bool running_;
};

} // namespace lynx

#endif
