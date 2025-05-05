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
 */
class Buffer {
public:
  static const size_t KCheapPrepend = 8;
  static const size_t KInitialSize = 1024;

  /**
   * @brief Constructs a Buffer with an initial size.
   *
   * @param initialSize The initial size of the buffer.
   */
  explicit Buffer(size_t InitialSize = KInitialSize)
      : Buf(KCheapPrepend + InitialSize), ReaderIndex(KCheapPrepend),
        WriterIndex(KCheapPrepend) {
    assert(readableBytes() == 0);
    assert(writableBytes() == InitialSize);
    assert(prependableBytes() == KCheapPrepend);
  }

  void swap(Buffer &Rhs) {
    Buf.swap(Rhs.Buf);
    std::swap(ReaderIndex, Rhs.ReaderIndex);
    std::swap(WriterIndex, Rhs.WriterIndex);
  }

  size_t readableBytes() const { return WriterIndex - ReaderIndex; }
  size_t writableBytes() const { return Buf.size() - WriterIndex; }
  size_t prependableBytes() const { return ReaderIndex; }

  /**
   * @brief Returns a pointer to the beginning of readable data.
   *
   * @return A pointer to the readable data.
   */
  const char *peek() const { return begin() + ReaderIndex; }

  /**
   * @brief Finds the first occurrence of CRLF in the readable data.
   *
   * @return A pointer to the CRLF if found, otherwise nullptr.
   */
  const char *findCRLF() const {
    const char *Crlf = std::search(peek(), beginWrite(), KCRLF, KCRLF + 2);
    return Crlf == beginWrite() ? nullptr : Crlf;
  }

  /**
   * @brief Finds the first occurrence of CRLF starting from a given position.
   *
   * @param start The starting position.
   *
   * @return A pointer to the CRLF if found, otherwise nullptr.
   */
  const char *findCRLF(const char *Start) const {
    assert(peek() <= Start);
    assert(Start <= beginWrite());
    const char *Crlf = std::search(Start, beginWrite(), KCRLF, KCRLF + 2);
    return Crlf == beginWrite() ? nullptr : Crlf;
  }

  /**
   * @brief Finds the first occurrence of EOL in the readable data.
   *
   * @return A pointer to the EOL if found, otherwise nullptr.
   */
  const char *findEOL() const {
    const void *Eol = memchr(peek(), '\n', readableBytes());
    return static_cast<const char *>(Eol);
  }

  /**
   * @brief Finds the first occurrence of EOL starting from a given position.
   *
   * @param start The starting position.
   *
   * @return A pointer to the EOL if found, otherwise nullptr.
   */
  const char *findEOL(const char *Start) const {
    assert(peek() <= Start);
    assert(Start <= beginWrite());
    const void *Eol = memchr(Start, '\n', beginWrite() - Start);
    return static_cast<const char *>(Eol);
  }

  /**
   * @brief Retrieves (consumes) a specified number of bytes from the buffer.
   *
   * @param len The number of bytes to retrieve.
   */
  void retrieve(size_t Len) {
    assert(Len <= readableBytes());
    if (Len < readableBytes()) {
      ReaderIndex += Len;
    } else {
      retrieveAll();
    }
  }

  /**
   * @brief Retrieves data up to a specified end position.
   *
   * @param end The end position.
   */
  void retrieveUntil(const char *End) {
    assert(peek() <= End);
    assert(End <= beginWrite());
    retrieve(End - peek());
  }

  /// Retrieves all readable data from the buffer.
  void retrieveAll() {
    ReaderIndex = KCheapPrepend;
    WriterIndex = KCheapPrepend;
  }

  /**
   * @brief Retrieves all readable data as a string.
   *
   * @return A string containing all readable data.
   */
  std::string retrieveAllAsString() {
    return retrieveAsString(readableBytes());
  }

  /**
   * @brief Retrieves a specified number of bytes as a string.
   *
   * @param len The number of bytes to retrieve.
   *
   * @return A string containing the retrieved data.
   */
  std::string retrieveAsString(size_t Len) {
    assert(Len <= readableBytes());
    std::string Result(peek(), Len);
    retrieve(Len);
    return Result;
  }

  /**
   * @brief Converts all readable data to a string.
   *
   * @return A string containing all readable data.
   */
  std::string toString() const { return {peek(), readableBytes()}; }

  /**
   * @brief Appends a string to the buffer.
   *
   * @param str The string to append.
   */
  void append(const std::string &Str) { append(Str.data(), Str.size()); }

  /**
   * @brief Appends data to the buffer.
   *
   * @param data A pointer to the data to append.
   * @param len The length of the data.
   */
  void append(const char *Data, size_t Len) {
    ensureWritableBytes(Len);
    std::copy(Data, Data + Len, beginWrite());
    hasWritten(Len);
  }

  /**
   * @brief Appends data to the buffer.
   *
   * @param data A pointer to the data to append.
   * @param len The length of the data.
   */
  void append(const void *Data, size_t Len) {
    append(static_cast<const char *>(Data), Len);
  }

  /**
   * @brief Ensures that the buffer has enough writable bytes.
   *
   * @param len The number of writable bytes required.
   */
  void ensureWritableBytes(size_t Len) {
    if (writableBytes() < Len)
      makeSpace(Len);

    assert(writableBytes() >= Len);
  }

  /**
   * @brief Returns a pointer to the beginning of the writable data.
   *
   * @return A pointer to the writable data.
   */
  char *beginWrite() { return begin() + WriterIndex; }
  const char *beginWrite() const { return begin() + WriterIndex; }

  /**
   * @brief Updates the write index after writing data.
   *
   * @param len The number of bytes written.
   */
  void hasWritten(size_t Len) {
    assert(Len <= writableBytes());
    WriterIndex += Len;
  }

  /**
   * @brief Reverts the write index by a specified number of bytes.
   *
   * @param len The number of bytes to unwrite.
   */
  void unwrite(size_t Len) {
    assert(Len <= readableBytes());
    WriterIndex -= Len;
  }

  /**
   * @brief Prepends data to the buffer.
   *
   * @param data A pointer to the data to prepend.
   * @param len The length of the data.
   */
  void prepend(const void *Data, size_t Len) {
    assert(Len <= prependableBytes());
    ReaderIndex -= Len;
    const char *D = static_cast<const char *>(Data);
    std::copy(D, D + Len, begin() + ReaderIndex);
  }

  /**
   * @brief Shrinks the buffer to fit its contents plus reserved space.
   *
   * @param reserve The number of bytes to reserve.
   */
  void shrink(size_t Reserve) {
    Buffer Other;
    Other.ensureWritableBytes(readableBytes() + Reserve);
    Other.append(toString());
    swap(Other);
  }

  size_t internalCapacity() const { return Buf.capacity(); }

  /**
   * @brief Reads data from a file descriptor into the buffer.
   *
   * @param fd The file descriptor to read from.
   * @param savedErrno Pointer to store the saved errno value in case of error.
   *
   * @return The number of bytes read, or -1 in case of error.
   */
  ssize_t readFd(int Fd, int *SavedErrno);

private:
  char *begin() { return &*Buf.begin(); }
  const char *begin() const { return &*Buf.begin(); }

  /**
   * @brief Ensures there is enough space to write data.
   *
   * @param len The number of bytes to ensure space for.
   */
  void makeSpace(size_t Len) {
    if (writableBytes() + prependableBytes() < Len + KCheapPrepend) {
      Buf.resize(WriterIndex + Len);
    } else {
      assert(KCheapPrepend < ReaderIndex);
      size_t Readable = readableBytes();
      std::copy(begin() + ReaderIndex, begin() + WriterIndex,
                begin() + KCheapPrepend);
      ReaderIndex = KCheapPrepend;
      WriterIndex = ReaderIndex + Readable;
      assert(Readable == readableBytes());
    }
  }

  std::vector<char> Buf;
  size_t ReaderIndex;
  size_t WriterIndex;

  static const char KCRLF[];
};

} // namespace lynx

#endif
