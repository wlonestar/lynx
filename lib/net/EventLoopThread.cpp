#include "lynx/net/EventLoop.h"
#include "lynx/net/EventLoopThread.h"

#include <cassert>

namespace lynx {

EventLoopThread::EventLoopThread(const ThreadInitCallback &Cb,
                                 const std::string &Name)
    : Loop(nullptr), Exiting(false), Thread([this] { threadFunc(); }, Name),
      Callback(Cb) {}

EventLoopThread::~EventLoopThread() {
  Exiting = true;
  if (Loop != nullptr) {
    Loop->quit();
    Thread.join();
  }
}

EventLoop *EventLoopThread::startLoop() {
  assert(!Thread.started());
  Thread.start();

  EventLoop *Eloop = nullptr;
  {
    std::unique_lock<std::mutex> Lock(Mutex);
    while (Loop == nullptr)
      Cond.wait(Lock);

    Eloop = Loop;
  }

  return Eloop;
}

void EventLoopThread::threadFunc() {
  EventLoop Eloop;

  if (Callback)
    Callback(&Eloop);

  {
    std::lock_guard<std::mutex> Lock(Mutex);
    Loop = &Eloop;
    Cond.notify_one();
  }

  Eloop.loop();
  std::lock_guard<std::mutex> Lock(Mutex);
  Loop = nullptr;
}

} // namespace lynx
