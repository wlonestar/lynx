#include "lynx/base/Timestamp.h"
#include "lynx/logger/AsyncLogging.h"
#include "lynx/logger/LogFile.h"

#include <cassert>

namespace lynx {

AsyncLogging::AsyncLogging(const std::string &Basename, off_t RollSize,
                           int FlushInterval)
    : FlushInterval(FlushInterval), Running(false), Basename(Basename),
      RollSize(RollSize), Thread([&] { threadFunc(); }, "Logging"), Latch(1),
      CurrentBuffer(new Buffer), NextBuffer(new Buffer) {
  CurrentBuffer->bzero();
  NextBuffer->bzero();
  Buffers.reserve(16);
}

AsyncLogging::~AsyncLogging() {
  if (Running)
    stop();
}

void AsyncLogging::append(const char *Logline, size_t Len) {
  std::lock_guard<std::mutex> Lock(Mutex);

  /// Check if the current buffer has enough space to append the log line
  if (CurrentBuffer->avail() > static_cast<int>(Len)) {
    /// Current buffer has space, append log line
    CurrentBuffer->append(Logline, Len);
  } else {
    /// Current buffer is full, move current buffer to filled buffers
    Buffers.push_back(std::move(CurrentBuffer));

    /// Use the next buffer if it exists, otherwise create a new buffer
    if (NextBuffer) {
      CurrentBuffer = std::move(NextBuffer);
    } else {
      CurrentBuffer = std::make_unique<Buffer>();
    }
    CurrentBuffer->append(Logline, Len);

    /// Notify the background thread that there is a new buffer ready to be
    /// flushed
    Cond.notify_one();
  }
}

void AsyncLogging::start() {
  Running = true;
  Thread.start();
  Latch.wait();
}

void AsyncLogging::stop() {
  Running = false;
  Cond.notify_one();
  Thread.join();
}

void AsyncLogging::threadFunc() {
  assert(Running == true);
  Latch.count_down();
  LogFile Output(Basename, RollSize, false);

  /// Create two new buffers to replace the current buffer
  BufferPtr NewBuffer1(new Buffer);
  NewBuffer1->bzero();
  BufferPtr NewBuffer2(new Buffer);
  NewBuffer2->bzero();

  /// Vector to store the buffers to be written
  BufferVector BuffersToWrite;
  BuffersToWrite.reserve(16);

  while (Running) {
    assert(BuffersToWrite.empty());

    /// Swap out what needs to be written
    {
      std::unique_lock<std::mutex> Lock(Mutex);
      if (Buffers.empty()) {
        /// If there are no buffers to write, wait for the specified interval
        Cond.wait_for(Lock, std::chrono::seconds(FlushInterval));
      }
      /// Move the current buffer to the buffers to be written
      Buffers.push_back(std::move(CurrentBuffer));
      /// Replace the current buffer with the new buffer
      CurrentBuffer = std::move(NewBuffer1);
      /// Swap the buffers to be written with the actual buffers
      BuffersToWrite.swap(Buffers);
      /// If there is no next buffer, create a new buffer
      if (!NextBuffer) {
        NextBuffer = std::move(NewBuffer2);
      }
    }

    assert(!BuffersToWrite.empty());

    /// If there are too many buffers to write, drop some
    if (BuffersToWrite.size() > 25) {
      char Buf[256];
      snprintf(Buf, sizeof(Buf),
               "Dropped log messages at %s, %zd larger buffers\n",
               Timestamp::now().toFormattedString().c_str(),
               BuffersToWrite.size() - 2);
      fputs(Buf, stderr);
      Output.append(Buf, static_cast<int>(strlen(Buf)));
      BuffersToWrite.erase(BuffersToWrite.begin() + 2, BuffersToWrite.end());
    }

    /// Append the log messages to the log file
    for (const auto &Buffer : BuffersToWrite) {
      Output.append(Buffer->data(), Buffer->length());
    }
    /// If there are too many buffers, resize the vector to 2
    if (BuffersToWrite.size() > 2) {
      BuffersToWrite.resize(2);
    }

    /// Replace the new buffers if they are empty
    if (!NewBuffer1) {
      assert(!BuffersToWrite.empty());
      NewBuffer1 = std::move(BuffersToWrite.back());
      BuffersToWrite.pop_back();
      NewBuffer1->reset();
    }
    if (!NewBuffer2) {
      assert(!BuffersToWrite.empty());
      NewBuffer2 = std::move(BuffersToWrite.back());
      BuffersToWrite.pop_back();
      NewBuffer2->reset();
    }

    BuffersToWrite.clear();
    Output.flush();
  }

  Output.flush();
}

} // namespace lynx
