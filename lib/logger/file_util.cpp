#include "lynx/logger/file_util.h"
#include "lynx/base/current_thread.h"

#include <cassert>

namespace lynx::util {

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
        fprintf(stderr, "AppendFile::append() failed %s\n",
                current_thread::strError(err));
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

} // namespace lynx::util
