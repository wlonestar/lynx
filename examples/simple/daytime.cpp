#include "daytime.h"

int main() {
  LOG_INFO << "pid = " << getpid();
  lynx::EventLoop loop;
  lynx::InetAddress listen_addr(2013);
  DaytimeServer server(&loop, listen_addr);
  server.start();
  loop.loop();
}
