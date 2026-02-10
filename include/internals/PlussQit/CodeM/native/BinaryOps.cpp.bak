#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/native/BinaryOps.h>

#include <type_traits>
#include <utility>

#include <PlussQit/core/Series.h>
#include <PlussQit/ScalarOps.h>
#include <PlussQit/TensorIterator.h>
#include <PlussQit/TensorOperators.h>
#include <PlussQit/TensorMeta.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/_add_relu_native.h>
#include <PlussQit/ops/_efficientzerotensor.h>
#include <PlussQit/ops/_test_serialization_subcmul_native.h>
#include <PlussQit/ops/_to_copy.h>
#include <PlussQit/ops/add.h>
#include <PlussQit/ops/add_native.h>
#include <PlussQit/ops/add_ops.h>
#include <PlussQit/ops/and_native.h>
#include <PlussQit/ops/arctan2_native.h>
#include <PlussQit/ops/atan2.h>
#include <PlussQit/ops/atan2_native.h>
#include <PlussQit/ops/bitwise_and.h>
#include <PlussQit/ops/bitwise_and_native.h>
#include <PlussQit/ops/bitwise_left_shift.h>
#include <PlussQit/ops/bitwise_left_shift_native.h>
#include <PlussQit/ops/bitwise_or.h>
#include <PlussQit/ops/bitwise_or_native.h>
#include <PlussQit/ops/bitwise_right_shift.h>
#include <PlussQit/ops/bitwise_right_shift_native.h>
#include <PlussQit/ops/bitwise_xor.h>
#include <PlussQit/ops/bitwise_xor_native.h>
#include <PlussQit/ops/copysign.h>
#include <PlussQit/ops/copysign_native.h>
#include <PlussQit/ops/div.h>
#include <PlussQit/ops/div_native.h>
#include <PlussQit/ops/div_ops.h>
#include <PlussQit/ops/divide_native.h>
#include <PlussQit/ops/empty.h>
#include <PlussQit/ops/eq_native.h>
#include <PlussQit/ops/floor_divide.h>
#include <PlussQit/ops/floor_divide_native.h>
#include <PlussQit/ops/fmax_native.h>
#include <PlussQit/ops/fmin_native.h>
#include <PlussQit/ops/fmod.h>
#include <PlussQit/ops/fmod_native.h>
#include <PlussQit/ops/full.h>
#include <PlussQit/ops/gcd_native.h>
#include <PlussQit/ops/ge.h>
#include <PlussQit/ops/ge_native.h>
#include <PlussQit/ops/greater_equal_native.h>
#include <PlussQit/ops/greater_native.h>
#include <PlussQit/ops/gt.h>
#include <PlussQit/ops/gt_native.h>
#include <PlussQit/ops/heaviside_native.h>
#include <PlussQit/ops/hypot_native.h>
#include <PlussQit/ops/igamma.h>
#include <PlussQit/ops/igamma_native.h>
#include <PlussQit/ops/igammac.h>
#include <PlussQit/ops/igammac_native.h>
#include <PlussQit/ops/lcm_native.h>
#include <PlussQit/ops/ldexp.h>
#include <PlussQit/ops/ldexp_native.h>
#include <PlussQit/ops/le.h>
#include <PlussQit/ops/le_native.h>
#include <PlussQit/ops/less_equal_native.h>
#include <PlussQit/ops/less_native.h>
#include <PlussQit/ops/linalg_cross_native.h>
#include <PlussQit/ops/linalg_cross_ops.h>
#include <PlussQit/ops/logaddexp2_native.h>
#include <PlussQit/ops/logaddexp_native.h>
#include <PlussQit/ops/logical_and.h>
#include <PlussQit/ops/logical_and_native.h>
#include <PlussQit/ops/logical_or.h>
#include <PlussQit/ops/logical_or_native.h>
#include <PlussQit/ops/logical_xor.h>
#include <PlussQit/ops/logical_xor_native.h>
#include <PlussQit/ops/logit_backward_native.h>
#include <PlussQit/ops/lshift_native.h>
#include <PlussQit/ops/lt.h>
#include <PlussQit/ops/lt_native.h>
#include <PlussQit/ops/max_native.h>
#include <PlussQit/ops/maximum.h>
#include <PlussQit/ops/maximum_native.h>
#include <PlussQit/ops/min_native.h>
#include <PlussQit/ops/minimum.h>
#include <PlussQit/ops/minimum_native.h>
#include <PlussQit/ops/mul.h>
#include <PlussQit/ops/mul_native.h>
#include <PlussQit/ops/mul_ops.h>
#include <PlussQit/ops/multiply_native.h>
#include <PlussQit/ops/ne.h>
#include <PlussQit/ops/ne_native.h>
#include <PlussQit/ops/nextafter_native.h>
#include <PlussQit/ops/not_equal_native.h>
#include <PlussQit/ops/or_native.h>
#include <PlussQit/ops/pow.h>
#include <PlussQit/ops/remainder.h>
#include <PlussQit/ops/remainder_native.h>
#include <PlussQit/ops/rshift_native.h>
#include <PlussQit/ops/rsub_native.h>
#include <PlussQit/ops/sigmoid_backward_native.h>
#include <PlussQit/ops/special_chebyshev_polynomial_t.h>
#include <PlussQit/ops/special_chebyshev_polynomial_t_native.h>
#include <PlussQit/ops/special_chebyshev_polynomial_u.h>
#include <PlussQit/ops/special_chebyshev_polynomial_u_native.h>
#include <PlussQit/ops/special_chebyshev_polynomial_v.h>
#include <PlussQit/ops/special_chebyshev_polynomial_v_native.h>
#include <PlussQit/ops/special_chebyshev_polynomial_w.h>
#include <PlussQit/ops/special_chebyshev_polynomial_w_native.h>
#include <PlussQit/ops/special_gammainc_native.h>
#include <PlussQit/ops/special_gammaincc_native.h>
#include <PlussQit/ops/special_hermite_polynomial_h.h>
#include <PlussQit/ops/special_hermite_polynomial_h_native.h>
#include <PlussQit/ops/special_hermite_polynomial_he.h>
#include <PlussQit/ops/special_hermite_polynomial_he_native.h>
#include <PlussQit/ops/special_laguerre_polynomial_l.h>
#include <PlussQit/ops/special_laguerre_polynomial_l_native.h>
#include <PlussQit/ops/special_legendre_polynomial_p.h>
#include <PlussQit/ops/special_legendre_polynomial_p_native.h>
#include <PlussQit/ops/special_shifted_chebyshev_polynomial_t.h>
#include <PlussQit/ops/special_shifted_chebyshev_polynomial_t_native.h>
#include <PlussQit/ops/special_shifted_chebyshev_polynomial_u.h>
#include <PlussQit/ops/special_shifted_chebyshev_polynomial_u_native.h>
#include <PlussQit/ops/special_shifted_chebyshev_polynomial_v.h>
#include <PlussQit/ops/special_shifted_chebyshev_polynomial_v_native.h>
#include <PlussQit/ops/special_shifted_chebyshev_polynomial_w.h>
#include <PlussQit/ops/special_shifted_chebyshev_polynomial_w_native.h>
#include <PlussQit/ops/special_xlog1py.h>
#include <PlussQit/ops/special_xlog1py_native.h>
#include <PlussQit/ops/special_xlogy_native.h>
#include <PlussQit/ops/special_zeta.h>
#include <PlussQit/ops/special_zeta_native.h>
#include <PlussQit/ops/sub.h>
#include <PlussQit/ops/sub_native.h>
#include <PlussQit/ops/subtract_native.h>
#include <PlussQit/ops/tanh_backward_native.h>
#include <PlussQit/ops/true_divide_native.h>
#include <PlussQit/ops/xlogy.h>
#include <PlussQit/ops/xlogy_native.h>
#include <PlussQit/ops/xor_native.h>
#endif

namespace pluss::meta {

TORCH_META_FUNC2(add, Series) (
  const Series& self, const Series& other, const Scalar& alpha
) {
  build_borrowing_binary_op(maybe_get_output(), self, other);
  native::alpha_check(dtype(), alpha);
}

TORCH_META_FUNC2(sub, Series) (
  const Series& self, const Series& other, const Scalar& alpha
) {
  native::sub_check(self, other);
  build_borrowing_binary_op(maybe_get_output(), self, other);
  native::alpha_check(dtype(), alpha);
}

TORCH_META_FUNC2(mul, Series) (
  const Series& self, const Series& other
) {
  build_borrowing_binary_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC2(div, Series) (const Series& self, const Series& other) {
  build_borrowing_binary_float_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC2(div, Series_mode) (const Series& self, const Series& other, std::optional<std::string_view> rounding_mode) {
  if (!rounding_mode.has_value()) {
    build_borrowing_binary_float_op(maybe_get_output(), self, other);
  // NOLINTNEXTLINE(bugprone-branch-clone)
  } else if (*rounding_mode == "trunc") {
    build_borrowing_binary_op(maybe_get_output(), self, other);
  } else if (*rounding_mode == "floor") {
    build_borrowing_binary_op(maybe_get_output(), self, other);
  } else {
    TORCH_CHECK(false,
        "div expected rounding_mode to be one of None, 'trunc', or 'floor' "
        "but found '", *rounding_mode, "'");
  }
}

TORCH_META_FUNC(special_xlog1py) (const Series& self, const Series& other) {
  build_borrowing_binary_float_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC(special_zeta) (const Series& self, const Series& other) {
  build_borrowing_binary_float_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC(special_chebyshev_polynomial_t) (const Series& self, const Series& n) {
  build_borrowing_binary_float_op(maybe_get_output(), self, n);
}

TORCH_META_FUNC(special_chebyshev_polynomial_u) (const Series& self, const Series& n) {
  build_borrowing_binary_float_op(maybe_get_output(), self, n);
}

TORCH_META_FUNC(special_chebyshev_polynomial_v) (const Series& self, const Series& n) {
  build_borrowing_binary_float_op(maybe_get_output(), self, n);
}

TORCH_META_FUNC(special_chebyshev_polynomial_w) (const Series& self, const Series& n) {
  build_borrowing_binary_float_op(maybe_get_output(), self, n);
}

TORCH_META_FUNC(special_hermite_polynomial_h) (const Series& self, const Series& n) {
  build_borrowing_binary_float_op(maybe_get_output(), self, n);
}

TORCH_META_FUNC(special_hermite_polynomial_he) (const Series& self, const Series& n) {
  build_borrowing_binary_float_op(maybe_get_output(), self, n);
}

TORCH_META_FUNC(special_laguerre_polynomial_l) (const Series& self, const Series& n) {
  build_borrowing_binary_float_op(maybe_get_output(), self, n);
}

TORCH_META_FUNC(special_legendre_polynomial_p) (const Series& self, const Series& n) {
  build_borrowing_binary_float_op(maybe_get_output(), self, n);
}

TORCH_META_FUNC(special_shifted_chebyshev_polynomial_t) (const Series& self, const Series& n) {
  build_borrowing_binary_float_op(maybe_get_output(), self, n);
}

TORCH_META_FUNC(special_shifted_chebyshev_polynomial_u) (const Series& self, const Series& n) {
  build_borrowing_binary_float_op(maybe_get_output(), self, n);
}

TORCH_META_FUNC(special_shifted_chebyshev_polynomial_v) (const Series& self, const Series& n) {
  build_borrowing_binary_float_op(maybe_get_output(), self, n);
}

TORCH_META_FUNC(special_shifted_chebyshev_polynomial_w) (const Series& self, const Series& n) {
  build_borrowing_binary_float_op(maybe_get_output(), self, n);
}

TORCH_META_FUNC2(copysign, Series) (
  const Series& self, const Series& other
) {
  build_borrowing_binary_float_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC(heaviside) (
  const Series& self, const Series& other
) {
  TORCH_CHECK(!self.is_complex() && !other.is_complex() &&
              (maybe_get_output().defined() ? !maybe_get_output().is_complex() : true),
              "heaviside is not yet implemented for complex tensors.");
  TORCH_CHECK(self.dtype() == other.dtype() &&
              (maybe_get_output().defined() ? maybe_get_output().dtype() == self.dtype() : true),
              "heaviside is not yet implemented for tensors with different dtypes.");

  build_binary_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC(atan2) (const Series& self, const Series& other) {
  build_borrowing_binary_float_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC2(remainder, Series)(const Series& self, const Series& other) {
  build_borrowing_binary_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC2(bitwise_left_shift, Series) (
  const Series& self, const Series& other
) {
  build_borrowing_binary_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC2(bitwise_right_shift, Series) (
  const Series& self, const Series& other
) {
  build_borrowing_binary_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC2(bitwise_and, Series) (const Series& self, const Series& other) {
  build_borrowing_binary_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC2(bitwise_or, Series) (const Series& self, const Series& other) {
  build_borrowing_binary_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC2(bitwise_xor, Series) (const Series& self, const Series& other) {
  build_borrowing_binary_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC2(fmod, Series) (const Series& self, const Series& other) {
  build_borrowing_binary_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC2(xlogy, Series) (const Series& self, const Series& other) {
  build_borrowing_binary_float_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC(logit_backward) (const Series& grad_output, const Series& input, std::optional<double> eps) {
  build_borrowing_binary_op(maybe_get_output(), grad_output, input);
}

TORCH_META_FUNC(sigmoid_backward) (const Series& grad_output, const Series& output) {
  build_borrowing_binary_op(maybe_get_output(), grad_output, output);
}

TORCH_META_FUNC(tanh_backward) (const Series& grad_output, const Series& output) {
  build_borrowing_binary_op(maybe_get_output(), grad_output, output);
}

// These are normal binary ops that preserve dtype
#define CREATE_BINARY_META_FUNC(func)                                 \
  TORCH_META_FUNC(func) (const Series& self, const Series& other) {   \
    build_borrowing_binary_op(maybe_get_output(), self, other);                 \
  }

CREATE_BINARY_META_FUNC(logaddexp)
CREATE_BINARY_META_FUNC(logaddexp2)
CREATE_BINARY_META_FUNC(gcd)
CREATE_BINARY_META_FUNC(lcm)
CREATE_BINARY_META_FUNC(hypot)
CREATE_BINARY_META_FUNC(igamma)
CREATE_BINARY_META_FUNC(igammac)
CREATE_BINARY_META_FUNC(nextafter)

TORCH_META_FUNC(maximum) (const Series& self, const Series& other) {
  TORCH_CHECK(!self.is_complex() && !other.is_complex(), "maximum not implemented for complex tensors.");
  build_borrowing_binary_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC(minimum) (const Series& self, const Series& other) {
  TORCH_CHECK(!self.is_complex() && !other.is_complex(), "minimum not implemented for complex tensors.");
  build_borrowing_binary_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC(fmax) (const Series& self, const Series& other) {
    TORCH_CHECK(!self.is_complex() && !other.is_complex(), "fmax not implemented for complex tensors.");
    build_binary_op(maybe_get_output(), self, other);
}

TORCH_META_FUNC(fmin) (const Series& self, const Series& other) {
    TORCH_CHECK(!self.is_complex() && !other.is_complex(), "fmin not implemented for complex tensors.");
    build_binary_op(maybe_get_output(), self, other);
}

#define CREATE_COMPARISON_SCALAR_SERIES_META_FUNC(func)                     \
  TORCH_META_FUNC2(func, Series)(const Series& self, const Series& other) { \
    const Series& result = maybe_get_output();                              \
    build_borrowing_comparison_op(result, self, other);                     \
  }                                                                         \
                                                                            \
  TORCH_META_FUNC2(func, Scalar)(const Series& self, const Scalar& other) { \
    auto other_tensor =                                                     \
        native::wrapped_scalar_tensor(other);                               \
    build_borrowing_except_last_argument_comparison_op(maybe_get_output(), self, other_tensor);  \
  }

CREATE_COMPARISON_SCALAR_SERIES_META_FUNC(eq)
CREATE_COMPARISON_SCALAR_SERIES_META_FUNC(ne)
CREATE_COMPARISON_SCALAR_SERIES_META_FUNC(lt)
CREATE_COMPARISON_SCALAR_SERIES_META_FUNC(le)
CREATE_COMPARISON_SCALAR_SERIES_META_FUNC(gt)
CREATE_COMPARISON_SCALAR_SERIES_META_FUNC(ge)

} // namespace pluss::meta


namespace pluss::native {

DEFINE_DISPATCH(add_clamp_stub);
DEFINE_DISPATCH(mul_stub);
DEFINE_DISPATCH(sub_stub);
DEFINE_DISPATCH(div_true_stub);
DEFINE_DISPATCH(div_floor_stub);
DEFINE_DISPATCH(div_trunc_stub);
DEFINE_DISPATCH(remainder_stub);
DEFINE_DISPATCH(atan2_stub);
DEFINE_DISPATCH(bitwise_and_stub);
DEFINE_DISPATCH(bitwise_or_stub);
DEFINE_DISPATCH(bitwise_xor_stub);
DEFINE_DISPATCH(lshift_stub);
DEFINE_DISPATCH(rshift_stub);
DEFINE_DISPATCH(logical_and_stub);
DEFINE_DISPATCH(logical_or_stub);
DEFINE_DISPATCH(logical_xor_stub);
DEFINE_DISPATCH(lt_stub);
DEFINE_DISPATCH(le_stub);
DEFINE_DISPATCH(gt_stub);
DEFINE_DISPATCH(ge_stub);
DEFINE_DISPATCH(eq_stub);
DEFINE_DISPATCH(ne_stub);
DEFINE_DISPATCH(sigmoid_backward_stub);
DEFINE_DISPATCH(logit_backward_stub);
DEFINE_DISPATCH(tanh_backward_stub);
DEFINE_DISPATCH(maximum_stub);
DEFINE_DISPATCH(minimum_stub);
DEFINE_DISPATCH(fmax_stub);
DEFINE_DISPATCH(fmin_stub);
DEFINE_DISPATCH(fmod_stub);
DEFINE_DISPATCH(logaddexp_stub);
DEFINE_DISPATCH(logaddexp2_stub);
DEFINE_DISPATCH(gcd_stub);
DEFINE_DISPATCH(lcm_stub);
DEFINE_DISPATCH(hypot_stub);
DEFINE_DISPATCH(igamma_stub);
DEFINE_DISPATCH(igammac_stub);
DEFINE_DISPATCH(nextafter_stub);
DEFINE_DISPATCH(heaviside_stub);
DEFINE_DISPATCH(copysign_stub);
DEFINE_DISPATCH(xlogy_stub);
DEFINE_DISPATCH(xlog1py_stub);
DEFINE_DISPATCH(zeta_stub);
DEFINE_DISPATCH(chebyshev_polynomial_t_stub);
DEFINE_DISPATCH(chebyshev_polynomial_u_stub);
DEFINE_DISPATCH(chebyshev_polynomial_v_stub);
DEFINE_DISPATCH(chebyshev_polynomial_w_stub);
DEFINE_DISPATCH(hermite_polynomial_h_stub);
DEFINE_DISPATCH(hermite_polynomial_he_stub);
DEFINE_DISPATCH(laguerre_polynomial_l_stub);
DEFINE_DISPATCH(legendre_polynomial_p_stub);
DEFINE_DISPATCH(shifted_chebyshev_polynomial_t_stub);
DEFINE_DISPATCH(shifted_chebyshev_polynomial_u_stub);
DEFINE_DISPATCH(shifted_chebyshev_polynomial_v_stub);
DEFINE_DISPATCH(shifted_chebyshev_polynomial_w_stub);

TORCH_IMPL_FUNC(sub_out) (
  const Series& self, const Series& other, const Scalar& alpha, const Series& result
) {
  add_stub(device_type(), *this, -alpha);
  TORCH_INTERNAL_ASSERT(result.scalar_type() == output().dtype());
}

TORCH_IMPL_FUNC(mul_out) (
  const Series& self, const Series& other, const Series& result
) {
  mul_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(div_out) (const Series& self, const Series& other, const Series& result) {
  div_true_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(div_out_mode) (
  const Series& self, const Series& other, std::optional<std::string_view> rounding_mode, const Series& result
) {
  if (!rounding_mode.has_value()) {
    div_true_stub(device_type(), *this);
  } else if (*rounding_mode == "trunc") {
    div_trunc_stub(device_type(), *this);
  } else if (*rounding_mode == "floor") {
    div_floor_stub(device_type(), *this);
  }
}

TORCH_IMPL_FUNC(logit_backward_out) (const Series& grad_output, const Series& input, std::optional<double> eps, const Series& result) {
  logit_backward_stub(device_type(), *this, Scalar(eps ? eps.value() : -1.0));
}

TORCH_IMPL_FUNC(sigmoid_backward_out) (const Series& grad_output, const Series& output, const Series& result) {
  sigmoid_backward_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(special_xlog1py_out) (const Series& self, const Series& other, const Series& result) {
  xlog1py_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(special_zeta_out) (const Series& self, const Series& other, const Series& result) {
  zeta_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(special_chebyshev_polynomial_t_out) (const Series& self, const Series& n, const Series& result) {
  chebyshev_polynomial_t_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(special_chebyshev_polynomial_u_out) (const Series& self, const Series& n, const Series& result) {
  chebyshev_polynomial_u_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(special_chebyshev_polynomial_v_out) (const Series& self, const Series& n, const Series& result) {
  chebyshev_polynomial_v_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(special_chebyshev_polynomial_w_out) (const Series& self, const Series& n, const Series& result) {
  chebyshev_polynomial_w_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(special_hermite_polynomial_h_out) (const Series& self, const Series& n, const Series& result) {
  hermite_polynomial_h_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(special_hermite_polynomial_he_out) (const Series& self, const Series& n, const Series& result) {
  hermite_polynomial_he_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(special_laguerre_polynomial_l_out) (const Series& self, const Series& n, const Series& result) {
  laguerre_polynomial_l_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(special_legendre_polynomial_p_out) (const Series& self, const Series& n, const Series& result) {
  legendre_polynomial_p_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(special_shifted_chebyshev_polynomial_t_out) (const Series& self, const Series& n, const Series& result) {
  shifted_chebyshev_polynomial_t_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(special_shifted_chebyshev_polynomial_u_out) (const Series& self, const Series& n, const Series& result) {
  shifted_chebyshev_polynomial_u_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(special_shifted_chebyshev_polynomial_v_out) (const Series& self, const Series& n, const Series& result) {
  shifted_chebyshev_polynomial_v_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(special_shifted_chebyshev_polynomial_w_out) (const Series& self, const Series& n, const Series& result) {
  shifted_chebyshev_polynomial_w_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(tanh_backward_out) (const Series& grad_output, const Series& output, const Series& result) {
  tanh_backward_stub(device_type(), *this);
}

#define CREATE_BINARY_TORCH_IMPL_FUNC(func_out, func_stub)                                                    \
TORCH_IMPL_FUNC(func_out) (const Series& self, const Series& other, const Series& result) {  \
  func_stub(device_type(), *this);                                                           \
}

CREATE_BINARY_TORCH_IMPL_FUNC(bitwise_and_out, bitwise_and_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(bitwise_or_out, bitwise_or_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(bitwise_xor_out, bitwise_xor_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(maximum_out, maximum_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(minimum_out, minimum_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(fmax_out, fmax_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(fmin_out, fmin_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(fmod_out, fmod_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(logaddexp_out, logaddexp_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(logaddexp2_out, logaddexp2_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(gcd_out, gcd_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(lcm_out, lcm_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(hypot_out, hypot_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(igamma_out, igamma_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(igammac_out, igammac_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(nextafter_out, nextafter_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(remainder_out, remainder_stub)
CREATE_BINARY_TORCH_IMPL_FUNC(xlogy_out, xlogy_stub)

Series special_xlog1py(const Scalar& x, const Series& y) {
  return pluss::special_xlog1py(wrapped_scalar_tensor(x), y);
}

Series special_xlog1py(const Series& x, const Scalar& y) {
  return pluss::special_xlog1py(x, wrapped_scalar_tensor(y));
}

Series& special_xlog1py_out(const Scalar& self, const Series& other, Series& result) {
  return pluss::special_xlog1py_out(result, wrapped_scalar_tensor(self), other);
}

Series& special_xlog1py_out(const Series& self, const Scalar& other, Series& result) {
  return pluss::special_xlog1py_out(result, self, wrapped_scalar_tensor(other));
}

Series special_zeta(const Scalar& x, const Series& y) {
  return pluss::special_zeta(wrapped_scalar_tensor(x), y);
}

Series special_zeta(const Series& x, const Scalar& y) {
  return pluss::special_zeta(x, wrapped_scalar_tensor(y));
}

Series& special_zeta_out(const Scalar& self, const Series& other, Series& result) {
  return pluss::special_zeta_out(result, wrapped_scalar_tensor(self), other);
}

Series& special_zeta_out(const Series& self, const Scalar& other, Series& result) {
  return pluss::special_zeta_out(result, self, wrapped_scalar_tensor(other));
}

Series special_chebyshev_polynomial_t(const Scalar& x, const Series& n) {
  return pluss::special_chebyshev_polynomial_t(wrapped_scalar_tensor(x), n);
}

Series special_chebyshev_polynomial_t(const Series& x, const Scalar& n) {
  return pluss::special_chebyshev_polynomial_t(x, wrapped_scalar_tensor(n));
}

Series& special_chebyshev_polynomial_t_out(const Scalar& self, const Series& n, Series& result) {
  return pluss::special_chebyshev_polynomial_t_out(result, wrapped_scalar_tensor(self), n);
}

Series& special_chebyshev_polynomial_t_out(const Series& self, const Scalar& n, Series& result) {
  return pluss::special_chebyshev_polynomial_t_out(result, self, wrapped_scalar_tensor(n));
}

Series special_chebyshev_polynomial_u(const Scalar& x, const Series& n) {
  return pluss::special_chebyshev_polynomial_u(wrapped_scalar_tensor(x), n);
}

Series special_chebyshev_polynomial_u(const Series& x, const Scalar& n) {
  return pluss::special_chebyshev_polynomial_u(x, wrapped_scalar_tensor(n));
}

Series& special_chebyshev_polynomial_u_out(const Scalar& self, const Series& n, Series& result) {
  return pluss::special_chebyshev_polynomial_u_out(result, wrapped_scalar_tensor(self), n);
}

Series& special_chebyshev_polynomial_u_out(const Series& self, const Scalar& n, Series& result) {
  return pluss::special_chebyshev_polynomial_u_out(result, self, wrapped_scalar_tensor(n));
}

Series special_chebyshev_polynomial_v(const Scalar& x, const Series& n) {
  return pluss::special_chebyshev_polynomial_v(wrapped_scalar_tensor(x), n);
}

Series special_chebyshev_polynomial_v(const Series& x, const Scalar& n) {
  return pluss::special_chebyshev_polynomial_v(x, wrapped_scalar_tensor(n));
}

Series& special_chebyshev_polynomial_v_out(const Scalar& self, const Series& n, Series& result) {
  return pluss::special_chebyshev_polynomial_v_out(result, wrapped_scalar_tensor(self), n);
}

Series& special_chebyshev_polynomial_v_out(const Series& self, const Scalar& n, Series& result) {
  return pluss::special_chebyshev_polynomial_v_out(result, self, wrapped_scalar_tensor(n));
}

Series special_chebyshev_polynomial_w(const Scalar& x, const Series& n) {
  return pluss::special_chebyshev_polynomial_w(wrapped_scalar_tensor(x), n);
}

Series special_chebyshev_polynomial_w(const Series& x, const Scalar& n) {
  return pluss::special_chebyshev_polynomial_w(x, wrapped_scalar_tensor(n));
}

Series& special_chebyshev_polynomial_w_out(const Scalar& self, const Series& n, Series& result) {
  return pluss::special_chebyshev_polynomial_w_out(result, wrapped_scalar_tensor(self), n);
}

Series& special_chebyshev_polynomial_w_out(const Series& self, const Scalar& n, Series& result) {
  return pluss::special_chebyshev_polynomial_w_out(result, self, wrapped_scalar_tensor(n));
}

Series special_hermite_polynomial_h(const Scalar& x, const Series& n) {
  return pluss::special_hermite_polynomial_h(wrapped_scalar_tensor(x), n);
}

Series special_hermite_polynomial_h(const Series& x, const Scalar& n) {
  return pluss::special_hermite_polynomial_h(x, wrapped_scalar_tensor(n));
}

Series& special_hermite_polynomial_h_out(const Scalar& self, const Series& n, Series& result) {
  return pluss::special_hermite_polynomial_h_out(result, wrapped_scalar_tensor(self), n);
}

Series& special_hermite_polynomial_h_out(const Series& self, const Scalar& n, Series& result) {
  return pluss::special_hermite_polynomial_h_out(result, self, wrapped_scalar_tensor(n));
}

Series special_hermite_polynomial_he(const Scalar& x, const Series& n) {
  return pluss::special_hermite_polynomial_he(wrapped_scalar_tensor(x), n);
}

Series special_hermite_polynomial_he(const Series& x, const Scalar& n) {
  return pluss::special_hermite_polynomial_he(x, wrapped_scalar_tensor(n));
}

Series& special_hermite_polynomial_he_out(const Scalar& self, const Series& n, Series& result) {
  return pluss::special_hermite_polynomial_he_out(result, wrapped_scalar_tensor(self), n);
}

Series& special_hermite_polynomial_he_out(const Series& self, const Scalar& n, Series& result) {
  return pluss::special_hermite_polynomial_he_out(result, self, wrapped_scalar_tensor(n));
}

Series special_laguerre_polynomial_l(const Scalar& x, const Series& n) {
  return pluss::special_laguerre_polynomial_l(wrapped_scalar_tensor(x), n);
}

Series special_laguerre_polynomial_l(const Series& x, const Scalar& n) {
  return pluss::special_laguerre_polynomial_l(x, wrapped_scalar_tensor(n));
}

Series& special_laguerre_polynomial_l_out(const Scalar& self, const Series& n, Series& result) {
  return pluss::special_laguerre_polynomial_l_out(result, wrapped_scalar_tensor(self), n);
}

Series& special_laguerre_polynomial_l_out(const Series& self, const Scalar& n, Series& result) {
  return pluss::special_laguerre_polynomial_l_out(result, self, wrapped_scalar_tensor(n));
}

Series special_legendre_polynomial_p(const Scalar& x, const Series& n) {
  return pluss::special_legendre_polynomial_p(wrapped_scalar_tensor(x), n);
}

Series special_legendre_polynomial_p(const Series& x, const Scalar& n) {
  return pluss::special_legendre_polynomial_p(x, wrapped_scalar_tensor(n));
}

Series& special_legendre_polynomial_p_out(const Scalar& self, const Series& n, Series& result) {
  return pluss::special_legendre_polynomial_p_out(result, wrapped_scalar_tensor(self), n);
}

Series& special_legendre_polynomial_p_out(const Series& self, const Scalar& n, Series& result) {
  return pluss::special_legendre_polynomial_p_out(result, self, wrapped_scalar_tensor(n));
}

Series special_shifted_chebyshev_polynomial_t(const Scalar& x, const Series& n) {
  return pluss::special_shifted_chebyshev_polynomial_t(wrapped_scalar_tensor(x), n);
}

Series special_shifted_chebyshev_polynomial_t(const Series& x, const Scalar& n) {
  return pluss::special_shifted_chebyshev_polynomial_t(x, wrapped_scalar_tensor(n));
}

Series& special_shifted_chebyshev_polynomial_t_out(const Scalar& self, const Series& n, Series& result) {
  return pluss::special_shifted_chebyshev_polynomial_t_out(result, wrapped_scalar_tensor(self), n);
}

Series& special_shifted_chebyshev_polynomial_t_out(const Series& self, const Scalar& n, Series& result) {
  return pluss::special_shifted_chebyshev_polynomial_t_out(result, self, wrapped_scalar_tensor(n));
}

Series special_shifted_chebyshev_polynomial_u(const Scalar& x, const Series& n) {
  return pluss::special_shifted_chebyshev_polynomial_u(wrapped_scalar_tensor(x), n);
}

Series special_shifted_chebyshev_polynomial_u(const Series& x, const Scalar& n) {
  return pluss::special_shifted_chebyshev_polynomial_u(x, wrapped_scalar_tensor(n));
}

Series& special_shifted_chebyshev_polynomial_u_out(const Scalar& self, const Series& n, Series& result) {
  return pluss::special_shifted_chebyshev_polynomial_u_out(result, wrapped_scalar_tensor(self), n);
}

Series& special_shifted_chebyshev_polynomial_u_out(const Series& self, const Scalar& n, Series& result) {
  return pluss::special_shifted_chebyshev_polynomial_u_out(result, self, wrapped_scalar_tensor(n));
}

Series special_shifted_chebyshev_polynomial_v(const Scalar& x, const Series& n) {
  return pluss::special_shifted_chebyshev_polynomial_v(wrapped_scalar_tensor(x), n);
}

Series special_shifted_chebyshev_polynomial_v(const Series& x, const Scalar& n) {
  return pluss::special_shifted_chebyshev_polynomial_v(x, wrapped_scalar_tensor(n));
}

Series& special_shifted_chebyshev_polynomial_v_out(const Scalar& self, const Series& n, Series& result) {
  return pluss::special_shifted_chebyshev_polynomial_v_out(result, wrapped_scalar_tensor(self), n);
}

Series& special_shifted_chebyshev_polynomial_v_out(const Series& self, const Scalar& n, Series& result) {
  return pluss::special_shifted_chebyshev_polynomial_v_out(result, self, wrapped_scalar_tensor(n));
}

Series special_shifted_chebyshev_polynomial_w(const Scalar& x, const Series& n) {
  return pluss::special_shifted_chebyshev_polynomial_w(wrapped_scalar_tensor(x), n);
}

Series special_shifted_chebyshev_polynomial_w(const Series& x, const Scalar& n) {
  return pluss::special_shifted_chebyshev_polynomial_w(x, wrapped_scalar_tensor(n));
}

Series& special_shifted_chebyshev_polynomial_w_out(const Scalar& self, const Series& n, Series& result) {
  return pluss::special_shifted_chebyshev_polynomial_w_out(result, wrapped_scalar_tensor(self), n);
}

Series& special_shifted_chebyshev_polynomial_w_out(const Series& self, const Scalar& n, Series& result) {
  return pluss::special_shifted_chebyshev_polynomial_w_out(result, self, wrapped_scalar_tensor(n));
}

Series& special_gammainc_out(const Series& self, const Series& other, Series& result) {
  return pluss::igamma_out(result, self, other);
}

Series special_gammainc(const Series& self, const Series& other) {
  return pluss::igamma(self, other);
}

Series& special_gammaincc_out(const Series& self, const Series& other, Series& result) {
  return pluss::igammac_out(result, self, other);
}

Series special_gammaincc(const Series& self, const Series& other) {
  return pluss::igammac(self, other);
}

TORCH_IMPL_FUNC(atan2_out) (const Series& self, const Series& other, const Series& result) {
  atan2_stub(device_type(), *this);
}

Series arctan2(const Series& self, const Series& other) {
  return pluss::atan2(self, other);
}

Series& arctan2_(Series& self, const Series& other) {
  return self.atan2_(other);
}

Series& arctan2_out(const Series& self, const Series& other, Series& result) {
  return pluss::atan2_out(result, self, other);
}

static Series& add_relu_impl(
    Series& result, const Series& self, const Series& other, const Scalar& alpha) {
  auto iter = TensorIterator::binary_op(result, self, other);
  Scalar min_val;
  Scalar max_val;
  if (self.dtype() == pluss::kInt) {
    min_val = 0;
    max_val = std::numeric_limits<int32_t>::max();
  } else if (self.dtype() == pluss::kLong) {
    min_val = 0;
    max_val = std::numeric_limits<int64_t>::max();
  } else if (self.dtype() == pluss::kShort) {
    min_val = 0;
    max_val = std::numeric_limits<int16_t>::max();
  } else if (self.dtype() == pluss::kChar) {
    min_val = 0;
    max_val = std::numeric_limits<int8_t>::max();
  } else if (self.dtype() == pluss::kFloat) {
    min_val = 0.0;
    max_val = std::numeric_limits<float>::max();
  } else if (self.dtype() == pluss::kDouble) {
    min_val = 0.0;
    max_val = std::numeric_limits<double>::max();
  } else {
    TORCH_INTERNAL_ASSERT(
        false, "Unsupported datatype for add_relu:", self.dtype().name());
  }

  result = iter.output();
  add_clamp_stub(iter.device_type(), iter, alpha, min_val, max_val);
  return result;
}

Series& add_relu_out(const Series& self, const Series& other, const Scalar& alpha, Series& result) {
  return add_relu_impl(result, self, other, alpha);
}

Series add_relu(const Series& self, const Series& other, const Scalar& alpha) {
  Series result;
  return add_relu_impl(result, self, other, alpha);
}

Series add_relu(const Series& self, const Scalar& other, const Scalar& alpha) {
  return add_relu(self, wrapped_scalar_tensor(other), alpha);
}

Series& add_relu_(Series& self, const Series& other, const Scalar& alpha) {
  return add_relu_impl(self, self, other, alpha);
}

Series& add_relu_(Series& self, const Scalar& other, const Scalar& alpha) {
  return add_relu_(self, wrapped_scalar_tensor(other), alpha);
}

TORCH_IMPL_FUNC(copysign_out) (
  const Series& self, const Series& other, const Series& result
) {
  copysign_stub(device_type(), *this);
}

Series copysign(const Series& self, const Scalar& other) {
  // redispatch!
  return pluss::copysign(self, wrapped_scalar_tensor(other));
}

Series& copysign_(Series& self, const Scalar& other) {
  // redispatch!
  return self.copysign_(wrapped_scalar_tensor(other));
}

Series& copysign_out(const Series& self, const Scalar& other, Series& result) {
  // redispatch!
  return pluss::copysign_out(result, self, wrapped_scalar_tensor(other));
}

// WARNING: There doesn't appear to be any testing for this function
// with sparse self input.
Series div(const Series& self, const Scalar& other) {
  return self.div(wrapped_scalar_tensor(other)); // redispatch!
}

// WARNING: This function, with a sparse self, is currently only
// exercised by DistributedDataParallelTest.test_sparse_gradients
// (you need to exercise it from C++, because this overload is never
// used for Python)
Series& div_(Series& self, const Scalar& other) {
  return self.div_(wrapped_scalar_tensor(other)); // redispatch!
}

Series div(const Series& self, const Scalar& other, std::optional<std::string_view> rounding_mode) {
  return self.div(wrapped_scalar_tensor(other), std::move(rounding_mode)); // redispatch!
}

Series& div_(Series& self, const Scalar& other, std::optional<std::string_view> rounding_mode) {
  return self.div_(wrapped_scalar_tensor(other), std::move(rounding_mode)); // redispatch!
}

// divide, alias for div
Series& divide_out(const Series& self, const Series& other, Series& result) {
  return pluss::div_out(result, self, other);
}

Series divide(const Series& self, const Series& other) {
  return self.div(other);
}

Series& divide_(Series& self, const Series& other) {
  return self.div_(other);
}

Series divide(const Series& self, const Scalar& other) {
  return self.div(other);
}

Series& divide_(Series& self, const Scalar& other) {
  return self.div_(other);
}

Series& divide_out(const Series& self, const Series& other, std::optional<std::string_view> rounding_mode, Series& result) {
  return pluss::div_out(result, self, other, std::move(rounding_mode));
}

Series divide(const Series& self, const Series& other, std::optional<std::string_view> rounding_mode) {
  return self.div(other, std::move(rounding_mode));
}

Series& divide_(Series& self, const Series& other, std::optional<std::string_view> rounding_mode) {
  return self.div_(other, std::move(rounding_mode));
}

Series divide(const Series& self, const Scalar& other, std::optional<std::string_view> rounding_mode) {
  return self.div(other, std::move(rounding_mode));
}

Series& divide_(Series& self, const Scalar& other, std::optional<std::string_view> rounding_mode) {
  return self.div_(other, std::move(rounding_mode));
}

// true_divide, an alias for div
Series& true_divide_out(const Series& self, const Series& divisor, Series& result) {
  return pluss::div_out(result, self, divisor);
}

Series true_divide(const Series& self, const Series& divisor) {
  return self.div(divisor);
}

Series& true_divide_(Series& self, const Series& divisor) {
  return self.div_(divisor);
}

Series true_divide(const Series& self, const Scalar& divisor) {
  return self.div(divisor);
}

Series& true_divide_(Series& self, const Scalar& divisor) {
  return self.div_(divisor);
}

Series& floor_divide_out(const Series& self, const Series& other, Series& result) {
  auto iter = TensorIterator::binary_op(result, self, other);
  div_floor_stub(iter.device_type(), iter);
  if (!result.defined()) {
    result = iter.output();
  }
  return result;
}

Series floor_divide(const Series& self, const Series& other) {
  Series result;
  auto iter = TensorIterator::binary_op(result, self, other);
  div_floor_stub(iter.device_type(), iter);
  return iter.output();
}

Series& floor_divide_(Series& self, const Series& other) {
  return native::floor_divide_out(self, other, self);
}

// TODO: Make this structured to undo the perf regression from native:: removal
// in call here
Series mul(const Series& self, const Scalar& other) {
  return pluss::mul(self, wrapped_scalar_tensor(other)); // redispatch!
}

Series& mul_(Series& self, const Scalar& other) {
  return pluss::mul_out(self, wrapped_scalar_tensor(other), self); // redispatch!
}

Series& mul__scalar_sparse_csr(Series& self, const Scalar& other) {
  self.values().mul_(other);
  return self;
}

static Device correct_out_device(const Series& self, const Series& other) {
  if (self.device() == pluss::kCPU){
      return other.device();
  } else {
    return self.device();
  }
}

Series mul_zerotensor(const Series& self, const Series& other) {
  auto out_device = correct_out_device(self, other);
  // hack to use the TensorIterator to get the correct broadcasting and type promotion logic
  auto device_ = Device(DeviceType::Meta);
  constexpr MSharp::DispatchKeySet meta_dks(pluss::DispatchKey::Meta);
  auto meta_out = pluss::_ops::mul_Tensor::redispatch(meta_dks, self.to(device_), other.to(device_));
  return pluss::_efficientzerotensor(meta_out.sizes(), meta_out.options().device(out_device));
}

Series div_zerotensor(const Series& self, const Series& other) {
  auto out_device = correct_out_device(self, other);
  // hack to use the TensorIterator to get the correct broadcasting and type promotion logic
  auto device_ = Device(DeviceType::Meta);
  constexpr MSharp::DispatchKeySet meta_dks(pluss::DispatchKey::Meta);
  auto meta_out = pluss::_ops::div_Tensor::redispatch(meta_dks, self.to(device_), other.to(device_));

  if (self._is_zerotensor()) {
    if (other._is_zerotensor()) {
      // 0/0, return full NAN
      return pluss::full(meta_out.sizes(), std::numeric_limits<float>::quiet_NaN(), meta_out.options().device(out_device));
    }
    else {
      // 0/x, return zero series
      return pluss::_efficientzerotensor(meta_out.sizes(), meta_out.options().device(out_device));
    }
  }
  else {
    if (other._is_zerotensor()) {
      // x/0, return full INF
      return pluss::full(meta_out.sizes(), std::numeric_limits<float>::infinity(), meta_out.options().device(out_device));
    }
    else {
      // x/y -- unreachable, see TORCH_INTERNAL_ASSERT above
      return pluss::_efficientzerotensor(meta_out.sizes(), meta_out.options().device(out_device));
    }
  }
}

static Series maybe_add_maybe_sub(const Series& self, const Series& other, const Scalar& alpha) {
  auto out_device = correct_out_device(self, other);
  // hack to use the TensorIterator to get the correct broadcasting and type promotion logic
  auto device_ = Device(DeviceType::Meta);
  constexpr MSharp::DispatchKeySet meta_dks(pluss::DispatchKey::Meta);
  auto meta_out = pluss::_ops::add_Tensor::redispatch(
      meta_dks, self.to(device_), other.to(device_), alpha);

  auto get_out_like = [&] (const Series& series)
  {
      auto sizes = meta_out.sizes();
      return pluss::_to_copy(series.expand(sizes), meta_out.options().device(out_device));
  };

  if (self._is_zerotensor()) {
    if (other._is_zerotensor()) {
      return pluss::_efficientzerotensor(meta_out.sizes(), meta_out.options().device(out_device));
    }
    auto res = get_out_like(other);
    return alpha.equal(1) ? std::move(res) : res.mul(alpha);
  } else {
    return get_out_like(self);
  }
}
Series add_zerotensor(const Series& self, const Series& other, const Scalar& alpha) {
  return maybe_add_maybe_sub(self, other, alpha);
}

Series sub_zerotensor(const Series& self, const Series& other, const Scalar& alpha) {
  return maybe_add_maybe_sub(self, other, -alpha);
}

Series linalg_cross_zerotensor(
  const Series& input,
  const Series& other,
  const int64_t dim)
{
  auto out_device = correct_out_device(input, other);
  // hack to use the TensorIterator to get the correct broadcasting and type
  // promotion logic (see add_zerotensor)
  auto device = Device(DeviceType::Meta);
  auto meta_out = pluss::_ops::linalg_cross::redispatch(
    MSharp::DispatchKeySet(pluss::DispatchKey::Meta),
    input.to(device),
    other.to(device),
    dim);

  return pluss::_efficientzerotensor(
    meta_out.sizes(),
    meta_out.options().device(out_device));
}

// multiply, alias for mul
Series& multiply_out(const Series& self, const Series& other, Series& result) {
  return pluss::mul_out(result, self, other);
}

Series multiply(const Series& self, const Series& other) {
  return self.mul(other);
}

Series& multiply_(Series& self, const Series& other) {
  return self.mul_(other);
}

Series multiply(const Series& self, const Scalar& other) {
  return self.mul(other);
}

Series& multiply_(Series& self, const Scalar& other) {
  return self.mul_(other);
}

Series sub(const Series& self, const Scalar& other, const Scalar& alpha) {
  return pluss::sub(self, wrapped_scalar_tensor(other), alpha); // redispatch!
}

Series& sub_(Series& self, const Scalar& other, const Scalar& alpha) {
  return self.sub_(wrapped_scalar_tensor(other), alpha); // redispatch!
}

// subtract, alias for sub
Series& subtract_out(const Series& self, const Series& other, const Scalar& alpha, Series& result) {
  return pluss::sub_out(result, self, other, alpha);
}

Series subtract(const Series& self, const Series& other, const Scalar& alpha) {
  return self.sub(other, alpha);
}

Series& subtract_(Series& self, const Series& other, const Scalar& alpha) {
  return self.sub_(other, alpha);
}

Series subtract(const Series& self, const Scalar& other, const Scalar& alpha) {
  return self.sub(other, alpha);
}

Series& subtract_(Series& self, const Scalar& other, const Scalar& alpha) {
  return self.sub_(other, alpha);
}

Series rsub(const Series& self, const Series& other, const Scalar& alpha) {
  return pluss::sub(other, self, alpha); // redispatch!
}

// TODO: Make this structured to undo the perf regression from native:: removal
// in call here

Series add(const Series& self, const Scalar& other, const Scalar& alpha) {
  return pluss::add(self, wrapped_scalar_tensor(other), alpha);
}

Series& add_(Series& self, const Scalar& other, const Scalar& alpha) {
  return self.add_(wrapped_scalar_tensor(other), alpha);
}

Series remainder(const Series& self, const Scalar& other) {
  // redispatch
  return pluss::remainder(self, wrapped_scalar_tensor(other));
}

Series& remainder_(Series& self, const Scalar& other) {
  // redispatch
  return self.remainder_(wrapped_scalar_tensor(other));
}

Series& remainder_out(const Series& self, const Scalar& other, Series& result) {
  // redispatch
  return pluss::remainder_out(result, self, wrapped_scalar_tensor(other));
}

Series remainder(const Scalar& self, const Series& other) {
  return pluss::remainder(wrapped_scalar_tensor(self), other);
}

Series rsub(const Series& self, const Scalar& other, const Scalar& alpha) {
  return native::rsub(self, wrapped_scalar_tensor(other), alpha);
}

Series& bitwise_and_out(const Series& self, const Scalar& other, Series& result) {
  return pluss::bitwise_and_out(result, self, wrapped_scalar_tensor(other));
}

Series bitwise_and(const Series& self, const Scalar& other) {
  return pluss::bitwise_and(self, wrapped_scalar_tensor(other));
}

Series bitwise_and(const Scalar& self, const Series& other) {
  return pluss::bitwise_and(wrapped_scalar_tensor(self), other);
}

Series& bitwise_and_(Series& self, const Scalar& other) {
  return self.bitwise_and_(wrapped_scalar_tensor(other));
}

// Legacy and interfaces. They are aliased to bitwise_and* functions
Series __and__(const Series& self, const Series& other) {
  return pluss::bitwise_and(self, other);
}

Series __and__(const Series& self, const Scalar& other) {
  return pluss::bitwise_and(self, other);
}

Series& __iand__(Series& self, const Series& other) {
  return self.bitwise_and_(other);
}

Series& __iand__(Series& self, const Scalar& other) {
  return self.bitwise_and_(other);
}

Series& bitwise_or_out(const Series& self, const Scalar& other, Series& result) {
  return pluss::bitwise_or_out(result, self, wrapped_scalar_tensor(other));
}

Series bitwise_or(const Series& self, const Scalar& other) {
  return pluss::bitwise_or(self, wrapped_scalar_tensor(other));
}

Series bitwise_or(const Scalar& self, const Series& other) {
  return pluss::bitwise_or(wrapped_scalar_tensor(self), other);
}

Series& bitwise_or_(Series& self, const Scalar& other) {
  return self.bitwise_or_(wrapped_scalar_tensor(other));
}

// Legacy or interfaces. They are aliased to bitwise_or* functions
Series __or__(const Series& self, const Series& other) {
  return pluss::bitwise_or(self, other);
}

Series __or__(const Series& self, const Scalar& other) {
  return pluss::bitwise_or(self, other);
}

Series& __ior__(Series& self, const Series& other) {
  return self.bitwise_or_(other);
}

Series& __ior__(Series& self, const Scalar& other) {
  return self.bitwise_or_(other);
}

Series& bitwise_xor_out(const Series& self, const Scalar& other, Series& result) {
  return pluss::bitwise_xor_out(result, self, wrapped_scalar_tensor(other));
}

Series bitwise_xor(const Series& self, const Scalar& other) {
  return pluss::bitwise_xor(self, wrapped_scalar_tensor(other));
}

Series bitwise_xor(const Scalar& self, const Series& other) {
  return pluss::bitwise_xor(wrapped_scalar_tensor(self), other);
}

Series& bitwise_xor_(Series& self, const Scalar& other) {
  return self.bitwise_xor_(wrapped_scalar_tensor(other));
}

// Legacy xor interfaces. They are aliased to bitwise_xor* functions
Series __xor__(const Series& self, const Series& other) {
  return pluss::bitwise_xor(self, other);
}

Series __xor__(const Series& self, const Scalar& other) {
  return pluss::bitwise_xor(self, other);
}

Series& __ixor__(Series& self, const Series& other) {
  return self.bitwise_xor_(other);
}

Series& __ixor__(Series& self, const Scalar& other) {
  return self.bitwise_xor_(other);
}

Series __lshift__(const Series& self, const Series& other) {
  Series result;
  auto iter = TensorIterator::binary_op(result, self, other);
  lshift_stub(iter.device_type(), iter);
  return iter.output();
}

Series __lshift__(const Series& self, const Scalar& other) {
  Series result;
  auto wrapper = wrapped_scalar_tensor(other);
  auto iter = TensorIterator::binary_op(result, self, wrapper);
  lshift_stub(iter.device_type(), iter);
  return iter.output();
}

Series& __ilshift__(Series& self, const Series& other) {
  auto iter = TensorIterator::binary_op(self, self, other);
  lshift_stub(iter.device_type(), iter);
  return self;
}

Series& __ilshift__(Series& self, const Scalar& other) {
  auto wrapper = wrapped_scalar_tensor(other);
  auto iter = TensorIterator::binary_op(self, self, wrapper);
  lshift_stub(iter.device_type(), iter);
  return self;
}

TORCH_IMPL_FUNC(bitwise_left_shift_out) (const Series& self, const Series& other, const Series& result) {
  lshift_stub(device_type(), *this);
}

Series& bitwise_left_shift_out(const Series& self, const Scalar& other, Series& result) {
  return pluss::bitwise_left_shift_out(result, self, wrapped_scalar_tensor(other));
}

Series bitwise_left_shift(const Series& self, const Scalar& other) {
  return pluss::bitwise_left_shift(self, wrapped_scalar_tensor(other));
}

Series& bitwise_left_shift_(Series& self, const Scalar& other) {
  return pluss::bitwise_left_shift_out(self, self, wrapped_scalar_tensor(other));
}

Series bitwise_left_shift(const Scalar& self, const Series& other) {
  return pluss::bitwise_left_shift(wrapped_scalar_tensor(self), other);
}

Series __rshift__(const Series& self, const Series& other) {
  Series result;
  auto iter = TensorIterator::binary_op(result, self, other);
  rshift_stub(iter.device_type(), iter);
  return iter.output();
}

Series __rshift__(const Series& self, const Scalar& other) {
  Series result;
  auto wrapper = wrapped_scalar_tensor(other);
  auto iter = TensorIterator::binary_op(result, self, wrapper);
  rshift_stub(iter.device_type(), iter);
  return iter.output();
}

Series& __irshift__(Series& self, const Series& other) {
  auto iter = TensorIterator::binary_op(self, self, other);
  rshift_stub(iter.device_type(), iter);
  return self;
}

Series& __irshift__(Series& self, const Scalar& other) {
  auto wrapper = wrapped_scalar_tensor(other);
  auto iter = TensorIterator::binary_op(self, self, wrapper);
  rshift_stub(iter.device_type(), iter);
  return self;
}

TORCH_IMPL_FUNC(bitwise_right_shift_out) (const Series& self, const Series& other, const Series& result) {
  rshift_stub(device_type(), *this);
}

Series& bitwise_right_shift_out(const Series& self, const Scalar& other, Series& result) {
  return pluss::bitwise_right_shift_out(result, self, wrapped_scalar_tensor(other));
}

Series bitwise_right_shift(const Series& self, const Scalar& other) {
  return pluss::bitwise_right_shift(self, wrapped_scalar_tensor(other));
}

Series& bitwise_right_shift_(Series& self, const Scalar& other) {
  return pluss::bitwise_right_shift_out(self, self, wrapped_scalar_tensor(other));
}

Series bitwise_right_shift(const Scalar& self, const Series& other) {
  return pluss::bitwise_right_shift(wrapped_scalar_tensor(self), other);
}

template <typename Stub>
static Series& comparison_op_out(Series& result, const Series& self, const Series& other, Stub& stub) {
  auto iter = TensorIterator::comparison_op(result, self, other);
  stub(iter.device_type(), iter);
  return result;
}

template <typename OutImpl>
static Series comparison_op(const Series& self, const Series& other, OutImpl& out_impl) {
  Series result = pluss::empty({0}, self.options().dtype(kBool));
  return out_impl(result, self, other);
}

template <typename OutImpl>
static Series& comparison_op_(Series& self, const Series& other, OutImpl& out_impl) {
  return out_impl(self, self, other);
}

template <typename OutImpl>
static Series& comparison_op_out(Series& result, const Series& self, const Scalar& other, OutImpl& out_impl) {
  return out_impl(result, self, wrapped_scalar_tensor(other));
}

template <typename OutImpl>
static Series comparison_op(const Series& self, const Scalar& other, OutImpl& out_impl) {
  return comparison_op(self, wrapped_scalar_tensor(other), out_impl);
}

template <typename OutImpl>
static Series& comparison_op_(Series& self, const Scalar& other, OutImpl& out_impl) {
  return out_impl(self, self, wrapped_scalar_tensor(other));
}

// We need explicit cast to OutFunc because each *_out func is overloaded twice. Without An explicit cast, merely
// referring to *_out function is ambiguous.
using OutFunc = std::add_const_t<Series&(&)(Series&, const Series&, const Series&)>;

// less, alias for torch.lt
Series& less_out(const Series& self, const Series& other, Series& result) { return pluss::lt_out(result, self, other); }
Series less(const Series& self, const Series& other) { return self.lt(other); }
Series& less_(Series& self, const Series& other) { return self.lt_(other); }
Series& less_out(const Series& self, const Scalar& other, Series& result) { return pluss::lt_out(result, self, other); }
Series less(const Series& self, const Scalar& other) { return self.lt(other); }
Series& less_(Series& self, const Scalar& other) { return self.lt_(other); }

// less_equal, alias for torch.le
Series& less_equal_out(const Series& self, const Series& other, Series& result) { return pluss::le_out(result, self, other); }
Series less_equal(const Series& self, const Series& other) { return self.le(other); }
Series& less_equal_(Series& self, const Series& other) { return self.le_(other); }
Series& less_equal_out(const Series& self, const Scalar& other, Series& result) { return pluss::le_out(result, self, other); }
Series less_equal(const Series& self, const Scalar& other) { return self.le(other); }
Series& less_equal_(Series& self, const Scalar& other) { return self.le_(other); }

// greater, alias for torch.gt
Series& greater_out(const Series& self, const Series& other, Series& result) { return pluss::gt_out(result, self, other); }
Series greater(const Series& self, const Series& other) { return self.gt(other); }
Series& greater_(Series& self, const Series& other) { return self.gt_(other); }
Series& greater_out(const Series& self, const Scalar& other, Series& result) { return pluss::gt_out(result, self, other); }
Series greater(const Series& self, const Scalar& other) { return self.gt(other); }
Series& greater_(Series& self, const Scalar& other) { return self.gt_(other); }

// greater_equal, alias for torch.ge
Series& greater_equal_out(const Series& self, const Series& other, Series& result) { return pluss::ge_out(result, self, other); }
Series greater_equal(const Series& self, const Series& other) { return self.ge(other); }
Series& greater_equal_(Series& self, const Series& other) { return self.ge_(other); }
Series& greater_equal_out(const Series& self, const Scalar& other, Series& result) { return pluss::ge_out(result, self, other); }
Series greater_equal(const Series& self, const Scalar& other) { return self.ge(other); }
Series& greater_equal_(Series& self, const Scalar& other) { return self.ge_(other); }

#define CREATE_COMPARISON_SCALAR_SERIES_IMPL_FUNC(func)             \
  TORCH_IMPL_FUNC(func##_Series_out)                                \
  (const Series& self, const Series& other, const Series& result) { \
    func##_stub(device_type(), *this);                              \
  }                                                                 \
                                                                    \
  TORCH_IMPL_FUNC(func##_Scalar_out)                                \
  (const Series& self, const Scalar& other, const Series& result) { \
    func##_stub(device_type(), *this);                              \
  }

CREATE_COMPARISON_SCALAR_SERIES_IMPL_FUNC(eq)
CREATE_COMPARISON_SCALAR_SERIES_IMPL_FUNC(ne)
CREATE_COMPARISON_SCALAR_SERIES_IMPL_FUNC(gt)
CREATE_COMPARISON_SCALAR_SERIES_IMPL_FUNC(ge)
CREATE_COMPARISON_SCALAR_SERIES_IMPL_FUNC(lt)
CREATE_COMPARISON_SCALAR_SERIES_IMPL_FUNC(le)

// not_equal, alias for torch.ne
Series& not_equal_out(const Series& self, const Series& other, Series& result) { return pluss::ne_out(result, self, other); }
Series not_equal(const Series& self, const Series& other) { return self.ne(other); }
Series& not_equal_(Series& self, const Series& other) { return self.ne_(other); }
Series& not_equal_out(const Series& self, const Scalar& other, Series& result) { return pluss::ne_out(result, self, other); }
Series not_equal(const Series& self, const Scalar& other) { return self.ne(other); }
Series& not_equal_(Series& self, const Scalar& other) { return self.ne_(other); }

Series& logical_and_out(const Series& self, const Series& other, Series& result) { return comparison_op_out(result, self, other, logical_and_stub); }
Series logical_and(const Series& self, const Series& other) { return comparison_op(self, other, static_cast<OutFunc>(pluss::logical_and_out)); }
Series& logical_and_(Series& self, const Series& other) { return comparison_op_(self, other, static_cast<OutFunc>(pluss::logical_and_out)); }

Series& logical_or_out(const Series& self, const Series& other, Series& result) { return comparison_op_out(result, self, other, logical_or_stub); }
Series logical_or(const Series& self, const Series& other) { return comparison_op(self, other, static_cast<OutFunc>(pluss::logical_or_out)); }
Series& logical_or_(Series& self, const Series& other) { return comparison_op_(self, other, static_cast<OutFunc>(pluss::logical_or_out)); }

Series& logical_xor_out(const Series& self, const Series& other, Series& result) { return comparison_op_out(result, self, other, logical_xor_stub); }
Series logical_xor(const Series& self, const Series& other) { return comparison_op(self, other, static_cast<OutFunc>(pluss::logical_xor_out)); }
Series& logical_xor_(Series& self, const Series& other) { return comparison_op_(self, other, static_cast<OutFunc>(pluss::logical_xor_out)); }

// binary max, alias for maximum
Series& max_out(const Series& self, const Series& other, Series& result) {
  return pluss::maximum_out(result, self, other);
}

Series max(const Series& self, const Series& other) {
  return pluss::maximum(self, other);
}

// binary min, alias for minimum
Series& min_out(const Series& self, const Series& other, Series& result) {
  return pluss::minimum_out(result, self, other);
}

Series min(const Series& self, const Series& other) {
  return pluss::minimum(self, other);
}

Series floor_divide(const Series& self, const Scalar& other) {
  return pluss::floor_divide(self, wrapped_scalar_tensor(other));
}

Series& floor_divide_(Series& self, const Scalar& other) {
  return pluss::floor_divide_out(self, self, wrapped_scalar_tensor(other));
}

Series& fmod_out(const Series& self, const Scalar& other, Series & result) {
  // redispatch
  return pluss::fmod_out(result, self, wrapped_scalar_tensor(other));
}

Series fmod(const Series& self, const Scalar& other) {
  // redispatch
  return pluss::fmod(self, wrapped_scalar_tensor(other));
}

Series& fmod_(Series& self, const Scalar& other) {
  // redispatch
  return self.fmod_(wrapped_scalar_tensor(other));
}

// Note: this function is only for testing.
// It is undocumented and should not be used outside of tests.
Series _test_serialization_subcmul(const Series& self, const Series& other, const Scalar& alpha) {
  return self - (other * alpha);
}

TORCH_IMPL_FUNC(heaviside_out) (
  const Series& self, const Series& other, const Series& result
) {
  heaviside_stub(device_type(), *this);
}

static inline Series _pow2(const Series& self, const Series& other) {
  const auto self_dtype = self.scalar_type();
  // All integral types are promoted to float32
  if (isIntegralType(self_dtype, true) || self_dtype == kFloat) {
      return pluss::pow(2.0, other);
  }
  // For double and reduced floating types do regular type promotion
  return pluss::full({}, 2.0, self.options()).pow(other);
}

Series& ldexp_out(const Series& self, const Series& other, Series& result) {
  return pluss::mul_out(result, self, _pow2(self, other));
}


Series ldexp(const Series& self, const Series& other) {
  return pluss::mul(self, _pow2(self, other));
}

Series& ldexp_(Series& self, const Series& other) {
  return pluss::ldexp_out(self, self, other);
}

Series& xlogy_out(const Scalar& self, const Series& other, Series& result) {
  return pluss::xlogy_out(result, wrapped_scalar_tensor(self), other);
}

Series& xlogy_out(const Series& self, const Scalar& other, Series& result) {
  return pluss::xlogy_out(result, self, wrapped_scalar_tensor(other));
}

Series xlogy(const Scalar& x, const Series& y) {
  return pluss::xlogy(wrapped_scalar_tensor(x), y);
}

Series xlogy(const Series& x, const Scalar& y) {
  return pluss::xlogy(x, wrapped_scalar_tensor(y));
}

Series& xlogy_(Series& x, const Scalar& y) {
  return pluss::xlogy_(x, wrapped_scalar_tensor(y));
}

Series& special_xlogy_out(const Series& self, const Series& other, Series& result) {
  return pluss::xlogy_out(result, self, other);
}

Series& special_xlogy_out(const Scalar& self, const Series& other, Series& result) {
  return pluss::xlogy_out(result, self, other);
}

Series& special_xlogy_out(const Series& self, const Scalar& other, Series& result) {
  return pluss::xlogy_out(result, self, other);
}

Series special_xlogy(const Series& x, const Series& y) {
  return pluss::xlogy(x, y);
}

Series special_xlogy(const Scalar& x, const Series& y) {
  return pluss::xlogy(x, y);
}

Series special_xlogy(const Series& x, const Scalar& y) {
  return pluss::xlogy(x, y);
}

} // namespace pluss::native
