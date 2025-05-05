#ifndef LYNX_NET_CHANNEL_H
#define LYNX_NET_CHANNEL_H

#include "lynx/base/Timestamp.h"

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
class Channel {
public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Timestamp)>;

  /**
   * @brief Constructs a Channel with the given EventLoop and file descriptor.
   *
   * @param loop The EventLoop that this Channel belongs to.
   * @param fd The file descriptor that this Channel will manage.
   */
  Channel(EventLoop *Loop, int Fd);
  ~Channel();

  /**
   * @brief Handles the event. Called by the EventLoop when an event occurs.
   *
   * @param receiveTime The timestamp when the event was received.
   */
  void handleEvent(Timestamp ReceiveTime);
  void setReadCallback(ReadEventCallback Cb) { ReadCallback = std::move(Cb); }
  void setWriteCallback(EventCallback Cb) { WriteCallback = std::move(Cb); }
  void setCloseCallback(EventCallback Cb) { CloseCallback = std::move(Cb); }
  void setErrorCallback(EventCallback Cb) { ErrorCallback = std::move(Cb); }

  /**
   * @brief Ties this Channel to a shared object to prevent the object from
   * being destructed.
   *
   * @param obj The shared object to tie to this Channel.
   */
  void tie(const std::shared_ptr<void> &Obj);

  int fd() const { return Fd; }
  int events() const { return Events; }
  void setRevents(int Revt) { Revents = Revt; }
  bool isNoneEvent() const { return Events == KNoneEvent; }

  void enableReading() {
    Events |= KReadEvent;
    update();
  }
  void disableReading() {
    Events &= ~KReadEvent;
    update();
  }
  void enableWriting() {
    Events |= KWriteEvent;
    update();
  }
  void disableWriting() {
    Events &= ~KWriteEvent;
    update();
  }
  void disableAll() {
    Events = KNoneEvent;
    update();
  }
  bool isWriting() const { return (Events & KWriteEvent) != 0; }
  bool isReading() const { return (Events & KReadEvent) != 0; }

  int index() { return Index; }
  void setIndex(int Idx) { Index = Idx; }

  std::string reventsToString() const;
  std::string eventsToString() const;

  void doNotLogHup() { LogHup = false; }
  EventLoop *ownerLoop() { return Loop; }

  /// Removes this channel from the EventLoop.
  void remove();

private:
  static std::string eventsToString(int Fd, int Ev);

  // Update the channel's status in the epoller
  void update();
  void handleEventWithGuard(Timestamp ReceiveTime);

  static const int KNoneEvent;
  static const int KReadEvent;
  static const int KWriteEvent;

  EventLoop *Loop; // Pointer to the EventLoop this channel belongs to
  const int Fd;    // File descriptor associated with the channel
  int Events;      // Events that the channel is interested in
  int Revents;     // Events that are returned after poll
  int Index;       // Used by Epoller
  bool LogHup;    // Flag to control logging of HUP event

  std::weak_ptr<void> Tie; // Weak pointer to tie the channel to an object
  bool Tied;
  bool EventHandling;
  bool AddedToLoop;
  ReadEventCallback ReadCallback;
  EventCallback WriteCallback;
  EventCallback CloseCallback;
  EventCallback ErrorCallback;
};

} // namespace lynx

#endif
