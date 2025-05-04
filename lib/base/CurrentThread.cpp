#include "lynx/base/CurrentThread.h"

#include <cstring>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>

namespace lynx {

namespace current_thread {

thread_local int CachedTid = 0;
thread_local char TidString[32];
thread_local int TidStringLength = 6;
thread_local const char *ThreadName = "unknown";
thread_local char ErrnoBuf[512];

void cachedTid() {
  /// Check if the thread ID has already been cached
  if (CachedTid == 0) {
    /// Retrieve the thread ID using the `SYS_gettid` system call
    CachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
    /// Format the thread ID into a string and store it in `TidString`
    TidStringLength = snprintf(TidString, sizeof(TidString), "%5d ", CachedTid);
  }
}

const char *strError(int Errnum) {
  return strerror_r(Errnum, ErrnoBuf, sizeof(ErrnoBuf));
}

} // namespace current_thread

} // namespace lynx
