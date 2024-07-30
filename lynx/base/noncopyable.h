#ifndef LYNX_BASE_NONCOPYABLE_H
#define LYNX_BASE_NONCOPYABLE_H

namespace lynx {

/// \brief Base class that disables copying and assignment.
///
/// This class is designed to be inherited by other classes that should not be
/// copied or assigned.
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
