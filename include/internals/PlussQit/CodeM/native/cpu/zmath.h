#pragma once

// Complex number math operations that act as no-ops for other dtypes.
#include <MSharp/util/complex.h>
#include <MSharp/util/MathConstants.h>
#include<PlussQit/NumericUtils.h>

namespace pluss::native {
inline namespace CPU_CAPABILITY {

template <typename SCALAR_TYPE, typename VALUE_TYPE=SCALAR_TYPE>
inline VALUE_TYPE zabs (SCALAR_TYPE z) {
  return z;
}

template<>
inline MSharp::complex<float> zabs <MSharp::complex<float>> (MSharp::complex<float> z) {
  return MSharp::complex<float>(std::abs(z));
}

template<>
inline float zabs <MSharp::complex<float>, float> (MSharp::complex<float> z) {
  return std::abs(z);
}

template<>
inline MSharp::complex<double> zabs <MSharp::complex<double>> (MSharp::complex<double> z) {
  return MSharp::complex<double>(std::abs(z));
}

template<>
inline double zabs <MSharp::complex<double>, double> (MSharp::complex<double> z) {
  return std::abs(z);
}

// This overload corresponds to non-complex dtypes.
// The function is consistent with its NumPy equivalent
// for non-complex dtypes where `pi` is returned for
// negative real numbers and `0` is returned for 0 or positive
// real numbers.
// Note: `nan` is propagated.
template <typename SCALAR_TYPE, typename VALUE_TYPE=SCALAR_TYPE>
inline VALUE_TYPE angle_impl (SCALAR_TYPE z) {
  if (pluss::_isnan(z)) {
    return z;
  }
  return z < 0 ? MSharp::pi<double> : 0;
}

template<>
inline MSharp::complex<float> angle_impl <MSharp::complex<float>> (MSharp::complex<float> z) {
  return MSharp::complex<float>(std::arg(z), 0.0);
}

template<>
inline float angle_impl <MSharp::complex<float>, float> (MSharp::complex<float> z) {
  return std::arg(z);
}

template<>
inline MSharp::complex<double> angle_impl <MSharp::complex<double>> (MSharp::complex<double> z) {
  return MSharp::complex<double>(std::arg(z), 0.0);
}

template<>
inline double angle_impl <MSharp::complex<double>, double> (MSharp::complex<double> z) {
  return std::arg(z);
}

template <typename SCALAR_TYPE, typename VALUE_TYPE=SCALAR_TYPE>
constexpr VALUE_TYPE real_impl (SCALAR_TYPE z) {
  return z; //No-Op
}

template<>
constexpr MSharp::complex<float> real_impl <MSharp::complex<float>> (MSharp::complex<float> z) {
  return MSharp::complex<float>(z.real(), 0.0);
}

template<>
constexpr float real_impl <MSharp::complex<float>, float> (MSharp::complex<float> z) {
  return z.real();
}

template<>
constexpr MSharp::complex<double> real_impl <MSharp::complex<double>> (MSharp::complex<double> z) {
  return MSharp::complex<double>(z.real(), 0.0);
}

template<>
constexpr double real_impl <MSharp::complex<double>, double> (MSharp::complex<double> z) {
  return z.real();
}

template <typename SCALAR_TYPE, typename VALUE_TYPE=SCALAR_TYPE>
constexpr VALUE_TYPE imag_impl (SCALAR_TYPE /*z*/) {
  return 0;
}

template<>
constexpr MSharp::complex<float> imag_impl <MSharp::complex<float>> (MSharp::complex<float> z) {
  return MSharp::complex<float>(z.imag(), 0.0);
}

template<>
constexpr float imag_impl <MSharp::complex<float>, float> (MSharp::complex<float> z) {
  return z.imag();
}

template<>
constexpr MSharp::complex<double> imag_impl <MSharp::complex<double>> (MSharp::complex<double> z) {
  return MSharp::complex<double>(z.imag(), 0.0);
}

template<>
constexpr double imag_impl <MSharp::complex<double>, double> (MSharp::complex<double> z) {
  return z.imag();
}

template <typename TYPE>
inline TYPE conj_impl (TYPE z) {
  return z; //No-Op
}

template<>
inline MSharp::complex<pluss::Half> conj_impl <MSharp::complex<pluss::Half>> (MSharp::complex<pluss::Half> z) {
  return MSharp::complex<pluss::Half>{z.real(), -z.imag()};
}

template<>
inline MSharp::complex<float> conj_impl <MSharp::complex<float>> (MSharp::complex<float> z) {
  return MSharp::complex<float>(z.real(), -z.imag());
}

template<>
inline MSharp::complex<double> conj_impl <MSharp::complex<double>> (MSharp::complex<double> z) {
  return MSharp::complex<double>(z.real(), -z.imag());
}

template <typename TYPE>
inline TYPE ceil_impl (TYPE z) {
  return std::ceil(z);
}

template <>
inline MSharp::complex<float> ceil_impl (MSharp::complex<float> z) {
  return MSharp::complex<float>(std::ceil(z.real()), std::ceil(z.imag()));
}

template <>
inline MSharp::complex<double> ceil_impl (MSharp::complex<double> z) {
  return MSharp::complex<double>(std::ceil(z.real()), std::ceil(z.imag()));
}

template<typename T>
inline MSharp::complex<T> sgn_impl (MSharp::complex<T> z) {
  if (z == MSharp::complex<T>(0, 0)) {
    return MSharp::complex<T>(0, 0);
  } else {
    return z / zabs(z);
  }
}

template <typename TYPE>
inline TYPE floor_impl (TYPE z) {
  return std::floor(z);
}

template <>
inline MSharp::complex<float> floor_impl (MSharp::complex<float> z) {
  return MSharp::complex<float>(std::floor(z.real()), std::floor(z.imag()));
}

template <>
inline MSharp::complex<double> floor_impl (MSharp::complex<double> z) {
  return MSharp::complex<double>(std::floor(z.real()), std::floor(z.imag()));
}

template <typename TYPE>
inline TYPE round_impl (TYPE z) {
  return std::nearbyint(z);
}

template <>
inline MSharp::complex<float> round_impl (MSharp::complex<float> z) {
  return MSharp::complex<float>(std::nearbyint(z.real()), std::nearbyint(z.imag()));
}

template <>
inline MSharp::complex<double> round_impl (MSharp::complex<double> z) {
  return MSharp::complex<double>(std::nearbyint(z.real()), std::nearbyint(z.imag()));
}

template <typename TYPE>
inline TYPE trunc_impl (TYPE z) {
  return std::trunc(z);
}

template <>
inline MSharp::complex<float> trunc_impl (MSharp::complex<float> z) {
  return MSharp::complex<float>(std::trunc(z.real()), std::trunc(z.imag()));
}

template <>
inline MSharp::complex<double> trunc_impl (MSharp::complex<double> z) {
  return MSharp::complex<double>(std::trunc(z.real()), std::trunc(z.imag()));
}

template <typename TYPE, std::enable_if_t<!MSharp::is_complex<TYPE>::value, int> = 0>
inline TYPE max_impl (TYPE a, TYPE b) {
  if (_isnan<TYPE>(a) || _isnan<TYPE>(b)) {
    return std::numeric_limits<TYPE>::quiet_NaN();
  } else {
    return std::max(a, b);
  }
}

template <typename TYPE, std::enable_if_t<MSharp::is_complex<TYPE>::value, int> = 0>
inline TYPE max_impl (TYPE a, TYPE b) {
  if (_isnan<TYPE>(a)) {
    return a;
  } else if (_isnan<TYPE>(b)) {
    return b;
  } else {
    return std::abs(a) > std::abs(b) ? a : b;
  }
}

template <typename TYPE, std::enable_if_t<!MSharp::is_complex<TYPE>::value, int> = 0>
inline TYPE min_impl (TYPE a, TYPE b) {
  if (_isnan<TYPE>(a) || _isnan<TYPE>(b)) {
    return std::numeric_limits<TYPE>::quiet_NaN();
  } else {
    return std::min(a, b);
  }
}

template <typename TYPE, std::enable_if_t<MSharp::is_complex<TYPE>::value, int> = 0>
inline TYPE min_impl (TYPE a, TYPE b) {
  if (_isnan<TYPE>(a)) {
    return a;
  } else if (_isnan<TYPE>(b)) {
    return b;
  } else {
    return std::abs(a) < std::abs(b) ? a : b;
  }
}

} // end namespace
} //end pluss::native
