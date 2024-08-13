#ifndef LYNX_ORM_TRAITS_UTIL_H
#define LYNX_ORM_TRAITS_UTIL_H

#include <cstddef>
#include <tuple>
#include <type_traits>

namespace lynx {

/**
 * @brief A struct template that takes a type and a tuple, and defines a static
 * member constant `value` of type `bool`, which is true if `T` is the same as
 * at least one type in the tuple, and false otherwise.
 *
 * @tparam T the type to check for
 * @tparam Tuple a tuple of types
 */
template <typename T, typename Tuple> struct HasType {};

/**
 * @brief Specialization of HasType for a tuple of types `Args`.
 *
 * @tparam T the type to check for
 * @tparam Args... variadic template parameter pack representing the types
 * contained in the tuple
 *
 * `std::disjunction` is a type trait that acts like a logical OR over its
 * template parameters.
 */
template <typename T, typename... Args>
struct HasType<T, std::tuple<Args...>>
    : std::disjunction<std::is_same<T, Args>...> {};

/**
 * @brief A struct template that takes a type `T` and defines a nested type
 * `type` which is an alias for `T`.
 *
 * @tparam T the type to alias
 */
template <typename T> struct Identity { using type = T; };

/**
 * @brief A struct template that takes a type `T` and defines a static member
 * constant `value` of type `std::size_t` which is the size of the array `T`.
 */
template <typename T> struct ArraySize;

/**
 * @brief Specialization of ArraySize for std::array.
 *
 * @tparam T the type of the elements in the array
 * @tparam N the size of the array
 */
template <typename T, std::size_t N> struct ArraySize<std::array<T, N>> {
  static constexpr std::size_t value = N; // NOLINT
};

/**
 * @brief Specialization of ArraySize for a char array.
 *
 * @tparam N the size of the array
 */
template <std::size_t N> struct ArraySize<char[N]> {
  static constexpr std::size_t value = N; // NOLINT
};

} // namespace lynx

#endif
