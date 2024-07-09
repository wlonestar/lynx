#include "lynx/base/thread_pool.h"
#include "lynx/base/exception.h"
#include "lynx/base/thread.h"

#include <cassert>
#include <mutex>

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
    snprintf(id, sizeof id, "%d", i + 1);
    threads_.emplace_back(new Thread([this] { runInThread(); }, name_ + id));
    threads_[i]->start();
  }
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
    while (isFull() && running_) {
      not_full_.wait(lock);
    }
    if (!running_) {
      return;
    }
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
    if (thread_init_callback_) {
      thread_init_callback_();
    }
    while (running_) {
      Task task(take());
      if (task) {
        task();
      }
    }
  } catch (const Exception &ex) {
    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
    abort();
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

  while (queue_.empty() && running_) {
    not_empty_.wait(lock);
  }

  Task task;
  if (!queue_.empty()) {
    task = queue_.front();
    queue_.pop_front();
    if (max_queue_size_ > 0) {
      not_full_.notify_one();
    }
  }
  return task;
}

} // namespace lynx
