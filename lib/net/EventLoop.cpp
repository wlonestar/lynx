#include "lynx/net/EventLoop.h"
#include "lynx/logger/Logging.h"
#include "lynx/net/Channel.h"
#include "lynx/net/Epoller.h"

#include "lynx/net/TimerQueue.h"

#include <cassert>
#include <csignal>
#include <sys/eventfd.h>

namespace lynx {

namespace {

thread_local EventLoop *TLoopInThisThread = nullptr;

const int KPollTimeMs = 10000;

int createEventfd() {
  int Evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (Evtfd < 0) {
    LOG_SYSERR << "Failed in eventfd";
    abort();
  }
  return Evtfd;
}

class IgnoreSigPipe {
public:
  IgnoreSigPipe() { ::signal(SIGPIPE, SIG_IGN); }
};
IgnoreSigPipe InitObj;

} // namespace

EventLoop *EventLoop::getEventLoopOfCurrentThread() {
  return TLoopInThisThread;
}

EventLoop::EventLoop()
    : Looping(false), Quit(false), EventHandling(false),
      CallingPendingFunctors(false), ThreadId(current_thread::tid()),
      Poller(new Epoller(this)), Queue(new TimerQueue(this)),
      WakeupFd(createEventfd()), WakeupChannel(new Channel(this, WakeupFd)),
      CurrentActiveChannel(nullptr) {
  LOG_DEBUG << "EventLoop created " << this << " in thread " << ThreadId;
  if (TLoopInThisThread != nullptr) {
    LOG_FATAL << "Another EventLoop " << TLoopInThisThread
              << " exists in this thread " << ThreadId;
  } else {
    TLoopInThisThread = this;
  }
  WakeupChannel->setReadCallback([this](auto && /*PH1*/) { handleRead(); });
  WakeupChannel->enableReading();
}

EventLoop::~EventLoop() {
  LOG_DEBUG << "EventLoop " << this << " of thread " << ThreadId
            << " destructs in thread " << current_thread::tid();
  WakeupChannel->disableAll();
  WakeupChannel->remove();
  ::close(WakeupFd);
  TLoopInThisThread = nullptr;
}

void EventLoop::loop() {
  assert(!Looping);
  assertInLoopThread();
  Looping = true;
  Quit = false;
  LOG_TRACE << "EventLoop " << this << " start looping";

  while (!Quit) {
    ActiveChannels.clear();
    PollReturnTime = Poller->poll(KPollTimeMs, &ActiveChannels);
    if (Logger::logLevel() <= Logger::TRACE)
      printActiveChannels();

    EventHandling = true;
    for (Channel *Channel : ActiveChannels) {
      CurrentActiveChannel = Channel;
      CurrentActiveChannel->handleEvent(PollReturnTime);
    }
    CurrentActiveChannel = nullptr;
    EventHandling = false;
    doPendingFunctors();
  }

  LOG_TRACE << "EventLoop " << this << " stop looping";
  Looping = false;
}

void EventLoop::quit() {
  Quit = true;
  if (!isInLoopThread())
    wakeup();
}

void EventLoop::runInLoop(Functor Cb) {
  if (isInLoopThread()) {
    Cb();
  } else {
    queueInLoop(std::move(Cb));
  }
}

void EventLoop::queueInLoop(Functor Cb) {
  {
    std::lock_guard<std::mutex> Lock(Mutex);
    PendingFunctors.push_back(std::move(Cb));
  }

  if (!isInLoopThread() || CallingPendingFunctors)
    wakeup();
}

size_t EventLoop::queueSize() const {
  std::lock_guard<std::mutex> Lock(Mutex);
  return PendingFunctors.size();
}

TimerId EventLoop::runAt(Timestamp Time, TimerCallback Cb) {
  return Queue->addTimer(std::move(Cb), Time, 0.0);
}

TimerId EventLoop::runAfter(double Delay, TimerCallback Cb) {
  Timestamp Time(addTime(Timestamp::now(), Delay));
  return runAt(Time, std::move(Cb));
}

TimerId EventLoop::runEvery(double Interval, TimerCallback Cb) {
  Timestamp Time(addTime(Timestamp::now(), Interval));
  return Queue->addTimer(std::move(Cb), Time, Interval);
}

void EventLoop::cancel(TimerId TimerId) { return Queue->cancel(TimerId); }

void EventLoop::updateChannel(Channel *Channel) {
  assert(Channel->ownerLoop() == this);
  assertInLoopThread();
  Poller->updateChannel(Channel);
}

void EventLoop::removeChannel(Channel *Channel) {
  assert(Channel->ownerLoop() == this);
  assertInLoopThread();
  if (EventHandling)
    assert(CurrentActiveChannel == Channel ||
           std::find(ActiveChannels.begin(), ActiveChannels.end(), Channel) ==
               ActiveChannels.end());

  Poller->removeChannel(Channel);
}

bool EventLoop::hasChannel(Channel *Channel) {
  assert(Channel->ownerLoop() == this);
  assertInLoopThread();
  return Poller->hasChannel(Channel);
}

void EventLoop::abortNotInLoopThread() {
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << ThreadId
            << ", current thread id = " << current_thread::tid();
}

void EventLoop::wakeup() {
  uint64_t One = 1;
  ssize_t N = ::write(WakeupFd, &One, sizeof(One));
  if (N != sizeof(One))
    LOG_ERROR << "EventLoop::wakeup() writes " << N << " bytes instead of 8";
}

void EventLoop::handleRead() {
  uint64_t One = 1;
  ssize_t N = ::read(WakeupFd, &One, sizeof(One));
  if (N != sizeof(One))
    LOG_ERROR << "EventLoop::handleRead() reads " << N << " bytes instead of 8";
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> Functors;
  CallingPendingFunctors = true;

  {
    std::lock_guard<std::mutex> Lock(Mutex);
    Functors.swap(PendingFunctors);
  }

  for (const Functor &Functor : Functors)
    Functor();

  CallingPendingFunctors = false;
}

void EventLoop::printActiveChannels() const {
  for (const Channel *Channel : ActiveChannels)
    LOG_TRACE << "{" << Channel->reventsToString() << "} ";
}

} // namespace lynx
