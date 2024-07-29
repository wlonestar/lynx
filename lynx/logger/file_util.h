#ifndef LYNX_LOGGER_FILE_UTIL_H
#define LYNX_LOGGER_FILE_UTIL_H

#include "lynx/base/noncopyable.h"

#include <string>

namespace lynx::util {

class AppendFile : Noncopyable {
public:
  explicit AppendFile(std::string filename);
  ~AppendFile();

  void append(const char *logline, size_t len);
  void flush();

  off_t writtenBytes() const { return written_bytes_; }

private:
  size_t write(const char *logline, size_t len);

  FILE *fp_;
  char buffer_[64 * 1024];
  off_t written_bytes_;
};

} // namespace lynx::util

#endif
