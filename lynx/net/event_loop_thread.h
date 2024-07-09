#ifndef LYNX_NET_EVENT_LOOP_THREAD_H
#define LYNX_NET_EVENT_LOOP_THREAD_H

#include "lynx/base/noncopyable.h"
#include "lynx/base/thread.h"

#include <condition_variable>
#include <functional>
#include <mutex>

namespace lynx {

class EventLoop;

class EventLoopThread : Noncopyable {
public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                  const std::string &name = std::string());
  ~EventLoopThread();

  EventLoop *startLoop();

private:
  void threadFunc();

  EventLoop *loop_;
  bool exiting_;
  Thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  ThreadInitCallback callback_;
};

} // namespace lynx

#endif
