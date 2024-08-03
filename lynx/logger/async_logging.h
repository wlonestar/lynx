#ifndef LYNX_LOGGER_ASYNC_LOGGING_H
#define LYNX_LOGGER_ASYNC_LOGGING_H

#include "lynx/base/thread.h"
#include "lynx/logger/log_stream.h"

#include <condition_variable>

namespace lynx {

/// @class AsyncLogging
/// @brief A thread-safe asynchronous logging class that allows logging messages
/// to be written to a file or multiple files without blocking the calling
/// thread.
///
/// It uses a background thread to perform the actual writing to disk, ensuring
/// that the logging operations is non-blocking.
class AsyncLogging : Noncopyable {
public:
  AsyncLogging(const std::string &basename, off_t rollSize,
               int flushInterval = 3);
  ~AsyncLogging();

  /// Adds a log line to the current buffer. If the current buffer is full, it
  /// will switch to the next buffer and potentially trigger a flush to disk.
  /// This method is thread-safe.
  void append(const char *logline, size_t len);

  /// Starts the background thread if it's not already running.
  void start();

  /// Stops the bavckground thread and waits for it to finish.
  void stop();

private:
  /// The function executed by the background thread. It continuously checks for
  /// buffers that need to be flushed, waits for new buffers to be filled, and
  /// handles file rollover when necessary.
  void threadFunc();

  using Buffer = detail::FixedBuffer<detail::K_LARGE_BUFFER>;
  using BufferVector = std::vector<std::unique_ptr<Buffer>>;
  using BufferPtr = BufferVector::value_type;

  const int flush_interval_; /// Interval (in seconds) at which the background
                             /// thread should check if it neeeds to flush
                             /// buffers to disk.
  std::atomic_bool running_;
  const std::string basename_;
  const off_t roll_size_;

  Thread thread_; /// Background thread for flushing buffers to disk.
  std::latch latch_;
  mutable std::mutex mutex_;
  std::condition_variable cond_;
  BufferPtr current_buffer_; /// Current buffer.
  BufferPtr next_buffer_;    /// Preparing buffer.
  BufferVector buffers_;     /// Filled buffers waiting for writing into file.
};

} // namespace lynx

#endif
