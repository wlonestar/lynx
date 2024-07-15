#ifndef LYNX_NET_BUFFER_H
#define LYNX_NET_BUFFER_H

#include <algorithm>
#include <cassert>
#include <cstring>
#include <endian.h>
#include <string>
#include <vector>

namespace lynx {

class Buffer {
public:
  static const size_t K_CHEAP_PREPEND = 8;
  static const size_t K_INITIAL_SIZE = 1024;

  explicit Buffer(size_t initialSize = K_INITIAL_SIZE)
      : buffer_(K_CHEAP_PREPEND + initialSize), reader_index_(K_CHEAP_PREPEND),
        writer_index_(K_CHEAP_PREPEND) {
    assert(readableBytes() == 0);
    assert(writableBytes() == initialSize);
    assert(prependableBytes() == K_CHEAP_PREPEND);
  }

  void swap(Buffer &rhs) {
    buffer_.swap(rhs.buffer_);
    std::swap(reader_index_, rhs.reader_index_);
    std::swap(writer_index_, rhs.writer_index_);
  }

  size_t readableBytes() const { return writer_index_ - reader_index_; }
  size_t writableBytes() const { return buffer_.size() - writer_index_; }
  size_t prependableBytes() const { return reader_index_; }

  const char *peek() const { return begin() + reader_index_; }

  const char *findCRLF() const {
    const char *crlf = std::search(peek(), beginWrite(), K_CRLF, K_CRLF + 2);
    return crlf == beginWrite() ? nullptr : crlf;
  }
  const char *findCRLF(const char *start) const {
    assert(peek() <= start);
    assert(start <= beginWrite());
    const char *crlf = std::search(start, beginWrite(), K_CRLF, K_CRLF + 2);
    return crlf == beginWrite() ? nullptr : crlf;
  }

  const char *findEOL() const {
    const void *eol = memchr(peek(), '\n', readableBytes());
    return static_cast<const char *>(eol);
  }
  const char *findEOL(const char *start) const {
    assert(peek() <= start);
    assert(start <= beginWrite());
    const void *eol = memchr(start, '\n', beginWrite() - start);
    return static_cast<const char *>(eol);
  }

  void retrieve(size_t len) {
    assert(len <= readableBytes());
    if (len < readableBytes()) {
      reader_index_ += len;
    } else {
      retrieveAll();
    }
  }

  void retrieveUntil(const char *end) {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
  }

  void retrieveInt64() { retrieve(sizeof(int64_t)); }
  void retrieveInt32() { retrieve(sizeof(int32_t)); }
  void retrieveInt16() { retrieve(sizeof(int16_t)); }
  void retrieveInt8() { retrieve(sizeof(int8_t)); }

  void retrieveAll() {
    reader_index_ = K_CHEAP_PREPEND;
    writer_index_ = K_CHEAP_PREPEND;
  }

  std::string retrieveAllAsString() {
    return retrieveAsString(readableBytes());
  }

  std::string retrieveAsString(size_t len) {
    assert(len <= readableBytes());
    std::string result(peek(), len);
    retrieve(len);
    return result;
  }

  std::string toString() const { return {peek(), readableBytes()}; }

  void append(const std::string &str) { append(str.data(), str.size()); }
  void append(const char * /*restrict*/ data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
  }
  void append(const void * /*restrict*/ data, size_t len) {
    append(static_cast<const char *>(data), len);
  }

  void ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
      makeSpace(len);
    }
    assert(writableBytes() >= len);
  }

  char *beginWrite() { return begin() + writer_index_; }
  const char *beginWrite() const { return begin() + writer_index_; }

  void hasWritten(size_t len) {
    assert(len <= writableBytes());
    writer_index_ += len;
  }

  void unwrite(size_t len) {
    assert(len <= readableBytes());
    writer_index_ -= len;
  }

  void appendInt64(int64_t x) {
    int64_t be64 = htobe64(x);
    append(&be64, sizeof(be64));
  }
  void appendInt32(int32_t x) {
    int32_t be32 = htobe32(x);
    append(&be32, sizeof(be32));
  }
  void appendInt16(int16_t x) {
    int16_t be16 = htobe16(x);
    append(&be16, sizeof(be16));
  }
  void appendInt8(int8_t x) { append(&x, sizeof(x)); }

  int64_t readInt64() {
    int64_t result = peekInt64();
    retrieveInt64();
    return result;
  }
  int32_t readInt32() {
    int32_t result = peekInt32();
    retrieveInt32();
    return result;
  }
  int16_t readInt16() {
    int16_t result = peekInt16();
    retrieveInt16();
    return result;
  }
  int8_t readInt8() {
    int8_t result = peekInt8();
    retrieveInt8();
    return result;
  }

  int64_t peekInt64() const {
    assert(readableBytes() >= sizeof(int64_t));
    int64_t be64 = 0;
    ::memcpy(&be64, peek(), sizeof(be64));
    return be64toh(be64);
  }
  int32_t peekInt32() const {
    assert(readableBytes() >= sizeof(int32_t));
    int32_t be32 = 0;
    ::memcpy(&be32, peek(), sizeof(be32));
    return be32toh(be32);
  }
  int16_t peekInt16() const {
    assert(readableBytes() >= sizeof(int16_t));
    int16_t be16 = 0;
    ::memcpy(&be16, peek(), sizeof(be16));
    return be16toh(be16);
  }
  int8_t peekInt8() const {
    assert(readableBytes() >= sizeof(int8_t));
    int8_t x = *peek();
    return x;
  }

  void prependInt64(int64_t x) {
    int64_t be64 = htobe64(x);
    prepend(&be64, sizeof(be64));
  }
  void prependInt32(int32_t x) {
    int32_t be32 = htobe32(x);
    prepend(&be32, sizeof(be32));
  }
  void prependInt16(int16_t x) {
    int16_t be16 = htobe16(x);
    prepend(&be16, sizeof(be16));
  }
  void prependInt8(int8_t x) { prepend(&x, sizeof(x)); }

  void prepend(const void * /*restrict*/ data, size_t len) {
    assert(len <= prependableBytes());
    reader_index_ -= len;
    const char *d = static_cast<const char *>(data);
    std::copy(d, d + len, begin() + reader_index_);
  }

  void shrink(size_t reserve) {
    Buffer other;
    other.ensureWritableBytes(readableBytes() + reserve);
    other.append(toString());
    swap(other);
  }

  size_t internalCapacity() const { return buffer_.capacity(); }

  ssize_t readFd(int fd, int *savedErrno);

private:
  char *begin() { return &*buffer_.begin(); }
  const char *begin() const { return &*buffer_.begin(); }

  void makeSpace(size_t len) {
    if (writableBytes() + prependableBytes() < len + K_CHEAP_PREPEND) {
      buffer_.resize(writer_index_ + len);
    } else {
      assert(K_CHEAP_PREPEND < reader_index_);
      size_t readable = readableBytes();
      std::copy(begin() + reader_index_, begin() + writer_index_,
                begin() + K_CHEAP_PREPEND);
      reader_index_ = K_CHEAP_PREPEND;
      writer_index_ = reader_index_ + readable;
      assert(readable == readableBytes());
    }
  }

  std::vector<char> buffer_;
  size_t reader_index_;
  size_t writer_index_;

  static const char K_CRLF[];
};

} // namespace lynx

#endif
