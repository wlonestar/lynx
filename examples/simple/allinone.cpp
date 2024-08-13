#include "./time.h"
#include "chargen.h"
#include "daytime.h"
#include "discard.h"
#include "echo.h"

int main() {
  LOG_INFO << "pid = " << getpid();
  lynx::EventLoop loop; /// one loop shared by multiple servers

  ChargenServer chargen_server(&loop, lynx::InetAddress(2019));
  chargen_server.start();

  DaytimeServer daytime_server(&loop, lynx::InetAddress(2013));
  daytime_server.start();

  DiscardServer discard_server(&loop, lynx::InetAddress(2009));
  discard_server.start();

  EchoServer echo_server(&loop, lynx::InetAddress(2007));
  echo_server.start();

  TimeServer time_server(&loop, lynx::InetAddress(2037));
  time_server.start();

  loop.loop();
}
