#pragma once

#include "pluss/codem/series_factory.h"

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::math::krylov {

struct CgOptions final {
  std::int64_t max_iterations{128};
  double relative_tolerance{1e-10};
  double absolute_tolerance{1e-12};
  double curvature_tolerance{0.0};
  // Zero keeps the solve fully asynchronous and executes the whole budget.
  std::int64_t check_interval{8};
};

struct CgResult final {
  Series solution;
  Series residual_norm;
  Series relative_residual;
  Series converged;
  Series breakdown;
  std::int64_t iterations{};
};

namespace detail {

inline std::vector<std::int64_t>
normalized_dims(const Series &value, const std::vector<std::int64_t> &dims,
                const char *operation) {
  if (dims.empty())
    throw std::invalid_argument(std::string(operation) +
                                ": reduction axes must not be empty");
  std::vector<std::int64_t> normalized;
  normalized.reserve(dims.size());
  for (std::int64_t axis : dims) {
    if (axis < 0)
      axis += value.dim();
    if (axis < 0 || axis >= value.dim())
      throw std::out_of_range(std::string(operation) +
                              ": reduction axis is out of range");
    for (const std::int64_t existing : normalized)
      if (existing == axis)
        throw std::invalid_argument(std::string(operation) +
                                    ": reduction axes must be unique");
    normalized.push_back(axis);
  }
  return normalized;
}

inline bool same_layout(const Series &lhs, const Series &rhs) noexcept {
  if (lhs.dim() != rhs.dim() || lhs.scalar_type() != rhs.scalar_type() ||
      lhs.platform() != rhs.platform())
    return false;
  for (std::int64_t axis = 0; axis < lhs.dim(); ++axis)
    if (lhs.size(axis) != rhs.size(axis))
      return false;
  return true;
}

inline void validate_options(const CgOptions &options) {
  if (options.max_iterations < 0)
    throw std::invalid_argument(
        "math::krylov::cg: max_iterations must be non-negative");
  if (options.check_interval < 0)
    throw std::invalid_argument(
        "math::krylov::cg: check_interval must be non-negative");
  if (!(options.relative_tolerance >= 0.0) ||
      !std::isfinite(options.relative_tolerance) ||
      !(options.absolute_tolerance >= 0.0) ||
      !std::isfinite(options.absolute_tolerance) ||
      !(options.curvature_tolerance >= 0.0) ||
      !std::isfinite(options.curvature_tolerance))
    throw std::invalid_argument(
        "math::krylov::cg: tolerances must be finite and non-negative");
}

inline Series inner(const Series &lhs, const Series &rhs,
                    const std::vector<std::int64_t> &dims) {
  return lhs.mul(rhs).sum(dims, true);
}

} // namespace detail

template <class Apply, class Precondition>
[[nodiscard]] CgResult
pcg(Apply &&apply, Precondition &&precondition, const Series &rhs,
    const std::vector<std::int64_t> &solve_dims, const CgOptions &options = {},
    const Series &initial = pluss::series::undefined()) {
  constexpr const char *operation = "math::krylov::pcg";
  detail::validate_options(options);
  if (!rhs.defined() || !rhs.dtype().is_floating() || rhs.numel() == 0)
    throw std::invalid_argument(
        "math::krylov::cg: rhs must be a non-empty real floating Series");
  if (!rhs.isfinite().all().item<bool>())
    throw std::invalid_argument("math::krylov::cg: rhs must be finite");
  const auto dims = detail::normalized_dims(rhs, solve_dims, operation);

  Series x = initial.defined() ? initial : pluss::series::zeros_like(rhs);
  if (!detail::same_layout(x, rhs))
    throw std::invalid_argument(
        "math::krylov::cg: initial guess must match rhs shape, dtype, and "
        "platform");
  if (!x.isfinite().all().item<bool>())
    throw std::invalid_argument(
        "math::krylov::cg: initial guess must be finite");

  Series applied = apply(x);
  if (!detail::same_layout(applied, rhs))
    throw std::invalid_argument(
        "math::krylov::cg: operator output must match rhs shape, dtype, and "
        "platform");
  if (!applied.isfinite().all().item<bool>())
    throw std::invalid_argument(
        "math::krylov::cg: operator output must be finite");

  Series residual = rhs.sub(applied);
  Series scaled = precondition(residual);
  if (!detail::same_layout(scaled, rhs))
    throw std::invalid_argument(
        "math::krylov::pcg: preconditioner output must match rhs shape, "
        "dtype, and platform");
  if (!scaled.isfinite().all().item<bool>())
    throw std::invalid_argument(
        "math::krylov::pcg: preconditioner output must be finite");
  Series direction = scaled;
  Series residual2 = detail::inner(residual, residual, dims);
  Series scaled_residual = detail::inner(residual, scaled, dims);
  const Series initial_norm = residual2.sqrt();
  const Series tolerance = initial_norm.mul(options.relative_tolerance)
                               .maximum(options.absolute_tolerance);
  const Series tolerance2 = tolerance.square();
  Series active = residual2.gt(tolerance2);
  Series breakdown = active.logical_and(
      scaled_residual.le(options.curvature_tolerance)
          .logical_or(scaled_residual.isfinite().logical_not()));
  active = active.logical_and(breakdown.logical_not());
  direction = direction.mul(active.to(direction.scalar_type()));
  std::int64_t iterations = 0;
  bool continue_solve =
      options.check_interval == 0 || active.any().item<bool>();

  for (std::int64_t iteration = 0; iteration < options.max_iterations;
       ++iteration) {
    if (!continue_solve)
      break;
    const Series image = apply(direction);
    if (!detail::same_layout(image, rhs))
      throw std::invalid_argument(
          "math::krylov::cg: operator output layout changed during solve");
    const Series curvature = detail::inner(direction, image, dims);
    const Series invalid =
        active.logical_and(curvature.le(options.curvature_tolerance)
                               .logical_or(curvature.isfinite().logical_not()));
    breakdown = breakdown.logical_or(invalid);
    const Series usable = active.logical_and(invalid.logical_not());
    const Series usable_values = usable.to(residual2.scalar_type());
    const Series safe_curvature =
        curvature.add(usable.logical_not().to(curvature.scalar_type()));
    const Series alpha = scaled_residual.div(safe_curvature).mul(usable_values);

    x = x.add(direction.mul(alpha));
    residual = residual.sub(image.mul(alpha));
    const Series next_residual2 = detail::inner(residual, residual, dims);
    Series next_active = usable.logical_and(next_residual2.gt(tolerance2));
    scaled = precondition(residual);
    if (!detail::same_layout(scaled, rhs))
      throw std::invalid_argument(
          "math::krylov::pcg: preconditioner output layout changed during "
          "solve");
    if (!scaled.isfinite().all().item<bool>())
      throw std::invalid_argument(
          "math::krylov::pcg: preconditioner output became non-finite");
    const Series next_scaled_residual = detail::inner(residual, scaled, dims);
    const Series preconditioner_breakdown = next_active.logical_and(
        next_scaled_residual.le(options.curvature_tolerance)
            .logical_or(next_scaled_residual.isfinite().logical_not()));
    breakdown = breakdown.logical_or(preconditioner_breakdown);
    next_active =
        next_active.logical_and(preconditioner_breakdown.logical_not());
    const Series safe_scaled_residual = scaled_residual.add(
        usable.logical_not().to(scaled_residual.scalar_type()));
    const Series beta = next_scaled_residual.div(safe_scaled_residual)
                            .mul(next_active.to(scaled_residual.scalar_type()));
    direction = scaled.mul(next_active.to(scaled.scalar_type()))
                    .add(direction.mul(beta));
    residual2 = next_residual2;
    scaled_residual = next_scaled_residual;
    active = next_active;
    iterations = iteration + 1;
    if (options.check_interval > 0 && iterations % options.check_interval == 0)
      continue_solve = active.any().item<bool>();
  }

  const Series residual_norm = residual2.sqrt();
  const Series safe_initial =
      initial_norm.add(initial_norm.eq(0.0).to(initial_norm.scalar_type()));
  return {std::move(x),
          residual_norm,
          residual_norm.div(safe_initial),
          residual2.le(tolerance2).logical_and(breakdown.logical_not()),
          breakdown,
          iterations};
}

template <class Apply>
[[nodiscard]] CgResult cg(Apply &&apply, const Series &rhs,
                          const std::vector<std::int64_t> &solve_dims,
                          const CgOptions &options = {},
                          const Series &initial = pluss::series::undefined()) {
  return pcg(
      std::forward<Apply>(apply),
      [](const Series &residual) { return residual; }, rhs, solve_dims, options,
      initial);
}

} // namespace pluss::subsp::math::krylov
