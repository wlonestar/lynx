#include "lynx/net/tcp_connection.h"
#include "lynx/logger/logging.h"
#include "lynx/net/channel.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/socket.h"

namespace lynx {

void defaultConnectionCallback(const TcpConnectionPtr &conn) {
  LOG_TRACE << conn->localAddress().toIpPort() << " -> "
            << conn->peerAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr & /*unused*/, Buffer *buf,
                            Timestamp /*unused*/) {
  buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &name,
                             int sockfd, const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CHECK_NOTNULL(loop)), name_(name), state_(CONNECTING),
      reading_(true), socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)), local_addr_(localAddr),
      peer_addr_(peerAddr), high_water_mark_(64 * 1024 * 1024) {
  channel_->setReadCallback(
      [this](auto &&PH1) { handleRead(std::forward<decltype(PH1)>(PH1)); });
  channel_->setWriteCallback([this] { handleWrite(); });
  channel_->setCloseCallback([this] { handleClose(); });
  channel_->setErrorCallback([this] { handleError(); });
  LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
            << " fd=" << sockfd;
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
  LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
            << " fd=" << channel_->fd() << " state=" << stateToString();
  assert(state_ == DISCONNECTED);
}

bool TcpConnection::getTcpInfo(struct tcp_info *tcpi) const {
  return socket_->getTcpInfo(tcpi);
}

std::string TcpConnection::getTcpInfoString() const {
  char buf[1024];
  buf[0] = '\0';
  socket_->getTcpInfoString(buf, sizeof(buf));
  return buf;
}

void TcpConnection::send(const void *data, int len) {
  send(std::string(static_cast<const char *>(data), len));
}

void TcpConnection::send(const std::string &message) {
  if (state_ == CONNECTED) {
    if (loop_->isInLoopThread()) {
      sendInLoop(message);
    } else {
      loop_->runInLoop([this, &message] { sendInLoop(message); });
    }
  }
}

void TcpConnection::send(Buffer *buf) {
  if (state_ == CONNECTED) {
    if (loop_->isInLoopThread()) {
      sendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
    } else {
      loop_->runInLoop(
          [this, &buf] { sendInLoop(buf->retrieveAllAsString()); });
    }
  }
}

void TcpConnection::sendInLoop(const std::string &message) {
  sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void *data, size_t len) {
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool fault_error = false;
  if (state_ == DISCONNECTED) {
    LOG_WARN << "disconnected, give up writing";
    return;
  }
  if (!channel_->isWriting() && output_buffer_.readableBytes() == 0) {
    nwrote = ::write(channel_->fd(), data, len);
    if (nwrote >= 0) {
      remaining = len - nwrote;
      if (remaining == 0 && write_complete_callback_) {
        loop_->queueInLoop(
            [this] { write_complete_callback_(shared_from_this()); });
      }
    } else {
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        LOG_SYSERR << "TcpConnection::sendInLoop";
        if (errno == EPIPE || errno == ECONNRESET) {
          fault_error = true;
        }
      }
    }
  }

  assert(remaining <= len);
  if (!fault_error && remaining > 0) {
    size_t old_len = output_buffer_.readableBytes();
    if (old_len + remaining >= high_water_mark_ && old_len < high_water_mark_ &&
        high_water_mark_callback_) {
      loop_->queueInLoop([this, &old_len, &remaining] {
        high_water_mark_callback_(shared_from_this(), old_len + remaining);
      });
    }
    output_buffer_.append(static_cast<const char *>(data) + nwrote, remaining);
    if (!channel_->isWriting()) {
      channel_->enableWriting();
    }
  }
}

void TcpConnection::shutdown() {
  if (state_ == CONNECTED) {
    setState(DISCONNECTING);
    loop_->runInLoop([this] { shutdownInLoop(); });
  }
}

void TcpConnection::shutdownInLoop() {
  loop_->assertInLoopThread();
  if (!channel_->isWriting()) {
    socket_->shutdownWrite();
  }
}

void TcpConnection::forceClose() {
  if (state_ == CONNECTED || state_ == DISCONNECTING) {
    setState(DISCONNECTING);
    loop_->queueInLoop(
        [capture0 = shared_from_this()] { capture0->forceCloseInLoop(); });
  }
}

void TcpConnection::forceCloseInLoop() {
  loop_->assertInLoopThread();
  if (state_ == CONNECTED || state_ == DISCONNECTING) {
    handleClose();
  }
}

const char *TcpConnection::stateToString() const {
  switch (state_) {
  case DISCONNECTED:
    return "DISCONNECTED";
  case CONNECTING:
    return "CONNECTING";
  case CONNECTED:
    return "CONNECTED";
  case DISCONNECTING:
    return "DISCONNECTING";
  default:
    return "unknown state";
  }
}

void TcpConnection::setTcpNoDelay(bool on) { socket_->setTcpNoDelay(on); }

void TcpConnection::startRead() {
  loop_->runInLoop([this] { startReadInLoop(); });
}

void TcpConnection::startReadInLoop() {
  loop_->assertInLoopThread();
  if (!reading_ || !channel_->isReading()) {
    channel_->enableReading();
    reading_ = true;
  }
}

void TcpConnection::stopRead() {
  loop_->runInLoop([this] { stopReadInLoop(); });
}

void TcpConnection::stopReadInLoop() {
  loop_->assertInLoopThread();
  if (reading_ || channel_->isReading()) {
    channel_->disableReading();
    reading_ = false;
  }
}

void TcpConnection::connectEstablished() {
  loop_->assertInLoopThread();
  assert(state_ == CONNECTING);
  setState(CONNECTED);
  channel_->tie(shared_from_this());
  channel_->enableReading();

  connection_callback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
  loop_->assertInLoopThread();
  if (state_ == CONNECTED) {
    setState(DISCONNECTED);
    channel_->disableAll();

    connection_callback_(shared_from_this());
  }
  channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime) {
  loop_->assertInLoopThread();
  int saved_errno = 0;
  ssize_t n = input_buffer_.readFd(channel_->fd(), &saved_errno);
  if (n > 0) {
    message_callback_(shared_from_this(), &input_buffer_, receiveTime);
  } else if (n == 0) {
    handleClose();
  } else {
    errno = saved_errno;
    LOG_SYSERR << "TcpConnection::handleRead";
    handleError();
  }
}

void TcpConnection::handleWrite() {
  loop_->assertInLoopThread();
  if (channel_->isWriting()) {
    ssize_t n = ::write(channel_->fd(), output_buffer_.peek(),
                        output_buffer_.readableBytes());
    if (n > 0) {
      output_buffer_.retrieve(n);
      if (output_buffer_.readableBytes() == 0) {
        channel_->disableWriting();
        if (write_complete_callback_) {
          loop_->queueInLoop(
              [this] { write_complete_callback_(shared_from_this()); });
        }
        if (state_ == DISCONNECTING) {
          shutdownInLoop();
        }
      }
    } else {
      LOG_SYSERR << "TcpConnection::handleWrite";
    }
  } else {
    LOG_TRACE << "Connection fd = " << channel_->fd()
              << " is down, no more writing";
  }
}

void TcpConnection::handleClose() {
  loop_->assertInLoopThread();
  LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
  assert(state_ == CONNECTED || state_ == DISCONNECTING);
  setState(DISCONNECTED);
  channel_->disableAll();

  TcpConnectionPtr guard_this(shared_from_this());
  connection_callback_(guard_this);
  close_callback_(guard_this);
}

void TcpConnection::handleError() {
  int optval;
  auto optlen = static_cast<socklen_t>(sizeof(optval));
  int err = 0;
  if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) <
      0) {
    err = errno;
  } else {
    err = optval;
  }
  LOG_ERROR << "TcpConnection::handleError [" << name_
            << "] - SO_ERROR = " << err << " " << current_thread::strError(err);
}

} // namespace lynx
