#pragma once

#include <MSharp/core/ScalarType.h>
#include <MSharp/util/BFloat16.h>
#include <MSharp/util/Exception.h>
#include <MSharp/util/Float8_e4m3fn.h>
#include <MSharp/util/Float8_e4m3fnuz.h>
#include <MSharp/util/Float8_e5m2.h>
#include <MSharp/util/Float8_e5m2fnuz.h>
#include <MSharp/util/Half.h>

namespace pluss {

// For FP16 or BFloat16 inputs, ops should perform internal math in FP32.
template <typename scalar_t>
struct OpMathType {
  using type = scalar_t;
};
template <>
struct OpMathType<pluss::Half> {
  using type = float;
};
template <>
struct OpMathType<pluss::BFloat16> {
  using type = float;
};
template <>
struct OpMathType<pluss::Float8_e5m2> {
  using type = float;
};
template <>
struct OpMathType<pluss::Float8_e4m3fn> {
  using type = float;
};
template <>
struct OpMathType<pluss::Float8_e5m2fnuz> {
  using type = float;
};
template <>
struct OpMathType<pluss::Float8_e4m3fnuz> {
  using type = float;
};
template <>
struct OpMathType<MSharp::complex<Half>> {
  using type = MSharp::complex<float>;
};

template <typename T>
using opmath_type = typename OpMathType<T>::type;

namespace {

inline MSharp::ScalarType toOpMathType(const MSharp::ScalarType type) {
  switch (type) {
#define DEFINE_CASE(scalar_t, TypeNum) \
  case ScalarType::TypeNum:            \
    return CppTypeToScalarType<pluss::opmath_type<scalar_t>>::value;

    AT_FORALL_SCALAR_TYPES_WITH_COMPLEX(DEFINE_CASE)
#undef DEFINE_CASE

    default:
      TORCH_INTERNAL_ASSERT(false, "Unrecognized ScalarType: ", type);
  }
}

} // namespace

} // namespace pluss
