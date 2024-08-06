#include "lynx/net/event_loop.h"

#include <iostream>

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
  int count = 0;

  lynx::EventLoop loop;
  loop.runEvery(1, [&loop, &count] { print(&loop, &count); });
  loop.loop();
  std::cout << "Final count is " << count << "\n";
}
