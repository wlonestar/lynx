#ifndef LYNX_BASE_FILE_UTIL_H
#define LYNX_BASE_FILE_UTIL_H

#include "lynx/base/noncopyable.h"

#include <string>
#include <sys/types.h>

namespace lynx::fs {

class ReadSmallFile : Noncopyable {
public:
  ReadSmallFile(std::string filename);
  ~ReadSmallFile();

  template <typename String>
  int readToString(int maxSize, String *content, int64_t *fileSize,
                   int64_t *modifyTime, int64_t *createTime);

  int readToBuffer(int *size);

  const char *buffer() const { return buf_; }

  static const int K_BUFFER_DIZE = 64 * 1024;

private:
  int fd_;
  int err_;
  char buf_[K_BUFFER_DIZE];
};

template <typename String>
int readFile(std::string filename, int maxSize, String *content,
             int64_t *fileSize = nullptr, int64_t *modifyTime = nullptr,
             int64_t *createTime = nullptr) {
  ReadSmallFile file(filename);
  return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
}

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

} // namespace lynx::fs

#endif
