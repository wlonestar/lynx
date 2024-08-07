#include "lynx/base/current_thread.h"

#include <cstring>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>

namespace lynx::current_thread {

thread_local int t_cached_tid = 0;
thread_local char t_tid_string[32];
thread_local int t_tid_string_length = 6;
thread_local const char *t_thread_name = "unknown";
thread_local char t_errnobuf[512];

void cachedTid() {
  if (t_cached_tid == 0) {
    t_cached_tid = static_cast<pid_t>(::syscall(SYS_gettid));
    t_tid_string_length =
        snprintf(t_tid_string, sizeof(t_tid_string), "%5d ", t_cached_tid);
  }
}

const char *strError(int errnum) {
  return strerror_r(errnum, t_errnobuf, sizeof(t_errnobuf));
}

} // namespace lynx::current_thread
