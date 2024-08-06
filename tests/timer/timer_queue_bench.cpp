#include "lynx/base/thread.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/event_loop_thread.h"

int cnt = 0;
lynx::EventLoop *g_loop;

void printTid() {
  printf("pid = %d, tid = %d\n", getpid(), lynx::current_thread::tid());
  printf("now %s\n", lynx::Timestamp::now().toString().c_str());
}

void print(const char *msg) {
  printf("msg %s %s\n", lynx::Timestamp::now().toString().c_str(), msg);
  if (++cnt == 20) {
    g_loop->quit();
  }
}

void cancel(lynx::TimerId timer) {
  g_loop->cancel(timer);
  printf("cancelled at %s\n", lynx::Timestamp::now().toString().c_str());
}

int main() {
  printTid();
  sleep(1);

  {
    lynx::EventLoop loop;
    g_loop = &loop;

    print("main");
    loop.runAfter(1, [] { return print("once1"); });
    loop.runAfter(1.5, [] { return print("once1.5"); });
    loop.runAfter(2.5, [] { return print("once2.5"); });
    loop.runAfter(3.5, [] { return print("once3.5"); });
    lynx::TimerId t45 = loop.runAfter(4.5, [] { return print("once4.5"); });
    loop.runAfter(4.2, [t45] { return cancel(t45); });
    loop.runAfter(4.8, [t45] { return cancel(t45); });
    loop.runEvery(2, [] { return print("every2"); });
    lynx::TimerId t3 = loop.runEvery(3, [] { return print("every3"); });
    loop.runAfter(9.001, [t3] { return cancel(t3); });

    loop.loop();
    print("main loop exits");
  }

  sleep(1);

  {
    lynx::EventLoopThread loop_thread;
    lynx::EventLoop *loop = loop_thread.startLoop();
    loop->runAfter(2, printTid);
    sleep(3);
    print("thread loop exits");
  }
}
