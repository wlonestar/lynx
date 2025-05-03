#ifndef LYNX_LOGGER_FILE_UTIL_H
#define LYNX_LOGGER_FILE_UTIL_H

#include "lynx/base/noncopyable.h"

#include <string>

namespace lynx::util {

/**
 * @class AppendFile
 * @brief A class for efficiently appending text lines to a file.
 *
 * The AppendFile class utilizes an internal buffer to minimize the number of
 * disk writes, thereby improving performance when appending large amounts of
 * data.
 *
 * @note Instances of AppendFile should not be copied or assigned.
 */
class AppendFile : Noncopyable {
public:
  explicit AppendFile(std::string filename);
  ~AppendFile();

  /**
   * @brief Appends a log line to the file.
   *
   * @param logline The log line to append.
   * @param len The length of the log line.
   */
  void append(const char *logline, size_t len);

  /// Flushes any remaining data in the buffer to the file.
  void flush();

  /**
   * @brief Returns the number of bytes written to the file.
   *
   * @return The number of bytes written to the file.
   */
  off_t writtenBytes() const { return written_bytes_; }

private:
  /**
   * @brief Writes the log line to the file.
   *
   * @param logline The log line to write.
   * @param len The length of the log line.
   *
   * @return The number of bytes written to the file.
   */
  size_t write(const char *logline, size_t len);

  FILE *fp_;               /// The file pointer.
  char buffer_[64 * 1024]; /// The internal buffer.
  off_t written_bytes_;    /// The number of bytes written to the file.
};

} // namespace lynx::util

#endif
