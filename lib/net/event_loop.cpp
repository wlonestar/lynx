#include "lynx/net/event_loop.h"
#include "lynx/logger/logging.h"
#include "lynx/net/channel.h"
#include "lynx/net/epoller.h"
#include "lynx/net/sockets_ops.h"
#include "lynx/net/timer_queue.h"

#include <algorithm>
#include <cassert>
#include <csignal>
#include <sys/eventfd.h>
#include <unistd.h>

namespace lynx {

namespace {

thread_local EventLoop *t_loop_in_this_thread = nullptr;

const int K_POLL_TIME_MS = 10000;

int createEventfd() {
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    LOG_SYSERR << "Failed in eventfd";
    abort();
  }
  return evtfd;
}

#pragma GCC diagnostic ignored "-Wold-style-cast"
class IgnoreSigPipe {
public:
  IgnoreSigPipe() { ::signal(SIGPIPE, SIG_IGN); }
};
#pragma GCC diagnostic error "-Wold-style-cast"

IgnoreSigPipe init_obj;

} // namespace

EventLoop *EventLoop::getEventLoopOfCurrentThread() {
  return t_loop_in_this_thread;
}

EventLoop::EventLoop()
    : looping_(false), quit_(false), event_handling_(false),
      calling_pending_functors_(false), iteration_(0),
      thread_id_(current_thread::tid()), poller_(new Epoller(this)),
      timer_queue_(new TimerQueue(this)), wakeup_fd_(createEventfd()),
      wakeup_channel_(new Channel(this, wakeup_fd_)),
      current_active_channel_(nullptr) {
  LOG_DEBUG << "EventLoop created " << this << " in thread " << thread_id_;
  if (t_loop_in_this_thread != nullptr) {
    LOG_FATAL << "Another EventLoop " << t_loop_in_this_thread
              << " exists in this thread " << thread_id_;
  } else {
    t_loop_in_this_thread = this;
  }
  wakeup_channel_->setReadCallback([this](auto && /*PH1*/) { handleRead(); });
  wakeup_channel_->enableReading();
}

EventLoop::~EventLoop() {
  LOG_DEBUG << "EventLoop " << this << " of thread " << thread_id_
            << " destructs in thread " << current_thread::tid();
  wakeup_channel_->disableAll();
  wakeup_channel_->remove();
  ::close(wakeup_fd_);
  t_loop_in_this_thread = nullptr;
}

void EventLoop::loop() {
  assert(!looping_);
  assertInLoopThread();
  looping_ = true;
  quit_ = false;
  LOG_TRACE << "EventLoop " << this << " start looping";

  while (!quit_) {
    active_channels_.clear();
    poll_return_time_ = poller_->poll(K_POLL_TIME_MS, &active_channels_);
    ++iteration_;
    if (Logger::logLevel() <= TRACE) {
      printActiveChannels();
    }
    event_handling_ = true;
    for (Channel *channel : active_channels_) {
      current_active_channel_ = channel;
      current_active_channel_->handleEvent(poll_return_time_);
    }
    current_active_channel_ = nullptr;
    event_handling_ = false;
    doPendingFunctors();
  }

  LOG_TRACE << "EventLoop " << this << " stop looping";
  looping_ = false;
}

void EventLoop::quit() {
  quit_ = true;
  if (!isInLoopThread()) {
    wakeup();
  }
}

void EventLoop::runInLoop(Functor cb) {
  if (isInLoopThread()) {
    cb();
  } else {
    queueInLoop(std::move(cb));
  }
}

void EventLoop::queueInLoop(Functor cb) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_functors_.push_back(std::move(cb));
  }

  if (!isInLoopThread() || calling_pending_functors_) {
    wakeup();
  }
}

size_t EventLoop::queueSize() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return pending_functors_.size();
}

TimerId EventLoop::runAt(Timestamp time, TimerCallback cb) {
  return timer_queue_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback cb) {
  Timestamp time(addTime(Timestamp::now(), delay));
  return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double interval, TimerCallback cb) {
  Timestamp time(addTime(Timestamp::now(), interval));
  return timer_queue_->addTimer(std::move(cb), time, interval);
}

void EventLoop::cancel(TimerId timerId) {
  return timer_queue_->cancel(timerId);
}

void EventLoop::updateChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  if (event_handling_) {
    assert(current_active_channel_ == channel ||
           std::find(active_channels_.begin(), active_channels_.end(),
                     channel) == active_channels_.end());
  }
  poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  return poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread() {
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << thread_id_
            << ", current thread id = " << current_thread::tid();
}

void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = sockets::write(wakeup_fd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

void EventLoop::handleRead() {
  uint64_t one = 1;
  ssize_t n = sockets::read(wakeup_fd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  calling_pending_functors_ = true;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    functors.swap(pending_functors_);
  }

  for (const Functor &functor : functors) {
    functor();
  }
  calling_pending_functors_ = false;
}

void EventLoop::printActiveChannels() const {
  for (const Channel *channel : active_channels_) {
    LOG_TRACE << "{" << channel->reventsToString() << "} ";
  }
}

} // namespace lynx
