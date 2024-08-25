#include "lynx/base/thread_pool.h"
#include "lynx/base/thread.h"

#include <cassert>

namespace lynx {

ThreadPool::ThreadPool(const std::string &name)
    : name_(name), max_queue_size_(0), running_(false) {}

ThreadPool::~ThreadPool() {
  if (running_) {
    stop();
  }
}

void ThreadPool::start(int numThreads) {
  assert(threads_.empty());
  running_ = true;

  threads_.reserve(numThreads);
  for (int i = 0; i < numThreads; ++i) {
    char id[32];
    snprintf(id, sizeof(id), "%d", i + 1);
    /// Create a new thread and add it to the vector
    threads_.emplace_back(new Thread([&] { runInThread(); }, name_ + id));
    threads_[i]->start();
  }

  /// If there are no threads and a thread_init_callback function is provided,
  /// call the callback function immediately
  if (numThreads == 0 && thread_init_callback_) {
    thread_init_callback_();
  }
}

void ThreadPool::stop() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    running_ = false;
    not_empty_.notify_all();
    not_full_.notify_all();
  }

  for (auto &thread : threads_) {
    thread->join();
  }
}

size_t ThreadPool::queueSize() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return queue_.size();
}

void ThreadPool::run(Task task) {
  if (threads_.empty()) {
    task();
  } else {
    std::unique_lock<std::mutex> lock(mutex_);
    /// If the queue is full and the thread pool is running, wait until
    /// there is space available in the queue.
    while (isFull() && running_) {
      not_full_.wait(lock);
    }
    /// If the thread pool is not running, return immediately.
    if (!running_) {
      return;
    }
    /// At this point, we know that there is space available in the queue,
    /// so add the task to the queue and notify the first available thread.
    assert(!isFull());
    queue_.push_back(std::move(task));
    not_empty_.notify_one();
  }
}

bool ThreadPool::isFull() const {
  return max_queue_size_ > 0 && queue_.size() >= max_queue_size_;
}

void ThreadPool::runInThread() {
  try {
    /// If a thread initialization callback is provided, call it.
    if (thread_init_callback_) {
      thread_init_callback_();
    }
    /// Continuously take tasks from the task queue and execute them.
    while (running_) {
      Task task(take()); /// Take a task from the task queue.
      /// If a task is available, execute it.
      if (task) {
        task();
      }
    }
  } catch (const std::exception &ex) {
    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    abort();
  } catch (...) {
    fprintf(stderr, "unknown exception caught in ThreadPool %s\n",
            name_.c_str());
    throw;
  }
}

ThreadPool::Task ThreadPool::take() {
  std::unique_lock<std::mutex> lock(mutex_);
  /// Wait until a task is available or the thread pool has stopped running.
  while (queue_.empty() && running_) {
    not_empty_.wait(lock);
  }

  Task task; /// The task to be returned.
  /// If there is at least one task in the queue, retrieve and remove it.
  if (!queue_.empty()) {
    task = queue_.front();
    queue_.pop_front();
    /// If the maximum queue size is set, notify the not_full condition
    /// variable.
    if (max_queue_size_ > 0) {
      not_full_.notify_one();
    }
  }

  return task;
}

} // namespace lynx
