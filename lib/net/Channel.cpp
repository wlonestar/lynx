#include "lynx/net/Channel.h"
#include "lynx/logger/Logging.h"
#include "lynx/net/EventLoop.h"

#include <cassert>
#include <poll.h>
#include <sstream>

namespace lynx {

const int Channel::KNoneEvent = 0;
const int Channel::KReadEvent = POLLIN | POLLPRI;
const int Channel::KWriteEvent = POLLOUT;

Channel::Channel(EventLoop *Loop, int Fd)
    : Loop(Loop), Fd(Fd), Events(0), Revents(0), Index(-1), LogHup(true),
      Tied(false), EventHandling(false), AddedToLoop(false) {}

Channel::~Channel() {
  assert(!EventHandling);
  assert(!AddedToLoop);
  if (Loop->isInLoopThread())
    assert(!Loop->hasChannel(this));
}

void Channel::tie(const std::shared_ptr<void> &Obj) {
  Tie = Obj;
  Tied = true;
}

void Channel::update() {
  AddedToLoop = true;
  Loop->updateChannel(this);
}

void Channel::remove() {
  assert(isNoneEvent());
  AddedToLoop = false;
  Loop->removeChannel(this);
}

void Channel::handleEvent(Timestamp ReceiveTime) {
  std::shared_ptr<void> Guard;
  if (Tied) {
    Guard = Tie.lock();
    if (Guard)
      handleEventWithGuard(ReceiveTime);

  } else {
    handleEventWithGuard(ReceiveTime);
  }
}

void Channel::handleEventWithGuard(Timestamp ReceiveTime) {
  EventHandling = true;
  LOG_TRACE << reventsToString();
  if (((Revents & POLLHUP) != 0) && ((Revents & POLLIN) == 0)) {
    if (LogHup)
      LOG_WARN << "fd = " << Fd << " Channel::handle_event() POLLHUP";

    if (CloseCallback)
      CloseCallback();
  }

  if ((Revents & POLLNVAL) != 0)
    LOG_WARN << "fd = " << Fd << " Channel::handle_event() POLLNVAL";

  if ((Revents & (POLLERR | POLLNVAL)) != 0) {
    if (ErrorCallback)
      ErrorCallback();
  }
  if ((Revents & (POLLIN | POLLPRI | POLLRDHUP)) != 0) {
    if (ReadCallback)
      ReadCallback(ReceiveTime);
  }
  if ((Revents & POLLOUT) != 0) {
    if (WriteCallback)
      WriteCallback();
  }
  EventHandling = false;
}

std::string Channel::reventsToString() const {
  return eventsToString(Fd, Revents);
}

std::string Channel::eventsToString() const {
  return eventsToString(Fd, Events);
}

std::string Channel::eventsToString(int Fd, int Ev) {
  std::ostringstream OSS;
  OSS << Fd << ": ";
  if ((Ev & POLLIN) != 0)
    OSS << "IN ";

  if ((Ev & POLLPRI) != 0)
    OSS << "PRI ";

  if ((Ev & POLLOUT) != 0)
    OSS << "OUT ";

  if ((Ev & POLLHUP) != 0)
    OSS << "HUP ";

  if ((Ev & POLLRDHUP) != 0)
    OSS << "RDHUP ";

  if ((Ev & POLLERR) != 0)
    OSS << "ERR ";

  if ((Ev & POLLNVAL) != 0)
    OSS << "NVAL ";

  return OSS.str();
}

} // namespace lynx
