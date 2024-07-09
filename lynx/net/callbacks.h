#ifndef LYNX_NET_CALLBACKS_H
#define LYNX_NET_CALLBACKS_H

#include "lynx/base/timestamp.h"

#include <functional>
#include <memory>

namespace lynx {

template <typename T> inline T *getPointer(const std::shared_ptr<T> &ptr) {
  return ptr.get();
}

template <typename T> inline T *getPointer(const std::unique_ptr<T> &ptr) {
  return ptr.get();
}

// Adapted from google-protobuf stubs/common.h
// see License in lynx/base/Types.h
template <typename To, typename From>
inline ::std::shared_ptr<To> downPointerCast(const ::std::shared_ptr<From> &f) {
#ifndef NDEBUG
  assert(f == NULL || dynamic_cast<To *>(get_pointer(f)) != NULL);
#endif
  return ::std::static_pointer_cast<To>(f);
}

// All client visible callbacks go here.

class Buffer;
class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using TimerCallback = std::function<void()>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using HighWaterMarkCallback =
    std::function<void(const TcpConnectionPtr &, size_t)>;
using MessageCallback =
    std::function<void(const TcpConnectionPtr &, Buffer *, Timestamp)>;

void defaultConnectionCallback(const TcpConnectionPtr &conn);
void defaultMessageCallback(const TcpConnectionPtr &conn, Buffer *buffer,
                            Timestamp receiveTime);

} // namespace lynx

#endif
