#ifndef LYNX_BASE_CURRENT_THREAD_H
#define LYNX_BASE_CURRENT_THREAD_H

namespace lynx::current_thread {

/* variables which are stored in every thread */
extern thread_local int t_cached_tid;          // cached thread id
extern thread_local char t_tid_string[32];     // tid in string
extern thread_local int t_tid_string_length;   // tid string length
extern thread_local const char *t_thread_name; // thread name
extern thread_local char t_errnobuf[512];      // describes the error code

/// use `::syscall(SYS_gettid)` to get current thread id
void cachedTid();
/// returns a string that describes the error code passed in the argument errnum
const char *strError(int errnum);

/// run `cachedTid()` when this thread first call `tid()`
inline int tid() {
  if (__builtin_expect(static_cast<long>(t_cached_tid == 0), 0) != 0) {
    cachedTid();
  }
  return t_cached_tid;
}

/* wrap thread local variable into function */
inline const char *tidString() { return t_tid_string; }
inline int tidStringLength() { return t_tid_string_length; }
inline const char *name() { return t_thread_name; }

} // namespace lynx::current_thread

#endif
