#pragma once

#include "pluss/subsp/math/diff.h"
#include "pluss/subsp/natsci/common/quantity.h"

#include <concepts>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace pluss::subsp::natsci::common::diff {

namespace math_diff = ::pluss::subsp::math::diff;

namespace detail {

template <class T> struct quantity_traits final {
  static constexpr bool valid = false;
  using dimension = void;
  using rep = void;
};

template <DimensionType D, QuantityRep Rep>
struct quantity_traits<Quantity<D, Rep>> final {
  static constexpr bool valid = true;
  using dimension = D;
  using rep = Rep;
};

template <class T>
using quantity_traits_t = quantity_traits<std::remove_cvref_t<T>>;

template <DimensionType OutputDimension, DimensionType InputDimension,
          class Fn>
[[nodiscard]] auto invoke_quantity(Fn &fn, const Series &input) {
  using Input = SeriesQuantity<InputDimension>;
  using Output = std::invoke_result_t<Fn &, const Input &>;
  using Traits = quantity_traits_t<Output>;
  static_assert(Traits::valid,
                "NatSci derivative callback must return a Quantity");
  static_assert(std::same_as<typename Traits::dimension, OutputDimension>,
                "NatSci derivative callback returned the wrong dimension");
  static_assert(std::same_as<typename Traits::rep, Series>,
                "NatSci derivative callback must return a Series quantity");
  return std::invoke(fn, Input::from_si(input));
}

inline void require_scalar(const Series &value, const char *operation,
                           const char *role) {
  if (value.numel() != 1)
    throw std::invalid_argument(std::string("natsci::common::diff::") +
                                operation + ": " + role +
                                " must contain exactly one value");
}

template <DimensionType OutputDimension, DimensionType InputDimension,
          class Fn>
[[nodiscard]] auto scalar_trace_callback(Fn &fn, const char *operation) {
  return [&fn, operation](const math_diff::Trace &input) {
    auto output =
        invoke_quantity<OutputDimension, InputDimension>(fn, input.value());
    require_scalar(output.si_value(), operation, "callback output");
    return math_diff::Trace(output.si_value());
  };
}

// math::diff::jacobian selects forward/reverse mode at runtime, so its
// callable must compile for both trace types. NatSci quantity callbacks are
// Series-based and therefore deliberately reverse-only; this adapter's dual
// branch is unreachable after the public mode check below.
template <DimensionType OutputDimension, DimensionType InputDimension,
          class Fn>
[[nodiscard]] auto jacobian_callback(Fn &fn) {
  return [&fn](const auto &input) {
    using Input = std::remove_cvref_t<decltype(input)>;
    if constexpr (std::same_as<Input, math_diff::Trace>) {
      return math_diff::Trace(
          invoke_quantity<OutputDimension, InputDimension>(fn, input.value())
              .si_value());
    } else {
      return math_diff::DualTrace(input.value(), input.tangent());
    }
  };
}

} // namespace detail

template <DimensionType OutputDimension, DimensionType InputDimension,
          class Fn>
[[nodiscard]] auto
grad(Fn &&fn, const SeriesQuantity<InputDimension> &input,
     const math_diff::DerivativePolicy &policy = {}) {
  using GradientDimension =
      DimensionQuotient<OutputDimension, InputDimension>;
  auto fn_copy = std::forward<Fn>(fn);
  auto callback = detail::scalar_trace_callback<OutputDimension,
                                                 InputDimension>(fn_copy,
                                                                 "grad");
  const auto [value, gradient] =
      math_diff::value_and_grad(callback, input.si_value(), policy);
  detail::require_scalar(value, "grad", "callback output");
  return SeriesQuantity<GradientDimension>::from_si(gradient);
}

template <DimensionType OutputDimension, DimensionType InputDimension,
          class Fn>
[[nodiscard]] auto
jacobian(Fn &&fn, const SeriesQuantity<InputDimension> &input,
         const math_diff::JacobianOptions &options = {}) {
  using JacobianDimension =
      DimensionQuotient<OutputDimension, InputDimension>;
  if (options.mode == math_diff::JacobianMode::Forward)
    throw std::invalid_argument(
        "natsci::common::diff::jacobian: SeriesQuantity callbacks support "
        "reverse mode; use subsp::math::diff with DualTrace for forward mode");
  auto fn_copy = std::forward<Fn>(fn);
  auto callback =
      detail::jacobian_callback<OutputDimension, InputDimension>(fn_copy);
  auto [value, derivative] =
      math_diff::jacobian(callback, input.si_value(), options);
  return std::pair{
      SeriesQuantity<OutputDimension>::from_si(std::move(value)),
      SeriesQuantity<JacobianDimension>::from_si(std::move(derivative))};
}

template <DimensionType OutputDimension, DimensionType InputDimension,
          class Fn>
[[nodiscard]] auto
hessian(Fn &&fn, const SeriesQuantity<InputDimension> &input,
        const math_diff::HessianOptions &options = {}) {
  using HessianDimension = DimensionQuotient<
      OutputDimension, DimensionPower<InputDimension, 2>>;
  auto fn_copy = std::forward<Fn>(fn);
  auto callback = detail::scalar_trace_callback<OutputDimension,
                                                 InputDimension>(fn_copy,
                                                                 "hessian");
  auto [value, derivative] =
      math_diff::hessian(callback, input.si_value(), options);
  detail::require_scalar(value, "hessian", "callback output");
  return std::pair{
      SeriesQuantity<OutputDimension>::from_si(std::move(value)),
      SeriesQuantity<HessianDimension>::from_si(std::move(derivative))};
}

template <DimensionType OutputDimension, DimensionType InputDimension,
          class Fn>
[[nodiscard]] auto
hvp(Fn &&fn, const SeriesQuantity<InputDimension> &input,
    const SeriesQuantity<InputDimension> &direction,
    const math_diff::HvpOptions &options = {},
    const math_diff::DerivativePolicy &policy = {}) {
  using ResultDimension =
      DimensionQuotient<OutputDimension, InputDimension>;
  auto fn_copy = std::forward<Fn>(fn);
  auto callback = detail::scalar_trace_callback<OutputDimension,
                                                 InputDimension>(fn_copy,
                                                                 "hvp");
  auto [value, product] = math_diff::hvp(
      callback, input.si_value(), direction.si_value(), options, policy);
  detail::require_scalar(value, "hvp", "callback output");
  return std::pair{
      SeriesQuantity<OutputDimension>::from_si(std::move(value)),
      SeriesQuantity<ResultDimension>::from_si(std::move(product))};
}

template <int Order, DimensionType OutputDimension,
          DimensionType InputDimension, class Fn>
[[nodiscard]] auto
nth_diff(Fn &&fn, const SeriesQuantity<InputDimension> &input,
         const math_diff::HvpOptions &options = {},
         const math_diff::DerivativePolicy &policy = {}) {
  static_assert(Order >= 0, "NatSci derivative order must be non-negative");
  using DerivativeDimension = DimensionQuotient<
      OutputDimension, DimensionPower<InputDimension, Order>>;

  auto fn_copy = std::forward<Fn>(fn);
  auto callback = [&fn_copy](const Series &value) {
    auto output = detail::invoke_quantity<OutputDimension, InputDimension>(
        fn_copy, value);
    return output.si_value();
  };
  return SeriesQuantity<DerivativeDimension>::from_si(math_diff::nth_diff(
      callback, input.si_value(), Order, options, policy));
}

template <DimensionType OutputDimension, DimensionType InputDimension,
          class Fn>
[[nodiscard]] SeriesQuantity<Dimensionless>
elasticity(Fn &&fn, const SeriesQuantity<InputDimension> &input,
           const math_diff::DerivativePolicy &policy = {}) {
  auto fn_copy = std::forward<Fn>(fn);
  auto callback = detail::scalar_trace_callback<OutputDimension,
                                                 InputDimension>(fn_copy,
                                                                 "elasticity");
  auto [value, gradient] =
      math_diff::value_and_grad(callback, input.si_value(), policy);
  detail::require_scalar(value, "elasticity", "callback output");
  if (value.eq(0.0).all().template item<bool>())
    throw std::domain_error(
        "natsci::common::diff::elasticity: callback output must be non-zero");
  return SeriesQuantity<Dimensionless>::from_si(
      input.si_value().mul(gradient).div(value));
}

} // namespace pluss::subsp::natsci::common::diff
