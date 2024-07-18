#include "lynx/logger/log_stream.h"

#include <algorithm>
#include <cinttypes>
#include <numeric>

namespace lynx {

namespace detail {

const char DIGITS[] = "9876543210123456789";
const char *zero = DIGITS + 9;
static_assert(sizeof(DIGITS) == 20, "wrong number of digits");

const char DIGITS_HEX[] = "0123456789ABCDEF";
static_assert(sizeof(DIGITS_HEX) == 17, "wrong number of digitsHex");

// Efficient Integer to String Conversions, by Matthew Wilson.
template <typename T> size_t convert(char buf[], T value) {
  T i = value;
  char *p = buf;

  do {
    int lsd = static_cast<int>(i % 10);
    i /= 10;
    *p++ = zero[lsd];
  } while (i != 0);

  if (value < 0) {
    *p++ = '-';
  }
  *p = '\0';
  std::reverse(buf, p);

  return p - buf;
}

size_t convertHex(char buf[], uintptr_t value) {
  uintptr_t i = value;
  char *p = buf;

  do {
    int lsd = static_cast<int>(i % 16);
    i /= 16;
    *p++ = DIGITS_HEX[lsd];
  } while (i != 0);

  *p = '\0';
  std::reverse(buf, p);

  return p - buf;
}

template class FixedBuffer<K_SMALL_BUFFER>;
template class FixedBuffer<K_LARGE_BUFFER>;

template <int SIZE> void FixedBuffer<SIZE>::cookieStart() {}
template <int SIZE> void FixedBuffer<SIZE>::cookieEnd() {}

} // namespace detail

void LogStream::staticCheck() {
  static_assert(K_MAX_NUMERIC_SIZE - 10 > std::numeric_limits<double>::digits10,
                "K_MAX_NUMERIC_SIZE is large enough");
  static_assert(K_MAX_NUMERIC_SIZE - 10 >
                    std::numeric_limits<long double>::digits10,
                "K_MAX_NUMERIC_SIZE is large enough");
  static_assert(K_MAX_NUMERIC_SIZE - 10 > std::numeric_limits<long>::digits10,
                "K_MAX_NUMERIC_SIZE is large enough");
  static_assert(K_MAX_NUMERIC_SIZE - 10 >
                    std::numeric_limits<long long>::digits10,
                "K_MAX_NUMERIC_SIZE is large enough");
}

template <typename T> void LogStream::formatInteger(T v) {
  if (buffer_.avail() >= K_MAX_NUMERIC_SIZE) {
    size_t len = detail::convert(buffer_.current(), v);
    buffer_.add(len);
  }
}

LogStream &LogStream::operator<<(bool v) {
  if (v) {
    buffer_.append("true", 4);
  } else {
    buffer_.append("false", 5);
  }
  return *this;
}

LogStream &LogStream::operator<<(int16_t v) {
  *this << static_cast<int32_t>(v);
  return *this;
}

LogStream &LogStream::operator<<(uint16_t v) {
  *this << static_cast<uint32_t>(v);
  return *this;
}

LogStream &LogStream::operator<<(int32_t v) {
  formatInteger(v);
  return *this;
}

LogStream &LogStream::operator<<(uint32_t v) {
  formatInteger(v);
  return *this;
}

LogStream &LogStream::operator<<(int64_t v) {
  formatInteger(v);
  return *this;
}

LogStream &LogStream::operator<<(uint64_t v) {
  formatInteger(v);
  return *this;
}

LogStream &LogStream::operator<<(float v) {
  *this << static_cast<double>(v);
  return *this;
}

LogStream &LogStream::operator<<(double v) {
  // if (buffer_.avail() >= K_MAX_NUMERIC_SIZE) {
  //   int len = snprintf(buffer_.current(), K_MAX_NUMERIC_SIZE, "%.12g", v);
  //   buffer_.add(len);
  // }
  int len = detail::fpconvDtoa(v, buffer_.current());
  buffer_.add(len);
  return *this;
}

LogStream &LogStream::operator<<(const void *p) {
  auto v = reinterpret_cast<uintptr_t>(p);
  if (buffer_.avail() >= K_MAX_NUMERIC_SIZE) {
    char *buf = buffer_.current();
    buf[0] = '0';
    buf[1] = 'x';
    size_t len = detail::convertHex(buf + 2, v);
    buffer_.add(len + 2);
  }
  return *this;
}

LogStream &LogStream::operator<<(char v) {
  buffer_.append(&v, 1);
  return *this;
}

LogStream &LogStream::operator<<(const char *str) {
  if (str != nullptr) {
    buffer_.append(str, strlen(str));
  } else {
    buffer_.append("(null)", 6);
  }
  return *this;
}

LogStream &LogStream::operator<<(const unsigned char *str) {
  return operator<<(reinterpret_cast<const char *>(str));
}

LogStream &LogStream::operator<<(const std::string &str) {
  buffer_.append(str.c_str(), str.size());
  return *this;
}

LogStream &LogStream::operator<<(const Buffer &v) {
  *this << v.toString();
  return *this;
}

} // namespace lynx
