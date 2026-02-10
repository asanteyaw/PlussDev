#if !defined(C10_INTERNAL_INCLUDE_COMPLEX_REMAINING_H)
#error \
    "c10/util/complex_math.h is not meant to be individually included. Include c10/util/complex.h instead."
#endif

namespace MSharp_complex_math {

// Exponential functions

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> exp(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::exp(static_cast<thrust::complex<T>>(x)));
#else
  return static_cast<MSharp::complex<T>>(
      std::exp(static_cast<std::complex<T>>(x)));
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> log(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::log(static_cast<thrust::complex<T>>(x)));
#else
  return static_cast<MSharp::complex<T>>(
      std::log(static_cast<std::complex<T>>(x)));
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> log10(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::log10(static_cast<thrust::complex<T>>(x)));
#else
  return static_cast<MSharp::complex<T>>(
      std::log10(static_cast<std::complex<T>>(x)));
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> log2(const MSharp::complex<T>& x) {
  const MSharp::complex<T> log2 = MSharp::complex<T>(::log(2.0), 0.0);
  return c10_complex_math::log(x) / log2;
}

// Power functions
//
#if defined(_LIBCPP_VERSION) || \
    (defined(__GLIBCXX__) && !defined(_GLIBCXX11_USE_C99_COMPLEX))
namespace _detail {
C10_API MSharp::complex<float> sqrt(const MSharp::complex<float>& in);
C10_API MSharp::complex<double> sqrt(const MSharp::complex<double>& in);
C10_API MSharp::complex<float> acos(const MSharp::complex<float>& in);
C10_API MSharp::complex<double> acos(const MSharp::complex<double>& in);
} // namespace _detail
#endif

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> sqrt(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::sqrt(static_cast<thrust::complex<T>>(x)));
#elif !(                        \
    defined(_LIBCPP_VERSION) || \
    (defined(__GLIBCXX__) && !defined(_GLIBCXX11_USE_C99_COMPLEX)))
  return static_cast<MSharp::complex<T>>(
      std::sqrt(static_cast<std::complex<T>>(x)));
#else
  return _detail::sqrt(x);
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> pow(
    const MSharp::complex<T>& x,
    const MSharp::complex<T>& y) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(thrust::pow(
      static_cast<thrust::complex<T>>(x), static_cast<thrust::complex<T>>(y)));
#else
  return static_cast<MSharp::complex<T>>(std::pow(
      static_cast<std::complex<T>>(x), static_cast<std::complex<T>>(y)));
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> pow(
    const MSharp::complex<T>& x,
    const T& y) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::pow(static_cast<thrust::complex<T>>(x), y));
#else
  return static_cast<MSharp::complex<T>>(
      std::pow(static_cast<std::complex<T>>(x), y));
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> pow(
    const T& x,
    const MSharp::complex<T>& y) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::pow(x, static_cast<thrust::complex<T>>(y)));
#else
  return static_cast<MSharp::complex<T>>(
      std::pow(x, static_cast<std::complex<T>>(y)));
#endif
}

template <typename T, typename U>
C10_HOST_DEVICE inline MSharp::complex<decltype(T() * U())> pow(
    const MSharp::complex<T>& x,
    const MSharp::complex<U>& y) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(thrust::pow(
      static_cast<thrust::complex<T>>(x), static_cast<thrust::complex<T>>(y)));
#else
  return static_cast<MSharp::complex<T>>(std::pow(
      static_cast<std::complex<T>>(x), static_cast<std::complex<T>>(y)));
#endif
}

template <typename T, typename U>
C10_HOST_DEVICE inline MSharp::complex<decltype(T() * U())> pow(
    const MSharp::complex<T>& x,
    const U& y) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::pow(static_cast<thrust::complex<T>>(x), y));
#else
  return static_cast<MSharp::complex<T>>(
      std::pow(static_cast<std::complex<T>>(x), y));
#endif
}

template <typename T, typename U>
C10_HOST_DEVICE inline MSharp::complex<decltype(T() * U())> pow(
    const T& x,
    const MSharp::complex<U>& y) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::pow(x, static_cast<thrust::complex<T>>(y)));
#else
  return static_cast<MSharp::complex<T>>(
      std::pow(x, static_cast<std::complex<T>>(y)));
#endif
}

// Trigonometric functions

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> sin(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::sin(static_cast<thrust::complex<T>>(x)));
#else
  return static_cast<MSharp::complex<T>>(
      std::sin(static_cast<std::complex<T>>(x)));
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> cos(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::cos(static_cast<thrust::complex<T>>(x)));
#else
  return static_cast<MSharp::complex<T>>(
      std::cos(static_cast<std::complex<T>>(x)));
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> tan(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::tan(static_cast<thrust::complex<T>>(x)));
#else
  return static_cast<MSharp::complex<T>>(
      std::tan(static_cast<std::complex<T>>(x)));
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> asin(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::asin(static_cast<thrust::complex<T>>(x)));
#else
  return static_cast<MSharp::complex<T>>(
      std::asin(static_cast<std::complex<T>>(x)));
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> acos(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::acos(static_cast<thrust::complex<T>>(x)));
#elif !defined(_LIBCPP_VERSION)
  return static_cast<MSharp::complex<T>>(
      std::acos(static_cast<std::complex<T>>(x)));
#else
  return _detail::acos(x);
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> atan(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::atan(static_cast<thrust::complex<T>>(x)));
#else
  return static_cast<MSharp::complex<T>>(
      std::atan(static_cast<std::complex<T>>(x)));
#endif
}

// Hyperbolic functions

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> sinh(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::sinh(static_cast<thrust::complex<T>>(x)));
#else
  return static_cast<MSharp::complex<T>>(
      std::sinh(static_cast<std::complex<T>>(x)));
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> cosh(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::cosh(static_cast<thrust::complex<T>>(x)));
#else
  return static_cast<MSharp::complex<T>>(
      std::cosh(static_cast<std::complex<T>>(x)));
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> tanh(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::tanh(static_cast<thrust::complex<T>>(x)));
#else
  return static_cast<MSharp::complex<T>>(
      std::tanh(static_cast<std::complex<T>>(x)));
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> asinh(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::asinh(static_cast<thrust::complex<T>>(x)));
#else
  return static_cast<MSharp::complex<T>>(
      std::asinh(static_cast<std::complex<T>>(x)));
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> acosh(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::acosh(static_cast<thrust::complex<T>>(x)));
#else
  return static_cast<MSharp::complex<T>>(
      std::acosh(static_cast<std::complex<T>>(x)));
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> atanh(const MSharp::complex<T>& x) {
#if defined(__CUDACC__) || defined(__HIPCC__)
  return static_cast<MSharp::complex<T>>(
      thrust::atanh(static_cast<thrust::complex<T>>(x)));
#else
  return static_cast<MSharp::complex<T>>(
      std::atanh(static_cast<std::complex<T>>(x)));
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> log1p(const MSharp::complex<T>& z) {
#if defined(__APPLE__) || defined(__MACOSX) || defined(__CUDACC__) || \
    defined(__HIPCC__)
  // For Mac, the new implementation yielded a high relative error. Falling back
  // to the old version for now.
  // See https://github.com/numpy/numpy/pull/22611#issuecomment-1667945354
  // For CUDA we also use this one, as thrust::log(thrust::complex) takes
  // *forever* to compile

  // log1p(z) = log(1 + z)
  // Let's define 1 + z = r * e ^ (i * a), then we have
  // log(r * e ^ (i * a)) = log(r) + i * a
  // With z = x + iy, the term r can be written as
  // r = ((1 + x) ^ 2 + y ^ 2) ^ 0.5
  //   = (1 + x ^ 2 + 2 * x + y ^ 2) ^ 0.5
  // So, log(r) is
  // log(r) = 0.5 * log(1 + x ^ 2 + 2 * x + y ^ 2)
  //        = 0.5 * log1p(x * (x + 2) + y ^ 2)
  // we need to use the expression only on certain condition to avoid overflow
  // and underflow from `(x * (x + 2) + y ^ 2)`
  T x = z.real();
  T y = z.imag();
  T zabs = std::abs(z);
  T theta = std::atan2(y, x + T(1));
  if (zabs < 0.5) {
    T r = x * (T(2) + x) + y * y;
    if (r == 0) { // handle underflow
      return {x, theta};
    }
    return {T(0.5) * std::log1p(r), theta};
  } else {
    T z0 = std::hypot(x + 1, y);
    return {std::log(z0), theta};
  }
#else
  // CPU path
  // Based on https://github.com/numpy/numpy/pull/22611#issuecomment-1667945354
  MSharp::complex<T> u = z + T(1);
  if (u == T(1)) {
    return z;
  } else {
    auto log_u = log(u);
    if (u - T(1) == z) {
      return log_u;
    }
    return log_u * (z / (u - T(1)));
  }
#endif
}

template <typename T>
C10_HOST_DEVICE inline MSharp::complex<T> expm1(const MSharp::complex<T>& z) {
  // expm1(z) = exp(z) - 1
  // Define z = x + i * y
  // f = e ^ (x + i * y) - 1
  //   = e ^ x * e ^ (i * y) - 1
  //   = (e ^ x * cos(y) - 1) + i * (e ^ x * sin(y))
  //   = (e ^ x - 1) * cos(y) - (1 - cos(y)) + i * e ^ x * sin(y)
  //   = expm1(x) * cos(y) - 2 * sin(y / 2) ^ 2 + i * e ^ x * sin(y)
  T x = z.real();
  T y = z.imag();
  T a = std::sin(y / 2);
  T er = std::expm1(x) * std::cos(y) - T(2) * a * a;
  T ei = std::exp(x) * std::sin(y);
  return {er, ei};
}

} // namespace MSharp_complex_math

using c10_complex_math::acos;
using c10_complex_math::acosh;
using c10_complex_math::asin;
using c10_complex_math::asinh;
using c10_complex_math::atan;
using c10_complex_math::atanh;
using c10_complex_math::cos;
using c10_complex_math::cosh;
using c10_complex_math::exp;
using c10_complex_math::expm1;
using c10_complex_math::log;
using c10_complex_math::log10;
using c10_complex_math::log1p;
using c10_complex_math::log2;
using c10_complex_math::pow;
using c10_complex_math::sin;
using c10_complex_math::sinh;
using c10_complex_math::sqrt;
using c10_complex_math::tan;
using c10_complex_math::tanh;

namespace std {

using c10_complex_math::acos;
using c10_complex_math::acosh;
using c10_complex_math::asin;
using c10_complex_math::asinh;
using c10_complex_math::atan;
using c10_complex_math::atanh;
using c10_complex_math::cos;
using c10_complex_math::cosh;
using c10_complex_math::exp;
using c10_complex_math::expm1;
using c10_complex_math::log;
using c10_complex_math::log10;
using c10_complex_math::log1p;
using c10_complex_math::log2;
using c10_complex_math::pow;
using c10_complex_math::sin;
using c10_complex_math::sinh;
using c10_complex_math::sqrt;
using c10_complex_math::tan;
using c10_complex_math::tanh;

} // namespace std
