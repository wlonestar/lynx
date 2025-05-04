#include "lynx/logger/LogStream.h"

#include <algorithm>

namespace lynx {

namespace detail {

const char digits[] = "9876543210123456789"; // NOLINT
const char *Zero = digits + 9;
static_assert(sizeof(digits) == 20, "wrong number of digits");
const char digits_hex[] = "0123456789ABCDEF"; // NOLINT
static_assert(sizeof(digits_hex) == 17, "wrong number of digitsHex");

/// Efficient Integer to String Conversions, by Matthew Wilson.
template <typename T> size_t convert(char Buf[], T Value) {
  T I = Value;
  char *P = Buf;

  do {
    int Lsd = static_cast<int>(I % 10);
    I /= 10;
    *P++ = Zero[Lsd];
  } while (I != 0);

  if (Value < 0)
    *P++ = '-';

  *P = '\0';
  std::reverse(Buf, P);

  return P - Buf;
}

size_t convertHex(char Buf[], uintptr_t Value) {
  uintptr_t I = Value;
  char *P = Buf;

  do {
    int Lsd = static_cast<int>(I % 16);
    I /= 16;
    *P++ = digits_hex[Lsd];
  } while (I != 0);

  *P = '\0';
  std::reverse(Buf, P);

  return P - Buf;
}

template class FixedBuffer<KSmallBuffer>;
template class FixedBuffer<KLargeBuffer>;

} // namespace detail

template <typename T> void LogStream::formatInteger(T V) {
  if (Buffer.avail() >= KMaxNumericSize) {
    size_t Len = detail::convert(Buffer.current(), V);
    Buffer.add(Len);
  }
}

LogStream &LogStream::operator<<(bool V) {
  if (V) {
    Buffer.append("true", 4);
  } else {
    Buffer.append("false", 5);
  }
  return *this;
}

LogStream &LogStream::operator<<(int16_t V) {
  *this << static_cast<int32_t>(V);
  return *this;
}

LogStream &LogStream::operator<<(uint16_t V) {
  *this << static_cast<uint32_t>(V);
  return *this;
}

LogStream &LogStream::operator<<(int32_t V) {
  formatInteger(V);
  return *this;
}

LogStream &LogStream::operator<<(uint32_t V) {
  formatInteger(V);
  return *this;
}

LogStream &LogStream::operator<<(int64_t V) {
  formatInteger(V);
  return *this;
}

LogStream &LogStream::operator<<(uint64_t V) {
  formatInteger(V);
  return *this;
}

LogStream &LogStream::operator<<(float V) {
  *this << static_cast<double>(V);
  return *this;
}

LogStream &LogStream::operator<<(double V) {
  if (Buffer.avail() >= KMaxNumericSize) {
    int Len = snprintf(Buffer.current(), KMaxNumericSize, "%.12g", V);
    Buffer.add(Len);
  }
  return *this;
}

LogStream &LogStream::operator<<(const void *P) {
  auto V = reinterpret_cast<uintptr_t>(P);
  if (Buffer.avail() >= KMaxNumericSize) {
    char *Buf = Buffer.current();
    Buf[0] = '0';
    Buf[1] = 'x';
    size_t Len = detail::convertHex(Buf + 2, V);
    Buffer.add(Len + 2);
  }
  return *this;
}

LogStream &LogStream::operator<<(char V) {
  Buffer.append(&V, 1);
  return *this;
}

LogStream &LogStream::operator<<(const char *Str) {
  if (Str != nullptr) {
    Buffer.append(Str, strlen(Str));
  } else {
    Buffer.append("(null)", 6);
  }
  return *this;
}

LogStream &LogStream::operator<<(const unsigned char *Str) {
  return operator<<(reinterpret_cast<const char *>(Str));
}

LogStream &LogStream::operator<<(const std::string &Str) {
  Buffer.append(Str.c_str(), Str.size());
  return *this;
}

LogStream &LogStream::operator<<(const BufferTy &V) {
  *this << V.toString();
  return *this;
}

} // namespace lynx
