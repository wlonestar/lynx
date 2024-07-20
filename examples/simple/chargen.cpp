#include "chargen.h"

int main() {
  LOG_INFO << "pid = " << getpid();
  lynx::EventLoop loop;
  lynx::InetAddress listen_addr(2019);
  ChargenServer server(&loop, listen_addr, true);
  server.start();
  loop.loop();
}
