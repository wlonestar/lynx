#ifndef LYNX_BASE_THREAD_LOCAL_SINGLETON_H
#define LYNX_BASE_THREAD_LOCAL_SINGLETON_H

#include "lynx/base/noncopyable.h"

#include <cassert>
#include <thread>

namespace lynx {

template <typename T> class ThreadLocalSingleton : Noncopyable {
public:
  ThreadLocalSingleton() = delete;
  ~ThreadLocalSingleton() = delete;

  static T &instance() {
    if (!t_value) {
      t_value = new T();
      deleter.set(t_value);
    }
    return *t_value;
  }

  static T *pointer() { return t_value; }

private:
  static void destructor(void *obj) {
    assert(obj == t_value);
    using T_must_be_complete_type = char[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy;
    (void)dummy;
    delete t_value;
    t_value = 0;
  }

  class Deleter {
  public:
    Deleter() = default;
    ~Deleter() {
      if (t_value) {
        destructor(t_value);
      }
    }

    void set(T *newObj) {
      assert(t_value == nullptr);
      t_value = newObj;
    }
  };

  static thread_local T *t_value;
  static Deleter deleter;
};

template <typename T> thread_local T *ThreadLocalSingleton<T>::t_value = 0;
template <typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter;

} // namespace lynx

#endif
