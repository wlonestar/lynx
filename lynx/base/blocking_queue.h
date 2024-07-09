#ifndef LYNX_BASE_BLOCKING_QUEUE_H
#define LYNX_BASE_BLOCKING_QUEUE_H

#include "lynx/base/noncopyable.h"

#include <cassert>
#include <condition_variable>
#include <deque>
#include <mutex>

namespace lynx {

template <typename T> class BlockingQueue : Noncopyable {
public:
  using queue_type = std::deque<T>;

  BlockingQueue() = default;

  void put(const T &x) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(x);
    not_empty_.notify_one();
  }

  void put(T &&x) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(std::move(x));
    not_empty_.notify_one();
  }

  T take() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.empty()) {
      not_empty_.wait(lock);
    }
    assert(!queue_.empty());
    T front(std::move(queue_.front()));
    queue_.pop_front();
    return front;
  }

  queue_type drain() {
    std::deque<T> queue;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      queue = std::move(queue_);
      assert(queue_.empty());
    }
    return queue;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

private:
  mutable std::mutex mutex_;
  std::condition_variable not_empty_;
  queue_type queue_;
};

} // namespace lynx

#endif
