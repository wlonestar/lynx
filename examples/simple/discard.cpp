#include "discard.h"

int main() {
  LOG_INFO << "pid = " << getpid();
  lynx::EventLoop loop;
  lynx::InetAddress listen_addr(2009);
  DiscardServer server(&loop, listen_addr);
  server.start();
  loop.loop();
}
