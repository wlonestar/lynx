#include "lynx/logger/Logging.h"
#include "lynx/net/Channel.h"
#include "lynx/net/EventLoop.h"
#include "lynx/net/Socket.h"
#include "lynx/net/TcpConnection.h"

namespace lynx {

void defaultConnectionCallback(const TcpConnectionPtr &Conn) {
  LOG_TRACE << Conn->localAddress().toIpPort() << " -> "
            << Conn->peerAddress().toIpPort() << " is "
            << (Conn->connected() ? "UP" : "DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr & /*unused*/, Buffer *Buf,
                            Timestamp /*unused*/) {
  Buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop *Loop, const std::string &Name,
                             int Sockfd, const InetAddress &LocalAddr,
                             const InetAddress &PeerAddr)
    : Loop(CHECK_NOTNULL(Loop)), Name(Name), State(CONNECTING), Reading(true),
      Sockt(new Socket(Sockfd)), Chann(new Channel(Loop, Sockfd)),
      LocalAddr(LocalAddr), PeerAddr(PeerAddr),
      HighWaterMark(64 * 1024 * 1024) {
  Chann->setReadCallback(
      [this](auto &&PH1) { handleRead(std::forward<decltype(PH1)>(PH1)); });
  Chann->setWriteCallback([this] { handleWrite(); });
  Chann->setCloseCallback([this] { handleClose(); });
  Chann->setErrorCallback([this] { handleError(); });
  LOG_DEBUG << "TcpConnection::ctor[" << Name << "] at " << this
            << " fd=" << Sockfd;
  Sockt->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
  LOG_DEBUG << "TcpConnection::dtor[" << Name << "] at " << this
            << " fd=" << Chann->fd() << " state=" << stateToString();
  assert(State == DISCONNECTED);
}

bool TcpConnection::getTcpInfo(struct tcp_info *Tcpi) const {
  return Sockt->getTcpInfo(Tcpi);
}

std::string TcpConnection::getTcpInfoString() const {
  char Buf[1024];
  Buf[0] = '\0';
  Sockt->getTcpInfoString(Buf, sizeof(Buf));
  return Buf;
}

void TcpConnection::send(const void *Data, int Len) {
  send(std::string(static_cast<const char *>(Data), Len));
}

void TcpConnection::send(const std::string &Message) {
  if (State == CONNECTED) {
    if (Loop->isInLoopThread()) {
      sendInLoop(Message);
    } else {
      Loop->runInLoop([this, &Message] { sendInLoop(Message); });
    }
  }
}

void TcpConnection::send(Buffer *Buf) {
  if (State == CONNECTED) {
    if (Loop->isInLoopThread()) {
      sendInLoop(Buf->peek(), Buf->readableBytes());
      Buf->retrieveAll();
    } else {
      Loop->runInLoop([this, &Buf] { sendInLoop(Buf->retrieveAllAsString()); });
    }
  }
}

void TcpConnection::sendInLoop(const std::string &Message) {
  sendInLoop(Message.data(), Message.size());
}

void TcpConnection::sendInLoop(const void *Data, size_t Len) {
  Loop->assertInLoopThread();
  ssize_t Nwrote = 0;
  size_t Remaining = Len;
  bool FaultError = false;
  if (State == DISCONNECTED) {
    LOG_WARN << "disconnected, give up writing";
    return;
  }
  if (!Chann->isWriting() && OutputBuffer.readableBytes() == 0) {
    Nwrote = ::write(Chann->fd(), Data, Len);
    if (Nwrote >= 0) {
      Remaining = Len - Nwrote;
      if (Remaining == 0 && WriteCompleteCallback)
        Loop->queueInLoop(
            [this] { WriteCompleteCallback(shared_from_this()); });

    } else {
      Nwrote = 0;
      if (errno != EWOULDBLOCK) {
        LOG_SYSERR << "TcpConnection::sendInLoop";
        if (errno == EPIPE || errno == ECONNRESET)
          FaultError = true;
      }
    }
  }

  assert(Remaining <= Len);
  if (!FaultError && Remaining > 0) {
    size_t OldLen = OutputBuffer.readableBytes();
    if (OldLen + Remaining >= HighWaterMark && OldLen < HighWaterMark &&
        HighWaterMarkCallback) {
      Loop->queueInLoop([this, &OldLen, &Remaining] {
        HighWaterMarkCallback(shared_from_this(), OldLen + Remaining);
      });
    }
    OutputBuffer.append(static_cast<const char *>(Data) + Nwrote, Remaining);
    if (!Chann->isWriting())
      Chann->enableWriting();
  }
}

void TcpConnection::shutdown() {
  if (State == CONNECTED) {
    setState(DISCONNECTING);
    Loop->runInLoop([this] { shutdownInLoop(); });
  }
}

void TcpConnection::shutdownInLoop() {
  Loop->assertInLoopThread();
  if (!Chann->isWriting())
    Sockt->shutdownWrite();
}

void TcpConnection::forceClose() {
  if (State == CONNECTED || State == DISCONNECTING) {
    setState(DISCONNECTING);
    Loop->queueInLoop(
        [Capture0 = shared_from_this()] { Capture0->forceCloseInLoop(); });
  }
}

void TcpConnection::forceCloseInLoop() {
  Loop->assertInLoopThread();
  if (State == CONNECTED || State == DISCONNECTING) {
    handleClose();
  }
}

const char *TcpConnection::stateToString() const {
  switch (State) {
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

void TcpConnection::setTcpNoDelay(bool On) { Sockt->setTcpNoDelay(On); }

void TcpConnection::startRead() {
  Loop->runInLoop([this] { startReadInLoop(); });
}

void TcpConnection::startReadInLoop() {
  Loop->assertInLoopThread();
  if (!Reading || !Chann->isReading()) {
    Chann->enableReading();
    Reading = true;
  }
}

void TcpConnection::stopRead() {
  Loop->runInLoop([this] { stopReadInLoop(); });
}

void TcpConnection::stopReadInLoop() {
  Loop->assertInLoopThread();
  if (Reading || Chann->isReading()) {
    Chann->disableReading();
    Reading = false;
  }
}

void TcpConnection::connectEstablished() {
  Loop->assertInLoopThread();
  assert(State == CONNECTING);
  setState(CONNECTED);
  Chann->tie(shared_from_this());
  Chann->enableReading();

  ConnectionCallback(shared_from_this());
}

void TcpConnection::connectDestroyed() {
  Loop->assertInLoopThread();
  if (State == CONNECTED) {
    setState(DISCONNECTED);
    Chann->disableAll();

    ConnectionCallback(shared_from_this());
  }
  Chann->remove();
}

void TcpConnection::handleRead(Timestamp ReceiveTime) {
  Loop->assertInLoopThread();
  int SavedErrno = 0;
  ssize_t N = InputBuffer.readFd(Chann->fd(), &SavedErrno);
  if (N > 0) {
    MessageCallback(shared_from_this(), &InputBuffer, ReceiveTime);
  } else if (N == 0) {
    handleClose();
  } else {
    errno = SavedErrno;
    LOG_SYSERR << "TcpConnection::handleRead";
    handleError();
  }
}

void TcpConnection::handleWrite() {
  Loop->assertInLoopThread();
  if (Chann->isWriting()) {
    ssize_t N =
        ::write(Chann->fd(), OutputBuffer.peek(), OutputBuffer.readableBytes());
    if (N > 0) {
      OutputBuffer.retrieve(N);
      if (OutputBuffer.readableBytes() == 0) {
        Chann->disableWriting();
        if (WriteCompleteCallback) {
          Loop->queueInLoop(
              [this] { WriteCompleteCallback(shared_from_this()); });
        }
        if (State == DISCONNECTING) {
          shutdownInLoop();
        }
      }
    } else {
      LOG_SYSERR << "TcpConnection::handleWrite";
    }
  } else {
    LOG_TRACE << "Connection fd = " << Chann->fd()
              << " is down, no more writing";
  }
}

void TcpConnection::handleClose() {
  Loop->assertInLoopThread();
  LOG_TRACE << "fd = " << Chann->fd() << " state = " << stateToString();
  assert(State == CONNECTED || State == DISCONNECTING);
  setState(DISCONNECTED);
  Chann->disableAll();

  TcpConnectionPtr GuardThis(shared_from_this());
  ConnectionCallback(GuardThis);
  CloseCallback(GuardThis);
}

void TcpConnection::handleError() {
  int Optval;
  auto Optlen = static_cast<socklen_t>(sizeof(Optval));
  int Err = 0;
  if (::getsockopt(Chann->fd(), SOL_SOCKET, SO_ERROR, &Optval, &Optlen) < 0) {
    Err = errno;
  } else {
    Err = Optval;
  }
  LOG_ERROR << "TcpConnection::handleError [" << Name
            << "] - SO_ERROR = " << Err << " " << current_thread::strError(Err);
}

} // namespace lynx
