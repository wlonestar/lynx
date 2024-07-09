#ifndef LYNX_BASE_NONCOPYABLE_H
#define LYNX_BASE_NONCOPYABLE_H

namespace lynx {

class Noncopyable {
public:
  Noncopyable(const Noncopyable &) = delete;

  void operator=(const Noncopyable &) = delete;

protected:
  Noncopyable() = default;
  ~Noncopyable() = default;
};

} // namespace lynx

#endif
