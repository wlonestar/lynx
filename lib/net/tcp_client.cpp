#include "lynx/net/tcp_client.h"
#include "lynx/logger/logging.h"
#include "lynx/net/connector.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/sockets_ops.h"

namespace lynx {

namespace detail {

void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn) {
  loop->queueInLoop([conn] { conn->connectDestroyed(); });
}

void removeConnector(const ConnectorPtr &connector) {}

} // namespace detail

TcpClient::TcpClient(EventLoop *loop, const InetAddress &serverAddr,
                     const std::string &nameArg)
    : loop_(CHECK_NOTNULL(loop)), connector_(new Connector(loop, serverAddr)),
      name_(nameArg), connection_callback_(defaultConnectionCallback),
      message_callback_(defaultMessageCallback), retry_(false), connect_(true),
      next_conn_id_(1) {
  connector_->setNewConnectionCallback(
      [this](auto &&PH1) { newConnection(std::forward<decltype(PH1)>(PH1)); });
  LOG_INFO << "TcpClient::TcpClient[" << name_ << "] - connector "
           << getPointer(connector_);
}

TcpClient::~TcpClient() {
  LOG_INFO << "TcpClient::~TcpClient[" << name_ << "] - connector "
           << getPointer(connector_);
  TcpConnectionPtr conn;
  bool unique = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    unique = connection_.unique();
    conn = connection_;
  }
  if (conn) {
    assert(loop_ == conn->getLoop());
    CloseCallback cb = [this](auto &&PH1) {
      return detail::removeConnection(loop_, std::forward<decltype(PH1)>(PH1));
    };
    loop_->runInLoop([conn, cb] { conn->setCloseCallback(cb); });
    if (unique) {
      conn->forceClose();
    }
  } else {
    connector_->stop();
    loop_->runAfter(1, [this] { return detail::removeConnector(connector_); });
  }
}

void TcpClient::connect() {
  LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to "
           << connector_->serverAddress().toIpPort();
  connect_ = true;
  connector_->start();
}

void TcpClient::disconnect() {
  connect_ = false;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connection_) {
      connection_->shutdown();
    }
  }
}

void TcpClient::stop() {
  connect_ = false;
  connector_->stop();
}

void TcpClient::newConnection(int sockfd) {
  loop_->assertInLoopThread();
  InetAddress peer_addr(sockets::getPeerAddr(sockfd));
  char buf[32];
  snprintf(buf, sizeof(buf), ":%s#%d", peer_addr.toIpPort().c_str(),
           next_conn_id_);
  ++next_conn_id_;
  std::string conn_name = name_ + buf;

  InetAddress local_addr(sockets::getLocalAddr(sockfd));
  TcpConnectionPtr conn(
      new TcpConnection(loop_, conn_name, sockfd, local_addr, peer_addr));

  conn->setConnectionCallback(connection_callback_);
  conn->setMessageCallback(message_callback_);
  conn->setWriteCompleteCallback(write_complete_callback_);
  conn->setCloseCallback([this](auto &&PH1) {
    removeConnection(std::forward<decltype(PH1)>(PH1));
  });
  {
    std::lock_guard<std::mutex> lock(mutex_);
    connection_ = conn;
  }
  conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn) {
  loop_->assertInLoopThread();
  assert(loop_ == conn->getLoop());

  {
    std::lock_guard<std::mutex> lock(mutex_);
    assert(connection_ == conn);
    connection_.reset();
  }

  loop_->queueInLoop([conn] { conn->connectDestroyed(); });
  if (retry_ && connect_) {
    LOG_INFO << "TcpClient::connect[" << name_ << "] - Reconnecting to "
             << connector_->serverAddress().toIpPort();
    connector_->restart();
  }
}

} // namespace lynx
