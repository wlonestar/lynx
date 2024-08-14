#ifndef LYNX_LOGGER_LOG_STREAM_H
#define LYNX_LOGGER_LOG_STREAM_H

#include "lynx/base/noncopyable.h"

#include <cstring>
#include <string>

namespace lynx {

namespace detail {

const int K_SMALL_BUFFER = 4000;
const int K_LARGE_BUFFER = 4000 * 1000;

/**
 * @class FixedBuffer
 * @brief A utility class for managing a fixed-size buffer.
 *
 * This class provides a fixed-size buffer for efficient data management.
 * It maintains a separation between the current write position, the
 * beginning of the buffer, and the end of the buffer. The buffer can be
 * used to accumulate log messages or any other data that needs to be
 * stored in a fixed-size buffer.
 */
template <int SIZE> class FixedBuffer : Noncopyable {
public:
  /**
   * @brief Constructs a FixedBuffer object with the current write position set
   * to the beginning of the buffer.
   */
  FixedBuffer() : cur_(data_) {}
  ~FixedBuffer() = default;

  /**
   * @brief Appends a given number of bytes from a buffer to the fixed buffer.
   *
   * @param buf The buffer to append.
   * @param len The number of bytes to append.
   */
  void append(const char *buf, size_t len) {
    // If the available space in the buffer is greater than the length of the
    // buffer to be appended, append the buffer to the end of the fixed buffer.
    if (static_cast<size_t>(avail()) > len) {
      memcpy(cur_, buf, len);
      cur_ += len;
    }
  }

  /// Returns a pointer to the beginning of the fixed buffer.
  const char *data() const { return data_; }

  /// Returns the length of the data in the fixed buffer.
  int length() const { return static_cast<int>(cur_ - data_); }

  /// Returns a pointer to the current write position in the fixed buffer.
  char *current() { return cur_; }

  /// Returns the number of bytes available in the fixed buffer.
  int avail() const { return static_cast<int>(end() - cur_); }

  /**
   * @brief Moves the current write position forward by a given number of bytes.
   *
   * @param len The number of bytes to move the write position forward.
   */
  void add(size_t len) { cur_ += len; }

  /// Sets the current write position to the beginning of the fixed buffer.
  void reset() { cur_ = data_; }

  /// Sets all bytes in the fixed buffer to zero.
  void bzero() { ::memset(data_, 0, sizeof(data_)); }

  /// Returns the contents of the fixed buffer as a string.
  std::string toString() const { return std::string(data_, length()); }

private:
  /// Returns a pointer to the end of the fixed buffer.
  const char *end() const { return data_ + sizeof(data_); }

  char data_[SIZE]; /// The fixed-size buffer.
  char *cur_;       // The current write position in the fixed buffer.
};

} // namespace detail

/**
 * @class LogStream
 * @brief A log stream class that uses a fixed-size buffer to accumulate log
 * messages.
 *
 * It provides overloaded insertion operators for various data types to
 * facilitate easy logging.
 */
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

  /**
   * @brief Appends a C-style string to the LogStream buffer
   *
   * @param data The C-style string to append
   * @param len The length of the string to append
   */
  void append(const char *data, size_t len) { buffer_.append(data, len); }

  /// Returns a const reference to the LogStream buffer
  const Buffer &buffer() const { return buffer_; }

  /// @brief Resets the LogStream buffer to its initial state
  void resetBuffer() { buffer_.reset(); }

private:
  /**
   * @brief Formats an integer value and appends it to the LogStream buffer
   *
   * @tparam T The integer type to format
   * @param v The integer value to format and append
   */
  template <typename T> void formatInteger(T v);

  Buffer buffer_; /// The fixed-size buffer used by LogStream

  /// The maximum size of a numeric value
  static const int K_MAX_NUMERIC_SIZE = 48;
};

} // namespace lynx

#endif
