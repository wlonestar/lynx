#ifndef LYNX_NET_EVENT_LOOP_THREAD_POOL_H
#define LYNX_NET_EVENT_LOOP_THREAD_POOL_H

#include "lynx/base/noncopyable.h"

#include <functional>
#include <memory>
#include <vector>

namespace lynx {

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : Noncopyable {
public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
  ~EventLoopThreadPool();

  void setThreadNum(int numThreads) { num_threads_ = numThreads; }
  void start(const ThreadInitCallback &cb = ThreadInitCallback());

  EventLoop *getNextLoop();

  EventLoop *getLoopForHash(size_t hashCode);

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
