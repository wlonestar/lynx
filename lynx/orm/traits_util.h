#ifndef LYNX_ORM_TRAITS_UTIL_H
#define LYNX_ORM_TRAITS_UTIL_H

#include <cstddef>
#include <tuple>
#include <type_traits>

namespace lynx {

template <typename T, typename Tuple> struct HasType {};

template <typename T, typename... Args>
struct HasType<T, std::tuple<Args...>>
    : std::disjunction<std::is_same<T, Args>...> {};

template <typename T> struct Identity {};

template <typename T> struct ArraySize;

template <typename T, std::size_t N> struct ArraySize<std::array<T, N>> {
  static constexpr size_t value = N;
};

template <std::size_t N> struct ArraySize<char[N]> {
  static constexpr std::size_t value = N;
};

} // namespace lynx

#endif
