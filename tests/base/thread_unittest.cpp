#include "lynx/base/current_thread.h"
#include "lynx/base/thread.h"

void mysleep(int seconds) {
  timespec t = {seconds, 0};
  nanosleep(&t, nullptr);
}

void threadFunc() { printf("tid=%d\n", lynx::current_thread::tid()); }

void threadFunc2(int x) {
  printf("tid=%d, x=%d\n", lynx::current_thread::tid(), x);
}

void threadFunc3() {
  printf("tid=%d\n", lynx::current_thread::tid());
  mysleep(1);
}

class Foo {
public:
  explicit Foo(double x) : x_(x) {}

  void memberFunc() {
    printf("tid=%d, Foo::x_=%f\n", lynx::current_thread::tid(), x_);
  }

  void memberFunc2(const std::string &text) {
    printf("tid=%d, Foo::x_=%f, text=%s\n", lynx::current_thread::tid(), x_,
           text.c_str());
  }

private:
  double x_;
};

int main() {
  printf("pid=%d, tid=%d\n", ::getpid(), lynx::current_thread::tid());

  lynx::Thread t1(threadFunc);
  t1.start();
  printf("t1.tid=%d\n", t1.tid());
  t1.join();

  lynx::Thread t2([] { return threadFunc2(42); },
                  "thread for free function with argument");
  t2.start();
  printf("t2.tid=%d\n", t2.tid());
  t2.join();

  Foo foo(87.53);
  lynx::Thread t3([object_ptr = &foo] { object_ptr->memberFunc(); },
                  "thread for member function without argument");
  t3.start();
  t3.join();

  lynx::Thread t4([&foo] { foo.memberFunc2(std::string("wjl")); });
  t4.start();
  t4.join();

  {
    lynx::Thread t5(threadFunc3);
    t5.start();
    // t5 may destruct eariler than thread creation.
  }
  mysleep(2);
  {
    lynx::Thread t6(threadFunc3);
    t6.start();
    mysleep(2);
    // t6 destruct later than thread creation.
  }
  sleep(2);
  printf("number of created threads %d\n", lynx::Thread::numCreated());
}
