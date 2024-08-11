#ifndef LYNX_NET_BUFFER_H
#define LYNX_NET_BUFFER_H

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>

namespace lynx {

/**
 * @class Buffer
 * @brief A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
 *
 * The Buffer class provides a dynamically resizable buffer for reading and
 * writing data. It maintains a separation between prependable, readable and
 * writable bytes for efficient data management.
 *
 * The buffer layout:
 * +-------------------+------------------+------------------+
 * | prependable bytes |  readable bytes  |  writable bytes  |
 * |                   |     (Content)    |                  |
 * +-------------------+------------------+------------------+
 * |                   |                  |                  |
 * 0      <=      readerIndex   <=   writerIndex    <=     size
 * +-------------------+------------------+------------------+
 *
 */
class Buffer {
public:
  static const size_t K_CHEAP_PREPEND = 8;
  static const size_t K_INITIAL_SIZE = 1024;

  /**
   * @brief Constructs a Buffer with an initial size.
   * @param initialSize The initial size of the buffer.
   */
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

  /**
   * @brief Returns a pointer to the beginning of readable data.
   * @return A pointer to the readable data.
   */
  const char *peek() const { return begin() + reader_index_; }

  /**
   * @brief Finds the first occurrence of CRLF in the readable data.
   * @return A pointer to the CRLF if found, otherwise nullptr.
   */
  const char *findCRLF() const {
    const char *crlf = std::search(peek(), beginWrite(), K_CRLF, K_CRLF + 2);
    return crlf == beginWrite() ? nullptr : crlf;
  }

  /**
   * @brief Finds the first occurrence of CRLF starting from a given position.
   * @param start The starting position.
   * @return A pointer to the CRLF if found, otherwise nullptr.
   */
  const char *findCRLF(const char *start) const {
    assert(peek() <= start);
    assert(start <= beginWrite());
    const char *crlf = std::search(start, beginWrite(), K_CRLF, K_CRLF + 2);
    return crlf == beginWrite() ? nullptr : crlf;
  }

  /**
   * @brief Finds the first occurrence of EOL in the readable data.
   * @return A pointer to the EOL if found, otherwise nullptr.
   */
  const char *findEOL() const {
    const void *eol = memchr(peek(), '\n', readableBytes());
    return static_cast<const char *>(eol);
  }

  /**
   * @brief Finds the first occurrence of EOL starting from a given position.
   * @param start The starting position.
   * @return A pointer to the EOL if found, otherwise nullptr.
   */
  const char *findEOL(const char *start) const {
    assert(peek() <= start);
    assert(start <= beginWrite());
    const void *eol = memchr(start, '\n', beginWrite() - start);
    return static_cast<const char *>(eol);
  }

  /**
   * @brief Retrieves (consumes) a specified number of bytes from the buffer.
   * @param len The number of bytes to retrieve.
   */
  void retrieve(size_t len) {
    assert(len <= readableBytes());
    if (len < readableBytes()) {
      reader_index_ += len;
    } else {
      retrieveAll();
    }
  }

  /**
   * @brief Retrieves data up to a specified end position.
   * @param end The end position.
   */
  void retrieveUntil(const char *end) {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
  }

  /// Retrieves all readable data from the buffer.
  void retrieveAll() {
    reader_index_ = K_CHEAP_PREPEND;
    writer_index_ = K_CHEAP_PREPEND;
  }

  /**
   * @brief Retrieves all readable data as a string.
   * @return A string containing all readable data.
   */
  std::string retrieveAllAsString() {
    return retrieveAsString(readableBytes());
  }

  /**
   * @brief Retrieves a specified number of bytes as a string.
   * @param len The number of bytes to retrieve.
   * @return A string containing the retrieved data.
   */
  std::string retrieveAsString(size_t len) {
    assert(len <= readableBytes());
    std::string result(peek(), len);
    retrieve(len);
    return result;
  }

  /**
   * @brief Converts all readable data to a string.
   * @return A string containing all readable data.
   */
  std::string toString() const { return {peek(), readableBytes()}; }

  /**
   * @brief Appends a string to the buffer.
   * @param str The string to append.
   */
  void append(const std::string &str) { append(str.data(), str.size()); }

  /**
   * @brief Appends data to the buffer.
   * @param data A pointer to the data to append.
   * @param len The length of the data.
   */
  void append(const char *data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
  }

  /**
   * @brief Appends data to the buffer.
   * @param data A pointer to the data to append.
   * @param len The length of the data.
   */
  void append(const void *data, size_t len) {
    append(static_cast<const char *>(data), len);
  }

  /**
   * @brief Ensures that the buffer has enough writable bytes.
   * @param len The number of writable bytes required.
   */
  void ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
      makeSpace(len);
    }
    assert(writableBytes() >= len);
  }

  /**
   * @brief Returns a pointer to the beginning of the writable data.
   * @return A pointer to the writable data.
   */
  char *beginWrite() { return begin() + writer_index_; }
  const char *beginWrite() const { return begin() + writer_index_; }

  /**
   * @brief Updates the write index after writing data.
   * @param len The number of bytes written.
   */
  void hasWritten(size_t len) {
    assert(len <= writableBytes());
    writer_index_ += len;
  }

  /**
   * @brief Reverts the write index by a specified number of bytes.
   * @param len The number of bytes to unwrite.
   */
  void unwrite(size_t len) {
    assert(len <= readableBytes());
    writer_index_ -= len;
  }

  /**
   * @brief Prepends data to the buffer.
   * @param data A pointer to the data to prepend.
   * @param len The length of the data.
   */
  void prepend(const void *data, size_t len) {
    assert(len <= prependableBytes());
    reader_index_ -= len;
    const char *d = static_cast<const char *>(data);
    std::copy(d, d + len, begin() + reader_index_);
  }

  /**
   * @brief Shrinks the buffer to fit its contents plus reserved space.
   * @param reserve The number of bytes to reserve.
   */
  void shrink(size_t reserve) {
    Buffer other;
    other.ensureWritableBytes(readableBytes() + reserve);
    other.append(toString());
    swap(other);
  }

  size_t internalCapacity() const { return buffer_.capacity(); }

  /**
   * @brief Reads data from a file descriptor into the buffer.
   * @param fd The file descriptor to read from.
   * @param savedErrno Pointer to store the saved errno value in case of error.
   * @return The number of bytes read, or -1 in case of error.
   */
  ssize_t readFd(int fd, int *savedErrno);

private:
  char *begin() { return &*buffer_.begin(); }
  const char *begin() const { return &*buffer_.begin(); }

  /**
   * @brief Ensures there is enough space to write data.
   * @param len The number of bytes to ensure space for.
   */
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
