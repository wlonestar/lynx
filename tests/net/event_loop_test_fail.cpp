#include "lynx/base/thread.h"
#include "lynx/net/event_loop.h"

#include <cassert>

lynx::EventLoop *g_loop;

void callback() {
  printf("callback(): pid = %d, tid = %d\n", getpid(),
         lynx::current_thread::tid());
  lynx::EventLoop another_loop;
}

void threadFunc() {
  printf("threadFunc(): pid = %d, tid = %d\n", getpid(),
         lynx::current_thread::tid());

  assert(lynx::EventLoop::getEventLoopOfCurrentThread() == nullptr);
  lynx::EventLoop loop;
  assert(lynx::EventLoop::getEventLoopOfCurrentThread() == &loop);
  loop.runAfter(1.0, callback);
  loop.loop();
}

int main() {
  printf("main(): pid = %d, tid = %d\n", getpid(), lynx::current_thread::tid());

  assert(lynx::EventLoop::getEventLoopOfCurrentThread() == nullptr);
  lynx::EventLoop loop;
  assert(lynx::EventLoop::getEventLoopOfCurrentThread() == &loop);

  lynx::Thread thread(threadFunc);
  thread.start();

  loop.loop();
}
