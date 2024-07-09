#include "lynx/net/channel.h"
#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"

#include <poll.h>
#include <sstream>

namespace lynx {

const int Channel::K_NONE_EVENT = 0;
const int Channel::K_READ_EVENT = POLLIN | POLLPRI;
const int Channel::K_WRITE_EVENT = POLLOUT;

Channel::Channel(EventLoop *loop, int fd__)
    : loop_(loop), fd_(fd__), events_(0), revents_(0), index_(-1),
      log_hup_(true), tied_(false), event_handling_(false),
      added_to_loop_(false) {}

Channel::~Channel() {
  assert(!event_handling_);
  assert(!added_to_loop_);
  if (loop_->isInLoopThread()) {
    assert(!loop_->hasChannel(this));
  }
}

void Channel::tie(const std::shared_ptr<void> &obj) {
  tie_ = obj;
  tied_ = true;
}

void Channel::update() {
  added_to_loop_ = true;
  loop_->updateChannel(this);
}

void Channel::remove() {
  assert(isNoneEvent());
  added_to_loop_ = false;
  loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime) {
  std::shared_ptr<void> guard;
  if (tied_) {
    guard = tie_.lock();
    if (guard) {
      handleEventWithGuard(receiveTime);
    }
  } else {
    handleEventWithGuard(receiveTime);
  }
}

void Channel::handleEventWithGuard(Timestamp receiveTime) {
  event_handling_ = true;
  LOG_TRACE << reventsToString();
  if (((revents_ & POLLHUP) != 0) && ((revents_ & POLLIN) == 0)) {
    if (log_hup_) {
      LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
    }
    if (close_callback_) {
      close_callback_();
    }
  }

  if ((revents_ & POLLNVAL) != 0) {
    LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
  }

  if ((revents_ & (POLLERR | POLLNVAL)) != 0) {
    if (error_callback_) {
      error_callback_();
    }
  }
  if ((revents_ & (POLLIN | POLLPRI | POLLRDHUP)) != 0) {
    if (read_callback_) {
      read_callback_(receiveTime);
    }
  }
  if ((revents_ & POLLOUT) != 0) {
    if (write_callback_) {
      write_callback_();
    }
  }
  event_handling_ = false;
}

std::string Channel::reventsToString() const {
  return eventsToString(fd_, revents_);
}

std::string Channel::eventsToString() const {
  return eventsToString(fd_, events_);
}

std::string Channel::eventsToString(int fd, int ev) {
  std::ostringstream oss;
  oss << fd << ": ";
  if ((ev & POLLIN) != 0) {
    oss << "IN ";
  }
  if ((ev & POLLPRI) != 0) {
    oss << "PRI ";
  }
  if ((ev & POLLOUT) != 0) {
    oss << "OUT ";
  }
  if ((ev & POLLHUP) != 0) {
    oss << "HUP ";
  }
  if ((ev & POLLRDHUP) != 0) {
    oss << "RDHUP ";
  }
  if ((ev & POLLERR) != 0) {
    oss << "ERR ";
  }
  if ((ev & POLLNVAL) != 0) {
    oss << "NVAL ";
  }

  return oss.str();
}

} // namespace lynx
