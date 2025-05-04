#include "lynx/base/CurrentThread.h"
#include "lynx/logger/FileUtil.h"

#include <cassert>

namespace lynx {

namespace util {

AppendFile::AppendFile(std::string Filename)
    : Fp(::fopen(Filename.c_str(), "ae")), WrittenBytes(0) {
  assert(Fp);
  ::setbuffer(Fp, Buffer, sizeof(Buffer));
}

AppendFile::~AppendFile() { ::fclose(Fp); }

void AppendFile::append(const char *Logline, size_t Len) {
  size_t Written = 0;

  while (Written != Len) {
    size_t Remain = Len - Written;
    size_t N = write(Logline + Written, Remain);
    if (N != Remain) {
      int Err = ::ferror(Fp);
      if (Err != 0) {
        fprintf(stderr, "AppendFile::append() failed %s\n",
                current_thread::strError(Err));
        break;
      }
    }
    Written += N;
  }

  WrittenBytes += Written;
}

void AppendFile::flush() { ::fflush(Fp); }

size_t AppendFile::write(const char *Logline, size_t Len) {
  return ::fwrite_unlocked(Logline, 1, Len, Fp);
}

} // namespace util

} // namespace lynx
