#include "lynx/logger/async_logging.h"
#include "lynx/base/thread.h"
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
  std::lock_guard<std::mutex> lock(mutex_);

  if (current_buffer_->avail() > static_cast<int>(len)) {
    current_buffer_->append(logline, len);
  } else {
    buffers_.push_back(std::move(current_buffer_));

    if (next_buffer_) {
      current_buffer_ = std::move(next_buffer_);
    } else {
      current_buffer_ = std::make_unique<Buffer>();
    }

    current_buffer_->append(logline, len);
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

  BufferPtr new_buffer1(new Buffer);
  new_buffer1->bzero();
  BufferPtr new_buffer2(new Buffer);
  new_buffer2->bzero();
  BufferVector buffers_to_write;
  buffers_to_write.reserve(16);

  while (running_) {
    assert(buffers_to_write.empty());

    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (buffers_.empty()) {
        cond_.wait_for(lock, std::chrono::seconds(flush_interval_));
      }
      buffers_.push_back(std::move(current_buffer_));
      current_buffer_ = std::move(new_buffer1);
      buffers_to_write.swap(buffers_);
      if (!next_buffer_) {
        next_buffer_ = std::move(new_buffer2);
      }
    }

    assert(!buffers_to_write.empty());

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

    for (const auto &buffer : buffers_to_write) {
      output.append(buffer->data(), buffer->length());
    }
    if (buffers_to_write.size() > 2) {
      buffers_to_write.resize(2);
    }

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
