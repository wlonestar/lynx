#ifndef LYNX_BASE_CURRENT_THREAD_H
#define LYNX_BASE_CURRENT_THREAD_H

namespace lynx::current_thread {

/// Thread-local variables for thread id and error handling.
extern thread_local int t_cached_tid;          // cached thread id
extern thread_local char t_tid_string[32];     // tid in string
extern thread_local int t_tid_string_length;   // tid string length
extern thread_local const char *t_thread_name; // thread name
extern thread_local char t_errnobuf[512];      // describes the error code

/**
 * @brief This function caches the thread ID if it hasn't been cached yet.
 *
 * This function first checks if the thread ID has already been cached. If not,
 * it retrieves the thread ID using the `SYS_gettid` system call and stores it
 * in the `t_cached_tid` variable. It then formats the thread ID into a string
 * and stores it in the `t_tid_string` variable. The length of the thread ID
 * string is also stored in the `t_tid_string_length` variable.
 */
void cachedTid();

/**
 * @brief Retrieve the error message corresponding to the specified error
 * number.
 *
 * @param errnum The error number.
 * @return const char* A pointer to the error message.
 *
 * This function retrieves the error message corresponding to the specified
 * error number using the `strerror_r` function. The error message is stored in
 * a thread-local buffer `t_errnobuf`. The function returns a pointer to the
 * error message.
 */
const char *strError(int errnum);

/// run `cachedTid()` when this thread first call `tid()`
inline int tid() {
  if (__builtin_expect(static_cast<long>(t_cached_tid == 0), 0) != 0) {
    cachedTid();
  }
  return t_cached_tid;
}

/// wrap thread local variable into function
inline const char *tidString() { return t_tid_string; }
inline int tidStringLength() { return t_tid_string_length; }
inline const char *name() { return t_thread_name; }

} // namespace lynx::current_thread

#endif
