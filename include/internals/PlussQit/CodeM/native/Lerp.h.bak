#pragma once

#include <PlussQit/native/DispatchStub.h>
#include <PlussQit/OpMathType.h>
#include <PlussQit/TensorIterator.h>
#include <MSharp/core/Scalar.h>

namespace pluss::native {

template <typename scalar_t>
C10_HOST_DEVICE C10_ALWAYS_INLINE bool is_lerp_weight_small(scalar_t weight) {
  return std::abs(weight) < scalar_t(0.5);
}
template <typename scalar_t>
C10_HOST_DEVICE C10_ALWAYS_INLINE bool is_lerp_weight_small(MSharp::complex<scalar_t> weight) {
  // Avoid the sqrt in abs(weight)
  return (weight.real() * weight.real() + weight.imag() * weight.imag()) < scalar_t(0.25);
}

template <typename scalar_t, typename weight_t>
C10_HOST_DEVICE C10_ALWAYS_INLINE scalar_t lerp(scalar_t self_, scalar_t end_, weight_t weight_) {
  using opmath_t = pluss::opmath_type<scalar_t>;
  using opmath_weight_t = pluss::opmath_type<weight_t>;

  opmath_t self = self_;
  opmath_t end = end_;
  opmath_weight_t weight = weight_;

  // Conditional for better numeric. This has been discussed in
  // https://github.com/pytorch/pytorch/pull/18871
  return is_lerp_weight_small(weight)
      ? self + weight * (end - self)
      : end - (end - self) * (opmath_t(1) - weight);
}

using lerp_fn_scalar = void (*)(
    pluss::TensorIteratorBase& iter,
    const Scalar& weight);

using lerp_fn_tensor = void (*)(
    pluss::TensorIteratorBase& iter);

DECLARE_DISPATCH(lerp_fn_scalar, lerp_kernel_scalar_weight)
DECLARE_DISPATCH(lerp_fn_tensor, lerp_kernel_tensor_weight)

} // namespace pluss::native
