#include "lynx/base/thread.h"
#include "lynx/base/current_thread.h"

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
  /// Ensure that the thread has not already been started
  assert(!started_);
  /// Set the started flag to true to indicate that the thread has started
  started_ = true;

  try {
    /// Create a new thread and run the provided function in it
    thread_ = std::make_shared<std::thread>([&] {
      /// Get the current thread ID
      tid_ = current_thread::tid();
      /// Decrement the latch count to indicate that the thread has started
      latch_.count_down();
      /// Set the thread name for the current thread
      lynx::current_thread::t_thread_name =
          name_.empty() ? "lynxThread" : name_.c_str();
      /// Set the process name using prctl(2)
      ::prctl(PR_SET_NAME, lynx::current_thread::t_thread_name);

      /// Run the provided function
      func_();
    });

    /// Set the thread name to "finished" to indicate that the thread has
    /// completed execution
    current_thread::t_thread_name = "finished";
  } catch (const std::exception &ex) {
    /// Set the thread name to "crashed" to indicate that an exception occurred
    /// during thread creation or execution
    current_thread::t_thread_name = "crashed";
    /// Print the exception details to stderr
    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    /// Terminate the program abnormally to indicate the exception
    abort();
  } catch (...) {
    /// Set the thread name to "crashed" to indicate that an unknown exception
    /// occurred during thread creation or execution
    current_thread::t_thread_name = "crashed";
    /// Print the exception details to stderr
    fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
    throw;
  }
  /// Wait for the latch to be decremented to indicate that the thread has
  /// started
  latch_.wait();
  /// Ensure that the thread ID is greater than 0 to indicate that the thread
  /// has successfully started
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
