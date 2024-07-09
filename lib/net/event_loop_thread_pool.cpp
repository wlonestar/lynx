#include "lynx/net/event_loop_thread_pool.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/event_loop_thread.h"

namespace lynx {

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop,
                                         const std::string &nameArg)
    : base_loop_(baseLoop), name_(nameArg), started_(false), num_threads_(0),
      next_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() = default;

void EventLoopThreadPool::start(const ThreadInitCallback &cb) {
  assert(!started_);
  base_loop_->assertInLoopThread();

  started_ = true;

  for (int i = 0; i < num_threads_; ++i) {
    char buf[name_.size() + 32];
    snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
    auto *t = new EventLoopThread(cb, buf);
    threads_.push_back(std::unique_ptr<EventLoopThread>(t));
    loops_.push_back(t->startLoop());
  }
  if (num_threads_ == 0 && cb) {
    cb(base_loop_);
  }
}

EventLoop *EventLoopThreadPool::getNextLoop() {
  base_loop_->assertInLoopThread();
  assert(started_);
  EventLoop *loop = base_loop_;

  if (!loops_.empty()) {
    // round-robin
    loop = loops_[next_];
    ++next_;
    if (static_cast<size_t>(next_) >= loops_.size()) {
      next_ = 0;
    }
  }
  return loop;
}

EventLoop *EventLoopThreadPool::getLoopForHash(size_t hashCode) {
  base_loop_->assertInLoopThread();
  EventLoop *loop = base_loop_;

  if (!loops_.empty()) {
    loop = loops_[hashCode % loops_.size()];
  }
  return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops() {
  base_loop_->assertInLoopThread();
  assert(started_);
  if (loops_.empty()) {
    return {1, base_loop_};
  }
  return loops_;
}

} // namespace lynx
