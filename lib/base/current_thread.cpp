#include "lynx/base/current_thread.h"
#include "lynx/base/timestamp.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cxxabi.h>
#include <execinfo.h>
#include <sys/syscall.h>
#include <type_traits>
#include <unistd.h>

namespace lynx::current_thread {

thread_local int t_cached_tid = 0;
thread_local char t_tid_string[32];
thread_local int t_tid_string_length = 6;
thread_local const char *t_thread_name = "unknown";

static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");

void cachedTid() {
  if (t_cached_tid == 0) {
    t_cached_tid = static_cast<pid_t>(::syscall(SYS_gettid));
    t_tid_string_length =
        snprintf(t_tid_string, sizeof(t_tid_string), "%5d ", t_cached_tid);
  }
}

bool isMainThread() { return tid() == ::getpid(); }

void sleepUsec(int64_t usec) {
  struct timespec ts = {0, 0};
  ts.tv_sec = static_cast<time_t>(usec / Timestamp::K_MICRO_SECONDS_PER_SECOND);
  ts.tv_nsec =
      static_cast<long>(usec % Timestamp::K_MICRO_SECONDS_PER_SECOND * 1000);
  ::nanosleep(&ts, nullptr);
}

std::string stackTrace(bool demangle) {
  std::string stack;
  const int max_frames = 200;
  void *frame[max_frames];
  int nptrs = ::backtrace(frame, max_frames);
  char **strings = ::backtrace_symbols(frame, nptrs);
  if (strings != nullptr) {
    size_t len = 256;
    char *demangled = demangle ? static_cast<char *>(::malloc(len)) : nullptr;
    for (int i = 1; i < nptrs; ++i) // skipping the 0-th, which is this function
    {
      if (demangle) {
        // https://panthema.net/2008/0901-stacktrace-demangled/
        // bin/exception_test(_ZN3Bar4testEv+0x79) [0x401909]
        char *left_par = nullptr;
        char *plus = nullptr;
        for (char *p = strings[i]; *p != 0; ++p) {
          if (*p == '(') {
            left_par = p;
          } else if (*p == '+') {
            plus = p;
          }
        }

        if ((left_par != nullptr) && (plus != nullptr)) {
          *plus = '\0';
          int status = 0;
          char *ret =
              abi::__cxa_demangle(left_par + 1, demangled, &len, &status);
          *plus = '+';
          if (status == 0) {
            demangled = ret; // ret could be realloc()
            stack.append(strings[i], left_par + 1);
            stack.append(demangled);
            stack.append(plus);
            stack.push_back('\n');
            continue;
          }
        }
      }
      // Fallback to mangled names
      stack.append(strings[i]);
      stack.push_back('\n');
    }
    free(demangled);
    free(strings);
  }
  return stack;
}

} // namespace lynx::current_thread
