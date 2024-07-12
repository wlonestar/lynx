#ifndef LYNX_ORM_TRAITS_UTIL_H
#define LYNX_ORM_TRAITS_UTIL_H

#include <cstddef>
#include <tuple>
#include <type_traits>

namespace lynx {

template <typename T, typename Tuple> struct HasType {};

///
/// Checks if `T` is the same as each type in `Args`.
///
/// typename `T`: The type we are checking for
/// typename `Args...`: A variadic template parameter pack representing the
/// types contained in the std::tuple
///
/// `std::disjuncion` is a type trait acts like a logical OR over its template
/// parameters.
template <typename T, typename... Args>
struct HasType<T, std::tuple<Args...>>
    : std::disjunction<std::is_same<T, Args>...> {};

template <typename T> struct Identity {};

template <typename T> struct ArraySize;

template <typename T, std::size_t N> struct ArraySize<std::array<T, N>> {
  static constexpr size_t value = N; // NOLINT
};

template <std::size_t N> struct ArraySize<char[N]> {
  static constexpr std::size_t value = N; // NOLINT
};

} // namespace lynx

#endif
