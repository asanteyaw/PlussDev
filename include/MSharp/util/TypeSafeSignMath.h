#pragma once

#include <MSharp/macros/Macros.h>
#include <limits>
#include <type_traits>

C10_CLANG_DIAGNOSTIC_PUSH()
#if C10_CLANG_HAS_WARNING("-Wstring-conversion")
C10_CLANG_DIAGNOSTIC_IGNORE("-Wstring-conversion")
#endif
#if C10_CLANG_HAS_WARNING("-Wimplicit-int-float-conversion")
C10_CLANG_DIAGNOSTIC_IGNORE("-Wimplicit-int-float-conversion")
#endif

namespace MSharp {

template <typename T>
inline constexpr bool is_negative(
    const T& /*x*/,
    std::true_type /*is_unsigned*/) {
  return false;
}

template <typename T>
inline constexpr bool is_negative(const T& x, std::false_type /*is_unsigned*/) {
  return x < T(0);
}

template <typename T>
inline constexpr bool is_negative(const T& x) {
  return is_negative(x, std::is_unsigned<T>());
}

template <typename T>
inline constexpr int signum(const T& x, std::true_type /*is_unsigned*/) {
  return T(0) < x;
}

template <typename T>
inline constexpr int signum(const T& x, std::false_type /*is_unsigned*/) {
  return (T(0) < x) - (x < T(0));
}

template <typename T>
inline constexpr int signum(const T& x) {
  return signum(x, std::is_unsigned<T>());
}

template <typename T, typename U>
inline constexpr bool signs_differ(const T& a, const U& b) {
  return is_negative(a) != is_negative(b);
}

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

template <typename Limit, typename T>
inline constexpr bool greater_than_max(const T& x) {
  constexpr bool can_overflow =
      std::numeric_limits<T>::digits > std::numeric_limits<Limit>::digits;
  return can_overflow && x > std::numeric_limits<Limit>::max();
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

template <typename Limit, typename T>
inline constexpr bool less_than_lowest(
    const T& x,
    std::false_type /*limit_is_unsigned*/,
    std::false_type /*x_is_unsigned*/) {
  return x < std::numeric_limits<Limit>::lowest();
}

template <typename Limit, typename T>
inline constexpr bool less_than_lowest(
    const T& /*x*/,
    std::false_type /*limit_is_unsigned*/,
    std::true_type /*x_is_unsigned*/) {
  return false;
}

template <typename Limit, typename T>
inline constexpr bool less_than_lowest(
    const T& x,
    std::true_type /*limit_is_unsigned*/,
    std::false_type /*x_is_unsigned*/) {
  return x < T(0);
}

template <typename Limit, typename T>
inline constexpr bool less_than_lowest(
    const T& /*x*/,
    std::true_type /*limit_is_unsigned*/,
    std::true_type /*x_is_unsigned*/) {
  return false;
}

template <typename Limit, typename T>
inline constexpr bool less_than_lowest(const T& x) {
  return less_than_lowest<Limit>(
      x, std::is_unsigned<Limit>(), std::is_unsigned<T>());
}

} // namespace MSharp

C10_CLANG_DIAGNOSTIC_POP()
