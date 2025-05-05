#include "lynx/net/Buffer.h"

#include <sys/uio.h>

namespace lynx {

const char Buffer::KCRLF[] = "\r\n";
const size_t Buffer::KCheapPrepend;
const size_t Buffer::KInitialSize;

ssize_t Buffer::readFd(int Fd, int *SavedErrno) {
  char Extrabuf[65536];
  struct iovec Vec[2];
  const size_t Writable = writableBytes();
  Vec[0].iov_base = begin() + WriterIndex;
  Vec[0].iov_len = Writable;
  Vec[1].iov_base = Extrabuf;
  Vec[1].iov_len = sizeof(Extrabuf);
  const int Iovcnt = (Writable < sizeof(Extrabuf)) ? 2 : 1;
  const ssize_t N = ::readv(Fd, Vec, Iovcnt);
  if (N < 0) {
    *SavedErrno = errno;
  } else if (static_cast<size_t>(N) <= Writable) {
    WriterIndex += N;
  } else {
    WriterIndex = Buf.size();
    append(Extrabuf, N - Writable);
  }
  return N;
}

} // namespace lynx
