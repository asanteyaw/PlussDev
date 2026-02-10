#pragma once

#include <MSharp/macros/Macros.h>
#include <MSharp/util/bit_cast.h>

#include <cstring>
#include <limits>

#ifdef __CUDACC__
#include <cuda_fp16.h>
#endif

#ifdef __HIPCC__
#include <hip/hip_fp16.h>
#endif

#if defined(CL_SYCL_LANGUAGE_VERSION)
#include <CL/sycl.hpp> // for SYCL 1.2.1
#elif defined(SYCL_LANGUAGE_VERSION)
#include <sycl/sycl.hpp> // for SYCL 2020
#endif

#if (defined(CPU_CAPABILITY_AVX2) || defined(CPU_CAPABILITY_AVX512)) && \
    !defined(__APPLE__)
#include <PlussQit/cpu/vec/vec_half.h>
#endif

C10_CLANG_DIAGNOSTIC_PUSH()
#if C10_CLANG_HAS_WARNING("-Wimplicit-int-float-conversion")
C10_CLANG_DIAGNOSTIC_IGNORE("-Wimplicit-int-float-conversion")
#endif

namespace MSharp {

#if defined(__aarch64__) && !defined(__CUDACC__)
/// Constructors
inline Half::Half(float16_t value) : x(detail::fp16_to_bits(value)) {}
inline Half::operator float16_t() const {
  return detail::fp16_from_bits(x);
}
#else

inline C10_HOST_DEVICE Half::Half(float value)
    :
#if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
      x(__half_as_short(__float2half(value)))
#elif defined(__SYCL_DEVICE_ONLY__)
      x(MSharp::bit_cast<uint16_t>(sycl::half(value)))
#elif (defined(CPU_CAPABILITY_AVX2) || defined(CPU_CAPABILITY_AVX512)) && \
    !defined(__APPLE__)
      x(pluss::vec::float2half_scalar(value))
#else
      x(detail::fp16_ieee_from_fp32_value(value))
#endif
{
}

/// Implicit conversions

inline C10_HOST_DEVICE Half::operator float() const {
#if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
  return __half2float(*reinterpret_cast<const __half*>(&x));
#elif defined(__SYCL_DEVICE_ONLY__)
  return float(MSharp::bit_cast<sycl::half>(x));
#elif (defined(CPU_CAPABILITY_AVX2) || defined(CPU_CAPABILITY_AVX512)) && \
    !defined(__APPLE__)
  return pluss::vec::half2float_scalar(x);
#elif defined(__aarch64__) && !defined(__CUDACC__)
  return detail::native_fp16_to_fp32_value(x);
#else
  return detail::fp16_ieee_to_fp32_value(x);
#endif
}

#endif /* !defined(__aarch64__) || defined(__CUDACC__) \
        */

#if defined(__CUDACC__) || defined(__HIPCC__)
inline C10_HOST_DEVICE Half::Half(const __half& value) {
  x = *reinterpret_cast<const unsigned short*>(&value);
}
inline C10_HOST_DEVICE Half::operator __half() const {
  return *reinterpret_cast<const __half*>(&x);
}
#endif

#ifdef SYCL_LANGUAGE_VERSION
inline C10_HOST_DEVICE Half::Half(const sycl::half& value) {
  x = *reinterpret_cast<const unsigned short*>(&value);
}
inline C10_HOST_DEVICE Half::operator sycl::half() const {
  return *reinterpret_cast<const sycl::half*>(&x);
}
#endif

// CUDA intrinsics

#if (defined(__CUDA_ARCH__) && (__CUDA_ARCH__ >= 350)) || \
    (defined(__clang__) && defined(__CUDA__))
inline __device__ Half __ldg(const Half* ptr) {
  return __ldg(reinterpret_cast<const __half*>(ptr));
}
#endif

/// Arithmetic

inline C10_HOST_DEVICE Half operator+(const Half& a, const Half& b) {
  return static_cast<float>(a) + static_cast<float>(b);
}

inline C10_HOST_DEVICE Half operator-(const Half& a, const Half& b) {
  return static_cast<float>(a) - static_cast<float>(b);
}

inline C10_HOST_DEVICE Half operator*(const Half& a, const Half& b) {
  return static_cast<float>(a) * static_cast<float>(b);
}

inline C10_HOST_DEVICE Half operator/(const Half& a, const Half& b)
    __ubsan_ignore_float_divide_by_zero__ {
  return static_cast<float>(a) / static_cast<float>(b);
}

inline C10_HOST_DEVICE Half operator-(const Half& a) {
#if (defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 530) || \
    defined(__HIP_DEVICE_COMPILE__)
  return __hneg(a);
#elif defined(__SYCL_DEVICE_ONLY__)
  return -MSharp::bit_cast<sycl::half>(a);
#else
  return -static_cast<float>(a);
#endif
}

inline C10_HOST_DEVICE Half& operator+=(Half& a, const Half& b) {
  a = a + b;
  return a;
}

inline C10_HOST_DEVICE Half& operator-=(Half& a, const Half& b) {
  a = a - b;
  return a;
}

inline C10_HOST_DEVICE Half& operator*=(Half& a, const Half& b) {
  a = a * b;
  return a;
}

inline C10_HOST_DEVICE Half& operator/=(Half& a, const Half& b) {
  a = a / b;
  return a;
}

/// Arithmetic with floats

inline C10_HOST_DEVICE float operator+(Half a, float b) {
  return static_cast<float>(a) + b;
}
inline C10_HOST_DEVICE float operator-(Half a, float b) {
  return static_cast<float>(a) - b;
}
inline C10_HOST_DEVICE float operator*(Half a, float b) {
  return static_cast<float>(a) * b;
}
inline C10_HOST_DEVICE float operator/(Half a, float b)
    __ubsan_ignore_float_divide_by_zero__ {
  return static_cast<float>(a) / b;
}

inline C10_HOST_DEVICE float operator+(float a, Half b) {
  return a + static_cast<float>(b);
}
inline C10_HOST_DEVICE float operator-(float a, Half b) {
  return a - static_cast<float>(b);
}
inline C10_HOST_DEVICE float operator*(float a, Half b) {
  return a * static_cast<float>(b);
}
inline C10_HOST_DEVICE float operator/(float a, Half b)
    __ubsan_ignore_float_divide_by_zero__ {
  return a / static_cast<float>(b);
}

inline C10_HOST_DEVICE float& operator+=(float& a, const Half& b) {
  return a += static_cast<float>(b);
}
inline C10_HOST_DEVICE float& operator-=(float& a, const Half& b) {
  return a -= static_cast<float>(b);
}
inline C10_HOST_DEVICE float& operator*=(float& a, const Half& b) {
  return a *= static_cast<float>(b);
}
inline C10_HOST_DEVICE float& operator/=(float& a, const Half& b) {
  return a /= static_cast<float>(b);
}

/// Arithmetic with doubles

inline C10_HOST_DEVICE double operator+(Half a, double b) {
  return static_cast<double>(a) + b;
}
inline C10_HOST_DEVICE double operator-(Half a, double b) {
  return static_cast<double>(a) - b;
}
inline C10_HOST_DEVICE double operator*(Half a, double b) {
  return static_cast<double>(a) * b;
}
inline C10_HOST_DEVICE double operator/(Half a, double b)
    __ubsan_ignore_float_divide_by_zero__ {
  return static_cast<double>(a) / b;
}

inline C10_HOST_DEVICE double operator+(double a, Half b) {
  return a + static_cast<double>(b);
}
inline C10_HOST_DEVICE double operator-(double a, Half b) {
  return a - static_cast<double>(b);
}
inline C10_HOST_DEVICE double operator*(double a, Half b) {
  return a * static_cast<double>(b);
}
inline C10_HOST_DEVICE double operator/(double a, Half b)
    __ubsan_ignore_float_divide_by_zero__ {
  return a / static_cast<double>(b);
}

/// Arithmetic with ints

inline C10_HOST_DEVICE Half operator+(Half a, int b) {
  return a + static_cast<Half>(b);
}
inline C10_HOST_DEVICE Half operator-(Half a, int b) {
  return a - static_cast<Half>(b);
}
inline C10_HOST_DEVICE Half operator*(Half a, int b) {
  return a * static_cast<Half>(b);
}
inline C10_HOST_DEVICE Half operator/(Half a, int b) {
  return a / static_cast<Half>(b);
}

inline C10_HOST_DEVICE Half operator+(int a, Half b) {
  return static_cast<Half>(a) + b;
}
inline C10_HOST_DEVICE Half operator-(int a, Half b) {
  return static_cast<Half>(a) - b;
}
inline C10_HOST_DEVICE Half operator*(int a, Half b) {
  return static_cast<Half>(a) * b;
}
inline C10_HOST_DEVICE Half operator/(int a, Half b) {
  return static_cast<Half>(a) / b;
}

//// Arithmetic with int64_t

inline C10_HOST_DEVICE Half operator+(Half a, int64_t b) {
  return a + static_cast<Half>(b);
}
inline C10_HOST_DEVICE Half operator-(Half a, int64_t b) {
  return a - static_cast<Half>(b);
}
inline C10_HOST_DEVICE Half operator*(Half a, int64_t b) {
  return a * static_cast<Half>(b);
}
inline C10_HOST_DEVICE Half operator/(Half a, int64_t b) {
  return a / static_cast<Half>(b);
}

inline C10_HOST_DEVICE Half operator+(int64_t a, Half b) {
  return static_cast<Half>(a) + b;
}
inline C10_HOST_DEVICE Half operator-(int64_t a, Half b) {
  return static_cast<Half>(a) - b;
}
inline C10_HOST_DEVICE Half operator*(int64_t a, Half b) {
  return static_cast<Half>(a) * b;
}
inline C10_HOST_DEVICE Half operator/(int64_t a, Half b) {
  return static_cast<Half>(a) / b;
}

/// NOTE: we do not define comparisons directly and instead rely on the implicit
/// conversion from MSharp::Half to float.

} // namespace MSharp

namespace std {

template <>
class numeric_limits<MSharp::Half> {
 public:
  static constexpr bool is_specialized = true;
  static constexpr bool is_signed = true;
  static constexpr bool is_integer = false;
  static constexpr bool is_exact = false;
  static constexpr bool has_infinity = true;
  static constexpr bool has_quiet_NaN = true;
  static constexpr bool has_signaling_NaN = true;
  static constexpr auto has_denorm = numeric_limits<float>::has_denorm;
  static constexpr auto has_denorm_loss =
      numeric_limits<float>::has_denorm_loss;
  static constexpr auto round_style = numeric_limits<float>::round_style;
  static constexpr bool is_iec559 = true;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;
  static constexpr int digits = 11;
  static constexpr int digits10 = 3;
  static constexpr int max_digits10 = 5;
  static constexpr int radix = 2;
  static constexpr int min_exponent = -13;
  static constexpr int min_exponent10 = -4;
  static constexpr int max_exponent = 16;
  static constexpr int max_exponent10 = 4;
  static constexpr auto traps = numeric_limits<float>::traps;
  static constexpr auto tinyness_before =
      numeric_limits<float>::tinyness_before;
  static constexpr MSharp::Half min() {
    return MSharp::Half(0x0400, MSharp::Half::from_bits());
  }
  static constexpr MSharp::Half lowest() {
    return MSharp::Half(0xFBFF, MSharp::Half::from_bits());
  }
  static constexpr MSharp::Half max() {
    return MSharp::Half(0x7BFF, MSharp::Half::from_bits());
  }
  static constexpr MSharp::Half epsilon() {
    return MSharp::Half(0x1400, MSharp::Half::from_bits());
  }
  static constexpr MSharp::Half round_error() {
    return MSharp::Half(0x3800, MSharp::Half::from_bits());
  }
  static constexpr MSharp::Half infinity() {
    return MSharp::Half(0x7C00, MSharp::Half::from_bits());
  }
  static constexpr MSharp::Half quiet_NaN() {
    return MSharp::Half(0x7E00, MSharp::Half::from_bits());
  }
  static constexpr MSharp::Half signaling_NaN() {
    return MSharp::Half(0x7D00, MSharp::Half::from_bits());
  }
  static constexpr MSharp::Half denorm_min() {
    return MSharp::Half(0x0001, MSharp::Half::from_bits());
  }
};

} // namespace std

C10_CLANG_DIAGNOSTIC_POP()
