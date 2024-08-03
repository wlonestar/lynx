#ifndef LYNX_LOGGER_LOG_STREAM_H
#define LYNX_LOGGER_LOG_STREAM_H

#include "lynx/base/noncopyable.h"

#include <cstring>
#include <string>

namespace lynx {

namespace detail {

const int K_SMALL_BUFFER = 4000;
const int K_LARGE_BUFFER = 4000 * 1000;

/// @class FixedBuffer
/// @brief A utility class for managing a fixed-size buffer.
template <int SIZE> class FixedBuffer : Noncopyable {
public:
  FixedBuffer() : cur_(data_) {}
  ~FixedBuffer() = default;

  void append(const char *buf, size_t len) {
    if (static_cast<size_t>(avail()) > len) {
      memcpy(cur_, buf, len);
      cur_ += len;
    }
  }

  const char *data() const { return data_; }
  int length() const { return static_cast<int>(cur_ - data_); }
  char *current() { return cur_; }
  int avail() const { return static_cast<int>(end() - cur_); }

  void add(size_t len) { cur_ += len; }
  void reset() { cur_ = data_; }
  void bzero() { ::memset(data_, 0, sizeof(data_)); }

  std::string toString() const { return std::string(data_, length()); }

private:
  /// Returns a pointer to the end of the buffer.
  const char *end() const { return data_ + sizeof(data_); }

  char data_[SIZE]; /// The fixed-size buffer.
  char *cur_;       // Pointer to the current write position in the buffer.
};

} // namespace detail

/// @class LogStream
/// @brief A log stream class that uses a fixed-size buffer to accumulate log
/// messages.
///
/// It provides overloaded insertion operators for various data types to
/// facilitate easy logging.
class LogStream : Noncopyable {
public:
  using Buffer = detail::FixedBuffer<detail::K_SMALL_BUFFER>;

  LogStream &operator<<(bool);
  LogStream &operator<<(int16_t);
  LogStream &operator<<(uint16_t);
  LogStream &operator<<(int32_t);
  LogStream &operator<<(uint32_t);
  LogStream &operator<<(int64_t);
  LogStream &operator<<(uint64_t);
  LogStream &operator<<(float);
  LogStream &operator<<(double);
  LogStream &operator<<(const void *);
  LogStream &operator<<(char);
  LogStream &operator<<(const char *);
  LogStream &operator<<(const unsigned char *);
  LogStream &operator<<(const std::string &);
  LogStream &operator<<(const Buffer &);

  void append(const char *data, size_t len) { buffer_.append(data, len); }
  const Buffer &buffer() const { return buffer_; }
  void resetBuffer() { buffer_.reset(); }

private:
  template <typename T> void formatInteger(T);

  Buffer buffer_;

  static const int K_MAX_NUMERIC_SIZE = 48;
};

} // namespace lynx

#endif
