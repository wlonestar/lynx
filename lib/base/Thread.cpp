#include "lynx/base/Thread.h"
#include "lynx/base/CurrentThread.h"

#include <cassert>
#include <sys/prctl.h>

#include <fmt/format.h>

namespace lynx {

std::atomic_int32_t Thread::NumCreated;

Thread::Thread(ThreadFunc Func, const std::string &Name)
    : Started(false), Joined(false), Tid(0), Func(std::move(Func)), Name(Name),
      Latch(1) {
  setDefaultName();
}

Thread::~Thread() {
  if (Started && !Joined)
    Thred->detach();
}

void Thread::start() {
  assert(!Started);
  Started = true;

  try {
    /// Create a new thread and run the provided function in it
    Thred = std::make_shared<std::thread>([&] {
      Tid = current_thread::tid(); /// Get the current thread ID
      /// Decrement the latch count to indicate that the thread has started
      Latch.count_down();
      /// Set the thread name for the current thread
      lynx::current_thread::ThreadName =
          Name.empty() ? "lynxThread" : Name.c_str();
      /// Set the process name using prctl(2)
      ::prctl(PR_SET_NAME, lynx::current_thread::ThreadName);

      Func(); /// Run the provided function
    });

    /// Set the thread name to "finished" to indicate that the thread has
    /// completed execution
    current_thread::ThreadName = "finished";
  } catch (const std::exception &Ex) {
    /// Set the thread name to "crashed" to indicate that an exception occurred
    /// during thread creation or execution
    current_thread::ThreadName = "crashed";
    fprintf(stderr, "exception caught in Thread %s\n", Name.c_str());
    fprintf(stderr, "reason: %s\n", Ex.what());
    /// Terminate the program abnormally to indicate the exception
    abort();
  } catch (...) {
    /// Set the thread name to "crashed" to indicate that an unknown exception
    /// occurred during thread creation or execution
    current_thread::ThreadName = "crashed";
    fprintf(stderr, "unknown exception caught in Thread %s\n", Name.c_str());
    throw;
  }
  /// Wait for the latch to be decremented to indicate that the thread has
  /// started
  Latch.wait();
  assert(Tid > 0);
}

void Thread::join() {
  assert(Started);
  assert(!Joined);
  Joined = true;
  Thred->join();
}

void Thread::setDefaultName() {
  int Num = NumCreated.fetch_add(1);
  if (Name.empty())
    Name = fmt::format("Thread{}", Num);
}

} // namespace lynx
