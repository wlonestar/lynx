#include "lynx/base/current_thread.h"
#include "lynx/base/exception.h"

#include <cstdio>
#include <functional>
#include <vector>

class Bar {
public:
  void test(std::vector<std::string> names = {}) {
    printf("Stack:\n%s\n", lynx::current_thread::stackTrace(true).c_str());
    [] {
      printf("Stack inside lambda:\n%s\n",
             lynx::current_thread::stackTrace(true).c_str());
    }();
    std::function<void()> func([] {
      printf("Stack inside std::function:\n%s\n",
             lynx::current_thread::stackTrace(true).c_str());
    });
    func();

    func = [this] { callback(); };
    func();

    throw lynx::Exception("oops");
  }

private:
  void callback() {
    printf("Stack inside std::bind:\n%s\n",
           lynx::current_thread::stackTrace(true).c_str());
  }
};

void foo() {
  Bar b;
  b.test();
}

int main() {
  try {
    foo();
  } catch (const lynx::Exception &ex) {
    printf("reason: %s\n", ex.what());
    printf("stack trace:\n%s\n", ex.stackTrace());
  }
}
