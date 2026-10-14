#include "CodeM/core/Series.h"
#include "pluss/subsp/math/diff.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace pluss::codem {
// Series method implementations live in OpsInternal.cpp.

Series Series::calc_diff_(const Series &input,
                          std::function<Series(const Series &)> callback,
                          std::int64_t order,
                          const SeriesCalcAccessor::CalcOptions &options) {
  PLUSS_CHECK(input.defined(), "Series.calc.diff: input must be defined");
  PLUSS_CHECK(static_cast<bool>(callback),
              "Series.calc.diff: function is required");
  PLUSS_CHECK(order >= 0, "Series.calc.diff: order must be non-negative");

  if (!options.elementwise) {
    pluss::subsp::math::diff::DerivativePolicy policy;
    policy.strict_dependency = options.strict_dependency;
    return pluss::subsp::math::diff::nth_diff(
        std::move(callback), input, order,
        pluss::subsp::math::diff::HvpOptions{}, policy);
  }

  // Elementwise derivatives d^n fn_i / d x_i^n: one reverse pass per order on
  // the sum of the outputs, each pass keeping the graph of its own gradient so
  // the next order can differentiate it. Cost is O(order) passes, not N^order.
  Series leaf = input.detach();
  pluss::subsp::math::diff::detail::set_tracks_slope_(leaf, true);
  Series current = callback(leaf);
  PLUSS_CHECK(current.defined(),
              "Series.calc.diff: function returned an undefined Series");
  PLUSS_CHECK(
      current.sizes() == input.sizes(),
      "Series.calc.diff: elementwise mode needs fn(x) to have the shape of x");
  for (std::int64_t level = 1; level <= order; ++level) {
    pluss::codem::series_tape::clear_sensitivity(leaf);
    const bool last = level == order;
    if (pluss::subsp::math::diff::detail::has_reverse_path_(current)) {
      current.sum().reverse(/*keep_graph=*/!last,
                            /*accumulate_sensitivity=*/false,
                            /*track_higher_order=*/!last);
    }
    if (leaf.has_sensitivity()) {
      current = leaf.sensitivity();
    } else {
      PLUSS_CHECK(!options.strict_dependency,
                  "Series.calc.diff: the function does not depend on x at "
                  "derivative order " +
                      std::to_string(level));
      current = pluss::series::zeros(input.sizes(), input.scalar_type())
                    .to(input.platform());
    }
  }
  return current;
}

namespace {

pluss::subsp::math::diff::DerivativePolicy
calc_policy(const Series::CalcAccessor::CalcOptions &options) {
  pluss::subsp::math::diff::DerivativePolicy policy;
  policy.strict_dependency = options.strict_dependency;
  return policy;
}

std::function<Series(const Series &)>
scalar_objective(std::function<Series(const Series &)> callback,
                 const char *operation) {
  return [callback = std::move(callback), operation](const Series &value) {
    Series output = callback(value);
    PLUSS_CHECK(output.defined(),
                std::string(operation) +
                    ": function returned an undefined Series");
    PLUSS_CHECK(output.dim() == 0,
                std::string(operation) +
                    ": function must return a rank-zero scalar Series");
    return output;
  };
}

} // namespace

Series Series::calc_grad_(const Series &input,
                          std::function<Series(const Series &)> callback,
                          const SeriesCalcAccessor::CalcOptions &options) {
  PLUSS_CHECK(input.defined(), "Series.calc.grad: input must be defined");
  PLUSS_CHECK(static_cast<bool>(callback),
              "Series.calc.grad: function is required");
  return pluss::subsp::math::diff::nth_diff(
      scalar_objective(std::move(callback), "Series.calc.grad"), input, 1,
      pluss::subsp::math::diff::HvpOptions{}, calc_policy(options));
}

Series Series::calc_jac_(const Series &input,
                         std::function<Series(const Series &)> callback,
                         const SeriesCalcAccessor::CalcOptions &options) {
  PLUSS_CHECK(input.defined(), "Series.calc.jac: input must be defined");
  PLUSS_CHECK(static_cast<bool>(callback),
              "Series.calc.jac: function is required");
  return pluss::subsp::math::diff::nth_diff(
      std::move(callback), input, 1, pluss::subsp::math::diff::HvpOptions{},
      calc_policy(options));
}

Series Series::calc_hess_(const Series &input,
                          std::function<Series(const Series &)> callback,
                          const SeriesCalcAccessor::CalcOptions &options) {
  PLUSS_CHECK(input.defined(), "Series.calc.hess: input must be defined");
  PLUSS_CHECK(static_cast<bool>(callback),
              "Series.calc.hess: function is required");
  return pluss::subsp::math::diff::nth_diff(
      scalar_objective(std::move(callback), "Series.calc.hess"), input, 2,
      pluss::subsp::math::diff::HvpOptions{}, calc_policy(options));
}

Series Series::calc_antidiff_(
    const Series &upper, std::function<Series(const Series &)> callback,
    double lower, const SeriesCalcAccessor::AntidiffOptions &options) {
  PLUSS_CHECK(upper.defined(),
              "Series.calc.antidiff: upper endpoint must be defined");
  PLUSS_CHECK(static_cast<bool>(callback),
              "Series.calc.antidiff: integrand callback is required");
  PLUSS_CHECK(
      upper.dtype().is_floating() && !upper.dtype().is_complex(),
      "Series.calc.antidiff: upper endpoints must have a real floating dtype");
  PLUSS_CHECK(std::isfinite(lower),
              "Series.calc.antidiff: lower endpoint must be finite");
  PLUSS_CHECK(options.order >= 1,
              "Series.calc.antidiff: order must be at least one");
  PLUSS_CHECK(options.intervals >= 1,
              "Series.calc.antidiff: intervals must be positive");
  PLUSS_CHECK(options.max_refinements >= 0,
              "Series.calc.antidiff: max_refinements must be non-negative");
  PLUSS_CHECK(std::isfinite(options.atol) && options.atol >= 0.0,
              "Series.calc.antidiff: atol must be finite and non-negative");
  PLUSS_CHECK(std::isfinite(options.rtol) && options.rtol >= 0.0,
              "Series.calc.antidiff: rtol must be finite and non-negative");
  PLUSS_CHECK(options.initial.empty() ||
                  options.initial.size() ==
                      static_cast<std::size_t>(options.order),
              "Series.calc.antidiff: initial must be empty or contain one "
              "value per order");

  if (upper.numel() == 0)
    return upper.clone();

  auto evaluate = [&](std::int64_t requested_intervals) {
    std::int64_t intervals = std::max<std::int64_t>(requested_intervals, 1);
    if (options.rule == SeriesCalcAccessor::Rule::Simpson) {
      intervals = std::max<std::int64_t>(intervals, 2);
      if (intervals % 2 != 0)
        ++intervals;
    }

    const std::int64_t nodes = intervals + 1;
    const auto dtype = upper.scalar_type();
    const auto platform = upper.platform();
    const Series unit = Series::linspace(0.0, 1.0, nodes, dtype, platform);
    const Series delta = upper.sub(lower);
    const Series points = delta.unsqueeze(-1).mul(unit).add(lower);
    Series values = callback(points);
    PLUSS_CHECK(values.defined(),
                "Series.calc.antidiff: integrand returned an undefined Series");
    PLUSS_CHECK(values.platform() == platform,
                "Series.calc.antidiff: integrand output must remain on the "
                "endpoint platform");
    PLUSS_CHECK(values.sizes() == points.sizes(),
                "Series.calc.antidiff: integrand output shape must match the "
                "quadrature-node shape");
    PLUSS_CHECK(
        values.isfinite().all().to(pluss::pCPU, ScalarType::Bool).item<bool>(),
        "Series.calc.antidiff: the integrand returned a non-finite value (nan "
        "or inf) at a "
        "quadrature node; the integral may be singular or the integrand "
        "undefined on "
        "[lower, upper]");

    if (options.order > 1) {
      values = values.mul(
          unit.mul(-1.0).add(1.0).pow(static_cast<double>(options.order - 1)));
    }

    Series unit_integral = pluss::series::undefined();
    if (options.rule == SeriesCalcAccessor::Rule::Trapezoid) {
      unit_integral =
          values.antidiff.trapezoid(1.0 / static_cast<double>(intervals), -1);
    } else {
      const Series indices =
          pluss::series::arange(0, nodes, 1, ScalarType::Int64).to(platform);
      const Series one =
          pluss::series::full({nodes}, 1.0, values.scalar_type()).to(platform);
      const Series two =
          pluss::series::full({nodes}, 2.0, values.scalar_type()).to(platform);
      const Series four =
          pluss::series::full({nodes}, 4.0, values.scalar_type()).to(platform);
      Series weights = indices.remainder(2.0).eq(0.0).where(two, four);
      const Series endpoint = indices.eq(0.0).logical_or(indices.eq(nodes - 1));
      weights = endpoint.where(one, weights);
      unit_integral = values.mul(weights).sum(-1).mul(
          1.0 / (3.0 * static_cast<double>(intervals)));
    }

    Series result =
        unit_integral.mul(delta.pow(static_cast<double>(options.order)) /
                          std::tgamma(static_cast<double>(options.order)));
    for (std::int64_t k = 0; k < options.order; ++k) {
      const double initial = options.initial.empty()
                                 ? 0.0
                                 : options.initial[static_cast<std::size_t>(k)];
      if (initial == 0.0)
        continue;
      if (k == 0) {
        result = result.add(initial);
      } else {
        result = result.add(
            delta.pow(static_cast<double>(k))
                .mul(initial / std::tgamma(static_cast<double>(k + 1))));
      }
    }
    // One value per endpoint, shaped like the endpoints (a 0-d endpoint stays
    // 0-d).
    return result.reshape(upper.sizes());
  };

  Series estimate = evaluate(options.intervals);
  if (!options.adaptive)
    return estimate;

  double last_error = std::numeric_limits<double>::infinity();
  double last_tolerance = 0.0;
  std::int64_t intervals = options.intervals;
  for (std::int64_t refinement = 0; refinement < options.max_refinements;
       ++refinement) {
    PLUSS_CHECK(intervals <= std::numeric_limits<std::int64_t>::max() / 2,
                "Series.calc.antidiff: interval refinement overflows int64");
    intervals *= 2;
    Series refined = evaluate(intervals);
    last_error = refined.sub(estimate)
                     .abs()
                     .max()
                     .to(pluss::pCPU, ScalarType::Float64)
                     .item<double>();
    const double scale =
        refined.abs().max().to(pluss::pCPU, ScalarType::Float64).item<double>();
    last_tolerance = options.atol + options.rtol * scale;
    estimate = std::move(refined);
    if (last_error <= last_tolerance)
      return estimate;
  }

  if (options.require_convergence) {
    PLUSS_FAIL(
        "Series.calc.antidiff: adaptive quadrature did not converge; error=" +
        std::to_string(last_error) +
        ", tolerance=" + std::to_string(last_tolerance));
  }
  return estimate;
}
} // namespace pluss::codem
