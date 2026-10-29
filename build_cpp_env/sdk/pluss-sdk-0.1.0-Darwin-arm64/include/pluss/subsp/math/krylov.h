#pragma once

#include "pluss/codem/series_factory.h"
#include "pluss/subsp/linearalg/linearalg.h"

#include <cmath>
#include <cstdint>
#include <limits>
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

struct GmresOptions final {
  std::int64_t max_iterations{128};
  std::int64_t restart{24};
  double relative_tolerance{1e-8};
  double absolute_tolerance{1e-10};
  double rank_tolerance{1e-12};
};

struct GmresResult final {
  Series solution;
  Series residual_norm;
  Series relative_residual;
  Series converged;
  std::int64_t iterations{};
  std::int64_t operator_evaluations{};
  std::int64_t restarts{};
};

struct ExpOptions final {
  std::int64_t max_iterations{32};
  double relative_tolerance{1e-9};
  double absolute_tolerance{1e-12};
};

struct ExpResult final {
  Series value;
  Series residual_estimate;
  Series converged;
  std::int64_t iterations{};
  std::int64_t operator_evaluations{};
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

inline void validate_options(const GmresOptions &options) {
  if (options.max_iterations < 0 || options.restart <= 0)
    throw std::invalid_argument(
        "math::krylov::gmres: iteration limits must be non-negative and "
        "restart must be positive");
  if (!(options.relative_tolerance >= 0.0) ||
      !std::isfinite(options.relative_tolerance) ||
      !(options.absolute_tolerance >= 0.0) ||
      !std::isfinite(options.absolute_tolerance) ||
      !(options.rank_tolerance >= 0.0) ||
      !std::isfinite(options.rank_tolerance))
    throw std::invalid_argument(
        "math::krylov::gmres: tolerances must be finite and non-negative");
}

inline void validate_options(const ExpOptions &options) {
  if (options.max_iterations <= 0 || !(options.relative_tolerance >= 0.0) ||
      !std::isfinite(options.relative_tolerance) ||
      !(options.absolute_tolerance >= 0.0) ||
      !std::isfinite(options.absolute_tolerance))
    throw std::invalid_argument(
        "math::krylov::exp_action: iteration limit must be positive and "
        "tolerances must be finite and non-negative");
}

inline bool finite(const Series &value) {
  return value.dtype().is_complex()
             ? value.real().isfinite().all().item<bool>() &&
                   value.imag().isfinite().all().item<bool>()
             : value.isfinite().all().item<bool>();
}

inline void require_trailing_dims(const Series &value,
                                  const std::vector<std::int64_t> &dims,
                                  const char *operation) {
  const auto rank = value.dim();
  const auto count = static_cast<std::int64_t>(dims.size());
  for (std::int64_t index = 0; index < count; ++index)
    if (dims[static_cast<std::size_t>(index)] != rank - count + index)
      throw std::invalid_argument(std::string(operation) +
                                  ": solve axes must be the trailing axes");
}

inline Series hermitian_inner(const Series &lhs, const Series &rhs,
                              const std::vector<std::int64_t> &dims) {
  return lhs.conj().mul(rhs).sum(dims, true);
}

inline Series vector_norm(const Series &value,
                          const std::vector<std::int64_t> &dims) {
  return value.abs().square().sum(dims, true).sqrt();
}

inline Series remove_trailing_singletons(Series value, std::size_t count) {
  for (std::size_t index = 0; index < count; ++index)
    value = value.squeeze(-1);
  return value;
}

inline Series append_singletons(Series value, std::size_t count) {
  for (std::size_t index = 0; index < count; ++index)
    value = value.unsqueeze(-1);
  return value;
}

inline Series stack_last(const std::vector<Series> &values) {
  Series out = values.front().unsqueeze(-1);
  if (values.size() == 1)
    return out;
  std::vector<Series> rest;
  rest.reserve(values.size() - 1);
  for (std::size_t index = 1; index < values.size(); ++index)
    rest.push_back(values[index].unsqueeze(-1));
  return out.unite(rest, -1);
}

inline Series stack_rows(const std::vector<Series> &rows) {
  Series out = rows.front().unsqueeze(-2);
  if (rows.size() == 1)
    return out;
  std::vector<Series> rest;
  rest.reserve(rows.size() - 1);
  for (std::size_t index = 1; index < rows.size(); ++index)
    rest.push_back(rows[index].unsqueeze(-2));
  return out.unite(rest, -2);
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

// Restarted GMRES for a possibly non-Hermitian real or complex linear
// operator. Every leading lane is an independent solve; `solve_dims` must be
// the trailing state axes so Hessenberg coefficients can remain batched.
template <class Apply>
[[nodiscard]] GmresResult
gmres(Apply &&apply, const Series &rhs,
      const std::vector<std::int64_t> &solve_dims,
      const GmresOptions &options = {},
      const Series &initial = pluss::series::undefined()) {
  constexpr const char *operation = "math::krylov::gmres";
  detail::validate_options(options);
  if (!rhs.defined() ||
      (!rhs.dtype().is_floating() && !rhs.dtype().is_complex()) ||
      rhs.numel() == 0)
    throw std::invalid_argument(
        "math::krylov::gmres: rhs must be a non-empty floating or complex "
        "Series");
  if (!detail::finite(rhs))
    throw std::invalid_argument("math::krylov::gmres: rhs must be finite");
  const auto dims = detail::normalized_dims(rhs, solve_dims, operation);
  detail::require_trailing_dims(rhs, dims, operation);

  Series x = initial.defined() ? initial : pluss::series::zeros_like(rhs);
  if (!detail::same_layout(x, rhs))
    throw std::invalid_argument(
        "math::krylov::gmres: initial guess must match rhs shape, dtype, and "
        "platform");
  if (!detail::finite(x))
    throw std::invalid_argument(
        "math::krylov::gmres: initial guess must be finite");

  auto checked_apply = [&](const Series &value) {
    Series image = apply(value);
    if (!detail::same_layout(image, rhs))
      throw std::invalid_argument(
          "math::krylov::gmres: operator output must match rhs shape, dtype, "
          "and platform");
    if (!detail::finite(image))
      throw std::invalid_argument(
          "math::krylov::gmres: operator output must be finite");
    return image;
  };

  std::int64_t evaluations = 0;
  Series residual = rhs.sub(checked_apply(x));
  ++evaluations;
  const Series initial_norm = detail::vector_norm(residual, dims);
  const Series tolerance = initial_norm.mul(options.relative_tolerance)
                               .maximum(options.absolute_tolerance);
  std::int64_t iterations = 0;
  std::int64_t restarts = 0;

  while (iterations < options.max_iterations) {
    Series beta = detail::vector_norm(residual, dims);
    if (beta.le(tolerance).all().item<bool>())
      break;

    const std::int64_t width =
        std::min(options.restart, options.max_iterations - iterations);
    const Series nonzero = beta.gt(0.0);
    const Series safe_beta =
        beta.add(nonzero.logical_not().to(beta.scalar_type()));
    std::vector<Series> basis;
    basis.reserve(static_cast<std::size_t>(width) + 1);
    basis.push_back(residual.div(safe_beta).mul(nonzero.to(rhs.scalar_type())));

    const Series coefficient_beta = beta.scalar_type() == rhs.scalar_type()
                                        ? beta
                                        : beta.to(rhs.scalar_type());
    const Series zero_coefficient = coefficient_beta.mul(0.0);
    std::vector<std::vector<Series>> h(
        static_cast<std::size_t>(width) + 1,
        std::vector<Series>(static_cast<std::size_t>(width), zero_coefficient));

    for (std::int64_t column = 0; column < width; ++column) {
      Series candidate = checked_apply(basis[static_cast<std::size_t>(column)]);
      ++evaluations;
      for (std::int64_t row = 0; row <= column; ++row) {
        Series coefficient = detail::hermitian_inner(
            basis[static_cast<std::size_t>(row)], candidate, dims);
        h[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] =
            coefficient;
        candidate = candidate.sub(
            basis[static_cast<std::size_t>(row)].mul(coefficient));
      }
      const Series next_norm = detail::vector_norm(candidate, dims);
      h[static_cast<std::size_t>(column + 1)]
       [static_cast<std::size_t>(column)] =
           next_norm.scalar_type() == rhs.scalar_type()
               ? next_norm
               : next_norm.to(rhs.scalar_type());
      const Series usable = next_norm.gt(0.0);
      const Series safe_norm =
          next_norm.add(usable.logical_not().to(next_norm.scalar_type()));
      basis.push_back(
          candidate.div(safe_norm).mul(usable.to(rhs.scalar_type())));
    }

    std::vector<Series> h_rows;
    h_rows.reserve(static_cast<std::size_t>(width) + 1);
    for (std::int64_t row = 0; row <= width; ++row) {
      std::vector<Series> cells;
      cells.reserve(static_cast<std::size_t>(width));
      for (std::int64_t column = 0; column < width; ++column)
        cells.push_back(detail::remove_trailing_singletons(
            h[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)],
            dims.size()));
      h_rows.push_back(detail::stack_last(cells));
    }
    const Series hessenberg = detail::stack_rows(h_rows);

    std::vector<Series> right_cells;
    right_cells.reserve(static_cast<std::size_t>(width) + 1);
    right_cells.push_back(
        detail::remove_trailing_singletons(coefficient_beta, dims.size()));
    for (std::int64_t row = 1; row <= width; ++row)
      right_cells.push_back(
          detail::remove_trailing_singletons(zero_coefficient, dims.size()));
    const Series small_rhs = detail::stack_last(right_cells).unsqueeze(-1);
    const Series coefficients =
        hessenberg.linearalg.pinverse(options.rank_tolerance)
            .matmul(small_rhs)
            .squeeze(-1);

    for (std::int64_t column = 0; column < width; ++column) {
      const Series coefficient = detail::append_singletons(
          coefficients.select(-1, column), dims.size());
      x = x.add(basis[static_cast<std::size_t>(column)].mul(coefficient));
    }
    iterations += width;
    ++restarts;
    residual = rhs.sub(checked_apply(x));
    ++evaluations;
  }

  const Series residual_norm = detail::vector_norm(residual, dims);
  const Series safe_initial =
      initial_norm.add(initial_norm.eq(0.0).to(initial_norm.scalar_type()));
  return {std::move(x),
          residual_norm,
          residual_norm.div(safe_initial),
          residual_norm.le(tolerance),
          iterations,
          evaluations,
          restarts};
}

// Arnoldi projection of exp(time * A) applied to a vector or batched state.
// The operator is never materialized; the final subdiagonal contribution is
// returned as the standard a-posteriori Krylov residual estimate.
template <class Apply>
[[nodiscard]] ExpResult exp_action(Apply &&apply, const Series &input,
                                   const std::vector<std::int64_t> &solve_dims,
                                   double time,
                                   const ExpOptions &options = {}) {
  constexpr const char *operation = "math::krylov::exp_action";
  detail::validate_options(options);
  if (!std::isfinite(time))
    throw std::invalid_argument(
        "math::krylov::exp_action: time must be finite");
  if (!input.defined() ||
      (!input.dtype().is_floating() && !input.dtype().is_complex()) ||
      input.numel() == 0 || !detail::finite(input))
    throw std::invalid_argument(
        "math::krylov::exp_action: input must be non-empty, finite, and "
        "floating or complex");
  const auto dims = detail::normalized_dims(input, solve_dims, operation);
  detail::require_trailing_dims(input, dims, operation);
  std::int64_t state_size = 1;
  for (const std::int64_t axis : dims) {
    if (state_size >
        std::numeric_limits<std::int64_t>::max() / input.size(axis))
      throw std::overflow_error(
          "math::krylov::exp_action: state size overflows int64");
    state_size *= input.size(axis);
  }
  const std::int64_t width = std::min(options.max_iterations, state_size);
  const Series beta = detail::vector_norm(input, dims);
  if (time == 0.0)
    return {input, beta.mul(0.0), beta.mul(0.0).eq(0.0), 0, 0};

  auto checked_apply = [&](const Series &value) {
    Series image = apply(value);
    if (!detail::same_layout(image, input) || !detail::finite(image))
      throw std::invalid_argument(
          "math::krylov::exp_action: operator output must be finite and "
          "match the input layout");
    return image;
  };

  const Series coefficient_beta = beta.scalar_type() == input.scalar_type()
                                      ? beta
                                      : beta.to(input.scalar_type());
  const Series zero_coefficient = coefficient_beta.mul(0.0);
  const Series nonzero = beta.gt(0.0);
  const Series safe_beta =
      beta.add(nonzero.logical_not().to(beta.scalar_type()));
  std::vector<Series> basis;
  basis.reserve(static_cast<std::size_t>(width) + 1);
  basis.push_back(input.div(safe_beta).mul(nonzero.to(input.scalar_type())));
  std::vector<std::vector<Series>> h(
      static_cast<std::size_t>(width) + 1,
      std::vector<Series>(static_cast<std::size_t>(width), zero_coefficient));

  for (std::int64_t column = 0; column < width; ++column) {
    Series candidate = checked_apply(basis[static_cast<std::size_t>(column)]);
    for (std::int64_t row = 0; row <= column; ++row) {
      const Series coefficient = detail::hermitian_inner(
          basis[static_cast<std::size_t>(row)], candidate, dims);
      h[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] =
          coefficient;
      candidate =
          candidate.sub(basis[static_cast<std::size_t>(row)].mul(coefficient));
    }
    const Series next_norm = detail::vector_norm(candidate, dims);
    h[static_cast<std::size_t>(column + 1)][static_cast<std::size_t>(column)] =
        next_norm.scalar_type() == input.scalar_type()
            ? next_norm
            : next_norm.to(input.scalar_type());
    const Series usable = next_norm.gt(0.0);
    const Series safe_norm =
        next_norm.add(usable.logical_not().to(next_norm.scalar_type()));
    basis.push_back(
        candidate.div(safe_norm).mul(usable.to(input.scalar_type())));
  }

  std::vector<Series> rows;
  rows.reserve(static_cast<std::size_t>(width));
  for (std::int64_t row = 0; row < width; ++row) {
    std::vector<Series> cells;
    cells.reserve(static_cast<std::size_t>(width));
    for (std::int64_t column = 0; column < width; ++column)
      cells.push_back(detail::remove_trailing_singletons(
          h[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)],
          dims.size()));
    rows.push_back(detail::stack_last(cells));
  }
  const Series projected = detail::stack_rows(rows);
  std::vector<Series> rhs_cells;
  rhs_cells.reserve(static_cast<std::size_t>(width));
  rhs_cells.push_back(
      detail::remove_trailing_singletons(coefficient_beta, dims.size()));
  for (std::int64_t row = 1; row < width; ++row)
    rhs_cells.push_back(
        detail::remove_trailing_singletons(zero_coefficient, dims.size()));
  const Series small_rhs = detail::stack_last(rhs_cells).unsqueeze(-1);
  const Series coefficients =
      projected.mul(time).linearalg.matrix_exp().matmul(small_rhs).squeeze(-1);

  Series result = pluss::series::zeros_like(input);
  for (std::int64_t column = 0; column < width; ++column) {
    const Series coefficient =
        detail::append_singletons(coefficients.select(-1, column), dims.size());
    result =
        result.add(basis[static_cast<std::size_t>(column)].mul(coefficient));
  }
  const Series tail =
      h[static_cast<std::size_t>(width)][static_cast<std::size_t>(width - 1)]
          .abs();
  const Series last_coefficient = detail::append_singletons(
      coefficients.select(-1, width - 1).abs(), dims.size());
  const Series residual_estimate = tail.mul(last_coefficient);
  const Series tolerance = detail::vector_norm(result, dims)
                               .mul(options.relative_tolerance)
                               .add(options.absolute_tolerance);
  return {std::move(result), residual_estimate, residual_estimate.le(tolerance),
          width, width};
}

} // namespace pluss::subsp::math::krylov
