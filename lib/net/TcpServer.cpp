#include "lynx/logger/Logging.h"
#include "lynx/net/Acceptor.h"
#include "lynx/net/EventLoop.h"
#include "lynx/net/EventLoopThreadPool.h"
#include "lynx/net/TcpServer.h"

namespace lynx {

namespace detail {

struct sockaddr_in getLocalAddr(int Sockfd) {
  struct sockaddr_in Localaddr;
  memset(&Localaddr, 0, sizeof(Localaddr));
  auto Addrlen = static_cast<socklen_t>(sizeof(Localaddr));
  if (::getsockname(
          Sockfd,
          static_cast<struct sockaddr *>(static_cast<void *>(&Localaddr)),
          &Addrlen) < 0)
    LOG_SYSERR << "getLocalAddr";

  return Localaddr;
}

} // namespace detail

TcpServer::TcpServer(EventLoop *Loop, const InetAddress &ListenAddr,
                     const std::string &Name, Option Option)
    : Loop(CHECK_NOTNULL(Loop)), IpPort(ListenAddr.toIpPort()), Name(Name),
      Acceptr(new Acceptor(Loop, ListenAddr, Option == REUSE_PORT)),
      ThreadPool(new EventLoopThreadPool(Loop, Name)),
      ConnectionCallback(defaultConnectionCallback),
      MessageCallback(defaultMessageCallback), NextConnId(1) {
  Acceptr->setNewConnectionCallback([this](auto &&PH1, auto &&PH2) {
    newConnection(std::forward<decltype(PH1)>(PH1),
                  std::forward<decltype(PH2)>(PH2));
  });
}

TcpServer::~TcpServer() {
  Loop->assertInLoopThread();
  LOG_TRACE << "TcpServer::~TcpServer [" << Name << "] destructing";

  for (auto &Item : Connections) {
    TcpConnectionPtr Conn(Item.second);
    Item.second.reset();
    Conn->getLoop()->runInLoop([Conn] { Conn->connectDestroyed(); });
  }
}

void TcpServer::setThreadNum(int NumThreads) {
  assert(0 <= NumThreads);
  ThreadPool->setThreadNum(NumThreads);
}

void TcpServer::start() {
  if (Started.exchange(1, std::memory_order_seq_cst) == 0) {
    ThreadPool->start(ThreadInitCallback);

    assert(!Acceptr->listening());
    Loop->runInLoop([Capture0 = Acceptr.get()] { Capture0->listen(); });
  }
}

void TcpServer::newConnection(int Sockfd, const InetAddress &PeerAddr) {
  Loop->assertInLoopThread();
  EventLoop *IoLoop = ThreadPool->getNextLoop();
  auto Buf = fmt::format("-{}#{:d}", IpPort, NextConnId);
  ++NextConnId;
  std::string ConnName = Name + Buf;

  LOG_INFO << "TcpServer::newConnection [" << Name << "] - new connection ["
           << ConnName << "] from " << PeerAddr.toIpPort();
  InetAddress LocalAddr(detail::getLocalAddr(Sockfd));
  TcpConnectionPtr Conn(
      new TcpConnection(IoLoop, ConnName, Sockfd, LocalAddr, PeerAddr));
  Connections[ConnName] = Conn;
  Conn->setConnectionCallback(ConnectionCallback);
  Conn->setMessageCallback(MessageCallback);
  Conn->setWriteCompleteCallback(WriteCompleteCallback);
  Conn->setCloseCallback([this](auto &&PH1) {
    removeConnection(std::forward<decltype(PH1)>(PH1));
  });
  IoLoop->runInLoop([Conn] { Conn->connectEstablished(); });
}

void TcpServer::removeConnection(const TcpConnectionPtr &Conn) {
  Loop->runInLoop([this, Conn] { removeConnectionInLoop(Conn); });
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &Conn) {
  Loop->assertInLoopThread();
  LOG_INFO << "TcpServer::removeConnectionInLoop [" << Name << "] - connection "
           << Conn->name();
  size_t N = Connections.erase(Conn->name());
  (void)N;
  assert(N == 1);
  EventLoop *IoLoop = Conn->getLoop();
  IoLoop->queueInLoop([Conn] { Conn->connectDestroyed(); });
}

} // namespace lynx
