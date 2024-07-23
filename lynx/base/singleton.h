#ifndef LYNX_BASE_SINGLETON_H
#define LYNX_BASE_SINGLETON_H

#include "lynx/base/noncopyable.h"

#include <cassert>
#include <cstdlib>
#include <mutex>
#include <thread>

namespace lynx {

namespace detail {

template <typename T> struct HasNoDestroy {
  template <typename C> static char test(decltype(&C::no_destroy));
  template <typename C> static int32_t test(...);
  const static bool value = sizeof(test<T>(0)) == 1; // NOLINT
};

} // namespace detail

template <typename T> class Singleton : Noncopyable {
public:
  Singleton() = delete;
  ~Singleton() = delete;

  static T &instance() {
    std::call_once(once, &Singleton::init);
    assert(value != nullptr);
    return *value;
  }

private:
  static void init() {
    value = new T();
    if (!detail::HasNoDestroy<T>::value) {
      ::atexit(destroy);
    }
  }

  static void destroy() {
    using T_must_be_complete_type = char[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy;
    (void)dummy;

    delete value;
    value = nullptr;
  }

  static std::once_flag once;
  static T *value;
};

template <typename T> std::once_flag Singleton<T>::once;
template <typename T> T *Singleton<T>::value = nullptr;

} // namespace lynx

#endif