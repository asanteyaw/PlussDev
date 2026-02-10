#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/native/Pow.h>

#include <PlussQit/core/Series.h>
#include <PlussQit/ScalarOps.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/float_power_native.h>
#include <PlussQit/ops/pow.h>
#include <PlussQit/ops/pow_native.h>
#include <PlussQit/ops/result_type.h>
#endif

namespace pluss::meta {

TORCH_META_FUNC2(pow, Series_Tensor) (const Series& base, const Series& exp) {
  build_borrowing_binary_op(maybe_get_output(), base, exp);
}

TORCH_META_FUNC2(pow, Series_Scalar) (const Series& base, const Scalar& exp) {
  // Numpy compatibility check:
  TORCH_CHECK(!(isIntegralType(base.scalar_type(), true) &&
              exp.isIntegral(true) && exp.toLong() < 0),
              "Integers to negative integer powers are not allowed.");

  auto common_dtype = pluss::result_type(base, exp);
  build_output_borrowing_argument_owning_unary_op(maybe_get_output(), base.to(common_dtype));
}

TORCH_META_FUNC2(pow, Scalar) (const Scalar& base, const Series& exp) {
    // This overload doesn't directly use TensorIterator. It attempts to short-circuit,
    // but otherwise redispatches to the Series_Tensor overload.
    auto dtype = maybe_get_output().defined() ? maybe_get_output().scalar_type() : pluss::result_type(base, exp);
    set_output_raw_strided(0, exp.sizes(), {}, exp.options().dtype(dtype), exp.has_names() ? exp.names() : ArrayRef<Dimname>());
}

} // namespace pluss::meta

namespace pluss::native {

DEFINE_DISPATCH(pow_tensor_tensor_stub);
DEFINE_DISPATCH(pow_tensor_scalar_stub);

TORCH_IMPL_FUNC(pow_Series_Series_out) (const Series& base, const Series& exp, const Series& out) {
  pow_tensor_tensor_stub(device_type(), *this);
}

TORCH_IMPL_FUNC(pow_Series_Scalar_out) (const Series& base, const Scalar& exp, const Series& out) {
  if (exp.equal(0.0) || exp.equal(false)) {
    out.fill_(1);
  } else if (exp.equal(1.0) || exp.equal(true) ) {
    out.copy_(base);
  } else {
    pow_tensor_scalar_stub(device_type(), *this, exp);
  }
}

TORCH_IMPL_FUNC(pow_Scalar_out) (const Scalar& base, const Series& exp, const Series& out) {
  if (base.equal(1.0)) {
    out.fill_(1);
  } else {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    pluss::pow_out(const_cast<Series&>(out), wrapped_scalar_tensor(base, exp.device()), exp); // redispatch!
  }
}

Series& float_power_out(const Series& base, const Series& exp, Series& result) {
  auto dtype = (pluss::isComplexType(base.scalar_type()) || pluss::isComplexType(exp.scalar_type())) ?
                pluss::kComplexDouble : pluss::kDouble;
  TORCH_CHECK(result.scalar_type() == dtype,
              "the output given to float_power has dtype ", result.scalar_type(),
              " but the operation's result requires dtype ", dtype);

  return pluss::pow_out(result, base.to(dtype), exp.to(dtype));
}

Series& float_power_out(const Series& base, const Scalar& exp, Series& result) {
  auto dtype = (pluss::isComplexType(base.scalar_type()) || exp.isComplex()) ? pluss::kComplexDouble : pluss::kDouble;
  TORCH_CHECK(result.scalar_type() == dtype,
              "the output given to float_power has dtype ", result.scalar_type(),
              " but the operation's result requires dtype ", dtype);

  // Note: need the casts inside the ternary because conversion functions return e.g. MSharp::complex,
  // which causes a complex scalar to always be returned.
  auto casted_exp = (dtype == pluss::kComplexDouble) ? Scalar(exp.toComplexDouble()) : Scalar(exp.toDouble());
  return pluss::pow_out(result, base.to(dtype), casted_exp);
}

Series& float_power_out(const Scalar& base, const Series& exp, Series& result) {
  auto dtype = (pluss::isComplexType(exp.scalar_type()) || base.isComplex()) ? pluss::kComplexDouble : pluss::kDouble;
  TORCH_CHECK(result.scalar_type() == dtype,
              "the output given to float_power has dtype ", result.scalar_type(),
              " but the operation's result requires dtype ", dtype);

  auto casted_base = (dtype == pluss::kComplexDouble) ? Scalar(base.toComplexDouble()) : Scalar(base.toDouble());
  return pluss::pow_out(result, casted_base, exp.to(dtype));
}

Series float_power(const Series& base, const Scalar& exp) {
  auto dtype = (pluss::isComplexType(base.scalar_type()) || exp.isComplex()) ? pluss::kComplexDouble : pluss::kDouble;
  auto casted_exp = (dtype == pluss::kComplexDouble) ? Scalar(exp.toComplexDouble()) : Scalar(exp.toDouble());
  return pluss::pow(base.to(dtype), casted_exp);
}

Series float_power(const Scalar& base, const Series& exp) {
  auto dtype = (pluss::isComplexType(exp.scalar_type()) || base.isComplex()) ? pluss::kComplexDouble : pluss::kDouble;
  auto casted_base = (dtype == pluss::kComplexDouble) ? Scalar(base.toComplexDouble()) : Scalar(base.toDouble());
  return pluss::pow(casted_base, exp.to(dtype));
}

Series float_power(const Series& base, const Series& exp) {
  auto dtype = (pluss::isComplexType(base.scalar_type()) || pluss::isComplexType(exp.scalar_type())) ? pluss::kComplexDouble : pluss::kDouble;
  return pluss::pow(base.to(dtype), exp.to(dtype));
}

Series& float_power_(Series& base, const Series& exp) {
  auto dtype = (pluss::isComplexType(base.scalar_type()) || pluss::isComplexType(exp.scalar_type())) ? pluss::kComplexDouble : pluss::kDouble;
  TORCH_CHECK(base.scalar_type() == dtype,
              "the base given to float_power_ has dtype ", base.scalar_type(),
              " but the operation's result requires dtype ", dtype);

  return base.pow_(exp.to(dtype));
}

Series& float_power_(Series& base, const Scalar& exp) {
  auto dtype = (pluss::isComplexType(base.scalar_type()) || exp.isComplex()) ? pluss::kComplexDouble : pluss::kDouble;
  TORCH_CHECK(base.scalar_type() == dtype,
              "the base given to float_power_ has dtype ", base.scalar_type(),
              " but the operation's result requires dtype ", dtype);

  auto casted_exp = (dtype == pluss::kComplexDouble) ? Scalar(exp.toComplexDouble()) : Scalar(exp.toDouble());
  return base.pow_(casted_exp);
}

} // namespace pluss::native
