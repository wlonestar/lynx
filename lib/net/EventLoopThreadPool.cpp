#include "lynx/net/EventLoop.h"
#include "lynx/net/EventLoopThread.h"
#include "lynx/net/EventLoopThreadPool.h"

#include <cassert>

#include <fmt/format.h>

namespace lynx {

EventLoopThreadPool::EventLoopThreadPool(EventLoop *BaseLoop,
                                         const std::string &Name)
    : BaseLoop(BaseLoop), Name(Name), Started(false), NumThreads(0), Next(0) {}

EventLoopThreadPool::~EventLoopThreadPool() = default;

void EventLoopThreadPool::start(const ThreadInitCallback &Cb) {
  assert(!Started);
  BaseLoop->assertInLoopThread();

  Started = true;

  for (int I = 0; I < NumThreads; ++I) {
    auto Buf = fmt::format("{}{:d}", Name, I);
    auto *T = new EventLoopThread(Cb, Buf);
    Threads.push_back(std::unique_ptr<EventLoopThread>(T));
    Loops.push_back(T->startLoop());
  }
  if (NumThreads == 0 && Cb)
    Cb(BaseLoop);
}

EventLoop *EventLoopThreadPool::getNextLoop() {
  BaseLoop->assertInLoopThread();
  assert(Started);
  EventLoop *Loop = BaseLoop;

  if (!Loops.empty()) {
    Loop = Loops[Next];
    ++Next;
    if (static_cast<size_t>(Next) >= Loops.size())
      Next = 0;
  }
  return Loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops() {
  BaseLoop->assertInLoopThread();
  assert(Started);
  if (Loops.empty())
    return {1, BaseLoop};

  return Loops;
}

} // namespace lynx
