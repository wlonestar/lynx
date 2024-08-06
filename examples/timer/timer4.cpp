#include "lynx/net/event_loop.h"
#include "lynx/net/event_loop_thread.h"

#include <iostream>

class Printer : lynx::Noncopyable {
public:
  Printer(lynx::EventLoop *loop1, lynx::EventLoop *loop2)
      : loop1_(loop1), loop2_(loop2) {
    loop1_->runAfter(1, [&] { print1(); });
    loop2_->runAfter(1, [&] { print2(); });
  }

  ~Printer() { std::cout << "Final count is " << count_ << "\n"; }

  void print1() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (count_ < 10) {
      std::cout << "Timer 1: " << count_ << "\n";
      ++count_;
      loop1_->runAfter(1, [&] { print1(); });
    } else {
      loop1_->quit();
    }
  }

  void print2() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (count_ < 10) {
      std::cout << "Timer 2: " << count_ << "\n";
      ++count_;
      loop2_->runAfter(1, [&] { print2(); });
    } else {
      loop2_->quit();
    }
  }

private:
  mutable std::mutex mutex_;
  lynx::EventLoop *loop1_;
  lynx::EventLoop *loop2_;
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
  std::unique_ptr<Printer> printer;

  lynx::EventLoop loop;
  lynx::EventLoopThread loop_thread;
  lynx::EventLoop *loop_in_another_thread = loop_thread.startLoop();

  printer = std::make_unique<Printer>(&loop, loop_in_another_thread);
  loop.loop();
}
