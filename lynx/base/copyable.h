#ifndef LYNX_BASE_COPYABLE_H
#define LYNX_BASE_COPYABLE_H

namespace lynx {

/// A tag class emphasises the objects are copyable.
/// The empty base class optimization applies.
/// Any derived class of Copyable should be a value type.
class Copyable {
protected:
  Copyable() = default;
  ~Copyable() = default;
};

} // namespace lynx

#endif
