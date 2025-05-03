#ifndef LYNX_NET_CHANNEL_H
#define LYNX_NET_CHANNEL_H

#include "lynx/base/noncopyable.h"
#include "lynx/base/timestamp.h"

#include <functional>
#include <memory>

namespace lynx {

class EventLoop;

/**
 * @class Channel
 * @brief A class that abstracts the concept of a channel.
 *
 * The Channel class represents a file descriptor and the events it is
 * interested in (read, write, etc.). It is used by the EventLoop to dispatch
 * events to the appropriate callback functions. This class is non-copyable to
 * ensure each file descriptor is managed by a single Channel instance.
 *
 * Key Features:
 * - Associates a file descriptor with a set of events.
 * - Provides methods to enable or disable specific events (reading, writing).
 * - Supports setting callback functions for various events (read, write, close,
 * error).
 * - Integrates with the EventLoop to handle events efficiently.
 * - Can be tied to a shared object to prevent the object from being destructed
 * while the Channel is active.
 */
class Channel : Noncopyable {
public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Timestamp)>;

  /**
   * @brief Constructs a Channel with the given EventLoop and file descriptor.
   *
   * @param loop The EventLoop that this Channel belongs to.
   * @param fd The file descriptor that this Channel will manage.
   */
  Channel(EventLoop *loop, int fd);
  ~Channel();

  /**
   * @brief Handles the event. Called by the EventLoop when an event occurs.
   *
   * @param receiveTime The timestamp when the event was received.
   */
  void handleEvent(Timestamp receiveTime);
  void setReadCallback(ReadEventCallback cb) { read_callback_ = std::move(cb); }
  void setWriteCallback(EventCallback cb) { write_callback_ = std::move(cb); }
  void setCloseCallback(EventCallback cb) { close_callback_ = std::move(cb); }
  void setErrorCallback(EventCallback cb) { error_callback_ = std::move(cb); }

  /**
   * @brief Ties this Channel to a shared object to prevent the object from
   * being destructed.
   *
   * @param obj The shared object to tie to this Channel.
   */
  void tie(const std::shared_ptr<void> &obj);

  int fd() const { return fd_; }
  int events() const { return events_; }
  void setRevents(int revt) { revents_ = revt; }
  bool isNoneEvent() const { return events_ == K_NONE_EVENT; }

  void enableReading() {
    events_ |= K_READ_EVENT;
    update();
  }
  void disableReading() {
    events_ &= ~K_READ_EVENT;
    update();
  }
  void enableWriting() {
    events_ |= K_WRITE_EVENT;
    update();
  }
  void disableWriting() {
    events_ &= ~K_WRITE_EVENT;
    update();
  }
  void disableAll() {
    events_ = K_NONE_EVENT;
    update();
  }
  bool isWriting() const { return (events_ & K_WRITE_EVENT) != 0; }
  bool isReading() const { return (events_ & K_READ_EVENT) != 0; }

  int index() { return index_; }
  void setIndex(int idx) { index_ = idx; }

  std::string reventsToString() const;
  std::string eventsToString() const;

  void doNotLogHup() { log_hup_ = false; }
  EventLoop *ownerLoop() { return loop_; }

  /// Removes this channel from the EventLoop.
  void remove();

private:
  static std::string eventsToString(int fd, int ev);

  // Update the channel's status in the epoller
  void update();
  void handleEventWithGuard(Timestamp receiveTime);

  static const int K_NONE_EVENT;
  static const int K_READ_EVENT;
  static const int K_WRITE_EVENT;

  EventLoop *loop_; // Pointer to the EventLoop this channel belongs to
  const int fd_;    // File descriptor associated with the channel
  int events_;      // Events that the channel is interested in
  int revents_;     // Events that are returned after poll
  int index_;       // Used by Epoller
  bool log_hup_;    // Flag to control logging of HUP event

  std::weak_ptr<void> tie_; // Weak pointer to tie the channel to an object
  bool tied_;
  bool event_handling_;
  bool added_to_loop_;
  ReadEventCallback read_callback_;
  EventCallback write_callback_;
  EventCallback close_callback_;
  EventCallback error_callback_;
};

} // namespace lynx

#endif
