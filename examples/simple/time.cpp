#include "./time.h"

int main() {
  LOG_INFO << "pid = " << getpid();
  lynx::EventLoop loop;
  lynx::InetAddress listen_addr(2037);
  TimeServer server(&loop, listen_addr);
  server.start();
  loop.loop();
}
