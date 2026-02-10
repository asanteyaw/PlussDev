#pragma once

#include <MSharp/macros/Macros.h>
#include <MSharp/util/BFloat16.h>
#include <MSharp/util/Half.h>

C10_CLANG_DIAGNOSTIC_PUSH()
#if C10_CLANG_HAS_WARNING("-Wimplicit-float-conversion")
C10_CLANG_DIAGNOSTIC_IGNORE("-Wimplicit-float-conversion")
#endif

namespace MSharp {
// TODO: Replace me with inline constexpr variable when C++17 becomes available
namespace detail {
template <typename T>
C10_HOST_DEVICE inline constexpr T e() {
  return static_cast<T>(2.718281828459045235360287471352662);
}

template <typename T>
C10_HOST_DEVICE inline constexpr T euler() {
  return static_cast<T>(0.577215664901532860606512090082402);
}

template <typename T>
C10_HOST_DEVICE inline constexpr T frac_1_pi() {
  return static_cast<T>(0.318309886183790671537767526745028);
}

template <typename T>
C10_HOST_DEVICE inline constexpr T frac_1_sqrt_pi() {
  return static_cast<T>(0.564189583547756286948079451560772);
}

template <typename T>
C10_HOST_DEVICE inline constexpr T frac_sqrt_2() {
  return static_cast<T>(0.707106781186547524400844362104849);
}

template <typename T>
C10_HOST_DEVICE inline constexpr T frac_sqrt_3() {
  return static_cast<T>(0.577350269189625764509148780501957);
}

template <typename T>
C10_HOST_DEVICE inline constexpr T golden_ratio() {
  return static_cast<T>(1.618033988749894848204586834365638);
}

template <typename T>
C10_HOST_DEVICE inline constexpr T ln_10() {
  return static_cast<T>(2.302585092994045684017991454684364);
}

template <typename T>
C10_HOST_DEVICE inline constexpr T ln_2() {
  return static_cast<T>(0.693147180559945309417232121458176);
}

template <typename T>
C10_HOST_DEVICE inline constexpr T log_10_e() {
  return static_cast<T>(0.434294481903251827651128918916605);
}

template <typename T>
C10_HOST_DEVICE inline constexpr T log_2_e() {
  return static_cast<T>(1.442695040888963407359924681001892);
}

template <typename T>
C10_HOST_DEVICE inline constexpr T pi() {
  return static_cast<T>(3.141592653589793238462643383279502);
}

template <typename T>
C10_HOST_DEVICE inline constexpr T sqrt_2() {
  return static_cast<T>(1.414213562373095048801688724209698);
}

template <typename T>
C10_HOST_DEVICE inline constexpr T sqrt_3() {
  return static_cast<T>(1.732050807568877293527446341505872);
}

template <>
C10_HOST_DEVICE inline constexpr BFloat16 pi<BFloat16>() {
  // According to
  // https://en.wikipedia.org/wiki/Bfloat16_floating-point_format#Special_values
  // pi is encoded as 4049
  return BFloat16(0x4049, BFloat16::from_bits());
}

template <>
C10_HOST_DEVICE inline constexpr Half pi<Half>() {
  return Half(0x4248, Half::from_bits());
}
} // namespace detail

template <typename T>
constexpr T e = MSharp::detail::e<T>();

template <typename T>
constexpr T euler = MSharp::detail::euler<T>();

template <typename T>
constexpr T frac_1_pi = MSharp::detail::frac_1_pi<T>();

template <typename T>
constexpr T frac_1_sqrt_pi = MSharp::detail::frac_1_sqrt_pi<T>();

template <typename T>
constexpr T frac_sqrt_2 = MSharp::detail::frac_sqrt_2<T>();

template <typename T>
constexpr T frac_sqrt_3 = MSharp::detail::frac_sqrt_3<T>();

template <typename T>
constexpr T golden_ratio = MSharp::detail::golden_ratio<T>();

template <typename T>
constexpr T ln_10 = MSharp::detail::ln_10<T>();

template <typename T>
constexpr T ln_2 = MSharp::detail::ln_2<T>();

template <typename T>
constexpr T log_10_e = MSharp::detail::log_10_e<T>();

template <typename T>
constexpr T log_2_e = MSharp::detail::log_2_e<T>();

template <typename T>
constexpr T pi = MSharp::detail::pi<T>();

template <typename T>
constexpr T sqrt_2 = MSharp::detail::sqrt_2<T>();

template <typename T>
constexpr T sqrt_3 = MSharp::detail::sqrt_3<T>();
} // namespace MSharp

C10_CLANG_DIAGNOSTIC_POP()
