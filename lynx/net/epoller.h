#ifndef LYNX_NET_EPOLLER_H
#define LYNX_NET_EPOLLER_H

#include "lynx/net/event_loop.h"

#include <map>
#include <sys/epoll.h>

namespace lynx {

class Channel;

/**
 * @class Epoller
 * @brief A wrapper around the epoll system call for multiplexing IO events.
 *
 * The Epoller class is responsible for managing and dispatching IO events
 * using the epoll API. It monitors multiple file descriptors to see if I/O
 * operations can be performed on any of them.
 */
class Epoller : Noncopyable {
public:
  using ChannelList = std::vector<Channel *>;

  /**
   * @brief Constructs an Epoller associated with the given EventLoop.
   * @param loop The EventLoop that manages this Epoller.
   */
  Epoller(EventLoop *loop);
  ~Epoller();

  /**
   * @brief Polls the epoll file descriptor for events.
   * @param timeoutMs The timeout in milliseconds.
   * @param activeChannels The list to store active channels.
   * @return The current time when polling returns.
   */
  Timestamp poll(int timeoutMs, ChannelList *activeChannels);

  /// Updates or adds a channel to the epoll interest list.
  void updateChannel(Channel *channel);

  /// Removes a channel from the epoll interest list.
  void removeChannel(Channel *channel);

  /**
   * @brief Checks if a channel is in the epoll interest list.
   * @param channel The channel to check.
   * @return True if the channel is in the list, otherwise false.
   */
  bool hasChannel(Channel *channel) const;

  void assertInLoopThread() const { owner_loop_->assertInLoopThread(); }

private:
  static const int K_INIT_EVENT_LIST_SIZE = 16;

  static const char *operationToString(int op);

  /**
   * @brief Fills the active channels list based on the number of events.
   * @param numEvents The number of events.
   * @param activeChannels The list to store active channels.
   */
  void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

  /**
   * @brief Updates the epoll interest list with a given operation.
   * @param operation The epoll operation (e.g., EPOLL_CTL_ADD).
   * @param channel The channel to update.
   */
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
