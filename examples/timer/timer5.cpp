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

  ~Printer() { printf("Final count is %d\n", count_); }

  void print1() {
    bool should_quit = false;
    int count = 0;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (count_ < 10) {
        count = count_;
        ++count_;
      } else {
        should_quit = true;
      }
    }

    if (should_quit) {
      loop1_->quit();
    } else {
      printf("Timer 1: %d\n", count);
      loop1_->runAfter(1, [&] { print1(); });
    }
  }

  void print2() {
    bool should_quit = false;
    int count = 0;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (count_ < 10) {
        count = count_;
        ++count_;
      } else {
        should_quit = true;
      }
    }

    if (should_quit) {
      loop2_->quit();
    } else {
      printf("Timer 2: %d\n", count);
      loop2_->runAfter(1, [&] { print2(); });
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
