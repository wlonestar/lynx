#include "lynx/net/buffer.h"
#include "lynx/net/sockets_ops.h"

#include <cerrno>
#include <sys/uio.h>

namespace lynx {

const char Buffer::K_CRLF[] = "\r\n";

const size_t Buffer::K_CHEAP_PREPEND;
const size_t Buffer::K_INITIAL_SIZE;

ssize_t Buffer::readFd(int fd, int *savedErrno) {
  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = writableBytes();
  vec[0].iov_base = begin() + writer_index_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof(extrabuf);
  const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
  const ssize_t n = sockets::readv(fd, vec, iovcnt);
  if (n < 0) {
    *savedErrno = errno;
  } else if (static_cast<size_t>(n) <= writable) {
    writer_index_ += n;
  } else {
    writer_index_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  return n;
}

} // namespace lynx
