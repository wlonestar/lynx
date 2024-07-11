#include "lynx/base/thread.h"
#include "lynx/base/current_thread.h"
#include "lynx/base/exception.h"
#include "lynx/logger/logging.h"

#include <cassert>
#include <sys/prctl.h>

namespace lynx {

namespace detail {

pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

void afterFork() {
  current_thread::t_cached_tid = 0;
  current_thread::t_thread_name = "main";
  current_thread::tid();
}

class ThreadNameInitializer {
public:
  ThreadNameInitializer() {
    current_thread::t_thread_name = "main";
    current_thread::tid();
    pthread_atfork(nullptr, nullptr, &afterFork);
  }
};

ThreadNameInitializer init;

struct ThreadData {
  using ThreadFunc = lynx::Thread::ThreadFunc;
  ThreadFunc func_;
  std::string name_;
  pid_t *tid_;
  std::latch *latch_;

  ThreadData(ThreadFunc func, const std::string &name, pid_t *tid,
             std::latch *latch)
      : func_(std::move(func)), name_(name), tid_(tid), latch_(latch) {}

  void runInThread() {
    *tid_ = current_thread::tid();
    tid_ = nullptr;
    latch_->count_down();
    latch_ = nullptr;

    lynx::current_thread::t_thread_name =
        name_.empty() ? "lynxThread" : name_.c_str();
    ::prctl(PR_SET_NAME, lynx::current_thread::t_thread_name);
    try {
      func_();
      current_thread::t_thread_name = "finished";
    } catch (const Exception &ex) {
      current_thread::t_thread_name = "crashed";
      fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
      fprintf(stderr, "reason: %s\n", ex.what());
      fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
      abort();
    } catch (const std::exception &ex) {
      current_thread::t_thread_name = "crashed";
      fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
      fprintf(stderr, "reason: %s\n", ex.what());
      abort();
    } catch (...) {
      current_thread::t_thread_name = "crashed";
      fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
      throw; // rethrow
    }
  }
};

void *startThread(void *obj) {
  auto *data = static_cast<ThreadData *>(obj);
  data->runInThread();
  delete data;
  return nullptr;
}

} // namespace detail

std::atomic_int32_t Thread::num_created;

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false), joined_(false), pthread_id_(0), tid_(0),
      func_(std::move(func)), name_(name), latch_(1) {
  setDefaultName();
}

Thread::~Thread() {
  if (started_ && !joined_) {
    pthread_detach(pthread_id_);
  }
}

void Thread::start() {
  assert(!started_);
  started_ = true;
  auto *data = new detail::ThreadData(func_, name_, &tid_, &latch_);
  if (pthread_create(&pthread_id_, nullptr, &detail::startThread, data) != 0) {
    started_ = false;
    delete data; // or no delete?
    LOG_SYSFATAL << "Failed in pthread_create";
  } else {
    latch_.wait();
    assert(tid_ > 0);
  }
}

int Thread::join() {
  assert(started_);
  assert(!joined_);
  joined_ = true;
  return pthread_join(pthread_id_, nullptr);
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
