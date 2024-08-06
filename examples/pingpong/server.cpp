#include "lynx/base/current_thread.h"
#include "lynx/base/timestamp.h"
#include "lynx/logger/logging.h"
#include "lynx/net/buffer.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/inet_address.h"
#include "lynx/net/tcp_connection.h"
#include "lynx/net/tcp_server.h"

void onConnection(const lynx::TcpConnectionPtr &conn) {
  if (conn->connected()) {
    conn->setTcpNoDelay(true);
  }
}

void onMessage(const lynx::TcpConnectionPtr &conn, lynx::Buffer *buf,
               lynx::Timestamp time) {
  conn->send(buf);
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    fprintf(stderr, "Usage: %s <address> <port> <threads>\n", argv[0]);
    exit(-1);
  }

  LOG_INFO << "pid = " << getpid() << ", tid = " << lynx::current_thread::tid();

  const char *ip = argv[1];
  auto port = static_cast<uint16_t>(atoi(argv[2]));
  lynx::InetAddress listen_addr(ip, port);
  int thread_count = atoi(argv[3]);

  lynx::EventLoop loop;
  lynx::TcpServer server(&loop, listen_addr, "PingPong");
  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);

  if (thread_count > 1) {
    server.setThreadNum(thread_count);
  }

  server.start();
  loop.loop();
}
