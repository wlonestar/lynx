#include "lynx/net/epoller.h"
#include "lynx/logger/logging.h"
#include "lynx/net/channel.h"

#include <cassert>
#include <cerrno>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace lynx {

static_assert(EPOLLIN == POLLIN, "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI, "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT, "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP, "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR, "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP, "epoll uses same flag values as poll");

const int K_NEW = -1;
const int K_ADDED = 1;
const int K_DELETED = 2;

Epoller::Epoller(EventLoop *loop)
    : owner_loop_(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(K_INIT_EVENT_LIST_SIZE) {
  if (epollfd_ < 0) {
    LOG_SYSFATAL << "EPollPoller::EPollPoller";
  }
}

Epoller::~Epoller() { ::close(epollfd_); }

Timestamp Epoller::poll(int timeoutMs, ChannelList *activeChannels) {
  LOG_TRACE << "fd total count " << channels_.size();
  int num_events = ::epoll_wait(epollfd_, &*events_.begin(),
                                static_cast<int>(events_.size()), timeoutMs);
  int saved_errno = errno;
  Timestamp now(Timestamp::now());
  if (num_events > 0) {
    LOG_TRACE << num_events << " events happened";
    fillActiveChannels(num_events, activeChannels);
    if (static_cast<size_t>(num_events) == events_.size()) {
      events_.resize(events_.size() * 2);
    }
  } else if (num_events == 0) {
    LOG_TRACE << "nothing happened";
  } else {
    if (saved_errno != EINTR) {
      errno = saved_errno;
      LOG_SYSERR << "EPollPoller::poll()";
    }
  }
  return now;
}

void Epoller::fillActiveChannels(int numEvents,
                                 ChannelList *activeChannels) const {
  assert(static_cast<size_t>(numEvents) <= events_.size());
  for (int i = 0; i < numEvents; ++i) {
    auto *channel = static_cast<Channel *>(events_[i].data.ptr);
#ifndef NDEBUG
    int fd = channel->fd();
    ChannelMap::const_iterator it = channels_.find(fd);
    assert(it != channels_.end());
    assert(it->second == channel);
#endif
    channel->setRevents(events_[i].events);
    activeChannels->push_back(channel);
  }
}

void Epoller::updateChannel(Channel *channel) {
  assertInLoopThread();
  const int index = channel->index();
  LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events()
            << " index = " << index;
  if (index == K_NEW || index == K_DELETED) {
    int fd = channel->fd();
    if (index == K_NEW) {
      assert(channels_.find(fd) == channels_.end());
      channels_[fd] = channel;
    } else {
      assert(channels_.find(fd) != channels_.end());
      assert(channels_[fd] == channel);
    }

    channel->setIndex(K_ADDED);
    update(EPOLL_CTL_ADD, channel);
  } else {

    int fd = channel->fd();
    (void)fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(index == K_ADDED);
    if (channel->isNoneEvent()) {
      update(EPOLL_CTL_DEL, channel);
      channel->setIndex(K_DELETED);
    } else {
      update(EPOLL_CTL_MOD, channel);
    }
  }
}

void Epoller::removeChannel(Channel *channel) {
  assertInLoopThread();
  int fd = channel->fd();
  LOG_TRACE << "fd = " << fd;
  assert(channels_.find(fd) != channels_.end());
  assert(channels_[fd] == channel);
  assert(channel->isNoneEvent());
  int index = channel->index();
  assert(index == K_ADDED || index == K_DELETED);
  size_t n = channels_.erase(fd);
  (void)n;
  assert(n == 1);

  if (index == K_ADDED) {
    update(EPOLL_CTL_DEL, channel);
  }
  channel->setIndex(K_NEW);
}

bool Epoller::hasChannel(Channel *channel) const {
  assertInLoopThread();
  auto it = channels_.find(channel->fd());
  return it != channels_.end() && it->second == channel;
}

void Epoller::update(int operation, Channel *channel) {
  struct epoll_event event;
  bzero(&event, sizeof event);
  event.events = channel->events();
  event.data.ptr = channel;
  int fd = channel->fd();
  LOG_TRACE << "epoll_ctl op = " << operationToString(operation)
            << " fd = " << fd << " event = { " << channel->eventsToString()
            << " }";
  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
    if (operation == EPOLL_CTL_DEL) {
      LOG_SYSERR << "epoll_ctl op =" << operationToString(operation)
                 << " fd =" << fd;
    } else {
      LOG_SYSFATAL << "epoll_ctl op =" << operationToString(operation)
                   << " fd =" << fd;
    }
  }
}

const char *Epoller::operationToString(int op) {
  switch (op) {
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
