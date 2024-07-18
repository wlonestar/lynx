#ifndef LYNX_LOGGER_LOG_STREAM_H
#define LYNX_LOGGER_LOG_STREAM_H

#include "lynx/base/noncopyable.h"

#include <cstring>
#include <string>

namespace lynx {

namespace detail {

const int K_SMALL_BUFFER = 4000;
const int K_LARGE_BUFFER = 4000 * 1000;

template <int SIZE> class FixedBuffer : Noncopyable {
public:
  FixedBuffer() : cur_(data_) { setCookie(cookieStart); }
  ~FixedBuffer() { setCookie(cookieEnd); }

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

  void setCookie(void (*cookie)()) { cookie_ = cookie; }

  std::string toString() const { return std::string(data_, length()); }

private:
  const char *end() const { return data_ + sizeof(data_); }

  static void cookieStart();
  static void cookieEnd();

  void (*cookie_)();
  char data_[SIZE];
  char *cur_;
};

/// Fast and accurate double to string conversion based on Florian Loitsch's
/// Grisu3 algorithm.
int fpconvDtoa(double d, char dest[24]);

} // namespace detail

class LogStream : Noncopyable {
  using self = LogStream;

public:
  using Buffer = detail::FixedBuffer<detail::K_SMALL_BUFFER>;

  self &operator<<(bool);
  self &operator<<(int16_t);
  self &operator<<(uint16_t);
  self &operator<<(int32_t);
  self &operator<<(uint32_t);
  self &operator<<(int64_t);
  self &operator<<(uint64_t);
  self &operator<<(float);
  self &operator<<(double);
  self &operator<<(const void *);
  self &operator<<(char);
  self &operator<<(const char *);
  self &operator<<(const unsigned char *);
  self &operator<<(const std::string &);
  self &operator<<(const Buffer &);

  void append(const char *data, size_t len) { buffer_.append(data, len); }
  const Buffer &buffer() const { return buffer_; }
  void resetBuffer() { buffer_.reset(); }

private:
  void staticCheck();

  template <typename T> void formatInteger(T);

  Buffer buffer_;

  static const int K_MAX_NUMERIC_SIZE = 48;
};

} // namespace lynx

#endif
