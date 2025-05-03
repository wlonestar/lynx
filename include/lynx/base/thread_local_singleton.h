#ifndef LYNX_BASE_THREAD_LOCAL_SINGLETON_H
#define LYNX_BASE_THREAD_LOCAL_SINGLETON_H

#include "lynx/base/noncopyable.h"

#include <cassert>
#include <thread>

namespace lynx {

/**
 * @class ThreadLocalSingleton
 * @brief Singleton class with thread local storage.
 *
 * The ThreadLocalSingleton class is a template class that provides a
 * thread-local singleton instance of a given type. It ensures that there is
 * only one instance of the class per thread.
 *
 * @tparam T The type of the singleton instance.
 */
template <typename T> class ThreadLocalSingleton : Noncopyable {
public:
  ThreadLocalSingleton() = delete;
  ~ThreadLocalSingleton() = delete;

  /**
   * @brief Returns the thread-local singleton instance.
   *
   * If the instance does not exist, it is created and stored in thread-local
   * storage.
   *
   * @return Reference to the thread-local singleton instance.
   */
  static T &instance() {
    if (!t_value) {
      t_value = new T();
      deleter.set(t_value);
    }
    return *t_value;
  }

  /**
   * @brief Returns a pointer to the thread-local singleton instance.
   *
   * @return Pointer to the thread-local singleton instance.
   */
  static T *pointer() { return t_value; }

private:
  /**
   * @brief Deletes the thread-local singleton instance.
   *
   * @param obj Pointer to the thread-local singleton instance.
   */
  static void destructor(void *obj) {
    assert(obj == t_value);
    using T_must_be_complete_type = char[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy;
    (void)dummy;
    delete t_value;
    t_value = 0;
  }

  /**
   * @class Deleter
   * @brief Helper class to delete the thread-local singleton instance.
   *
   * The Deleter class is used to delete the thread-local singleton instance
   * when the thread exits.
   */
  class Deleter {
  public:
    Deleter() = default;
    ~Deleter() {
      if (t_value) {
        destructor(t_value);
      }
    }

    /**
     * @brief Sets the pointer to the thread-local singleton instance.
     *
     * @param newObj Pointer to the thread-local singleton instance.
     */
    void set(T *newObj) {
      assert(t_value == nullptr);
      t_value = newObj;
    }
  };

  /// Thread-local storage for the singleton instance.
  static thread_local T *t_value;
  /// Deleter for the thread-local singleton instance.
  static Deleter deleter;
};

// Instantiate the thread-local storage and deleter
template <typename T> thread_local T *ThreadLocalSingleton<T>::t_value = 0;
template <typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter;

} // namespace lynx

#endif
