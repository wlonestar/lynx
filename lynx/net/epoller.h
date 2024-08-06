#ifndef LYNX_NET_EPOLLER_H
#define LYNX_NET_EPOLLER_H

#include "lynx/net/event_loop.h"

#include <map>
#include <sys/epoll.h>

namespace lynx {

class Channel;

class Epoller : Noncopyable {
public:
  using ChannelList = std::vector<Channel *>;

  Epoller(EventLoop *loop);
  ~Epoller();

  Timestamp poll(int timeoutMs, ChannelList *activeChannels);

  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);
  bool hasChannel(Channel *channel) const;

  void assertInLoopThread() const { owner_loop_->assertInLoopThread(); }

private:
  static const int K_INIT_EVENT_LIST_SIZE = 16;

  static const char *operationToString(int op);

  void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
  void update(int operation, Channel *channel);

  using ChannelMap = std::map<int, Channel *>;
  using EventList = std::vector<struct epoll_event>;

  EventLoop *owner_loop_;
  int epollfd_;
  EventList events_;
  ChannelMap channels_;
};

} // namespace lynx

#endif
