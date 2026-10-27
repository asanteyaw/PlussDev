#pragma once

#include "pluss/subsp/math/krylov.h"
#include "pluss/subsp/math/multigrid.h"
#include "pluss/subsp/natsci/common/boundary.h"
#include "pluss/subsp/natsci/common/spectral.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::common::spatial {

// The discrete operator inverted by a periodic solve. Compact is the
// three-point Laplacian. Wide is divergence(gradient(.)) of the centered
// operators; it is the operator a centered projection needs and has Nyquist
// null modes that its compatible right-hand sides never excite.
enum class PoissonStencil : std::uint8_t { Compact = 0, Wide = 1 };
enum class PoissonPc : std::uint8_t { None = 0, Diag = 1 };
// Auto picks the best direct/accelerated path the problem admits: the exact spectral inverse for constant-coefficient periodic fields (spectral.h; needs a
// real Float32/Float64 CPU field), multigrid-preconditioned conjugate gradients (math/multigrid.h) for bounded and variable-coefficient problems, diagonal
// PCG otherwise. The other values request that path and throw if the problem does not admit it; ConjugateGradient is the plain PCG baseline.
enum class PoissonMethod : std::uint8_t { Auto = 0, ConjugateGradient = 1, Fft = 2, Multigrid = 3 };

struct PoissonOptions final {
  PoissonStencil stencil{PoissonStencil::Compact};
  PoissonMethod method{PoissonMethod::Auto};
  PoissonPc pc{PoissonPc::Diag};
  math::krylov::CgOptions solve{};
  double mean_tolerance{1e-10};
  bool require_convergence{true};
};

struct PoissonResult final {
  Series solution;
  Series residual_norm;
  Series relative_residual;
  Series converged;
  Series breakdown;
  std::int64_t iterations{};
};

namespace detail {

inline std::vector<std::int64_t> spatial_axes(const Series &value,
                                              const PeriodicGrid &grid) {
  std::vector<std::int64_t> axes;
  axes.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t axis = value.dim() - grid.dim(); axis < value.dim(); ++axis)
    axes.push_back(axis);
  return axes;
}

inline std::vector<std::int64_t> spatial_axes(const Series &value,
                                              const BoxGrid &grid) {
  std::vector<std::int64_t> axes;
  axes.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t axis = value.dim() - grid.dim(); axis < value.dim(); ++axis)
    axes.push_back(axis);
  return axes;
}

// Component of `value` in the null space of the periodic operator. The compact
// Laplacian annihilates constants only. The wide operator
// (u[i+2] - 2u[i] + u[i-2]) acts on the even and odd cells of an even-extent
// axis independently, so its null space on such an axis also holds the
// alternating mode: the component is the mean over each parity class, taken
// axis by axis (the null space is a tensor product of the per-axis spaces).
// The result broadcasts against `value`.
inline Series null_component(const Series &value, const PeriodicGrid &grid,
                             PoissonStencil stencil) {
  const auto axes = spatial_axes(value, grid);
  if (stencil == PoissonStencil::Compact)
    return value.mean(axes, true);
  Series component = value;
  for (const std::int64_t axis : axes) {
    const std::int64_t extent = component.size(axis);
    if (extent % 2 != 0) {
      component = component.mean(axis, true).expand(component.sizes(), true);
      continue;
    }
    std::vector<std::int64_t> split(component.sizes().begin(),
                                    component.sizes().end());
    split[static_cast<std::size_t>(axis)] = extent / 2;
    split.insert(split.begin() + axis + 1, 2);
    const Series paired = component.reshape(split);
    component = paired.mean(axis, true)
                    .expand(split, true)
                    .reshape(std::vector<std::int64_t>(
                        component.sizes().begin(), component.sizes().end()));
  }
  return component;
}

inline math::multigrid::Problem periodic_problem(const PeriodicGrid &grid, std::vector<Series> face_high = {}) {
  math::multigrid::Problem p;
  for (std::int64_t a = 0; a < grid.dim(); ++a)
    p.axes.push_back({grid.shape()[static_cast<std::size_t>(a)], grid.spacing(a).si_value(), math::multigrid::Edge::Periodic, math::multigrid::Edge::Periodic});
  p.face_high = std::move(face_high);
  return p;
}
template <DimensionType D>
inline math::multigrid::Problem box_problem(const BoxGrid &grid, const Bounds<D> &bounds, std::vector<Series> face_high = {}) {
  using math::multigrid::Edge;
  math::multigrid::Problem p;
  for (std::int64_t a = 0; a < grid.dim(); ++a)
    p.axes.push_back({grid.shape()[static_cast<std::size_t>(a)], grid.spacing(a).si_value(), bounds.at(a, Side::Low).kind() == BoundKind::Value ? Edge::Dirichlet : Edge::Neumann,
                      bounds.at(a, Side::High).kind() == BoundKind::Value ? Edge::Dirichlet : Edge::Neumann});
  p.face_high = std::move(face_high);
  return p;
}

// The wide (centred divergence of centred gradient) operator couples only cells two apart, so on an even-extent periodic grid it is 2^d independent
// compact problems on the parity sub-lattices. Fields [B..., n_0, ..., n_{d-1}] become [B..., 2 x d parity axes, n_0/2, ..., n_{d-1}/2]: the classes are
// ordinary batch lanes for the compact multigrid, spacing 2h.
inline bool even_extents(const PeriodicGrid &grid) {
  for (const std::int64_t n : grid.shape())
    if (n % 2 != 0)
      return false;
  return true;
}
inline Series split_classes(const Series &x, std::int64_t d) {
  const std::int64_t b = x.dim() - d;
  std::vector<std::int64_t> shape(x.sizes().begin(), x.sizes().begin() + b), perm;
  for (std::int64_t i = 0; i < b; ++i)
    perm.push_back(i);
  for (std::int64_t a = 0; a < d; ++a) {
    shape.push_back(x.size(b + a) / 2);
    shape.push_back(2);
  }
  for (std::int64_t a = 0; a < d; ++a)
    perm.push_back(b + 2 * a + 1);
  for (std::int64_t a = 0; a < d; ++a)
    perm.push_back(b + 2 * a);
  return x.reshape(shape).permute(perm).contiguous();
}
inline Series merge_classes(const Series &y, std::int64_t d) {
  const std::int64_t b = y.dim() - 2 * d;
  std::vector<std::int64_t> perm, shape(y.sizes().begin(), y.sizes().begin() + b);
  for (std::int64_t i = 0; i < b; ++i)
    perm.push_back(i);
  for (std::int64_t a = 0; a < d; ++a) {
    perm.push_back(b + d + a); // m_a
    perm.push_back(b + a);     // parity_a
    shape.push_back(2 * y.size(b + d + a));
  }
  return y.permute(perm).contiguous().reshape(shape);
}

// reduce the class axes of a parity-split solve back to the batch layout [B..., 1 x d] used by every other Poisson result, and merge the solution
inline math::krylov::CgResult fold_classes(math::krylov::CgResult r, std::int64_t d) {
  const std::int64_t b = r.residual_norm.dim() - 2 * d;
  std::vector<std::int64_t> cls, shape(r.residual_norm.sizes().begin(), r.residual_norm.sizes().begin() + b);
  for (std::int64_t i = 0; i < d; ++i) {
    cls.push_back(b + i);
    shape.push_back(1);
  }
  auto all = [&](const Series &x) { return x.to(pluss::ScalarType::Float64).amin(cls, true).reshape(shape).gt(0.5); };
  auto any = [&](const Series &x) { return x.to(pluss::ScalarType::Float64).amax(cls, true).reshape(shape).gt(0.5); };
  r.solution = merge_classes(r.solution, d);
  r.residual_norm = r.residual_norm.square().sum(cls, true).reshape(shape).sqrt();
  r.relative_residual = r.relative_residual.amax(cls, true).reshape(shape);
  r.converged = all(r.converged);
  r.breakdown = any(r.breakdown);
  return r;
}

template <class Apply>
inline math::krylov::CgResult solve_spd(Apply &&apply, const Series &rhs,
                                        const std::vector<std::int64_t> &axes,
                                        const PoissonOptions &options,
                                        const Series &diagonal) {
  switch (options.pc) {
  case PoissonPc::None:
    return math::krylov::cg(std::forward<Apply>(apply), rhs, axes,
                            options.solve);
  case PoissonPc::Diag:
    if (!diagonal.defined() ||
        !diagonal.gt(0.0).logical_and(diagonal.isfinite()).all().item<bool>())
      throw std::invalid_argument(
          "natsci::spatial::solve_poisson: diagonal preconditioner must be "
          "finite and positive");
    return math::krylov::pcg(
        std::forward<Apply>(apply),
        [&diagonal](const Series &residual) { return residual.div(diagonal); },
        rhs, axes, options.solve);
  }
  throw std::invalid_argument(
      "natsci::spatial::solve_poisson: unknown preconditioner");
}

inline double constant_diagonal(const PeriodicGrid &grid,
                                PoissonStencil stencil) {
  double diagonal = 0.0;
  const double factor = stencil == PoissonStencil::Wide ? 0.5 : 2.0;
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const double width = grid.spacing(axis).si_value();
    diagonal += factor / (width * width);
  }
  return diagonal;
}

inline double constant_diagonal(const BoxGrid &grid) {
  double diagonal = 0.0;
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const double width = grid.spacing(axis).si_value();
    diagonal += 2.0 / (width * width);
  }
  return diagonal;
}

} // namespace detail

[[nodiscard]] inline PoissonResult
solve_poisson(const Series &rhs, const PeriodicGrid &grid,
              const PoissonOptions &options = {}) {
  constexpr const char *operation = "natsci::spatial::solve_poisson";
  detail::require_scalar_layout(rhs, grid, operation);
  if (!rhs.dtype().is_floating())
    throw std::invalid_argument(
        "natsci::spatial::solve_poisson: rhs must have a real floating dtype");
  if (!(options.mean_tolerance >= 0.0) ||
      !std::isfinite(options.mean_tolerance))
    throw std::invalid_argument(
        "natsci::spatial::solve_poisson: mean_tolerance must be finite and "
        "non-negative");

  const bool wide = options.stencil == PoissonStencil::Wide;
  if (options.method == PoissonMethod::Multigrid && wide)
    throw std::invalid_argument("natsci::spatial::solve_poisson: the wide stencil is solved by Fft or ConjugateGradient (its parity classes decouple)");
  const auto axes = detail::spatial_axes(rhs, grid);
  const Series null_part = detail::null_component(rhs, grid, options.stencil);
  if (!null_part.abs().le(options.mean_tolerance).all().item<bool>())
    throw std::domain_error(
        "natsci::spatial::solve_poisson: periodic rhs must have zero spatial "
        "mean in every batch lane");
  const Series compatible_rhs = rhs.sub(null_part);
  const bool spectral_ok = rhs.platform() == pluss::Platform::CPU && (rhs.scalar_type() == pluss::ScalarType::Float64 || rhs.scalar_type() == pluss::ScalarType::Float32);
  if (options.method == PoissonMethod::Fft && !spectral_ok)
    throw std::invalid_argument("natsci::spatial::solve_poisson: the Fft method needs a Float32/Float64 CPU field");
  if (options.method == PoissonMethod::Auto || options.method == PoissonMethod::Fft) {
    if (spectral_ok) {
      Series solution = spectral::solve(compatible_rhs, grid, wide);
      const Series residual = (wide ? wide_laplacian(solution, grid) : laplacian(solution, grid)).sub(compatible_rhs);
      const Series norm = residual.square().sum(axes, true).sqrt();
      const Series scale = compatible_rhs.square().sum(axes, true).sqrt();
      Series relative = norm.div(scale.clamp_min(std::numeric_limits<double>::min()));
      return {std::move(solution), norm, std::move(relative), norm.mul(0.0).eq(0.0), norm.mul(0.0).ne(0.0), 0};
    }
  }
  if (options.method == PoissonMethod::Multigrid) {
    const math::multigrid::Hierarchy h(detail::periodic_problem(grid), rhs);
    auto result = math::multigrid::solve(h, compatible_rhs.neg(), static_cast<std::size_t>(grid.dim()), options.solve);
    Series solution = result.solution.sub(detail::null_component(result.solution, grid, options.stencil));
    if (options.require_convergence && (!result.converged.all().item<bool>() || result.breakdown.any().item<bool>()))
      throw std::runtime_error("natsci::spatial::solve_poisson: multigrid-preconditioned conjugate gradient did not converge");
    return {std::move(solution), std::move(result.residual_norm), std::move(result.relative_residual), std::move(result.converged), std::move(result.breakdown), result.iterations};
  }
  auto result = detail::solve_spd(
      [&grid, wide](const Series &value) {
        return (wide ? wide_laplacian(value, grid) : laplacian(value, grid))
            .neg();
      },
      compatible_rhs.neg(), axes, options,
      pluss::codem::make_scalar_like(
          rhs, detail::constant_diagonal(grid, options.stencil)));
  Series solution = result.solution.sub(
      detail::null_component(result.solution, grid, options.stencil));

  if (options.require_convergence && (!result.converged.all().item<bool>() ||
                                      result.breakdown.any().item<bool>()))
    throw std::runtime_error(
        "natsci::spatial::solve_poisson: conjugate gradient did not converge");

  return {std::move(solution),
          std::move(result.residual_norm),
          std::move(result.relative_residual),
          std::move(result.converged),
          std::move(result.breakdown),
          result.iterations};
}

template <DimensionType D> struct QuantityPoissonResult final {
  SeriesQuantity<DimensionProduct<D, Area>> solution;
  Series residual_norm;
  Series relative_residual;
  Series converged;
  Series breakdown;
  std::int64_t iterations{};
};

template <DimensionType D>
[[nodiscard]] inline QuantityPoissonResult<D>
solve_poisson(const SeriesQuantity<D> &rhs, const PeriodicGrid &grid,
              const PoissonOptions &options = {}) {
  auto raw = solve_poisson(rhs.si_value(), grid, options);
  return {SeriesQuantity<DimensionProduct<D, Area>>::from_si(
              std::move(raw.solution)),
          std::move(raw.residual_norm),
          std::move(raw.relative_residual),
          std::move(raw.converged),
          std::move(raw.breakdown),
          raw.iterations};
}

template <DimensionType SolutionD>
[[nodiscard]] inline PoissonResult
solve_poisson(const Series &rhs, const BoxGrid &grid,
              const Bounds<SolutionD> &bounds,
              const PoissonOptions &options = {}) {
  constexpr const char *operation = "natsci::spatial::solve_poisson";
  detail::require_scalar_layout(rhs, grid, operation);
  if (!rhs.dtype().is_floating())
    throw std::invalid_argument(
        "natsci::spatial::solve_poisson: rhs must have a real floating dtype");
  if (options.stencil != PoissonStencil::Compact)
    throw std::invalid_argument(
        "natsci::spatial::solve_poisson: the wide stencil is defined for "
        "periodic grids only");
  if (bounds.dim() != grid.dim())
    throw std::invalid_argument(
        "natsci::spatial::solve_poisson: boundary and grid rank mismatch");
  if (!(options.mean_tolerance >= 0.0) ||
      !std::isfinite(options.mean_tolerance))
    throw std::invalid_argument(
        "natsci::spatial::solve_poisson: mean_tolerance must be finite and "
        "non-negative");

  const auto axes = detail::spatial_axes(rhs, grid);
  const auto zero =
      SeriesQuantity<SolutionD>::from_si(pluss::series::zeros_like(rhs));
  const auto homogeneous = bounds.zeroed();
  const Series affine = laplacian(zero, grid, bounds).si_value();
  Series compatible_rhs = rhs.sub(affine);

  if (bounds.all_flux()) {
    const Series mean = compatible_rhs.mean(axes, true);
    if (!mean.abs().le(options.mean_tolerance).all().item<bool>())
      throw std::domain_error(
          "natsci::spatial::solve_poisson: all-flux rhs violates the "
          "boundary compatibility condition in at least one batch lane");
    compatible_rhs = compatible_rhs.sub(mean);
  }

  if (options.method == PoissonMethod::Fft)
    throw std::invalid_argument("natsci::spatial::solve_poisson: the Fft method is defined for periodic grids only");
  std::optional<math::multigrid::Hierarchy> hierarchy;
  // Auto: multigrid once the grid is large enough for its O(1) iteration count to repay the V-cycle (small grids keep the cheap PCG baseline)
  const bool use_multigrid = options.method == PoissonMethod::Multigrid ||
                             (options.method == PoissonMethod::Auto && *std::max_element(grid.shape().begin(), grid.shape().end()) >= 64);
  if (use_multigrid)
    hierarchy.emplace(detail::box_problem(grid, bounds), rhs);
  auto result = hierarchy ? math::multigrid::solve(*hierarchy, compatible_rhs.neg(), static_cast<std::size_t>(grid.dim()), options.solve)
                          : detail::solve_spd(
      [&grid, &homogeneous](const Series &value) {
        return laplacian(SeriesQuantity<SolutionD>::from_si(value), grid,
                         homogeneous)
            .si_value()
            .neg();
      },
      compatible_rhs.neg(), axes, options,
      pluss::codem::make_scalar_like(rhs, detail::constant_diagonal(grid)));
  Series solution = std::move(result.solution);
  if (bounds.all_flux())
    solution = solution.sub(solution.mean(axes, true));

  if (options.require_convergence &&
      (!result.converged.all().template item<bool>() ||
       result.breakdown.any().template item<bool>()))
    throw std::runtime_error(
        "natsci::spatial::solve_poisson: conjugate gradient did not converge");

  return {std::move(solution),
          std::move(result.residual_norm),
          std::move(result.relative_residual),
          std::move(result.converged),
          std::move(result.breakdown),
          result.iterations};
}

template <DimensionType SourceD>
[[nodiscard]] inline QuantityPoissonResult<SourceD>
solve_poisson(const SeriesQuantity<SourceD> &rhs, const BoxGrid &grid,
              const Bounds<DimensionProduct<SourceD, Area>> &solution_bounds,
              const PoissonOptions &options = {}) {
  auto raw = solve_poisson(rhs.si_value(), grid, solution_bounds, options);
  return {SeriesQuantity<DimensionProduct<SourceD, Area>>::from_si(
              std::move(raw.solution)),
          std::move(raw.residual_norm),
          std::move(raw.relative_residual),
          std::move(raw.converged),
          std::move(raw.breakdown),
          raw.iterations};
}

} // namespace pluss::subsp::natsci::common::spatial
