#ifndef LYNX_ORM_REFLECTION_H
#define LYNX_ORM_REFLECTION_H

#include "lynx/orm/json.h"

#include <algorithm>
#include <array>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace lynx {

#define MACRO_EXPAND(...) __VA_ARGS__

#define MAKE_ARG_LIST_1(op, arg, ...) op(arg)
#define MAKE_ARG_LIST_2(op, arg, ...)                                          \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_1(op, __VA_ARGS__))
#define MAKE_ARG_LIST_3(op, arg, ...)                                          \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_2(op, __VA_ARGS__))
#define MAKE_ARG_LIST_4(op, arg, ...)                                          \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_3(op, __VA_ARGS__))
#define MAKE_ARG_LIST_5(op, arg, ...)                                          \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_4(op, __VA_ARGS__))
#define MAKE_ARG_LIST_6(op, arg, ...)                                          \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_5(op, __VA_ARGS__))
#define MAKE_ARG_LIST_7(op, arg, ...)                                          \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_6(op, __VA_ARGS__))
#define MAKE_ARG_LIST_8(op, arg, ...)                                          \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_7(op, __VA_ARGS__))
#define MAKE_ARG_LIST_9(op, arg, ...)                                          \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_8(op, __VA_ARGS__))
#define MAKE_ARG_LIST_10(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_9(op, __VA_ARGS__))
#define MAKE_ARG_LIST_11(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_10(op, __VA_ARGS__))
#define MAKE_ARG_LIST_12(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_11(op, __VA_ARGS__))
#define MAKE_ARG_LIST_13(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_12(op, __VA_ARGS__))
#define MAKE_ARG_LIST_14(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_13(op, __VA_ARGS__))
#define MAKE_ARG_LIST_15(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_14(op, __VA_ARGS__))
#define MAKE_ARG_LIST_16(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_15(op, __VA_ARGS__))
#define MAKE_ARG_LIST_17(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_16(op, __VA_ARGS__))
#define MAKE_ARG_LIST_18(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_17(op, __VA_ARGS__))
#define MAKE_ARG_LIST_19(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_18(op, __VA_ARGS__))
#define MAKE_ARG_LIST_20(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_19(op, __VA_ARGS__))
#define MAKE_ARG_LIST_21(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_20(op, __VA_ARGS__))
#define MAKE_ARG_LIST_22(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_21(op, __VA_ARGS__))
#define MAKE_ARG_LIST_23(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_22(op, __VA_ARGS__))
#define MAKE_ARG_LIST_24(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_23(op, __VA_ARGS__))
#define MAKE_ARG_LIST_25(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_24(op, __VA_ARGS__))
#define MAKE_ARG_LIST_26(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_25(op, __VA_ARGS__))
#define MAKE_ARG_LIST_27(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_26(op, __VA_ARGS__))
#define MAKE_ARG_LIST_28(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_27(op, __VA_ARGS__))
#define MAKE_ARG_LIST_29(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_28(op, __VA_ARGS__))
#define MAKE_ARG_LIST_30(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_29(op, __VA_ARGS__))
#define MAKE_ARG_LIST_31(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_30(op, __VA_ARGS__))
#define MAKE_ARG_LIST_32(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_31(op, __VA_ARGS__))
#define MAKE_ARG_LIST_33(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_32(op, __VA_ARGS__))
#define MAKE_ARG_LIST_34(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_33(op, __VA_ARGS__))
#define MAKE_ARG_LIST_35(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_34(op, __VA_ARGS__))
#define MAKE_ARG_LIST_36(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_35(op, __VA_ARGS__))
#define MAKE_ARG_LIST_37(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_36(op, __VA_ARGS__))
#define MAKE_ARG_LIST_38(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_37(op, __VA_ARGS__))
#define MAKE_ARG_LIST_39(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_38(op, __VA_ARGS__))
#define MAKE_ARG_LIST_40(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_39(op, __VA_ARGS__))
#define MAKE_ARG_LIST_41(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_40(op, __VA_ARGS__))
#define MAKE_ARG_LIST_42(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_41(op, __VA_ARGS__))
#define MAKE_ARG_LIST_43(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_42(op, __VA_ARGS__))
#define MAKE_ARG_LIST_44(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_43(op, __VA_ARGS__))
#define MAKE_ARG_LIST_45(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_44(op, __VA_ARGS__))
#define MAKE_ARG_LIST_46(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_45(op, __VA_ARGS__))
#define MAKE_ARG_LIST_47(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_46(op, __VA_ARGS__))
#define MAKE_ARG_LIST_48(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_47(op, __VA_ARGS__))
#define MAKE_ARG_LIST_49(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_48(op, __VA_ARGS__))
#define MAKE_ARG_LIST_50(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_49(op, __VA_ARGS__))
#define MAKE_ARG_LIST_51(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_50(op, __VA_ARGS__))
#define MAKE_ARG_LIST_52(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_51(op, __VA_ARGS__))
#define MAKE_ARG_LIST_53(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_52(op, __VA_ARGS__))
#define MAKE_ARG_LIST_54(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_53(op, __VA_ARGS__))
#define MAKE_ARG_LIST_55(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_54(op, __VA_ARGS__))
#define MAKE_ARG_LIST_56(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_55(op, __VA_ARGS__))
#define MAKE_ARG_LIST_57(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_56(op, __VA_ARGS__))
#define MAKE_ARG_LIST_58(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_57(op, __VA_ARGS__))
#define MAKE_ARG_LIST_59(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_58(op, __VA_ARGS__))
#define MAKE_ARG_LIST_60(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_59(op, __VA_ARGS__))
#define MAKE_ARG_LIST_61(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_60(op, __VA_ARGS__))
#define MAKE_ARG_LIST_62(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_61(op, __VA_ARGS__))
#define MAKE_ARG_LIST_63(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_62(op, __VA_ARGS__))
#define MAKE_ARG_LIST_64(op, arg, ...)                                         \
  op(arg), MACRO_EXPAND(MAKE_ARG_LIST_63(op, __VA_ARGS__))

#define ADD_VIEW(str) std::string_view(#str, sizeof(#str) - 1)
#define SEPERATOR ,

#define CON_STR_1(elem, ...) ADD_VIEW(elem)
#define CON_STR_2(elem, ...)                                                   \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_1(__VA_ARGS__))
#define CON_STR_3(elem, ...)                                                   \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_2(__VA_ARGS__))
#define CON_STR_4(elem, ...)                                                   \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_3(__VA_ARGS__))
#define CON_STR_5(elem, ...)                                                   \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_4(__VA_ARGS__))
#define CON_STR_6(elem, ...)                                                   \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_5(__VA_ARGS__))
#define CON_STR_7(elem, ...)                                                   \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_6(__VA_ARGS__))
#define CON_STR_8(elem, ...)                                                   \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_7(__VA_ARGS__))
#define CON_STR_9(elem, ...)                                                   \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_8(__VA_ARGS__))
#define CON_STR_10(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_9(__VA_ARGS__))
#define CON_STR_11(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_10(__VA_ARGS__))
#define CON_STR_12(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_11(__VA_ARGS__))
#define CON_STR_13(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_12(__VA_ARGS__))
#define CON_STR_14(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_13(__VA_ARGS__))
#define CON_STR_15(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_14(__VA_ARGS__))
#define CON_STR_16(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_15(__VA_ARGS__))
#define CON_STR_17(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_16(__VA_ARGS__))
#define CON_STR_18(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_17(__VA_ARGS__))
#define CON_STR_19(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_18(__VA_ARGS__))
#define CON_STR_20(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_19(__VA_ARGS__))
#define CON_STR_21(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_20(__VA_ARGS__))
#define CON_STR_22(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_21(__VA_ARGS__))
#define CON_STR_23(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_22(__VA_ARGS__))
#define CON_STR_24(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_23(__VA_ARGS__))
#define CON_STR_25(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_24(__VA_ARGS__))
#define CON_STR_26(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_25(__VA_ARGS__))
#define CON_STR_27(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_26(__VA_ARGS__))
#define CON_STR_28(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_27(__VA_ARGS__))
#define CON_STR_29(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_28(__VA_ARGS__))
#define CON_STR_30(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_29(__VA_ARGS__))
#define CON_STR_31(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_30(__VA_ARGS__))
#define CON_STR_32(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_31(__VA_ARGS__))
#define CON_STR_33(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_32(__VA_ARGS__))
#define CON_STR_34(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_33(__VA_ARGS__))
#define CON_STR_35(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_34(__VA_ARGS__))
#define CON_STR_36(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_35(__VA_ARGS__))
#define CON_STR_37(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_36(__VA_ARGS__))
#define CON_STR_38(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_37(__VA_ARGS__))
#define CON_STR_39(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_38(__VA_ARGS__))
#define CON_STR_40(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_39(__VA_ARGS__))
#define CON_STR_41(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_40(__VA_ARGS__))
#define CON_STR_42(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_41(__VA_ARGS__))
#define CON_STR_43(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_42(__VA_ARGS__))
#define CON_STR_44(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_43(__VA_ARGS__))
#define CON_STR_45(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_44(__VA_ARGS__))
#define CON_STR_46(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_45(__VA_ARGS__))
#define CON_STR_47(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_46(__VA_ARGS__))
#define CON_STR_48(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_47(__VA_ARGS__))
#define CON_STR_49(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_48(__VA_ARGS__))
#define CON_STR_50(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_49(__VA_ARGS__))
#define CON_STR_51(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_50(__VA_ARGS__))
#define CON_STR_52(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_51(__VA_ARGS__))
#define CON_STR_53(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_52(__VA_ARGS__))
#define CON_STR_54(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_53(__VA_ARGS__))
#define CON_STR_55(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_54(__VA_ARGS__))
#define CON_STR_56(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_55(__VA_ARGS__))
#define CON_STR_57(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_56(__VA_ARGS__))
#define CON_STR_58(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_57(__VA_ARGS__))
#define CON_STR_59(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_58(__VA_ARGS__))
#define CON_STR_60(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_59(__VA_ARGS__))
#define CON_STR_61(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_60(__VA_ARGS__))
#define CON_STR_62(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_61(__VA_ARGS__))
#define CON_STR_63(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_62(__VA_ARGS__))
#define CON_STR_64(elem, ...)                                                  \
  ADD_VIEW(elem) SEPERATOR MACRO_EXPAND(CON_STR_63(__VA_ARGS__))

/// Get args size (less than 65 args)
#define MACRO_FILTER(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12,    \
                     _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23,    \
                     _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34,    \
                     _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45,    \
                     _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56,    \
                     _57, _58, _59, _60, _61, _62, _63, _64, _N, ...)          \
  _N
#define REC_N()                                                                \
  64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46,  \
      45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28,  \
      27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10,  \
      9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define MACRO_ARGS_INNER(...) MACRO_FILTER(__VA_ARGS__)
#define MACRO_ARGS_SIZE(...) MACRO_ARGS_INNER(0, ##__VA_ARGS__, REC_N())

#define MACRO_CONCAT(a, b) a##_##b
#define FD(f) f

#define MAKE_ARG_LIST(N, op, arg, ...)                                         \
  MACRO_CONCAT(MAKE_ARG_LIST, N)(op, arg, __VA_ARGS__)

/**
 * @brief A macro used to define the reflection members of a class.
 *
 * @param class_name The name of the class whose members are being reflected.
 * @param ... A variadic parameter pack representing the members of the class.
 *
 * @return A struct containing the reflection members of the class.
 *
 * The struct contains the following members:
 * - `applyImpl`: Returns a tuple of the class members.
 * - `size_type`: A type alias representing the number of members.
 * - `name`: Returns the name of the class as a string view.
 * - `struct_name`: Returns the name of the class as a string view.
 * - `fields`: Returns the field names of the class as a string view.
 * - `value`: Returns the number of members by accessing `size_type::value`.
 * - `arr`: Returns an array of string views representing the field names.
 */
#define MAKE_REFLECT_MEMBERS(class_name, ...)                                  \
  inline auto reflectMembersFunc(class_name const &) {                         \
    struct ReflectMembers {                                                    \
      constexpr decltype(auto) static applyImpl() {                            \
        return std::make_tuple(__VA_ARGS__);                                   \
      }                                                                        \
      using size_type =                                                        \
          std::integral_constant<size_t, MACRO_ARGS_SIZE(__VA_ARGS__)>;        \
      constexpr static std::string_view name() { return name_##class_name; }   \
      constexpr static std::string_view struct_name() {                        \
        return std::string_view(#class_name, sizeof(#class_name) - 1);         \
      }                                                                        \
      constexpr static std::string_view fields() {                             \
        return fields_##class_name;                                            \
      }                                                                        \
      constexpr static size_t value() { return size_type::value; }             \
      constexpr static std::array<std::string_view, size_type::value> arr() {  \
        return arr_##class_name;                                               \
      }                                                                        \
    };                                                                         \
    return ReflectMembers{};                                                   \
  }

/**
 * This macro is used to define metadata for a class.
 *
 * @param class_name The name of the class.
 * @param table_name A string representing the table name, used for metadata
 * purposes.
 * @param N The number of class members.
 * @param ... Variadic arguments representing the class members.
 */
#define MAKE_META_DATA(class_name, table_name, N, ...)                         \
  constexpr std::array<std::string_view, N> arr_##class_name = {               \
      MACRO_EXPAND(MACRO_CONCAT(CON_STR, N)(__VA_ARGS__))};                    \
  constexpr std::string_view fields_##class_name = {#__VA_ARGS__};             \
  constexpr std::string_view name_##class_name = table_name;                   \
  MAKE_REFLECT_MEMBERS(class_name,                                             \
                       MAKE_ARG_LIST(N, &class_name::FD, __VA_ARGS__))

/**
 * @brief This macro defines a function named `to_json` which takes a
 * `nlohmann::json` object and a constant reference to an object of type `T`.
 * It serializes the data members of `T` into the `json` object.
 *
 * @param T The type of object to serialize.
 */
#define TO_JSON(T)                                                             \
  /* NOLINTNEXTLINE */                                                         \
  void to_json(lynx::json &j, const T &t) {                                    \
    lynx::forEach(                                                             \
        t, [&t, &j](auto item, auto field, auto i) { j[field] = t.*item; });   \
  }

/**
 * @brief This macro defines a function named `from_json` which takes a
 * constant reference to a `nlohmann::json` object and a non-constant reference
 * to an object of type `T`. It deserializes the data members of `T` from the
 * `json` object.
 *
 * @param T The type of object to deserialize.
 */
#define FROM_JSON(T)                                                           \
  /* NOLINTNEXTLINE */                                                         \
  void from_json(const lynx::json &j, T &t) {                                  \
    lynx::forEach(t, [&t, &j](auto item, auto field, auto i) {                 \
      j.at(field).get_to(t.*item);                                             \
    });                                                                        \
  }

#define JSON_SERIALIZE(T)                                                      \
  TO_JSON(T);                                                                  \
  FROM_JSON(T);

#define REFLECTION_TEMPLATE(class_name, ...)                                   \
  MAKE_META_DATA(class_name, #class_name, MACRO_ARGS_SIZE(__VA_ARGS__),        \
                 __VA_ARGS__)                                                  \
  JSON_SERIALIZE(class_name)

#define REFLECTION_TEMPLATE_WITH_NAME(class_name, table_name, ...)             \
  MAKE_META_DATA(class_name, table_name, MACRO_ARGS_SIZE(__VA_ARGS__),         \
                 __VA_ARGS__)                                                  \
  JSON_SERIALIZE(class_name)

template <typename T>
using reflect_members = decltype(reflectMembersFunc(std::declval<T>()));

/**
 * @brief Trait class to check if a type has reflection capabilities.
 *
 * This class template is used to determine if a type `T` has reflection
 * capabilities. It provides a static member constant `value` which is of type
 * `bool` and is `true` if `T` is considered reflective, and `false` otherwise.
 *
 * @tparam T The type to check for reflection capabilities.
 * @tparam Enable A placeholder template parameter. It is not used in the class
 * definition but is used to enable partial specialization.
 */
template <typename T, typename = void>
struct is_reflection : std::false_type {}; // NOLINT

/**
 * @brief Partial specialization of `is_reflection` for types `T` that have
 * reflection capabilities.
 *
 * This specialization of `is_reflection` uses
 * `std::void_t<decltype(reflect_members<T>::arr())>` as the second template
 * parameter. `std::void_t` is a helper template that converts any valid type
 * expression into `void`.
 *
 * If `decltype(reflect_members<T>::arr())` is valid, `std::void_t` converts it
 * into `void`, making the specialization applicable, indicating that the type
 * `T` is considered reflective.
 *
 * @tparam T The type to check for reflection capabilities.
 */
template <typename T>
struct is_reflection<T, std::void_t<decltype(reflect_members<T>::arr())>>
    : std::true_type {};

/**
 * @brief Trait class template to check if a given type `T` is an instantiation
 * of a template class `U`.
 *
 * This trait class template is used to check if a type `T` is an instantiation
 * of a template class `U`. It provides a static member constant `value` of type
 * `bool`, which is `true` if `T` is an instantiation of `U`, and `false`
 * otherwise.
 *
 * @tparam U The template class to check against.
 * @tparam T The type to check for instantiation.
 */
template <template <typename...> class U, typename T>
struct is_template_instant_of : std::false_type {}; // NOLINT

/**
 * @brief Partial specialization of `is_template_instant_of` for types `T` that
 * are an instantiation of `U`.
 *
 * This partial specialization of `is_template_instant_of` is used to check if a
 * type `T` is an instantiation of the template class `U`. It defines `value` as
 * `true` for the given specialization.
 *
 * @tparam U The template class to check against.
 * @tparam args... The variadic template parameter pack representing the
 * argument(s) of the template class.
 */
template <template <typename...> class U, typename... args>
struct is_template_instant_of<U, U<args...>> : std::true_type {};

template <typename T>
struct is_stdstring : is_template_instant_of<std::basic_string, T> {}; // NOLINT

template <typename T>
struct is_tuple : is_template_instant_of<std::tuple, T> {}; // NOLINT

template <typename T>
inline constexpr bool is_reflection_v = is_reflection<T>::value; // NOLINT

/**
 * @brief `forEach` function with array
 *
 * This function applies the callable `f` to each element of the tuple `t` and
 * each corresponding element of the array `arr`.
 *
 * @tparam Args variadic template parameters representing the types of the
 * elements of the tuple.
 * @tparam A the type of the array.
 * @tparam F the type of the callable.
 * @tparam Idx the index sequence.
 * @param t the tuple to iterate over.
 * @param arr the array to iterate over.
 * @param f the callable to apply to each element.
 * @param unused unused parameter, used to disambiguate the function.
 */
template <typename... Args, typename A, typename F, std::size_t... Idx>
constexpr void forEach(const std::tuple<Args...> &t, const A &arr, F &&f,
                       std::index_sequence<Idx...> /*unused*/) {
  /// Apply the callable `f` to each element of the tuple and each corresponding
  /// element of the array.
  (std::forward<F>(f)(std::get<Idx>(t), arr[Idx],
                      std::integral_constant<size_t, Idx>{}),
   ...);
}

/**
 * @brief `forEach` function without array
 *
 * This function applies the callable `f` to each element of the tuple `t`.
 *
 * @tparam Args variadic template parameters representing the types of the
 * elements of the tuple.
 * @tparam F the type of the callable.
 * @tparam Idx the index sequence.
 * @param t the tuple to iterate over.
 * @param f the callable to apply to each element.
 * @param unused unused parameter, used to disambiguate the function.
 */
template <typename... Args, typename F, std::size_t... Idx>
constexpr void forEach(std::tuple<Args...> &t, F &&f,
                       std::index_sequence<Idx...> /*unused*/) {
  /// Apply the callable `f` to each element of the tuple.
  (std::forward<F>(f)(std::get<Idx>(t), std::integral_constant<size_t, Idx>{}),
   ...);
}

/**
 * @brief `forEach` function for reflective types
 *
 * This function applies the callable `f` to each element of the tuple resulting
 * from `reflectMembersFunc(t)` and each corresponding element of the array
 * resulting from `M::arr()`.
 *
 * @tparam T the type to check for reflections.
 * @tparam F the type of the callable.
 */
template <typename T, typename F>
constexpr std::enable_if_t<is_reflection<T>::value> forEach(T &&t, F &&f) {
  using M = decltype(reflectMembersFunc(std::forward<T>(t)));
  /// Apply the callable `f` to each element of the tuple resulting from
  /// `reflectMembersFunc(t)` and each corresponding element of the array
  /// resulting from `M::arr()`.
  forEach(M::applyImpl(), M::arr(), std::forward<F>(f),
          std::make_index_sequence<M::value()>{});
}

/**
 * @brief `forEach` function for tuple
 *
 * This function applies the callable `f` to each element of the tuple `t`.
 *
 * @tparam T the type of the tuple.
 * @tparam F the type of the callable.
 */
template <typename T, typename F>
constexpr std::enable_if_t<!is_reflection<T>::value &&
                           is_tuple<std::decay_t<T>>::value>
forEach(T &&t, F &&f) {
  /// Apply the callable `f` to each element of the tuple `t`.
  forEach(std::forward<T>(t), std::forward<F>(f),
          std::make_index_sequence<std::tuple_size_v<std::decay_t<T>>>{});
}

template <typename T>
constexpr void setParamValues(std::ostream &os, const std::string_view &field,
                              T &&value, size_t /*idx*/) {
  os << field << ":" << value << " ";
}

template <typename T> std::string serialize(T &t) {
  std::stringstream ss;
  forEach(t, [&t, &ss](auto item, auto field, auto i) {
    setParamValues(ss, field, t.*item, i);
  });
  return ss.str();
}

template <typename T>
constexpr typename std::enable_if<!is_reflection<T>::value, std::size_t>::type
getValue() {
  return 0;
}

template <typename T>
constexpr typename std::enable_if<is_reflection<T>::value, std::size_t>::type
getValue() {
  using M = decltype(reflectMembersFunc(std::declval<T>()));
  return M::value();
}

template <typename T> constexpr std::size_t getIndex(std::string_view field) {
  using M = decltype(reflectMembersFunc(std::declval<T>()));
  auto arr = M::arr();
  auto it = std::find_if(arr.begin(), arr.end(), [&field](auto f) {
    return std::string_view(f) == field;
  });
  return std::distance(arr.begin(), it);
}

template <typename T> constexpr auto getArray() {
  using M = decltype(reflectMembersFunc(std::declval<T>()));
  return M::arr();
}

template <typename T> constexpr std::string_view getField() {
  using M = decltype(reflectMembersFunc(std::declval<T>()));
  return M::fields();
}

template <typename T> constexpr auto getName() {
  using M = decltype(reflectMembersFunc(std::declval<T>()));
  return M::name();
}

template <typename T> constexpr auto getName(size_t idx) {
  using M = decltype(reflectMembersFunc(std::declval<T>()));
  return M::arr()[idx];
}

template <typename T, std::size_t I> constexpr auto getName() {
  using M = decltype(reflectMembersFunc(std::declval<T>()));
  static_assert(I < M::value(), "index out of range");
  return M::arr()[I];
}

template <typename T> std::string_view getNameImpl(const T &t, std::size_t i) {
  return getName<T>(i);
}

template <size_t I, typename T> constexpr decltype(auto) get(T &&t) {
  using M = decltype(reflectMembersFunc(std::forward<T>(t)));
  using U = decltype(std::forward<T>(t).*(std::get<I>(M::applyImpl())));

  if constexpr (std::is_array_v<U>) {
    auto s = std::forward<T>(t).*(std::get<I>(M::applyImpl()));
    std::array<char, sizeof(U)> arr;
    memcpy(arr.data(), s, arr.size());
    return arr;
  } else {
    return std::forward<T>(t).*(std::get<I>(M::applyImpl()));
  }
}

} // namespace lynx

#endif
