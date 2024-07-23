#include "lynx/base/thread.h"
#include "lynx/base/current_thread.h"
#include "lynx/logger/logging.h"

#include <cassert>
#include <sys/prctl.h>

namespace lynx {

std::atomic_int32_t Thread::num_created;

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false), joined_(false), tid_(0), func_(std::move(func)),
      name_(name), latch_(1) {
  setDefaultName();
}

Thread::~Thread() {
  if (started_ && !joined_) {
    thread_->detach();
  }
}

void Thread::start() {
  assert(!started_);
  started_ = true;

  try {
    thread_ = std::make_shared<std::thread>([&] {
      tid_ = current_thread::tid();
      latch_.count_down();
      lynx::current_thread::t_thread_name =
          name_.empty() ? "lynxThread" : name_.c_str();
      ::prctl(PR_SET_NAME, lynx::current_thread::t_thread_name);

      func_();
    });
    current_thread::t_thread_name = "finished";
  } catch (const std::exception &ex) {
    current_thread::t_thread_name = "crashed";
    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    abort();
  } catch (...) {
    current_thread::t_thread_name = "crashed";
    fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
    throw;
  }
  latch_.wait();
  assert(tid_ > 0);
}

void Thread::join() {
  assert(started_);
  assert(!joined_);
  joined_ = true;
  thread_->join();
}

void Thread::setDefaultName() {
  int num = num_created.fetch_add(1);
  if (name_.empty()) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Thread%d", num);
    name_ = buf;
  }
}

} // namespace lynx
