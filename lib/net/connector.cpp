#include "lynx/net/connector.h"
#include "lynx/logger/logging.h"
#include "lynx/net/channel.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/sockets_ops.h"

#include <memory>

namespace lynx {

const int Connector::K_MAX_RETRY_DELAY_MS;

Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop), server_addr_(serverAddr), connect_(false),
      state_(kDisconnected), retry_delay_ms_(K_INIT_RETRY_DELAY_MS) {
  LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector() {
  LOG_DEBUG << "dtor[" << this << "]";
  assert(!channel_);
}

void Connector::start() {
  connect_ = true;
  loop_->runInLoop([this] { startInLoop(); });
}

void Connector::startInLoop() {
  loop_->assertInLoopThread();
  assert(state_ == kDisconnected);
  if (connect_) {
    connect();
  } else {
    LOG_DEBUG << "do not connect";
  }
}

void Connector::stop() {
  connect_ = false;
  loop_->queueInLoop([this] { stopInLoop(); });
}

void Connector::stopInLoop() {
  loop_->assertInLoopThread();
  if (state_ == kConnecting) {
    setState(kDisconnected);
    int sockfd = removeAndResetChannel();
    retry(sockfd);
  }
}

void Connector::connect() {
  int sockfd = sockets::createNonblockingOrDie(server_addr_.family());
  int ret = sockets::connect(sockfd, server_addr_.getSockAddr());
  int saved_errno = (ret == 0) ? 0 : errno;
  switch (saved_errno) {
  case 0:
  case EINPROGRESS:
  case EINTR:
  case EISCONN:
    connecting(sockfd);
    break;
  case EAGAIN:
  case EADDRINUSE:
  case EADDRNOTAVAIL:
  case ECONNREFUSED:
  case ENETUNREACH:
    retry(sockfd);
    break;
  case EACCES:
  case EPERM:
  case EAFNOSUPPORT:
  case EALREADY:
  case EBADF:
  case EFAULT:
  case ENOTSOCK:
    LOG_SYSERR << "connect error in Connector::startInLoop " << saved_errno;
    sockets::close(sockfd);
    break;
  default:
    LOG_SYSERR << "Unexpected error in Connector::startInLoop " << saved_errno;
    sockets::close(sockfd);
    break;
  }
}

void Connector::restart() {
  loop_->assertInLoopThread();
  setState(kDisconnected);
  retry_delay_ms_ = K_INIT_RETRY_DELAY_MS;
  connect_ = true;
  startInLoop();
}

void Connector::connecting(int sockfd) {
  setState(kConnecting);
  assert(!channel_);
  channel_ = std::make_unique<Channel>(loop_, sockfd);
  channel_->setWriteCallback([this] { handleWrite(); });
  channel_->setErrorCallback([this] { handleError(); });
  channel_->enableWriting();
}

int Connector::removeAndResetChannel() {
  channel_->disableAll();
  channel_->remove();
  int sockfd = channel_->fd();
  loop_->queueInLoop([this] { resetChannel(); });
  return sockfd;
}

void Connector::resetChannel() { channel_.reset(); }

void Connector::handleWrite() {
  LOG_TRACE << "Connector::handleWrite " << state_;

  if (state_ == kConnecting) {
    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);
    if (err != 0) {
      LOG_WARN << "Connector::handleWrite - SO_ERROR = " << err << " "
               << current_thread::strError(err);
      retry(sockfd);
    } else if (sockets::isSelfConnect(sockfd)) {
      LOG_WARN << "Connector::handleWrite - Self connect";
      retry(sockfd);
    } else {
      setState(kConnected);
      if (connect_) {
        new_connection_callback_(sockfd);
      } else {
        sockets::close(sockfd);
      }
    }
  } else {
    assert(state_ == kDisconnected);
  }
}

void Connector::handleError() {
  LOG_ERROR << "Connector::handleError state=" << state_;
  if (state_ == kConnecting) {
    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);
    LOG_TRACE << "SO_ERROR = " << err << " " << current_thread::strError(err);
    retry(sockfd);
  }
}

void Connector::retry(int sockfd) {
  sockets::close(sockfd);
  setState(kDisconnected);
  if (connect_) {
    LOG_INFO << "Connector::retry - Retry connecting to "
             << server_addr_.toIpPort() << " in " << retry_delay_ms_
             << " milliseconds. ";
    loop_->runAfter(retry_delay_ms_ / 1000.0, [capture0 = shared_from_this()] {
      capture0->startInLoop();
    });
    retry_delay_ms_ = std::min(retry_delay_ms_ * 2, K_MAX_RETRY_DELAY_MS);
  } else {
    LOG_DEBUG << "do not connect";
  }
}

} // namespace lynx
