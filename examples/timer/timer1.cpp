#include "lynx/net/event_loop.h"

#include <iostream>

void print() { std::cout << "Hello, world\n"; }

int main() {
  lynx::EventLoop loop;
  loop.runAfter(5, print);
  loop.loop();
}
