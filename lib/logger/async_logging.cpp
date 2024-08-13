#include "lynx/logger/async_logging.h"
#include "lynx/base/timestamp.h"
#include "lynx/logger/log_file.h"

#include <cassert>

namespace lynx {

AsyncLogging::AsyncLogging(const std::string &basename, off_t rollSize,
                           int flushInterval)
    : flush_interval_(flushInterval), running_(false), basename_(basename),
      roll_size_(rollSize), thread_([this] { threadFunc(); }, "Logging"),
      latch_(1), current_buffer_(new Buffer), next_buffer_(new Buffer) {
  current_buffer_->bzero();
  next_buffer_->bzero();
  buffers_.reserve(16);
}

AsyncLogging::~AsyncLogging() {
  if (running_) {
    stop();
  }
}

void AsyncLogging::append(const char *logline, size_t len) {
  std::lock_guard<std::mutex> lock(
      mutex_); /// Acquire the lock to ensure thread safety

  /// Check if the current buffer has enough space to append the log line
  if (current_buffer_->avail() > static_cast<int>(len)) {
    /// Current buffer has space, append log line
    current_buffer_->append(logline, len);
  } else {
    /// Current buffer is full, move current buffer to filled buffers
    buffers_.push_back(std::move(current_buffer_));

    /// Use the next buffer if it exists, otherwise create a new buffer
    if (next_buffer_) {
      current_buffer_ = std::move(next_buffer_);
    } else {
      current_buffer_ = std::make_unique<Buffer>();
    }
    current_buffer_->append(logline, len);

    /// Notify the background thread that there is a new buffer ready to be
    /// flushed
    cond_.notify_one();
  }
}

void AsyncLogging::start() {
  running_ = true;
  thread_.start();
  latch_.wait();
}

void AsyncLogging::stop() {
  running_ = false;
  cond_.notify_one();
  thread_.join();
}

void AsyncLogging::threadFunc() {
  assert(running_ == true);
  latch_.count_down();
  LogFile output(basename_, roll_size_, false);

  /// Create two new buffers to replace the current buffer
  BufferPtr new_buffer1(new Buffer);
  new_buffer1->bzero();
  BufferPtr new_buffer2(new Buffer);
  new_buffer2->bzero();

  /// Vector to store the buffers to be written
  BufferVector buffers_to_write;
  buffers_to_write.reserve(16);

  while (running_) {
    assert(buffers_to_write.empty());

    /// Swap out what needs to be written
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (buffers_.empty()) {
        /// If there are no buffers to write, wait for the specified interval
        cond_.wait_for(lock, std::chrono::seconds(flush_interval_));
      }
      /// Move the current buffer to the buffers to be written
      buffers_.push_back(std::move(current_buffer_));
      /// Replace the current buffer with the new buffer
      current_buffer_ = std::move(new_buffer1);
      /// Swap the buffers to be written with the actual buffers
      buffers_to_write.swap(buffers_);
      /// If there is no next buffer, create a new buffer
      if (!next_buffer_) {
        next_buffer_ = std::move(new_buffer2);
      }
    }

    assert(!buffers_to_write.empty());

    /// If there are too many buffers to write, drop some
    if (buffers_to_write.size() > 25) {
      char buf[256];
      snprintf(buf, sizeof(buf),
               "Dropped log messages at %s, %zd larger buffers\n",
               Timestamp::now().toFormattedString().c_str(),
               buffers_to_write.size() - 2);
      fputs(buf, stderr);
      output.append(buf, static_cast<int>(strlen(buf)));
      buffers_to_write.erase(buffers_to_write.begin() + 2,
                             buffers_to_write.end());
    }

    /// Append the log messages to the log file
    for (const auto &buffer : buffers_to_write) {
      output.append(buffer->data(), buffer->length());
    }
    /// If there are too many buffers, resize the vector to 2
    if (buffers_to_write.size() > 2) {
      buffers_to_write.resize(2);
    }

    /// Replace the new buffers if they are empty
    if (!new_buffer1) {
      assert(!buffers_to_write.empty());
      new_buffer1 = std::move(buffers_to_write.back());
      buffers_to_write.pop_back();
      new_buffer1->reset();
    }
    if (!new_buffer2) {
      assert(!buffers_to_write.empty());
      new_buffer2 = std::move(buffers_to_write.back());
      buffers_to_write.pop_back();
      new_buffer2->reset();
    }

    buffers_to_write.clear();
    output.flush();
  }

  output.flush();
}

} // namespace lynx
