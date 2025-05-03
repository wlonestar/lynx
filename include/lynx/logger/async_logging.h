#ifndef LYNX_LOGGER_ASYNC_LOGGING_H
#define LYNX_LOGGER_ASYNC_LOGGING_H

#include "lynx/base/thread.h"
#include "lynx/logger/log_stream.h"

#include <condition_variable>

namespace lynx {

/**
 * @class AsyncLogging
 * @brief A thread-safe asynchronous logging class that allows logging messages
 * to be written to a file or multiple files without blocking the calling
 * thread.
 *
 * It uses a background thread to perform the actual writing to disk, ensuring
 * that the logging operations is non-blocking.
 */
class AsyncLogging : Noncopyable {
public:
  /**
   * @brief Constructs an AsyncLogging object.
   *
   * @param basename The base name of the log file.
   * @param rollSize The maximum size of a log file before rolling.
   * @param flushInterval The interval (in seconds) at which the background
   * thread should check if it needs to flush buffers to disk. Default is 3.
   */
  AsyncLogging(const std::string &basename, off_t rollSize,
               int flushInterval = 3);

  /**
   * @brief Destructs the AsyncLogging object and stops the background thread.
   */
  ~AsyncLogging();

  /**
   * @brief Appends a log line to the current buffer.
   *
   * If the current buffer has enough space, the log line is appended to it.
   * If the current buffer is full, the current buffer is moved to the list of
   * filled buffers. A new buffer is created if necessary. The log line is
   * appended to the new buffer. The function signals the condition variable to
   * notify the background thread that there is a new buffer ready to be
   * flushed.
   *
   * @param logline The log line to be appended.
   * @param len The length of the log line.
   */
  void append(const char *logline, size_t len);

  /**
   * @brief Starts the background thread if it's not already running.
   */
  void start();

  /**
   * @brief Stops the background thread and waits for it to finish.
   */
  void stop();

private:
  /**
   * @brief The function that runs in the background thread.
   *
   * This function runs in the background thread and continuously checks if
   * there are any buffers to be written to the log file. If there are, it
   * appends the log messages to the log file and swaps out the buffers.
   */
  void threadFunc();

  using Buffer = detail::FixedBuffer<detail::K_LARGE_BUFFER>;
  using BufferVector = std::vector<std::unique_ptr<Buffer>>;
  using BufferPtr = BufferVector::value_type;

  const int flush_interval_;   /// Interval (in seconds) at which the background
                               /// thread should check if it needs to flush
                               /// buffers to disk.
  std::atomic_bool running_;   /// Flag indicating whether the background thread
                               /// is running.
  const std::string basename_; /// The base name of the log file.
  const off_t roll_size_; /// The maximum size of a log file before rolling.

  Thread thread_; /// The background thread for flushing buffers to disk.
  std::latch latch_;
  mutable std::mutex mutex_;     /// Mutex for thread-safe access.
  std::condition_variable cond_; /// Used to notify the background thread when
                                 /// a new buffer is ready to be flushed.
  BufferPtr current_buffer_;     /// The current buffer.
  BufferPtr next_buffer_;        /// The preparing buffer.
  BufferVector buffers_; /// Filled buffers waiting for writing into file.
};

} // namespace lynx

#endif
