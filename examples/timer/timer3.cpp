#include "lynx/net/event_loop.h"

#include <iostream>

class Printer : lynx::Noncopyable {
public:
  Printer(lynx::EventLoop *loop) : loop_(loop) {
    loop_->runEvery(1, [&] { print(); });
  }

  ~Printer() { std::cout << "Final count is " << count_ << "\n"; }

  void print() {
    if (count_ < 5) {
      std::cout << count_ << "\n";
      ++count_;
      loop_->runAfter(1, [&] { print(); });
    } else {
      loop_->quit();
    }
  }

private:
  lynx::EventLoop *loop_;
  int count_{};
};

void print(lynx::EventLoop *loop, int *count) {
  if (*count < 5) {
    std::cout << *count << "\n";
    ++(*count);
    loop->runAfter(1, [&loop, &count] { print(loop, count); });
  } else {
    loop->quit();
  }
}

int main() {
  lynx::EventLoop loop;
  Printer printer(&loop);
  loop.loop();
}
