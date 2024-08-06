#include "lynx/base/thread.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/event_loop_thread_pool.h"

#include <cassert>

void print(lynx::EventLoop *p = nullptr) {
  printf("main(): pid = %d, tid = %d, loop = %p\n", getpid(),
         lynx::current_thread::tid(), p);
}

void init(lynx::EventLoop *p) {
  printf("init(): pid = %d, tid = %d, loop = %p\n", getpid(),
         lynx::current_thread::tid(), p);
}

int main() {
  print();

  lynx::EventLoop loop;
  loop.runAfter(11, [object_ptr = &loop] { object_ptr->quit(); });

  {
    printf("Single thread %p:\n", &loop);
    lynx::EventLoopThreadPool model(&loop, "single");
    model.setThreadNum(0);
    model.start(init);
    assert(model.getNextLoop() == &loop);
    assert(model.getNextLoop() == &loop);
    assert(model.getNextLoop() == &loop);
  }

  {
    printf("Another thread:\n");
    lynx::EventLoopThreadPool model(&loop, "another");
    model.setThreadNum(1);
    model.start(init);
    lynx::EventLoop *next_loop = model.getNextLoop();
    next_loop->runAfter(2, [next_loop] { return print(next_loop); });
    assert(next_loop != &loop);
    assert(next_loop == model.getNextLoop());
    assert(next_loop == model.getNextLoop());
    ::sleep(3);
  }

  {
    printf("Three threads:\n");
    lynx::EventLoopThreadPool model(&loop, "three");
    model.setThreadNum(3);
    model.start(init);
    lynx::EventLoop *next_loop = model.getNextLoop();
    next_loop->runInLoop([next_loop] { return print(next_loop); });
    assert(next_loop != &loop);
    assert(next_loop != model.getNextLoop());
    assert(next_loop != model.getNextLoop());
    assert(next_loop == model.getNextLoop());
  }

  loop.loop();
}
