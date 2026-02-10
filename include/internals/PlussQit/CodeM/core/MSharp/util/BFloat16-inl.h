#pragma once

#include <MSharp/macros/Macros.h>
#include <MSharp/util/bit_cast.h>

#include <limits>

C10_CLANG_DIAGNOSTIC_PUSH()
#if C10_CLANG_HAS_WARNING("-Wimplicit-int-float-conversion")
C10_CLANG_DIAGNOSTIC_IGNORE("-Wimplicit-int-float-conversion")
#endif

#if defined(SYCL_EXT_ONEAPI_BFLOAT16_MATH_FUNCTIONS)
#if defined(CL_SYCL_LANGUAGE_VERSION)
#include <CL/sycl.hpp> // for SYCL 1.2.1
#else
#include <sycl/sycl.hpp> // for SYCL 2020
#endif
#include <ext/oneapi/bfloat16.hpp>
#endif

namespace MSharp {

/// Constructors
inline C10_HOST_DEVICE BFloat16::BFloat16(float value)
    :
#if defined(__CUDACC__) && !defined(USE_ROCM) && defined(__CUDA_ARCH__) && \
    __CUDA_ARCH__ >= 800
      x(__bfloat16_as_ushort(__float2bfloat16(value)))
#elif defined(__SYCL_DEVICE_ONLY__) && \
    defined(SYCL_EXT_ONEAPI_BFLOAT16_MATH_FUNCTIONS)
      x(MSharp::bit_cast<uint16_t>(sycl::ext::oneapi::bfloat16(value)))
#else
      // RNE by default
      x(detail::round_to_nearest_even(value))
#endif
{
}

/// Implicit conversions
inline C10_HOST_DEVICE BFloat16::operator float() const {
#if defined(__CUDACC__) && !defined(USE_ROCM)
  return __bfloat162float(*reinterpret_cast<const __nv_bfloat16*>(&x));
#elif defined(__SYCL_DEVICE_ONLY__) && \
    defined(SYCL_EXT_ONEAPI_BFLOAT16_MATH_FUNCTIONS)
  return float(*reinterpret_cast<const sycl::ext::oneapi::bfloat16*>(&x));
#else
  return detail::f32_from_bits(x);
#endif
}

#if defined(__CUDACC__) && !defined(USE_ROCM)
inline C10_HOST_DEVICE BFloat16::BFloat16(const __nv_bfloat16& value) {
  x = *reinterpret_cast<const unsigned short*>(&value);
}
inline C10_HOST_DEVICE BFloat16::operator __nv_bfloat16() const {
  return *reinterpret_cast<const __nv_bfloat16*>(&x);
}
#endif

#if defined(SYCL_EXT_ONEAPI_BFLOAT16_MATH_FUNCTIONS)
inline C10_HOST_DEVICE BFloat16::BFloat16(
    const sycl::ext::oneapi::bfloat16& value) {
  x = *reinterpret_cast<const unsigned short*>(&value);
}
inline C10_HOST_DEVICE BFloat16::operator sycl::ext::oneapi::bfloat16() const {
  return *reinterpret_cast<const sycl::ext::oneapi::bfloat16*>(&x);
}
#endif

// CUDA intrinsics

#if defined(__CUDACC__) || defined(__HIPCC__)
inline C10_DEVICE BFloat16 __ldg(const BFloat16* ptr) {
#if !defined(USE_ROCM) && defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 800
  return __ldg(reinterpret_cast<const __nv_bfloat16*>(ptr));
#else
  return *ptr;
#endif
}
#endif

/// Arithmetic

inline C10_HOST_DEVICE BFloat16
operator+(const BFloat16& a, const BFloat16& b) {
  return static_cast<float>(a) + static_cast<float>(b);
}

inline C10_HOST_DEVICE BFloat16
operator-(const BFloat16& a, const BFloat16& b) {
  return static_cast<float>(a) - static_cast<float>(b);
}

inline C10_HOST_DEVICE BFloat16
operator*(const BFloat16& a, const BFloat16& b) {
  return static_cast<float>(a) * static_cast<float>(b);
}

inline C10_HOST_DEVICE BFloat16 operator/(const BFloat16& a, const BFloat16& b)
    __ubsan_ignore_float_divide_by_zero__ {
  return static_cast<float>(a) / static_cast<float>(b);
}

inline C10_HOST_DEVICE BFloat16 operator-(const BFloat16& a) {
  return -static_cast<float>(a);
}

inline C10_HOST_DEVICE BFloat16& operator+=(BFloat16& a, const BFloat16& b) {
  a = a + b;
  return a;
}

inline C10_HOST_DEVICE BFloat16& operator-=(BFloat16& a, const BFloat16& b) {
  a = a - b;
  return a;
}

inline C10_HOST_DEVICE BFloat16& operator*=(BFloat16& a, const BFloat16& b) {
  a = a * b;
  return a;
}

inline C10_HOST_DEVICE BFloat16& operator/=(BFloat16& a, const BFloat16& b) {
  a = a / b;
  return a;
}

inline C10_HOST_DEVICE BFloat16& operator|(BFloat16& a, const BFloat16& b) {
  a.x = a.x | b.x;
  return a;
}

inline C10_HOST_DEVICE BFloat16& operator^(BFloat16& a, const BFloat16& b) {
  a.x = a.x ^ b.x;
  return a;
}

inline C10_HOST_DEVICE BFloat16& operator&(BFloat16& a, const BFloat16& b) {
  a.x = a.x & b.x;
  return a;
}

/// Arithmetic with floats

inline C10_HOST_DEVICE float operator+(BFloat16 a, float b) {
  return static_cast<float>(a) + b;
}
inline C10_HOST_DEVICE float operator-(BFloat16 a, float b) {
  return static_cast<float>(a) - b;
}
inline C10_HOST_DEVICE float operator*(BFloat16 a, float b) {
  return static_cast<float>(a) * b;
}
inline C10_HOST_DEVICE float operator/(BFloat16 a, float b) {
  return static_cast<float>(a) / b;
}

inline C10_HOST_DEVICE float operator+(float a, BFloat16 b) {
  return a + static_cast<float>(b);
}
inline C10_HOST_DEVICE float operator-(float a, BFloat16 b) {
  return a - static_cast<float>(b);
}
inline C10_HOST_DEVICE float operator*(float a, BFloat16 b) {
  return a * static_cast<float>(b);
}
inline C10_HOST_DEVICE float operator/(float a, BFloat16 b) {
  return a / static_cast<float>(b);
}

inline C10_HOST_DEVICE float& operator+=(float& a, const BFloat16& b) {
  return a += static_cast<float>(b);
}
inline C10_HOST_DEVICE float& operator-=(float& a, const BFloat16& b) {
  return a -= static_cast<float>(b);
}
inline C10_HOST_DEVICE float& operator*=(float& a, const BFloat16& b) {
  return a *= static_cast<float>(b);
}
inline C10_HOST_DEVICE float& operator/=(float& a, const BFloat16& b) {
  return a /= static_cast<float>(b);
}

/// Arithmetic with doubles

inline C10_HOST_DEVICE double operator+(BFloat16 a, double b) {
  return static_cast<double>(a) + b;
}
inline C10_HOST_DEVICE double operator-(BFloat16 a, double b) {
  return static_cast<double>(a) - b;
}
inline C10_HOST_DEVICE double operator*(BFloat16 a, double b) {
  return static_cast<double>(a) * b;
}
inline C10_HOST_DEVICE double operator/(BFloat16 a, double b) {
  return static_cast<double>(a) / b;
}

inline C10_HOST_DEVICE double operator+(double a, BFloat16 b) {
  return a + static_cast<double>(b);
}
inline C10_HOST_DEVICE double operator-(double a, BFloat16 b) {
  return a - static_cast<double>(b);
}
inline C10_HOST_DEVICE double operator*(double a, BFloat16 b) {
  return a * static_cast<double>(b);
}
inline C10_HOST_DEVICE double operator/(double a, BFloat16 b) {
  return a / static_cast<double>(b);
}

/// Arithmetic with ints

inline C10_HOST_DEVICE BFloat16 operator+(BFloat16 a, int b) {
  return a + static_cast<BFloat16>(b);
}
inline C10_HOST_DEVICE BFloat16 operator-(BFloat16 a, int b) {
  return a - static_cast<BFloat16>(b);
}
inline C10_HOST_DEVICE BFloat16 operator*(BFloat16 a, int b) {
  return a * static_cast<BFloat16>(b);
}
inline C10_HOST_DEVICE BFloat16 operator/(BFloat16 a, int b) {
  return a / static_cast<BFloat16>(b);
}

inline C10_HOST_DEVICE BFloat16 operator+(int a, BFloat16 b) {
  return static_cast<BFloat16>(a) + b;
}
inline C10_HOST_DEVICE BFloat16 operator-(int a, BFloat16 b) {
  return static_cast<BFloat16>(a) - b;
}
inline C10_HOST_DEVICE BFloat16 operator*(int a, BFloat16 b) {
  return static_cast<BFloat16>(a) * b;
}
inline C10_HOST_DEVICE BFloat16 operator/(int a, BFloat16 b) {
  return static_cast<BFloat16>(a) / b;
}

//// Arithmetic with int64_t

inline C10_HOST_DEVICE BFloat16 operator+(BFloat16 a, int64_t b) {
  return a + static_cast<BFloat16>(b);
}
inline C10_HOST_DEVICE BFloat16 operator-(BFloat16 a, int64_t b) {
  return a - static_cast<BFloat16>(b);
}
inline C10_HOST_DEVICE BFloat16 operator*(BFloat16 a, int64_t b) {
  return a * static_cast<BFloat16>(b);
}
inline C10_HOST_DEVICE BFloat16 operator/(BFloat16 a, int64_t b) {
  return a / static_cast<BFloat16>(b);
}

inline C10_HOST_DEVICE BFloat16 operator+(int64_t a, BFloat16 b) {
  return static_cast<BFloat16>(a) + b;
}
inline C10_HOST_DEVICE BFloat16 operator-(int64_t a, BFloat16 b) {
  return static_cast<BFloat16>(a) - b;
}
inline C10_HOST_DEVICE BFloat16 operator*(int64_t a, BFloat16 b) {
  return static_cast<BFloat16>(a) * b;
}
inline C10_HOST_DEVICE BFloat16 operator/(int64_t a, BFloat16 b) {
  return static_cast<BFloat16>(a) / b;
}

// Overloading < and > operators, because std::max and std::min use them.

inline C10_HOST_DEVICE bool operator>(BFloat16& lhs, BFloat16& rhs) {
  return float(lhs) > float(rhs);
}

inline C10_HOST_DEVICE bool operator<(BFloat16& lhs, BFloat16& rhs) {
  return float(lhs) < float(rhs);
}

} // namespace MSharp

namespace std {

template <>
class numeric_limits<MSharp::BFloat16> {
 public:
  static constexpr bool is_signed = true;
  static constexpr bool is_specialized = true;
  static constexpr bool is_integer = false;
  static constexpr bool is_exact = false;
  static constexpr bool has_infinity = true;
  static constexpr bool has_quiet_NaN = true;
  static constexpr bool has_signaling_NaN = true;
  static constexpr auto has_denorm = numeric_limits<float>::has_denorm;
  static constexpr auto has_denorm_loss =
      numeric_limits<float>::has_denorm_loss;
  static constexpr auto round_style = numeric_limits<float>::round_style;
  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;
  static constexpr int digits = 8;
  static constexpr int digits10 = 2;
  static constexpr int max_digits10 = 4;
  static constexpr int radix = 2;
  static constexpr int min_exponent = -125;
  static constexpr int min_exponent10 = -37;
  static constexpr int max_exponent = 128;
  static constexpr int max_exponent10 = 38;
  static constexpr auto traps = numeric_limits<float>::traps;
  static constexpr auto tinyness_before =
      numeric_limits<float>::tinyness_before;

  static constexpr MSharp::BFloat16 min() {
    return MSharp::BFloat16(0x0080, MSharp::BFloat16::from_bits());
  }
  static constexpr MSharp::BFloat16 lowest() {
    return MSharp::BFloat16(0xFF7F, MSharp::BFloat16::from_bits());
  }
  static constexpr MSharp::BFloat16 max() {
    return MSharp::BFloat16(0x7F7F, MSharp::BFloat16::from_bits());
  }
  static constexpr MSharp::BFloat16 epsilon() {
    return MSharp::BFloat16(0x3C00, MSharp::BFloat16::from_bits());
  }
  static constexpr MSharp::BFloat16 round_error() {
    return MSharp::BFloat16(0x3F00, MSharp::BFloat16::from_bits());
  }
  static constexpr MSharp::BFloat16 infinity() {
    return MSharp::BFloat16(0x7F80, MSharp::BFloat16::from_bits());
  }
  static constexpr MSharp::BFloat16 quiet_NaN() {
    return MSharp::BFloat16(0x7FC0, MSharp::BFloat16::from_bits());
  }
  static constexpr MSharp::BFloat16 signaling_NaN() {
    return MSharp::BFloat16(0x7F80, MSharp::BFloat16::from_bits());
  }
  static constexpr MSharp::BFloat16 denorm_min() {
    return MSharp::BFloat16(0x0001, MSharp::BFloat16::from_bits());
  }
};

} // namespace std

C10_CLANG_DIAGNOSTIC_POP()
