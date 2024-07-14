#include "lynx/base/thread.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/event_loop_thread.h"

#include <cstdio>
#include <unistd.h>

void print(lynx::EventLoop *p = nullptr) {
  printf("print: pid = %d, tid = %d, loop = %p\n", getpid(),
         lynx::current_thread::tid(), p);
}

void quit(lynx::EventLoop *p) {
  print(p);
  p->quit();
}

int main() {
  print();

  {
    lynx::EventLoopThread thr1; // never start
  }

  {
    // dtor calls quit()
    lynx::EventLoopThread thr2;
    lynx::EventLoop *loop = thr2.startLoop();
    loop->runInLoop([loop] { return print(loop); });
    std::this_thread::sleep_for(std::chrono::microseconds(500 * 1000));
  }

  {
    // quit() before dtor
    lynx::EventLoopThread thr3;
    lynx::EventLoop *loop = thr3.startLoop();
    loop->runInLoop([loop] { return quit(loop); });
    std::this_thread::sleep_for(std::chrono::microseconds(500 * 1000));
  }
}
