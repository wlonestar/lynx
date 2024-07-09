#ifndef LYNX_NET_CHANNEL_H
#define LYNX_NET_CHANNEL_H

#include "lynx/base/noncopyable.h"
#include "lynx/base/timestamp.h"

#include <functional>
#include <memory>

namespace lynx {

class EventLoop;

class Channel : Noncopyable {
public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Timestamp)>;

  Channel(EventLoop *loop, int fd);
  ~Channel();

  void handleEvent(Timestamp receiveTime);
  void setReadCallback(ReadEventCallback cb) { read_callback_ = std::move(cb); }
  void setWriteCallback(EventCallback cb) { write_callback_ = std::move(cb); }
  void setCloseCallback(EventCallback cb) { close_callback_ = std::move(cb); }
  void setErrorCallback(EventCallback cb) { error_callback_ = std::move(cb); }

  void tie(const std::shared_ptr<void> &);

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
  void remove();

private:
  static std::string eventsToString(int fd, int ev);

  void update();
  void handleEventWithGuard(Timestamp receiveTime);

  static const int K_NONE_EVENT;
  static const int K_READ_EVENT;
  static const int K_WRITE_EVENT;

  EventLoop *loop_;
  const int fd_;
  int events_;
  int revents_;
  int index_;
  bool log_hup_;

  std::weak_ptr<void> tie_;
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
