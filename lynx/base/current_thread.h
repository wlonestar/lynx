#ifndef LYNX_BASE_CURRENT_THREAD_H
#define LYNX_BASE_CURRENT_THREAD_H

#include <string>

namespace lynx::current_thread {

extern thread_local int t_cached_tid;
extern thread_local char t_tid_string[32];
extern thread_local int t_tid_string_length;
extern thread_local const char *t_thread_name;
extern thread_local char t_errnobuf[512];

void cachedTid();

inline int tid() {
  if (__builtin_expect(static_cast<long>(t_cached_tid == 0), 0) != 0) {
    cachedTid();
  }
  return t_cached_tid;
}

inline const char *tidString() { return t_tid_string; }
inline int tidStringLength() { return t_tid_string_length; }
inline const char *name() { return t_thread_name; }

const char *strError(int savedErrno);

} // namespace lynx::current_thread

#endif
