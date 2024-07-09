#include "lynx/base/file_util.h"
#include "lynx/logger/logging.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace lynx::fs {

ReadSmallFile::ReadSmallFile(std::string filename)
    : fd_(::open(filename.c_str(), O_RDONLY | O_CLOEXEC)), err_(0) {
  buf_[0] = '\0';
  if (fd_ < 0) {
    err_ = errno;
  }
}

ReadSmallFile::~ReadSmallFile() {
  if (fd_ >= 0) {
    ::close(fd_);
  }
}

template <typename String>
int ReadSmallFile::readToString(int maxSize, String *content, int64_t *fileSize,
                                int64_t *modifyTime, int64_t *createTime) {
  static_assert(sizeof(off_t) == 8, "_FILE_OFFSET_BITS = 64");
  assert(content != NULL);
  int err = err_;
  if (fd_ >= 0) {
    content->clear();

    if (fileSize) {
      struct stat statbuf;
      if (::fstat(fd_, &statbuf) == 0) {
        if (S_ISREG(statbuf.st_mode)) {
          *fileSize = statbuf.st_size;
          content->reserve(static_cast<int>(
              std::min(static_cast<int64_t>(maxSize), *fileSize)));
        } else if (S_ISDIR(statbuf.st_mode)) {
          err = EISDIR;
        }
        if (modifyTime) {
          *modifyTime = statbuf.st_mtime;
        }
        if (createTime) {
          *createTime = statbuf.st_ctime;
        }
      } else {
        err = errno;
      }
    }

    while (content->size() < static_cast<size_t>(maxSize)) {
      size_t to_read = std::min(static_cast<size_t>(maxSize) - content->size(),
                                sizeof(buf_));
      ssize_t n = ::read(fd_, buf_, to_read);
      if (n > 0) {
        content->append(buf_, n);
      } else {
        if (n < 0) {
          err = errno;
        }
        break;
      }
    }
  }
  return err;
}

int ReadSmallFile::readToBuffer(int *size) {
  int err = err_;
  if (fd_ >= 0) {
    ssize_t n = ::pread(fd_, buf_, sizeof(buf_) - 1, 0);
    if (n >= 0) {
      if (size != nullptr) {
        *size = static_cast<int>(n);
      }
      buf_[n] = '\0';
    } else {
      err = errno;
    }
  }
  return err;
}

AppendFile::AppendFile(std::string filename)
    : fp_(::fopen(filename.c_str(), "ae")), written_bytes_(0) {
  assert(fp_);
  ::setbuffer(fp_, buffer_, sizeof(buffer_));
}

AppendFile::~AppendFile() { ::fclose(fp_); }

void AppendFile::append(const char *logline, size_t len) {
  size_t written = 0;

  while (written != len) {
    size_t remain = len - written;
    size_t n = write(logline + written, remain);
    if (n != remain) {
      int err = ::ferror(fp_);
      if (err != 0) {
        fprintf(stderr, "AppendFile::append() failed %s\n", strErrorTl(err));
        break;
      }
    }
    written += n;
  }

  written_bytes_ += written;
}

void AppendFile::flush() { ::fflush(fp_); }

size_t AppendFile::write(const char *logline, size_t len) {
  return ::fwrite_unlocked(logline, 1, len, fp_);
}

} // namespace lynx::fs

template int lynx::fs::readFile(std::string filename, int maxSize,
                                std::string *content, int64_t *, int64_t *,
                                int64_t *);

template int lynx::fs::ReadSmallFile::readToString(int maxSize,
                                                   std::string *content,
                                                   int64_t *, int64_t *,
                                                   int64_t *);
