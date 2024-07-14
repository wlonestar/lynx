#ifndef LYNX_BASE_WEAK_CALLBACK_H
#define LYNX_BASE_WEAK_CALLBACK_H

#include <functional>
#include <memory>

namespace lynx {

template <typename Class, typename... Args> class WeakCallback {
public:
  WeakCallback(const std::weak_ptr<Class> &object,
               const std::function<void(Class *, Args...)> &function)
      : object_(object), function_(function) {}

  void operator()(Args &&...args) const {
    std::shared_ptr<Class> ptr(object_.lock());
    if (ptr) {
      function_(ptr.get(), std::forward<Args>(args)...);
    }
  }

private:
  std::weak_ptr<Class> object_;
  std::function<void(Class *, Args...)> function_;
};

template <typename Class, typename... Args>
WeakCallback<Class, Args...>
makeWeakCallback(const std::shared_ptr<Class> &object,
                 void (Class::*function)(Args...)) {
  return WeakCallback<Class, Args...>(object, function);
}

template <typename Class, typename... Args>
WeakCallback<Class, Args...>
makeWeakCallback(const std::shared_ptr<Class> &object,
                 void (Class::*function)(Args...) const) {
  return WeakCallback<Class, Args...>(object, function);
}

} // namespace lynx

#endif
