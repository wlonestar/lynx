#include "lynx/net/tcp_server.h"
#include "lynx/logger/logging.h"
#include "lynx/net/acceptor.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/event_loop_thread_pool.h"
#include "lynx/net/sockets_ops.h"

#include <atomic>
#include <cstdio>

namespace lynx {

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr,
                     const std::string &nameArg, Option option)
    : loop_(CHECK_NOTNULL(loop)), ip_port_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      thread_pool_(new EventLoopThreadPool(loop, name_)),
      connection_callback_(defaultConnectionCallback),
      message_callback_(defaultMessageCallback), next_conn_id_(1) {
  acceptor_->setNewConnectionCallback([this](auto &&PH1, auto &&PH2) {
    newConnection(std::forward<decltype(PH1)>(PH1),
                  std::forward<decltype(PH2)>(PH2));
  });
}

TcpServer::~TcpServer() {
  loop_->assertInLoopThread();
  LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

  for (auto &item : connections_) {
    TcpConnectionPtr conn(item.second);
    item.second.reset();
    conn->getLoop()->runInLoop([conn] { conn->connectDestroyed(); });
  }
}

void TcpServer::setThreadNum(int numThreads) {
  assert(0 <= numThreads);
  thread_pool_->setThreadNum(numThreads);
}

void TcpServer::start() {
  if (started_.exchange(1, std::memory_order_seq_cst) == 0) {
    thread_pool_->start(thread_init_callback_);

    assert(!acceptor_->listening());
    loop_->runInLoop(
        [capture0 = getPointer(acceptor_)] { capture0->listen(); });
  }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
  loop_->assertInLoopThread();
  EventLoop *io_loop = thread_pool_->getNextLoop();
  char buf[64];
  snprintf(buf, sizeof buf, "-%s#%d", ip_port_.c_str(), next_conn_id_);
  ++next_conn_id_;
  std::string conn_name = name_ + buf;

  LOG_INFO << "TcpServer::newConnection [" << name_ << "] - new connection ["
           << conn_name << "] from " << peerAddr.toIpPort();
  InetAddress local_addr(sockets::getLocalAddr(sockfd));
  TcpConnectionPtr conn(
      new TcpConnection(io_loop, conn_name, sockfd, local_addr, peerAddr));
  connections_[conn_name] = conn;
  conn->setConnectionCallback(connection_callback_);
  conn->setMessageCallback(message_callback_);
  conn->setWriteCompleteCallback(write_complete_callback_);
  conn->setCloseCallback([this](auto &&PH1) {
    removeConnection(std::forward<decltype(PH1)>(PH1));
  });
  io_loop->runInLoop([conn] { conn->connectEstablished(); });
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
  loop_->runInLoop([this, conn] { removeConnectionInLoop(conn); });
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
  loop_->assertInLoopThread();
  LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
           << "] - connection " << conn->name();
  size_t n = connections_.erase(conn->name());
  (void)n;
  assert(n == 1);
  EventLoop *io_loop = conn->getLoop();
  io_loop->queueInLoop([conn] { conn->connectDestroyed(); });
}

} // namespace lynx
