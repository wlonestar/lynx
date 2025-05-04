#ifndef LYNX_BASE_CURRENT_THREAD_H
#define LYNX_BASE_CURRENT_THREAD_H

namespace lynx::current_thread {

/// Thread-local variables for thread id and error handling.
extern thread_local int CachedTid;          // cached thread id
extern thread_local char TidString[32];     // tid in string
extern thread_local int TidStringLength;    // tid string length
extern thread_local const char *ThreadName; // thread name
extern thread_local char ErrnoBuf[512];     // describes the error code

/**
 * @brief This function caches the thread ID if it hasn't been cached yet.
 *
 * This function first checks if the thread ID has already been cached. If not,
 * it retrieves the thread ID using the `SYS_gettid` system call and stores it
 * in the `CachedTid` variable. It then formats the thread ID into a string
 * and stores it in the `TidString` variable. The length of the thread ID
 * string is also stored in the `TidStringLength` variable.
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
 * a thread-local buffer `ErrnoBuf`. The function returns a pointer to the
 * error message.
 */
const char *strError(int Errnum);

/// run `cachedTid()` when this thread first call `tid()`
inline int tid() {
  if (__builtin_expect(static_cast<long>(CachedTid == 0), 0) != 0)
    cachedTid();
  return CachedTid;
}

/// wrap thread local variable into function
inline const char *tidString() { return TidString; }
inline int tidStringLength() { return TidStringLength; }
inline const char *name() { return ThreadName; }

} // namespace lynx::current_thread

#endif
