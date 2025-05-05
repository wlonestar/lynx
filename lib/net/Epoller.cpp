#include "lynx/net/Epoller.h"
#include "lynx/logger/Logging.h"
#include "lynx/net/Channel.h"

#include <cassert>
#include <poll.h>

namespace lynx {

const int KNew = -1;
const int KAdded = 1;
const int KDeleted = 2;

Epoller::Epoller(EventLoop *Loop)
    : OwnerLoop(Loop), Epollfd(::epoll_create1(EPOLL_CLOEXEC)),
      Events(KInitEventListSize) {
  if (Epollfd < 0) {
    LOG_SYSFATAL << "Epoller::Epoller";
  }
}

Epoller::~Epoller() { ::close(Epollfd); }

Timestamp Epoller::poll(int TimeoutMs, ChannelList *ActiveChannels) {
  LOG_TRACE << "fd total count " << Channels.size();
  int NumEvents = ::epoll_wait(Epollfd, &*Events.begin(),
                               static_cast<int>(Events.size()), TimeoutMs);
  int SavedErrno = errno;
  Timestamp Now(Timestamp::now());
  if (NumEvents > 0) {
    LOG_TRACE << NumEvents << " events happened";
    fillActiveChannels(NumEvents, ActiveChannels);
    if (static_cast<size_t>(NumEvents) == Events.size()) {
      Events.resize(Events.size() * 2);
    }
  } else if (NumEvents == 0) {
    LOG_TRACE << "nothing happened";
  } else {
    if (SavedErrno != EINTR) {
      errno = SavedErrno;
      LOG_SYSERR << "Epoller::poll()";
    }
  }
  return Now;
}

void Epoller::fillActiveChannels(int NumEvents,
                                 ChannelList *ActiveChannels) const {
  assert(static_cast<size_t>(NumEvents) <= Events.size());
  for (int I = 0; I < NumEvents; ++I) {
    auto *Chann = static_cast<Channel *>(Events[I].data.ptr);
    Chann->setRevents(Events[I].events);
    ActiveChannels->push_back(Chann);
  }
}

void Epoller::updateChannel(Channel *Channel) {
  assertInLoopThread();
  const int Index = Channel->index();
  LOG_TRACE << "fd = " << Channel->fd() << " events = " << Channel->events()
            << " index = " << Index;
  if (Index == KNew || Index == KDeleted) {
    int Fd = Channel->fd();
    if (Index == KNew) {
      assert(Channels.find(Fd) == Channels.end());
      Channels[Fd] = Channel;
    } else {
      assert(Channels.find(Fd) != Channels.end());
      assert(Channels[Fd] == Channel);
    }

    Channel->setIndex(KAdded);
    update(EPOLL_CTL_ADD, Channel);
  } else {
    int Fd = Channel->fd();
    (void)Fd;
    assert(Channels.find(Fd) != Channels.end());
    assert(Channels[Fd] == Channel);
    assert(Index == KAdded);
    if (Channel->isNoneEvent()) {
      update(EPOLL_CTL_DEL, Channel);
      Channel->setIndex(KDeleted);
    } else {
      update(EPOLL_CTL_MOD, Channel);
    }
  }
}

void Epoller::removeChannel(Channel *Channel) {
  assertInLoopThread();
  int Fd = Channel->fd();
  LOG_TRACE << "fd = " << Fd;
  assert(Channels.find(Fd) != Channels.end());
  assert(Channels[Fd] == Channel);
  assert(Channel->isNoneEvent());
  int Index = Channel->index();
  assert(Index == KAdded || Index == KDeleted);
  size_t N = Channels.erase(Fd);
  (void)N;
  assert(N == 1);

  if (Index == KAdded)
    update(EPOLL_CTL_DEL, Channel);

  Channel->setIndex(KNew);
}

bool Epoller::hasChannel(Channel *Channel) const {
  assertInLoopThread();
  auto It = Channels.find(Channel->fd());
  return It != Channels.end() && It->second == Channel;
}

void Epoller::update(int Operation, Channel *Channel) {
  struct epoll_event Event;
  memset(&Event, 0, sizeof(Event));
  Event.events = Channel->events();
  Event.data.ptr = Channel;
  int Fd = Channel->fd();
  LOG_TRACE << "epoll_ctl op = " << operationToString(Operation)
            << " fd = " << Fd << " event = { " << Channel->eventsToString()
            << " }";
  if (::epoll_ctl(Epollfd, Operation, Fd, &Event) < 0) {
    if (Operation == EPOLL_CTL_DEL) {
      LOG_SYSERR << "epoll_ctl op =" << operationToString(Operation)
                 << " fd =" << Fd;
    } else {
      LOG_SYSFATAL << "epoll_ctl op =" << operationToString(Operation)
                   << " fd =" << Fd;
    }
  }
}

const char *Epoller::operationToString(int Op) {
  switch (Op) {
  case EPOLL_CTL_ADD:
    return "ADD";
  case EPOLL_CTL_DEL:
    return "DEL";
  case EPOLL_CTL_MOD:
    return "MOD";
  default:
    assert(false && "ERROR op");
    return "Unknown Operation";
  }
}

} // namespace lynx
