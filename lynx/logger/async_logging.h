#ifndef LYNX_LOGGER_ASYNC_LOGGING_H
#define LYNX_LOGGER_ASYNC_LOGGING_H

#include "lynx/base/noncopyable.h"
#include "lynx/base/thread.h"
#include "lynx/logger/log_stream.h"

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <latch>
#include <memory>
#include <mutex>
#include <vector>

namespace lynx {

class AsyncLogging : Noncopyable {
public:
  AsyncLogging(const std::string &basename, off_t rollSize,
               int flushInterval = 3);
  ~AsyncLogging();

  void append(const char *logline, size_t len);
  void start();
  void stop();

private:
  void threadFunc();

  using Buffer = detail::FixedBuffer<detail::K_LARGE_BUFFER>;
  using BufferVector = std::vector<std::unique_ptr<Buffer>>;
  using BufferPtr = BufferVector::value_type;

  const int flush_interval_;
  std::atomic_bool running_;
  const std::string basename_;
  const off_t roll_size_;

  Thread thread_;
  std::latch latch_;
  mutable std::mutex mutex_;
  std::condition_variable cond_;

  BufferPtr current_buffer_;
  BufferPtr next_buffer_;
  BufferVector buffers_;
};

} // namespace lynx

#endif
