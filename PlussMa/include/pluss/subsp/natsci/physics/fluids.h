#pragma once

#include "pluss/subsp/natsci/common/poisson.h"
#include "pluss/subsp/natsci/common/solve.h"
#include "pluss/subsp/natsci/physics/fluids/boundary.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::physics::fluids {

namespace common = ::pluss::subsp::natsci::common;
namespace spatial = ::pluss::subsp::natsci::common::spatial;

using Density = common::DimensionQuotient<common::Mass, common::Volume>;
using KinematicViscosity =
    common::DimensionQuotient<common::Area, common::Time>;
using DynamicViscosity = common::DimensionProduct<Density, KinematicViscosity>;
using PressureSource =
    common::DimensionQuotient<common::Pressure, common::Area>;
using SpeedSquared = common::DimensionProduct<common::Speed, common::Speed>;
using VortSquared =
    common::DimensionProduct<common::Frequency, common::Frequency>;
using SpecificPower = common::DimensionQuotient<SpeedSquared, common::Time>;

enum class FlowForm {
  Advect,
  Conserve,
  Skew,
};

namespace detail {

inline bool same_shape(const Series &lhs, const Series &rhs) noexcept {
  if (lhs.dim() != rhs.dim())
    return false;
  for (std::int64_t axis = 0; axis < lhs.dim(); ++axis)
    if (lhs.size(axis) != rhs.size(axis))
      return false;
  return true;
}

inline void require_real_finite(const Series &value, const char *operation,
                                const char *role) {
  if (!value.defined() || !value.dtype().is_floating() || value.numel() == 0)
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " must be a non-empty real floating Series");
  if (!value.isfinite().all().item<bool>())
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " must be finite");
}

template <common::DimensionType D>
inline void require_positive(const common::ScalarQuantity<D> &value,
                             const char *operation, const char *role) {
  if (!(value.si_value() > 0.0) || !std::isfinite(value.si_value()))
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " must be finite and positive");
}

template <common::DimensionType D>
inline void require_nonnegative(const common::ScalarQuantity<D> &value,
                                const char *operation, const char *role) {
  if (!(value.si_value() >= 0.0) || !std::isfinite(value.si_value()))
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " must be finite and non-negative");
}

template <class Grid>
inline std::vector<std::int64_t> batch_shape(const Series &pressure,
                                             const Grid &grid) {
  return {pressure.sizes().begin(), pressure.sizes().end() - grid.dim()};
}

template <common::DimensionType D, class Grid>
inline void require_prop(const common::SeriesQuantity<D> &property,
                         const Series &reference, const Grid &grid,
                         const char *operation, const char *role,
                         bool strictly_positive) {
  const Series &value = property.si_value();
  require_real_finite(value, operation, role);
  spatial::detail::require_scalar_layout(value, grid, operation);
  if (!same_shape(value, reference) ||
      value.scalar_type() != reference.scalar_type() ||
      value.platform() != reference.platform())
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " must match pressure layout, dtype, and "
                                "platform");
  const bool valid = strictly_positive
                         ? value.gt(0.0).all().template item<bool>()
                         : value.ge(0.0).all().template item<bool>();
  if (!valid)
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                (strictly_positive
                                     ? " must be positive in every cell"
                                     : " must be non-negative in every cell"));
}

} // namespace detail

namespace detail {
struct StateAccess;
}

class IncompressibleState final {
public:
  [[nodiscard]] const common::SeriesQuantity<common::Speed> &
  velocity() const noexcept {
    return velocity_;
  }

  [[nodiscard]] const common::SeriesQuantity<common::Pressure> &
  pressure() const noexcept {
    return pressure_;
  }

  [[nodiscard]] std::vector<std::int64_t>
  batch_shape(const spatial::PeriodicGrid &grid) const {
    spatial::detail::require_scalar_layout(
        pressure_.si_value(), grid, "natsci::fluids::IncompressibleState");
    return detail::batch_shape(pressure_.si_value(), grid);
  }

  [[nodiscard]] std::vector<std::int64_t>
  batch_shape(const spatial::BoxGrid &grid) const {
    spatial::detail::require_scalar_layout(
        pressure_.si_value(), grid, "natsci::fluids::IncompressibleState");
    return detail::batch_shape(pressure_.si_value(), grid);
  }

private:
  IncompressibleState(common::SeriesQuantity<common::Speed> velocity,
                      common::SeriesQuantity<common::Pressure> pressure)
      : velocity_(std::move(velocity)), pressure_(std::move(pressure)) {}

  common::SeriesQuantity<common::Speed> velocity_;
  common::SeriesQuantity<common::Pressure> pressure_;

  friend IncompressibleState
  make_state(const common::SeriesQuantity<common::Speed> &,
             const common::SeriesQuantity<common::Pressure> &,
             const spatial::PeriodicGrid &);
  friend IncompressibleState
  make_state(const common::SeriesQuantity<common::Speed> &,
             const common::SeriesQuantity<common::Pressure> &,
             const spatial::BoxGrid &);
  friend struct detail::StateAccess;
};

namespace detail {
// Build a state from fields whose layout is already known to be valid (they
// are arithmetic combinations of validated states). Skips the finiteness and
// layout scans of make_state, which are device synchronizations; the stepper
// validates once per step on its result instead of once per stage.
struct StateAccess {
  static IncompressibleState
  make(common::SeriesQuantity<common::Speed> velocity,
       common::SeriesQuantity<common::Pressure> pressure) {
    return IncompressibleState(std::move(velocity), std::move(pressure));
  }
};

template <class Grid>
inline IncompressibleState
validated_state(const common::SeriesQuantity<common::Speed> &velocity,
                const common::SeriesQuantity<common::Pressure> &pressure,
                const Grid &grid) {
  constexpr const char *operation = "natsci::fluids::make_state";
  const Series &u = velocity.si_value();
  const Series &p = pressure.si_value();
  require_real_finite(u, operation, "velocity");
  require_real_finite(p, operation, "pressure");
  const std::int64_t component_axis =
      spatial::detail::require_vector_layout(u, grid, operation);
  spatial::detail::require_scalar_layout(p, grid, operation);
  if (p.dim() != u.dim() - 1)
    throw std::invalid_argument(
        "natsci::fluids::make_state: pressure must omit only the velocity "
        "component axis");
  for (std::int64_t axis = 0; axis < p.dim(); ++axis) {
    const std::int64_t velocity_axis = axis < component_axis ? axis : axis + 1;
    if (p.size(axis) != u.size(velocity_axis))
      throw std::invalid_argument(
          "natsci::fluids::make_state: pressure and velocity field shapes "
          "do not match");
  }
  if (u.scalar_type() != p.scalar_type() || u.platform() != p.platform())
    throw std::invalid_argument(
        "natsci::fluids::make_state: pressure and velocity must share dtype "
        "and platform");
  return StateAccess::make(velocity, pressure);
}
} // namespace detail

[[nodiscard]] inline IncompressibleState
make_state(const common::SeriesQuantity<common::Speed> &velocity,
           const common::SeriesQuantity<common::Pressure> &pressure,
           const spatial::PeriodicGrid &grid) {
  return detail::validated_state(velocity, pressure, grid);
}

[[nodiscard]] inline IncompressibleState
make_state(const common::SeriesQuantity<common::Speed> &velocity,
           const common::SeriesQuantity<common::Pressure> &pressure,
           const spatial::BoxGrid &grid) {
  return detail::validated_state(velocity, pressure, grid);
}

struct DivergenceReport final {
  common::SeriesQuantity<common::Frequency> field;
  common::SeriesQuantity<common::Frequency> mean;
  common::SeriesQuantity<common::Frequency> rms;
  common::SeriesQuantity<common::Frequency> max_abs;
};

// Divergence of the cell-centered velocity with the centered operator, the
// same operator the projection drives to zero.
[[nodiscard]] inline DivergenceReport
div_report(const common::SeriesQuantity<common::Speed> &velocity,
           const spatial::PeriodicGrid &grid) {
  const Series divergence = spatial::divergence(velocity.si_value(), grid);
  const auto axes = spatial::detail::spatial_axes(divergence, grid);
  return {common::SeriesQuantity<common::Frequency>::from_si(divergence),
          common::SeriesQuantity<common::Frequency>::from_si(
              divergence.mean(axes, false)),
          common::SeriesQuantity<common::Frequency>::from_si(
              divergence.square().mean(axes, false).sqrt()),
          common::SeriesQuantity<common::Frequency>::from_si(
              divergence.abs().amax(axes, false))};
}

[[nodiscard]] inline DivergenceReport
div_report(const common::SeriesQuantity<common::Speed> &velocity,
           const spatial::BoxGrid &grid, const FlowBounds &bounds);

namespace detail {

inline std::pair<Series, Series>
vel_neighbors(const common::SeriesQuantity<common::Speed> &velocity,
              const spatial::BoxGrid &grid, const FlowBounds &bounds,
              std::int64_t axis, const char *operation) {
  const Series &field = velocity.si_value();
  spatial::detail::require_vector_layout(field, grid, operation);
  spatial::detail::require_axis(grid, axis, operation);
  const std::int64_t series_axis = field.dim() - grid.dim() + axis;
  const std::int64_t extent = field.size(series_axis);
  const auto ghost = vel_ghosts(velocity, grid, bounds, axis);
  Series lower =
      ghost.low.si_value()
          .unsqueeze(series_axis)
          .unite({field.slice(series_axis, 0, extent - 1)}, series_axis);
  Series upper =
      field.slice(series_axis, 1, extent)
          .unite({ghost.high.si_value().unsqueeze(series_axis)}, series_axis);
  return {std::move(lower), std::move(upper)};
}

inline Series vel_partial(const common::SeriesQuantity<common::Speed> &velocity,
                          const spatial::BoxGrid &grid,
                          const FlowBounds &bounds, std::int64_t axis) {
  auto [lower, upper] = vel_neighbors(velocity, grid, bounds, axis,
                                      "natsci::fluids::vel_partial");
  return upper.sub(lower).div(pluss::codem::make_scalar_like(
      upper, 2.0 * grid.spacing(axis).si_value()));
}

inline Series
vel_laplacian(const common::SeriesQuantity<common::Speed> &velocity,
              const spatial::BoxGrid &grid, const FlowBounds &bounds) {
  const Series &field = velocity.si_value();
  Series result = pluss::series::zeros_like(field);
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    auto [lower, upper] = vel_neighbors(velocity, grid, bounds, axis,
                                        "natsci::fluids::vel_laplacian");
    const double h = grid.spacing(axis).si_value();
    result = result.add(lower.add(upper).sub(field.mul(2.0)).div(h * h));
  }
  return result;
}

inline Series
vel_divergence(const common::SeriesQuantity<common::Speed> &velocity,
               const spatial::BoxGrid &grid, const FlowBounds &bounds) {
  const Series &field = velocity.si_value();
  const std::int64_t component_axis = spatial::detail::require_vector_layout(
      field, grid, "natsci::fluids::vel_divergence");
  Series result =
      vel_partial(velocity, grid, bounds, 0).select(component_axis, 0);
  for (std::int64_t axis = 1; axis < grid.dim(); ++axis)
    result = result.add(
        vel_partial(velocity, grid, bounds, axis).select(component_axis, axis));
  return result;
}

inline Series
vel_advection(const common::SeriesQuantity<common::Speed> &velocity,
              const spatial::BoxGrid &grid, const FlowBounds &bounds) {
  const Series &field = velocity.si_value();
  const std::int64_t component_axis = spatial::detail::require_vector_layout(
      field, grid, "natsci::fluids::vel_advection");
  Series result = pluss::series::zeros_like(field);
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const Series advector =
        field.select(component_axis, axis).unsqueeze(component_axis);
    result =
        result.add(vel_partial(velocity, grid, bounds, axis).mul(advector));
  }
  return result;
}

inline Series
vel_flux_div(const common::SeriesQuantity<common::Speed> &velocity,
             const spatial::BoxGrid &grid, const FlowBounds &bounds) {
  const Series &field = velocity.si_value();
  const std::int64_t component_axis = spatial::detail::require_vector_layout(
      field, grid, "natsci::fluids::vel_flux_div");
  Series result = pluss::series::zeros_like(field);
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    auto [lower, upper] = vel_neighbors(velocity, grid, bounds, axis,
                                        "natsci::fluids::vel_flux_div");
    const Series lower_speed =
        lower.select(component_axis, axis).unsqueeze(component_axis);
    const Series upper_speed =
        upper.select(component_axis, axis).unsqueeze(component_axis);
    const double width = 2.0 * grid.spacing(axis).si_value();
    result = result.add(
        upper.mul(upper_speed).sub(lower.mul(lower_speed)).div(width));
  }
  return result;
}

inline Series vel_flow(const common::SeriesQuantity<common::Speed> &velocity,
                       const spatial::BoxGrid &grid, const FlowBounds &bounds,
                       FlowForm form) {
  if (form == FlowForm::Advect)
    return vel_advection(velocity, grid, bounds);
  const Series conservative = vel_flux_div(velocity, grid, bounds);
  if (form == FlowForm::Conserve)
    return conservative;
  return vel_advection(velocity, grid, bounds).add(conservative).mul(0.5);
}

inline Series vel_grad(const common::SeriesQuantity<common::Speed> &velocity,
                       const spatial::BoxGrid &grid, const FlowBounds &bounds) {
  const Series &field = velocity.si_value();
  const std::int64_t component_axis = spatial::detail::require_vector_layout(
      field, grid, "natsci::fluids::vel_grad");
  std::vector<Series> derivatives;
  derivatives.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis)
    derivatives.push_back(vel_partial(velocity, grid, bounds, axis)
                              .unsqueeze(component_axis + 1));
  Series result = derivatives.front();
  if (derivatives.size() > 1) {
    std::vector<Series> rest(derivatives.begin() + 1, derivatives.end());
    result = result.unite(rest, component_axis + 1);
  }
  return result;
}

inline std::pair<Series, Series> edge_neighbors(const Series &field,
                                                const spatial::BoxGrid &grid,
                                                std::int64_t axis,
                                                const char *operation) {
  spatial::detail::require_scalar_layout(field, grid, operation);
  spatial::detail::require_axis(grid, axis, operation);
  const std::int64_t series_axis = field.dim() - grid.dim() + axis;
  const std::int64_t extent = field.size(series_axis);
  const Series low = field.select(series_axis, 0).unsqueeze(series_axis);
  const Series high =
      field.select(series_axis, extent - 1).unsqueeze(series_axis);
  return {low.unite({field.slice(series_axis, 0, extent - 1)}, series_axis),
          field.slice(series_axis, 1, extent).unite({high}, series_axis)};
}

inline Series harmonic_mean(const Series &left, const Series &right) {
  const Series sum = left.add(right);
  const Series nonzero = sum.gt(0.0);
  return left.mul(right)
      .mul(2.0)
      .div(sum.add(nonzero.logical_not().to(sum.scalar_type())))
      .mul(nonzero.to(sum.scalar_type()));
}

inline Series strain_from_grad(const Series &gradient,
                               std::int64_t component_axis) {
  return gradient.add(gradient.transpose(component_axis, component_axis + 1))
      .mul(0.5);
}

inline Series vort_from_grad(const Series &gradient, std::int64_t grid_dim,
                             std::int64_t component_axis) {
  if (grid_dim == 1)
    return pluss::series::zeros_like(
        gradient.select(component_axis, 0).select(component_axis, 0));
  if (grid_dim == 2)
    return gradient.select(component_axis, 1)
        .select(component_axis, 0)
        .sub(gradient.select(component_axis, 0).select(component_axis, 1));
  return spatial::detail::stack_components(
      {gradient.select(component_axis, 2)
           .select(component_axis, 1)
           .sub(gradient.select(component_axis, 1).select(component_axis, 2)),
       gradient.select(component_axis, 0)
           .select(component_axis, 2)
           .sub(gradient.select(component_axis, 2).select(component_axis, 0)),
       gradient.select(component_axis, 1)
           .select(component_axis, 0)
           .sub(gradient.select(component_axis, 0).select(component_axis, 1))},
      component_axis);
}

// Conservative divergence of 2 mu S on a cell-centred bounded grid. Normal
// derivatives at a physical face use the existing velocity ghost contract;
// material values and tangential derivatives are extended constantly to that
// face. Only the one-to-three geometric axes are visited on the host.
inline Series
var_stress_div(const common::SeriesQuantity<common::Speed> &velocity,
               const spatial::BoxGrid &grid, const FlowBounds &bounds,
               const common::SeriesQuantity<DynamicViscosity> &viscosity) {
  const Series &field = velocity.si_value();
  const Series &mu = viscosity.si_value();
  const std::int64_t component_axis = spatial::detail::require_vector_layout(
      field, grid, "natsci::fluids::var_stress_div");
  const Series gradient = vel_grad(velocity, grid, bounds);
  Series result = pluss::series::zeros_like(field);

  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const double width = grid.spacing(axis).si_value();
    auto [u_low, u_high] = vel_neighbors(velocity, grid, bounds, axis,
                                         "natsci::fluids::var_stress_div");
    auto [mu_low, mu_high] =
        edge_neighbors(mu, grid, axis, "natsci::fluids::var_stress_div");
    auto [grad_low, grad_high] =
        edge_neighbors(gradient, grid, axis, "natsci::fluids::var_stress_div");

    const Series normal_low = field.sub(u_low).div(width);
    const Series normal_high = u_high.sub(field).div(width);
    const Series row = gradient.select(component_axis, axis);
    const Series row_low = grad_low.select(component_axis, axis);
    const Series row_high = grad_high.select(component_axis, axis);

    std::vector<Series> cross_low;
    std::vector<Series> cross_high;
    cross_low.reserve(static_cast<std::size_t>(grid.dim()));
    cross_high.reserve(static_cast<std::size_t>(grid.dim()));
    for (std::int64_t component = 0; component < grid.dim(); ++component) {
      if (component == axis) {
        cross_low.push_back(normal_low.select(component_axis, component));
        cross_high.push_back(normal_high.select(component_axis, component));
      } else {
        cross_low.push_back(row.select(component_axis, component)
                                .add(row_low.select(component_axis, component))
                                .mul(0.5));
        cross_high.push_back(
            row.select(component_axis, component)
                .add(row_high.select(component_axis, component))
                .mul(0.5));
      }
    }

    const Series trans_low =
        spatial::detail::stack_components(cross_low, component_axis);
    const Series trans_high =
        spatial::detail::stack_components(cross_high, component_axis);
    const Series face_mu_low =
        harmonic_mean(mu_low, mu).unsqueeze(component_axis);
    const Series face_mu_high =
        harmonic_mean(mu, mu_high).unsqueeze(component_axis);
    const Series flux_low = normal_low.add(trans_low).mul(face_mu_low);
    const Series flux_high = normal_high.add(trans_high).mul(face_mu_high);
    result = result.add(flux_high.sub(flux_low).div(width));
  }
  return result;
}

} // namespace detail

[[nodiscard]] inline common::SeriesQuantity<common::Acceleration>
flow_term(const common::SeriesQuantity<common::Speed> &velocity,
          const spatial::PeriodicGrid &grid, FlowForm form = FlowForm::Advect) {
  if (form == FlowForm::Conserve)
    return spatial::flux_div(velocity, grid);
  if (form == FlowForm::Skew)
    return spatial::skew_advect(velocity, grid);
  return spatial::vector_advection(velocity, grid);
}

[[nodiscard]] inline common::SeriesQuantity<common::Acceleration>
flow_term(const common::SeriesQuantity<common::Speed> &velocity,
          const spatial::BoxGrid &grid, const FlowBounds &bounds,
          FlowForm form = FlowForm::Advect) {
  return common::SeriesQuantity<common::Acceleration>::from_si(
      detail::vel_flow(velocity, grid, bounds, form));
}

[[nodiscard]] inline common::SeriesQuantity<common::Frequency>
strain(const common::SeriesQuantity<common::Speed> &velocity,
       const spatial::PeriodicGrid &grid) {
  const Series &field = velocity.si_value();
  spatial::detail::require_vector_layout(field, grid,
                                         "natsci::fluids::strain");
  return spatial::sym_grad(velocity, grid);
}

[[nodiscard]] inline common::SeriesQuantity<common::Frequency>
strain(const common::SeriesQuantity<common::Speed> &velocity,
       const spatial::BoxGrid &grid, const FlowBounds &bounds) {
  const Series &field = velocity.si_value();
  const std::int64_t component_axis = spatial::detail::require_vector_layout(
      field, grid, "natsci::fluids::strain");
  return common::SeriesQuantity<common::Frequency>::from_si(
      detail::strain_from_grad(detail::vel_grad(velocity, grid, bounds),
                               component_axis));
}

[[nodiscard]] inline common::SeriesQuantity<common::Frequency>
vort(const common::SeriesQuantity<common::Speed> &velocity,
     const spatial::PeriodicGrid &grid) {
  const Series &field = velocity.si_value();
  const std::int64_t component_axis = spatial::detail::require_vector_layout(
      field, grid, "natsci::fluids::vort");
  return common::SeriesQuantity<common::Frequency>::from_si(
      detail::vort_from_grad(spatial::gradient(field, grid), grid.dim(),
                             component_axis));
}

[[nodiscard]] inline common::SeriesQuantity<common::Frequency>
vort(const common::SeriesQuantity<common::Speed> &velocity,
     const spatial::BoxGrid &grid, const FlowBounds &bounds) {
  const Series &field = velocity.si_value();
  const std::int64_t component_axis = spatial::detail::require_vector_layout(
      field, grid, "natsci::fluids::vort");
  return common::SeriesQuantity<common::Frequency>::from_si(
      detail::vort_from_grad(detail::vel_grad(velocity, grid, bounds),
                             grid.dim(), component_axis));
}

[[nodiscard]] inline common::SeriesQuantity<common::Pressure>
stress(const common::SeriesQuantity<common::Speed> &velocity,
       const spatial::PeriodicGrid &grid,
       const common::ScalarQuantity<DynamicViscosity> &viscosity) {
  detail::require_nonnegative(viscosity, "natsci::fluids::stress",
                              "dynamic viscosity");
  return strain(velocity, grid) * viscosity * 2.0;
}

[[nodiscard]] inline common::SeriesQuantity<common::Pressure>
stress(const common::SeriesQuantity<common::Speed> &velocity,
       const spatial::PeriodicGrid &grid,
       const common::SeriesQuantity<DynamicViscosity> &viscosity) {
  constexpr const char *operation = "natsci::fluids::stress";
  const Series &field = velocity.si_value();
  const std::int64_t component_axis =
      spatial::detail::require_vector_layout(field, grid, operation);
  const Series reference = field.select(component_axis, 0);
  detail::require_prop(viscosity, reference, grid, operation,
                       "dynamic viscosity", false);
  const Series expanded =
      viscosity.si_value().unsqueeze(component_axis).unsqueeze(component_axis);
  return common::SeriesQuantity<common::Pressure>::from_si(
      strain(velocity, grid).si_value().mul(expanded).mul(2.0));
}

[[nodiscard]] inline common::SeriesQuantity<common::Pressure>
stress(const common::SeriesQuantity<common::Speed> &velocity,
       const spatial::BoxGrid &grid, const FlowBounds &bounds,
       const common::ScalarQuantity<DynamicViscosity> &viscosity) {
  detail::require_nonnegative(viscosity, "natsci::fluids::stress",
                              "dynamic viscosity");
  return strain(velocity, grid, bounds) * viscosity * 2.0;
}

[[nodiscard]] inline common::SeriesQuantity<common::Pressure>
stress(const common::SeriesQuantity<common::Speed> &velocity,
       const spatial::BoxGrid &grid, const FlowBounds &bounds,
       const common::SeriesQuantity<DynamicViscosity> &viscosity) {
  constexpr const char *operation = "natsci::fluids::stress";
  const Series &field = velocity.si_value();
  const std::int64_t component_axis =
      spatial::detail::require_vector_layout(field, grid, operation);
  const Series reference = field.select(component_axis, 0);
  detail::require_prop(viscosity, reference, grid, operation,
                       "dynamic viscosity", false);
  const Series expanded =
      viscosity.si_value().unsqueeze(component_axis).unsqueeze(component_axis);
  return common::SeriesQuantity<common::Pressure>::from_si(
      strain(velocity, grid, bounds).si_value().mul(expanded).mul(2.0));
}

struct FlowStats final {
  common::SeriesQuantity<common::Frequency> vorticity;
  common::SeriesQuantity<common::Frequency> strain_rate;
  common::SeriesQuantity<common::Pressure> kinetic_density;
  common::SeriesQuantity<SpecificPower> dissipation;
  common::SeriesQuantity<common::Pressure> kinetic_mean;
  common::SeriesQuantity<VortSquared> enstrophy_mean;
  common::SeriesQuantity<SpecificPower> dissipation_mean;
  common::SeriesQuantity<common::Speed> speed_max;
  DivergenceReport divergence;
};

namespace detail {

template <class Grid, class Strain, class Vort, class Div>
inline FlowStats
flow_stats_impl(const common::SeriesQuantity<common::Speed> &velocity,
                const Grid &grid,
                const common::ScalarQuantity<Density> &density,
                const common::ScalarQuantity<KinematicViscosity> &viscosity,
                Strain &&strain_fn, Vort &&vort_fn, Div &&div_fn) {
  require_positive(density, "natsci::fluids::flow_stats", "density");
  require_nonnegative(viscosity, "natsci::fluids::flow_stats", "viscosity");
  const Series &field = velocity.si_value();
  const std::int64_t component_axis = spatial::detail::require_vector_layout(
      field, grid, "natsci::fluids::flow_stats");
  const auto spatial_axes =
      spatial::detail::spatial_axes(field.select(component_axis, 0), grid);
  auto strain_value = strain_fn();
  auto vort_value = vort_fn();
  const Series speed_sq = field.square().sum(component_axis, false);
  const Series kinetic = speed_sq.mul(0.5 * density.si_value());
  const Series strain_sq = strain_value.si_value().square().sum(
      {component_axis, component_axis + 1}, false);
  const Series dissipated = strain_sq.mul(2.0 * viscosity.si_value());
  Series vort_sq = vort_value.si_value().square();
  if (grid.dim() == 3)
    vort_sq = vort_sq.sum(component_axis, false);
  const Series speed_peak = speed_sq.sqrt().amax(spatial_axes, false);
  return {std::move(vort_value),
          std::move(strain_value),
          common::SeriesQuantity<common::Pressure>::from_si(kinetic),
          common::SeriesQuantity<SpecificPower>::from_si(dissipated),
          common::SeriesQuantity<common::Pressure>::from_si(
              kinetic.mean(spatial_axes, false)),
          common::SeriesQuantity<VortSquared>::from_si(
              vort_sq.mul(0.5).mean(spatial_axes, false)),
          common::SeriesQuantity<SpecificPower>::from_si(
              dissipated.mean(spatial_axes, false)),
          common::SeriesQuantity<common::Speed>::from_si(speed_peak),
          div_fn()};
}

} // namespace detail

[[nodiscard]] inline FlowStats
flow_stats(const common::SeriesQuantity<common::Speed> &velocity,
           const spatial::PeriodicGrid &grid,
           const common::ScalarQuantity<Density> &density,
           const common::ScalarQuantity<KinematicViscosity> &viscosity) {
  return detail::flow_stats_impl(
      velocity, grid, density, viscosity,
      [&] { return strain(velocity, grid); },
      [&] { return vort(velocity, grid); },
      [&] { return div_report(velocity, grid); });
}

[[nodiscard]] inline FlowStats
flow_stats(const common::SeriesQuantity<common::Speed> &velocity,
           const spatial::BoxGrid &grid, const FlowBounds &bounds,
           const common::ScalarQuantity<Density> &density,
           const common::ScalarQuantity<KinematicViscosity> &viscosity) {
  return detail::flow_stats_impl(
      velocity, grid, density, viscosity,
      [&] { return strain(velocity, grid, bounds); },
      [&] { return vort(velocity, grid, bounds); },
      [&] { return div_report(velocity, grid, bounds); });
}

[[nodiscard]] inline DivergenceReport
div_report(const common::SeriesQuantity<common::Speed> &velocity,
           const spatial::BoxGrid &grid, const FlowBounds &bounds) {
  const Series divergence = detail::vel_divergence(velocity, grid, bounds);
  const auto axes = spatial::detail::spatial_axes(divergence, grid);
  return {common::SeriesQuantity<common::Frequency>::from_si(divergence),
          common::SeriesQuantity<common::Frequency>::from_si(
              divergence.mean(axes, false)),
          common::SeriesQuantity<common::Frequency>::from_si(
              divergence.square().mean(axes, false).sqrt()),
          common::SeriesQuantity<common::Frequency>::from_si(
              divergence.abs().amax(axes, false))};
}

[[nodiscard]] inline common::SeriesQuantity<common::Acceleration>
mom_rate(const IncompressibleState &state, const spatial::PeriodicGrid &grid,
         const common::ScalarQuantity<Density> &density,
         const common::ScalarQuantity<KinematicViscosity> &viscosity,
         FlowForm form = FlowForm::Advect) {
  constexpr const char *operation = "natsci::fluids::mom_rate";
  detail::require_positive(density, operation, "density");
  detail::require_nonnegative(viscosity, operation, "viscosity");
  const auto convection = flow_term(state.velocity(), grid, form);
  const auto pressure_acceleration =
      spatial::gradient(state.pressure(), grid) / density;
  const auto diffusion = spatial::laplacian(state.velocity(), grid) * viscosity;
  return -convection - pressure_acceleration + diffusion;
}

[[nodiscard]] inline common::SeriesQuantity<common::Acceleration>
mom_rate(const IncompressibleState &state, const spatial::PeriodicGrid &grid,
         const common::ScalarQuantity<Density> &density,
         const common::ScalarQuantity<KinematicViscosity> &viscosity,
         const common::SeriesQuantity<common::Acceleration> &body_force,
         FlowForm form = FlowForm::Advect) {
  const auto rate = mom_rate(state, grid, density, viscosity, form);
  const Series &body = body_force.si_value();
  detail::require_real_finite(body, "natsci::fluids::mom_rate", "body force");
  if (!detail::same_shape(rate.si_value(), body) ||
      rate.si_value().scalar_type() != body.scalar_type() ||
      rate.si_value().platform() != body.platform())
    throw std::invalid_argument(
        "natsci::fluids::mom_rate: body force must match velocity "
        "layout, dtype, and platform");
  return rate + body_force;
}

[[nodiscard]] inline common::SeriesQuantity<common::Acceleration>
mom_rate(const IncompressibleState &state, const spatial::PeriodicGrid &grid,
         const common::SeriesQuantity<Density> &density,
         const common::SeriesQuantity<DynamicViscosity> &viscosity,
         FlowForm form = FlowForm::Advect);

namespace detail {

inline common::SeriesQuantity<common::Acceleration>
var_rate(const IncompressibleState &state, const spatial::PeriodicGrid &grid,
         const common::SeriesQuantity<Density> &density,
         const common::SeriesQuantity<DynamicViscosity> &viscosity,
         FlowForm form) {
  const Series &pressure = state.pressure().si_value();
  const Series &velocity = state.velocity().si_value();
  const std::int64_t component_axis = velocity.dim() - grid.dim() - 1;
  const Series rho = density.si_value().unsqueeze(component_axis);
  const Series mu =
      viscosity.si_value().unsqueeze(component_axis).unsqueeze(component_axis);
  const Series grad = spatial::gradient(velocity, grid);
  const Series tau =
      detail::strain_from_grad(grad, component_axis).mul(mu).mul(2.0);
  const Series diffusion = spatial::tensor_div(tau, grid).div(rho);
  const Series pressure_rate = spatial::gradient(pressure, grid).div(rho);
  return common::SeriesQuantity<common::Acceleration>::from_si(
      flow_term(state.velocity(), grid, form)
          .si_value()
          .neg()
          .sub(pressure_rate)
          .add(diffusion));
}

inline common::SeriesQuantity<common::Acceleration> var_rate(
    const IncompressibleState &state, const spatial::BoxGrid &grid,
    const FlowBounds &bounds, const common::SeriesQuantity<Density> &density,
    const common::SeriesQuantity<DynamicViscosity> &viscosity, FlowForm form) {
  const Series &velocity = state.velocity().si_value();
  const std::int64_t component_axis = velocity.dim() - grid.dim() - 1;
  const Series rho = density.si_value().unsqueeze(component_axis);
  const Series pressure_rate =
      spatial::gradient(state.pressure(), grid, p_bounds(grid, bounds))
          .si_value()
          .div(rho);
  const Series diffusion =
      var_stress_div(state.velocity(), grid, bounds, viscosity).div(rho);
  return common::SeriesQuantity<common::Acceleration>::from_si(
      flow_term(state.velocity(), grid, bounds, form)
          .si_value()
          .neg()
          .sub(pressure_rate)
          .add(diffusion));
}

} // namespace detail

[[nodiscard]] inline common::SeriesQuantity<common::Acceleration>
mom_rate(const IncompressibleState &state, const spatial::PeriodicGrid &grid,
         const common::SeriesQuantity<Density> &density,
         const common::SeriesQuantity<DynamicViscosity> &viscosity,
         FlowForm form) {
  constexpr const char *operation = "natsci::fluids::mom_rate";
  const Series &pressure = state.pressure().si_value();
  detail::require_prop(density, pressure, grid, operation, "density", true);
  detail::require_prop(viscosity, pressure, grid, operation, "viscosity",
                       false);
  return detail::var_rate(state, grid, density, viscosity, form);
}

[[nodiscard]] inline common::SeriesQuantity<common::Acceleration>
mom_rate(const IncompressibleState &state, const spatial::PeriodicGrid &grid,
         const common::SeriesQuantity<Density> &density,
         const common::SeriesQuantity<DynamicViscosity> &viscosity,
         const common::SeriesQuantity<common::Acceleration> &body_force,
         FlowForm form = FlowForm::Advect) {
  const auto rate = mom_rate(state, grid, density, viscosity, form);
  const Series &body = body_force.si_value();
  detail::require_real_finite(body, "natsci::fluids::mom_rate", "body force");
  if (!detail::same_shape(rate.si_value(), body) ||
      rate.si_value().scalar_type() != body.scalar_type() ||
      rate.si_value().platform() != body.platform())
    throw std::invalid_argument(
        "natsci::fluids::mom_rate: body force must match velocity "
        "layout, dtype, and platform");
  return rate + body_force;
}

[[nodiscard]] inline common::SeriesQuantity<common::Acceleration>
mom_rate(const IncompressibleState &state, const spatial::BoxGrid &grid,
         const FlowBounds &bounds,
         const common::ScalarQuantity<Density> &density,
         const common::ScalarQuantity<KinematicViscosity> &viscosity,
         FlowForm form = FlowForm::Advect) {
  constexpr const char *operation = "natsci::fluids::mom_rate";
  detail::require_positive(density, operation, "density");
  detail::require_nonnegative(viscosity, operation, "viscosity");
  const auto convection = flow_term(state.velocity(), grid, bounds, form);
  const auto pressure_acceleration =
      spatial::gradient(state.pressure(), grid, p_bounds(grid, bounds)) /
      density;
  const auto diffusion =
      common::SeriesQuantity<
          common::DimensionQuotient<common::Speed, common::Area>>::
          from_si(detail::vel_laplacian(state.velocity(), grid, bounds)) *
      viscosity;
  return -convection - pressure_acceleration + diffusion;
}

[[nodiscard]] inline common::SeriesQuantity<common::Acceleration>
mom_rate(const IncompressibleState &state, const spatial::BoxGrid &grid,
         const FlowBounds &bounds,
         const common::SeriesQuantity<Density> &density,
         const common::SeriesQuantity<DynamicViscosity> &viscosity,
         FlowForm form = FlowForm::Advect) {
  constexpr const char *operation = "natsci::fluids::mom_rate";
  if (bounds.dim() != grid.dim())
    throw std::invalid_argument(
        "natsci::fluids::mom_rate: boundary and grid rank mismatch");
  const Series &pressure = state.pressure().si_value();
  detail::require_prop(density, pressure, grid, operation, "density", true);
  detail::require_prop(viscosity, pressure, grid, operation, "viscosity",
                       false);
  return detail::var_rate(state, grid, bounds, density, viscosity, form);
}

[[nodiscard]] inline common::SeriesQuantity<common::Acceleration>
mom_rate(const IncompressibleState &state, const spatial::BoxGrid &grid,
         const FlowBounds &bounds,
         const common::ScalarQuantity<Density> &density,
         const common::ScalarQuantity<KinematicViscosity> &viscosity,
         const common::SeriesQuantity<common::Acceleration> &body_force,
         FlowForm form = FlowForm::Advect) {
  const auto rate = mom_rate(state, grid, bounds, density, viscosity, form);
  const Series &body = body_force.si_value();
  detail::require_real_finite(body, "natsci::fluids::mom_rate", "body force");
  if (!detail::same_shape(rate.si_value(), body) ||
      rate.si_value().scalar_type() != body.scalar_type() ||
      rate.si_value().platform() != body.platform())
    throw std::invalid_argument(
        "natsci::fluids::mom_rate: body force must match velocity "
        "layout, dtype, and platform");
  return rate + body_force;
}

[[nodiscard]] inline common::SeriesQuantity<common::Acceleration>
mom_rate(const IncompressibleState &state, const spatial::BoxGrid &grid,
         const FlowBounds &bounds,
         const common::SeriesQuantity<Density> &density,
         const common::SeriesQuantity<DynamicViscosity> &viscosity,
         const common::SeriesQuantity<common::Acceleration> &body_force,
         FlowForm form = FlowForm::Advect) {
  const auto rate = mom_rate(state, grid, bounds, density, viscosity, form);
  const Series &body = body_force.si_value();
  detail::require_real_finite(body, "natsci::fluids::mom_rate", "body force");
  if (!detail::same_shape(rate.si_value(), body) ||
      rate.si_value().scalar_type() != body.scalar_type() ||
      rate.si_value().platform() != body.platform())
    throw std::invalid_argument(
        "natsci::fluids::mom_rate: body force must match velocity "
        "layout, dtype, and platform");
  return rate + body_force;
}

struct ProjectionOptions final {
  // Periodic projection selects the matched wide operator. Bounded projection
  // selects its own ghost-reconstructed D(G) operator and rejects Wide.
  spatial::PoissonOptions poisson{};
};

struct ProjectionResult final {
  IncompressibleState state;
  common::SeriesQuantity<common::Pressure> pressure_correction;
  DivergenceReport before;
  DivergenceReport after;
  Series residual_norm;
  Series relative_residual;
  Series converged;
  Series breakdown;
  std::int64_t iterations{};
};

namespace detail {

struct ProjectionCore final {
  IncompressibleState state;
  common::SeriesQuantity<common::Pressure> correction;
  spatial::QuantityPoissonResult<PressureSource> solve;
};

inline Series pressure_diag(const Series &density,
                            const spatial::PeriodicGrid &grid) {
  const Series inverse = density.reciprocal();
  Series diagonal = pluss::series::zeros_like(inverse);
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const std::int64_t series_axis = density.dim() - grid.dim() + axis;
    const double width = grid.spacing(axis).si_value();
    diagonal = diagonal.add(inverse.roll(1, series_axis)
                                .add(inverse.roll(-1, series_axis))
                                .div(4.0 * width * width));
  }
  return diagonal;
}

inline Series pressure_diag(const Series &density,
                            const spatial::BoxGrid &grid) {
  double inverse_width2 = 0.0;
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const double width = grid.spacing(axis).si_value();
    inverse_width2 += 0.5 / (width * width);
  }
  // The centered bounded D(G) operator has the same interior diagonal as the
  // wide periodic operator. Ghost reconstruction changes boundary rows; this
  // positive local scale remains an SPD preconditioner rather than pretending
  // to be the exact boundary diagonal.
  return density.reciprocal().mul(inverse_width2);
}

inline double pressure_diag(const spatial::BoxGrid &grid) {
  double diagonal = 0.0;
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const double width = grid.spacing(axis).si_value();
    diagonal += 0.5 / (width * width);
  }
  return diagonal;
}

// Auto uses multigrid once the grid is big enough for the iteration-count advantage to repay the cost of a V-cycle (conjugate gradients on the
// diagonally scaled operator needs O(n) iterations, multigrid O(1)); small grids keep the cheap baseline.
inline bool pick_multigrid(spatial::PoissonMethod method, const std::vector<std::int64_t> &shape, bool admissible) {
  if (method == spatial::PoissonMethod::Multigrid)
    return true;
  if (method != spatial::PoissonMethod::Auto || !admissible)
    return false;
  return *std::max_element(shape.begin(), shape.end()) >= 64;
}
inline void require_pressure_method(spatial::PoissonMethod method, bool multigrid_ok, const char *what) {
  if (method == spatial::PoissonMethod::Fft)
    throw std::invalid_argument(std::string("natsci::fluids::project: the Fft method needs constant density on a periodic grid (") + what + ")");
  if (method == spatial::PoissonMethod::Multigrid && !multigrid_ok)
    throw std::invalid_argument(std::string("natsci::fluids::project: multigrid needs even periodic extents (") + what + ")");
}
// Multigrid-preconditioned CG on the exact bounded operator D(beta G). The centred operator couples cells two apart, so the preconditioner is the
// multigrid of its 2^d parity sub-lattices (spacing 2h, conductance beta of the cell between, the wall condition reflected on each sub-lattice); the
// exact operator couples the classes only through the wall rows, which CG absorbs. Odd extents have no such splitting and use the compact lattice.
template <class Apply>
inline math::krylov::CgResult bounded_multigrid(Apply &&apply, const Series &rhs, const spatial::BoxGrid &grid, const auto &bounds, const Series &rho, const spatial::PoissonOptions &options) {
  namespace mgrid = pluss::subsp::math::multigrid;
  const std::int64_t d = grid.dim();
  bool even = true;
  for (const std::int64_t n : grid.shape())
    even = even && n % 2 == 0;
  const auto edge_of = [&](std::int64_t a, spatial::Side side) {
    return bounds.at(a, side).kind() == spatial::BoundKind::Value ? mgrid::Edge::Dirichlet : mgrid::Edge::Neumann;
  };
  mgrid::Problem problem;
  for (std::int64_t a = 0; a < d; ++a) {
    const std::int64_t n = grid.shape()[static_cast<std::size_t>(a)];
    problem.axes.push_back({even ? n / 2 : n, (even ? 2.0 : 1.0) * grid.spacing(a).si_value(), edge_of(a, spatial::Side::Low), edge_of(a, spatial::Side::High)});
  }
  if (even) {
    // The exact operator couples the parity classes only through the wall rows, so a class-wise constant is a weakly damped mode of it, not a null mode:
    // a mass term of the size of that coupling (one wall row of n per lattice, beta / (n H^2), local conductance beta) keeps each class lattice definite.
    double n_lat = 1.0, h_lat = 0.0;
    for (const auto &ax : problem.axes) {
      n_lat = std::max(n_lat, static_cast<double>(ax.cells));
      h_lat = std::max(h_lat, ax.spacing);
    }
    const double scale = 1.0 / (n_lat * h_lat * h_lat);
    const Series beta_field = rho.defined() ? rho.reciprocal().mul(scale) : pluss::series::ones_like(rhs).mul(scale);
    problem.mass = spatial::detail::split_classes(beta_field, d);
  }
  if (rho.defined()) {
    for (std::int64_t a = 0; a < d; ++a) {
      const std::int64_t axis = rho.dim() - d + a;
      const mgrid::Axis wall{grid.shape()[static_cast<std::size_t>(a)], 1.0, mgrid::Edge::Neumann, mgrid::Edge::Neumann};
      const Series beta = rho.reciprocal();
      // split: conductance of the cell between lattice neighbours (beta_{i+1}); compact: harmonic mean of the two cells sharing the face
      const Series face = even ? mgrid::detail::upper_neighbor(beta, axis, wall) : rho.add(mgrid::detail::upper_neighbor(rho, axis, wall)).mul(0.5).reciprocal();
      problem.face_high.push_back(even ? spatial::detail::split_classes(face, d) : face);
    }
  }
  std::vector<std::int64_t> dims;
  for (std::int64_t a = 0; a < d; ++a)
    dims.push_back(rhs.dim() - d + a);
  if (!even) {
    const mgrid::Hierarchy hierarchy(std::move(problem), rhs);
    return math::krylov::pcg(std::forward<Apply>(apply), [&](const Series &r) { return hierarchy.vcycle(r); }, rhs, dims, options.solve);
  }
  const mgrid::Hierarchy hierarchy(std::move(problem), spatial::detail::split_classes(rhs, d));
  const bool singular = hierarchy.singular(); // false: the mass term makes every class lattice definite
  const auto precondition = [&](const Series &r) {
    Series split = spatial::detail::split_classes(r, d);
    if (singular) {
      std::vector<std::int64_t> spatial_dims;
      for (std::int64_t a = 0; a < d; ++a)
        spatial_dims.push_back(split.dim() - d + a);
      split = split.sub(split.mean(spatial_dims, true)); // each class lattice is singular: keep its equation consistent
    }
    return spatial::detail::merge_classes(hierarchy.vcycle(split), d);
  };
  return math::krylov::pcg(std::forward<Apply>(apply), precondition, rhs, dims, options.solve);
}

// Centered divergence and gradient with their exact composition (the wide
// stencil) as the Poisson operator: second-order consistent on a cell-centered
// grid, and the centered divergence of the projected velocity is zero to solver
// tolerance. Takes the already-computed divergence field so callers decide
// whether to report on it.
inline ProjectionCore
project_core(const IncompressibleState &state,
             const spatial::PeriodicGrid &grid,
             const common::ScalarQuantity<Density> &density,
             const common::ScalarQuantity<common::Time> &step,
             const common::SeriesQuantity<common::Frequency> &divergence,
             const spatial::PoissonOptions &poisson_options) {
  const auto rhs = (divergence * density) / step;
  static_assert(
      std::same_as<typename decltype(rhs)::dimension, PressureSource>);
  spatial::PoissonOptions poisson = poisson_options;
  poisson.stencil = spatial::PoissonStencil::Wide;
  auto pressure_solve = spatial::solve_poisson(rhs, grid, poisson);
  auto correction = pressure_solve.solution;
  const auto velocity_correction =
      (spatial::gradient(correction, grid) * step) / density;
  static_assert(std::same_as<typename decltype(velocity_correction)::dimension,
                             common::Speed>);
  return {StateAccess::make(state.velocity() - velocity_correction,
                            state.pressure() + correction),
          std::move(correction), std::move(pressure_solve)};
}

inline ProjectionCore
project_core(const IncompressibleState &state,
             const spatial::PeriodicGrid &grid,
             const common::SeriesQuantity<Density> &density,
             const common::ScalarQuantity<common::Time> &step,
             const common::SeriesQuantity<common::Frequency> &divergence,
             const spatial::PoissonOptions &poisson_options) {
  if (!(poisson_options.mean_tolerance >= 0.0) ||
      !std::isfinite(poisson_options.mean_tolerance))
    throw std::invalid_argument(
        "natsci::fluids::project: mean_tolerance must be finite and "
        "non-negative");
  const Series &rho = density.si_value();
  const Series rhs = divergence.si_value().div(step.si_value());
  const auto axes = spatial::detail::spatial_axes(rhs, grid);
  const Series null_part =
      spatial::detail::null_component(rhs, grid, spatial::PoissonStencil::Wide);
  if (!null_part.abs().le(poisson_options.mean_tolerance).all().item<bool>())
    throw std::domain_error(
        "natsci::fluids::project: periodic variable-density rhs is not "
        "compatible with the centered operator null space");
  const Series compatible_rhs = rhs.sub(null_part);
  const std::int64_t component_axis = rho.dim() - grid.dim();
  const Series inv_rho = rho.reciprocal().unsqueeze(component_axis);
  const auto apply = [&](const Series &value) {
    return spatial::divergence(spatial::gradient(value, grid).mul(inv_rho),
                               grid);
  };
  spatial::PoissonOptions poisson = poisson_options;
  poisson.stencil = spatial::PoissonStencil::Wide;
  require_pressure_method(poisson_options.method, spatial::detail::even_extents(grid), "periodic variable-density");
  const bool multigrid = pick_multigrid(poisson_options.method, grid.shape(), spatial::detail::even_extents(grid));
  auto solved = multigrid
      ? [&] {
          // the centred D(beta G) couples cells two apart: 2^d exact compact problems on the parity sub-lattices, conductance beta of the cell between
          const std::int64_t d = grid.dim();
          const Series beta = rho.reciprocal();
          pluss::subsp::math::multigrid::Problem problem;
          for (std::int64_t a = 0; a < d; ++a) {
            problem.axes.push_back({grid.shape()[static_cast<std::size_t>(a)] / 2, 2.0 * grid.spacing(a).si_value(),
                                    pluss::subsp::math::multigrid::Edge::Periodic, pluss::subsp::math::multigrid::Edge::Periodic});
            problem.face_high.push_back(spatial::detail::split_classes(beta.roll(-1, beta.dim() - d + a), d));
          }
          const Series split_rhs = spatial::detail::split_classes(compatible_rhs.neg(), d);
          const pluss::subsp::math::multigrid::Hierarchy hierarchy(std::move(problem), split_rhs);
          return spatial::detail::fold_classes(
              pluss::subsp::math::multigrid::solve(hierarchy, split_rhs, static_cast<std::size_t>(d), poisson_options.solve), d);
        }()
      : spatial::detail::solve_spd(
            [&](const Series &value) { return apply(value).neg(); },
            compatible_rhs.neg(), axes, poisson, pressure_diag(rho, grid));
  Series solution = solved.solution.sub(spatial::detail::null_component(
      solved.solution, grid, spatial::PoissonStencil::Wide));
  if (poisson_options.require_convergence &&
      (!solved.converged.all().item<bool>() ||
       solved.breakdown.any().item<bool>()))
    throw std::runtime_error(
        "natsci::fluids::project: variable-density pressure correction did "
        "not converge");

  spatial::QuantityPoissonResult<PressureSource> pressure_solve{
      common::SeriesQuantity<common::Pressure>::from_si(std::move(solution)),
      std::move(solved.residual_norm),
      std::move(solved.relative_residual),
      std::move(solved.converged),
      std::move(solved.breakdown),
      solved.iterations};
  auto correction = pressure_solve.solution;
  const Series velocity_correction =
      spatial::gradient(correction.si_value(), grid)
          .mul(inv_rho)
          .mul(step.si_value());
  return {StateAccess::make(state.velocity() -
                                common::SeriesQuantity<common::Speed>::from_si(
                                    velocity_correction),
                            state.pressure() + correction),
          std::move(correction), std::move(pressure_solve)};
}

inline ProjectionCore
project_core(const IncompressibleState &state, const spatial::BoxGrid &grid,
             const FlowBounds &bounds,
             const common::ScalarQuantity<Density> &density,
             const common::ScalarQuantity<common::Time> &step,
             const common::SeriesQuantity<common::Frequency> &divergence,
             const spatial::PoissonOptions &poisson_options) {
  const auto rhs = (divergence * density) / step;
  static_assert(
      std::same_as<typename decltype(rhs)::dimension, PressureSource>);

  if (poisson_options.stencil != spatial::PoissonStencil::Compact)
    throw std::invalid_argument(
        "natsci::fluids::project: bounded projection selects its matched "
        "operator; a periodic wide stencil cannot be requested");
  if (!(poisson_options.mean_tolerance >= 0.0) ||
      !std::isfinite(poisson_options.mean_tolerance))
    throw std::invalid_argument(
        "natsci::fluids::project: mean_tolerance must be finite and "
        "non-negative");

  const auto correction_bounds = p_bounds(grid, bounds).zeroed();
  const auto homogeneous_flow = bounds.zeroed();
  const auto axes = spatial::detail::spatial_axes(rhs.si_value(), grid);
  Series compatible_rhs = rhs.si_value();
  const bool gauged = correction_bounds.all_flux();
  if (gauged) {
    const Series mean = compatible_rhs.mean(axes, true);
    if (!mean.abs().le(poisson_options.mean_tolerance).all().item<bool>())
      throw std::domain_error(
          "natsci::fluids::project: closed bounded flow violates the net "
          "flux compatibility condition");
    compatible_rhs = compatible_rhs.sub(mean);
  }

  const auto apply = [&](const Series &value) {
    const auto gradient = spatial::gradient(
        common::SeriesQuantity<common::Pressure>::from_si(value), grid,
        correction_bounds);
    // Boundary data are homogeneous, so the numerical vector operator is
    // independent of the quantity tag used by velocity reconstruction.
    return vel_divergence(
        common::SeriesQuantity<common::Speed>::from_si(gradient.si_value()),
        grid, homogeneous_flow);
  };
  const Series diagonal =
      pluss::codem::make_scalar_like(compatible_rhs, pressure_diag(grid));
  require_pressure_method(poisson_options.method, true, "bounded");
  auto solved = pick_multigrid(poisson_options.method, grid.shape(), true)
      ? bounded_multigrid([&](const Series &value) { return apply(value).neg(); }, compatible_rhs.neg(), grid, correction_bounds, pluss::series::undefined(), poisson_options)
      : spatial::detail::solve_spd(
            [&](const Series &value) { return apply(value).neg(); },
            compatible_rhs.neg(), axes, poisson_options, diagonal);
  Series solution = std::move(solved.solution);
  if (gauged)
    solution = solution.sub(solution.mean(axes, true));
  if (poisson_options.require_convergence &&
      (!solved.converged.all().item<bool>() ||
       solved.breakdown.any().item<bool>()))
    throw std::runtime_error(
        "natsci::fluids::project: bounded pressure correction did not "
        "converge");

  spatial::QuantityPoissonResult<PressureSource> pressure_solve{
      common::SeriesQuantity<common::Pressure>::from_si(std::move(solution)),
      std::move(solved.residual_norm),
      std::move(solved.relative_residual),
      std::move(solved.converged),
      std::move(solved.breakdown),
      solved.iterations};
  auto correction = pressure_solve.solution;
  const auto velocity_correction =
      (spatial::gradient(correction, grid, correction_bounds) * step) / density;
  static_assert(std::same_as<typename decltype(velocity_correction)::dimension,
                             common::Speed>);
  return {StateAccess::make(state.velocity() - velocity_correction,
                            state.pressure() + correction),
          std::move(correction), std::move(pressure_solve)};
}

inline ProjectionCore
project_core(const IncompressibleState &state, const spatial::BoxGrid &grid,
             const FlowBounds &bounds,
             const common::SeriesQuantity<Density> &density,
             const common::ScalarQuantity<common::Time> &step,
             const common::SeriesQuantity<common::Frequency> &divergence,
             const spatial::PoissonOptions &poisson_options) {
  if (poisson_options.stencil != spatial::PoissonStencil::Compact)
    throw std::invalid_argument(
        "natsci::fluids::project: bounded projection selects its matched "
        "operator; a periodic wide stencil cannot be requested");
  if (!(poisson_options.mean_tolerance >= 0.0) ||
      !std::isfinite(poisson_options.mean_tolerance))
    throw std::invalid_argument(
        "natsci::fluids::project: mean_tolerance must be finite and "
        "non-negative");

  const Series &rho = density.si_value();
  const Series rhs = divergence.si_value().div(step.si_value());
  const auto correction_bounds = p_bounds(grid, bounds).zeroed();
  const auto homogeneous_flow = bounds.zeroed();
  const auto axes = spatial::detail::spatial_axes(rhs, grid);
  Series compatible_rhs = rhs;
  const bool gauged = correction_bounds.all_flux();
  if (gauged) {
    const Series mean = compatible_rhs.mean(axes, true);
    if (!mean.abs().le(poisson_options.mean_tolerance).all().item<bool>())
      throw std::domain_error(
          "natsci::fluids::project: closed bounded flow violates the net "
          "flux compatibility condition");
    compatible_rhs = compatible_rhs.sub(mean);
  }

  const std::int64_t component_axis = rho.dim() - grid.dim();
  const Series inv_rho = rho.reciprocal().unsqueeze(component_axis);
  const auto weighted_gradient = [&](const Series &value) {
    return spatial::gradient(
               common::SeriesQuantity<common::Pressure>::from_si(value), grid,
               correction_bounds)
        .si_value()
        .mul(inv_rho);
  };
  const auto apply = [&](const Series &value) {
    return vel_divergence(common::SeriesQuantity<common::Speed>::from_si(
                              weighted_gradient(value)),
                          grid, homogeneous_flow);
  };
  require_pressure_method(poisson_options.method, true, "bounded variable-density");
  auto solved = pick_multigrid(poisson_options.method, grid.shape(), true)
      ? bounded_multigrid([&](const Series &value) { return apply(value).neg(); }, compatible_rhs.neg(), grid, correction_bounds, rho, poisson_options)
      : spatial::detail::solve_spd(
            [&](const Series &value) { return apply(value).neg(); },
            compatible_rhs.neg(), axes, poisson_options, pressure_diag(rho, grid));
  Series solution = std::move(solved.solution);
  if (gauged)
    solution = solution.sub(solution.mean(axes, true));
  if (poisson_options.require_convergence &&
      (!solved.converged.all().item<bool>() ||
       solved.breakdown.any().item<bool>()))
    throw std::runtime_error(
        "natsci::fluids::project: bounded variable-density pressure "
        "correction did not converge");

  spatial::QuantityPoissonResult<PressureSource> pressure_solve{
      common::SeriesQuantity<common::Pressure>::from_si(std::move(solution)),
      std::move(solved.residual_norm),
      std::move(solved.relative_residual),
      std::move(solved.converged),
      std::move(solved.breakdown),
      solved.iterations};
  auto correction = pressure_solve.solution;
  const Series velocity_correction =
      weighted_gradient(correction.si_value()).mul(step.si_value());
  return {StateAccess::make(state.velocity() -
                                common::SeriesQuantity<common::Speed>::from_si(
                                    velocity_correction),
                            state.pressure() + correction),
          std::move(correction), std::move(pressure_solve)};
}

} // namespace detail

[[nodiscard]] inline ProjectionResult
project(const IncompressibleState &state, const spatial::PeriodicGrid &grid,
        const common::ScalarQuantity<Density> &density,
        const common::ScalarQuantity<common::Time> &step,
        const ProjectionOptions &options = {}) {
  constexpr const char *operation = "natsci::fluids::project";
  detail::require_positive(density, operation, "density");
  detail::require_positive(step, operation, "step");

  const DivergenceReport before = div_report(state.velocity(), grid);
  auto core = detail::project_core(state, grid, density, step, before.field,
                                   options.poisson);
  IncompressibleState next =
      make_state(core.state.velocity(), core.state.pressure(), grid);
  DivergenceReport after = div_report(next.velocity(), grid);

  return {std::move(next),
          std::move(core.correction),
          before,
          std::move(after),
          std::move(core.solve.residual_norm),
          std::move(core.solve.relative_residual),
          std::move(core.solve.converged),
          std::move(core.solve.breakdown),
          core.solve.iterations};
}

[[nodiscard]] inline ProjectionResult
project(const IncompressibleState &state, const spatial::PeriodicGrid &grid,
        const common::SeriesQuantity<Density> &density,
        const common::ScalarQuantity<common::Time> &step,
        const ProjectionOptions &options = {}) {
  constexpr const char *operation = "natsci::fluids::project";
  detail::require_prop(density, state.pressure().si_value(), grid, operation,
                       "density", true);
  detail::require_positive(step, operation, "step");

  const DivergenceReport before = div_report(state.velocity(), grid);
  auto core = detail::project_core(state, grid, density, step, before.field,
                                   options.poisson);
  IncompressibleState next =
      make_state(core.state.velocity(), core.state.pressure(), grid);
  DivergenceReport after = div_report(next.velocity(), grid);
  return {std::move(next),
          std::move(core.correction),
          before,
          std::move(after),
          std::move(core.solve.residual_norm),
          std::move(core.solve.relative_residual),
          std::move(core.solve.converged),
          std::move(core.solve.breakdown),
          core.solve.iterations};
}

[[nodiscard]] inline ProjectionResult
project(const IncompressibleState &state, const spatial::BoxGrid &grid,
        const FlowBounds &bounds,
        const common::ScalarQuantity<Density> &density,
        const common::ScalarQuantity<common::Time> &step,
        const ProjectionOptions &options = {}) {
  constexpr const char *operation = "natsci::fluids::project";
  detail::require_positive(density, operation, "density");
  detail::require_positive(step, operation, "step");
  if (bounds.dim() != grid.dim())
    throw std::invalid_argument(
        "natsci::fluids::project: boundary and grid rank mismatch");

  const DivergenceReport before = div_report(state.velocity(), grid, bounds);
  auto core = detail::project_core(state, grid, bounds, density, step,
                                   before.field, options.poisson);
  IncompressibleState next =
      make_state(core.state.velocity(), core.state.pressure(), grid);
  DivergenceReport after = div_report(next.velocity(), grid, bounds);
  return {std::move(next),
          std::move(core.correction),
          before,
          std::move(after),
          std::move(core.solve.residual_norm),
          std::move(core.solve.relative_residual),
          std::move(core.solve.converged),
          std::move(core.solve.breakdown),
          core.solve.iterations};
}

[[nodiscard]] inline ProjectionResult
project(const IncompressibleState &state, const spatial::BoxGrid &grid,
        const FlowBounds &bounds,
        const common::SeriesQuantity<Density> &density,
        const common::ScalarQuantity<common::Time> &step,
        const ProjectionOptions &options = {}) {
  constexpr const char *operation = "natsci::fluids::project";
  if (bounds.dim() != grid.dim())
    throw std::invalid_argument(
        "natsci::fluids::project: boundary and grid rank mismatch");
  detail::require_prop(density, state.pressure().si_value(), grid, operation,
                       "density", true);
  detail::require_positive(step, operation, "step");

  const DivergenceReport before = div_report(state.velocity(), grid, bounds);
  auto core = detail::project_core(state, grid, bounds, density, step,
                                   before.field, options.poisson);
  IncompressibleState next =
      make_state(core.state.velocity(), core.state.pressure(), grid);
  DivergenceReport after = div_report(next.velocity(), grid, bounds);
  return {std::move(next),
          std::move(core.correction),
          before,
          std::move(after),
          std::move(core.solve.residual_norm),
          std::move(core.solve.relative_residual),
          std::move(core.solve.converged),
          std::move(core.solve.breakdown),
          core.solve.iterations};
}

enum class StepRule : std::uint8_t {
  Euler = 0,
  Rk2 = 1,
  Rk3 = 2,
  Imex1 = 3,
};

struct StepGuard final {
  // SSPRK2/3 are convex combinations of forward-Euler stages, so these
  // coefficients conservatively enforce the forward-Euler spatial bounds.
  double courant{0.5};
  double diffusion{0.5};
  bool enforce{true};
};

struct StepLimits final {
  common::SeriesQuantity<common::Time> convection;
  common::SeriesQuantity<common::Time> diffusion;
  common::SeriesQuantity<common::Time> allowed;
};

namespace detail {

template <class Grid>
[[nodiscard]] inline StepLimits
step_limit_for_nu(const IncompressibleState &state, const Grid &grid,
                  const Series &viscosity_max, const StepGuard &guard) {
  constexpr const char *operation = "natsci::fluids::step_limit";
  if (!(guard.courant > 0.0) || !std::isfinite(guard.courant) ||
      !(guard.diffusion > 0.0) || !std::isfinite(guard.diffusion))
    throw std::invalid_argument(
        "natsci::fluids::step_limit: stability coefficients must be finite "
        "and positive");

  const Series &velocity = state.velocity().si_value();
  const auto batches = state.batch_shape(grid);
  Series conv_rate = pluss::series::zeros(batches, velocity.scalar_type())
                         .to(velocity.platform());
  double inverse_h2 = 0.0;
  const std::int64_t component_axis = velocity.dim() - grid.dim() - 1;
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const Series component = velocity.select(component_axis, axis);
    const auto axes = spatial::detail::spatial_axes(component, grid);
    conv_rate = conv_rate.add(
        component.abs().amax(axes, false).div(grid.spacing(axis).si_value()));
    const double h = grid.spacing(axis).si_value();
    inverse_h2 += 1.0 / (h * h);
  }

  const double infinity = std::numeric_limits<double>::infinity();
  const Series infinite =
      pluss::series::full(batches, infinity, velocity.scalar_type())
          .to(velocity.platform());
  if (!same_shape(viscosity_max, infinite) ||
      viscosity_max.scalar_type() != velocity.scalar_type() ||
      viscosity_max.platform() != velocity.platform() ||
      !viscosity_max.isfinite().all().item<bool>() ||
      !viscosity_max.ge(0.0).all().item<bool>())
    throw std::invalid_argument(
        "natsci::fluids::step_limit: viscosity maximum must be finite, "
        "non-negative, and match the state batch layout");
  const Series conv = conv_rate.gt(0.0).where(
      pluss::codem::make_scalar_like(conv_rate, guard.courant).div(conv_rate),
      infinite);
  const Series viscous_rate = viscosity_max.mul(inverse_h2);
  const Series visc = viscous_rate.gt(0.0).where(
      pluss::codem::make_scalar_like(viscous_rate, guard.diffusion)
          .div(viscous_rate.add(
              viscous_rate.eq(0.0).to(viscous_rate.scalar_type()))),
      infinite);
  const Series allowed = conv.minimum(visc);
  return {common::SeriesQuantity<common::Time>::from_si(conv),
          common::SeriesQuantity<common::Time>::from_si(visc),
          common::SeriesQuantity<common::Time>::from_si(allowed)};
}

template <class Grid>
[[nodiscard]] inline StepLimits
step_limit_for(const IncompressibleState &state, const Grid &grid,
               const common::ScalarQuantity<KinematicViscosity> &viscosity,
               const StepGuard &guard) {
  constexpr const char *operation = "natsci::fluids::step_limit";
  detail::require_nonnegative(viscosity, operation, "viscosity");
  const Series &velocity = state.velocity().si_value();
  const Series maximum =
      pluss::series::full(state.batch_shape(grid), viscosity.si_value(),
                          velocity.scalar_type())
          .to(velocity.platform());
  return step_limit_for_nu(state, grid, maximum, guard);
}

} // namespace detail

[[nodiscard]] inline StepLimits
step_limit(const IncompressibleState &state, const spatial::PeriodicGrid &grid,
           const common::ScalarQuantity<KinematicViscosity> &viscosity,
           const StepGuard &guard = {}) {
  return detail::step_limit_for(state, grid, viscosity, guard);
}

[[nodiscard]] inline StepLimits
step_limit(const IncompressibleState &state, const spatial::BoxGrid &grid,
           const common::ScalarQuantity<KinematicViscosity> &viscosity,
           const StepGuard &guard = {}) {
  return detail::step_limit_for(state, grid, viscosity, guard);
}

[[nodiscard]] inline StepLimits
step_limit(const IncompressibleState &state, const spatial::PeriodicGrid &grid,
           const common::SeriesQuantity<Density> &density,
           const common::SeriesQuantity<DynamicViscosity> &viscosity,
           const StepGuard &guard = {}) {
  constexpr const char *operation = "natsci::fluids::step_limit";
  const Series &pressure = state.pressure().si_value();
  detail::require_prop(density, pressure, grid, operation, "density", true);
  detail::require_prop(viscosity, pressure, grid, operation, "viscosity",
                       false);
  const auto axes = spatial::detail::spatial_axes(pressure, grid);
  const Series maximum =
      viscosity.si_value().div(density.si_value()).amax(axes, false);
  return detail::step_limit_for_nu(state, grid, maximum, guard);
}

[[nodiscard]] inline StepLimits
step_limit(const IncompressibleState &state, const spatial::BoxGrid &grid,
           const common::SeriesQuantity<Density> &density,
           const common::SeriesQuantity<DynamicViscosity> &viscosity,
           const StepGuard &guard = {}) {
  constexpr const char *operation = "natsci::fluids::step_limit";
  const Series &pressure = state.pressure().si_value();
  detail::require_prop(density, pressure, grid, operation, "density", true);
  detail::require_prop(viscosity, pressure, grid, operation, "viscosity",
                       false);
  const auto axes = spatial::detail::spatial_axes(pressure, grid);
  const Series maximum =
      viscosity.si_value().div(density.si_value()).amax(axes, false);
  return detail::step_limit_for_nu(state, grid, maximum, guard);
}

using BodyFn = std::function<common::SeriesQuantity<common::Acceleration>(
    common::ScalarQuantity<common::Time>, const IncompressibleState &)>;

struct StepOptions final {
  StepRule rule{StepRule::Rk3};
  FlowForm form{FlowForm::Advect};
  StepGuard guard{};
  ProjectionOptions projection{};
  math::krylov::CgOptions implicit{};
};

struct StepResult final {
  IncompressibleState state;
  StepLimits limits;
  DivergenceReport divergence;
  std::int64_t rate_evaluations{0};
  std::int64_t projections{0};
  std::int64_t linear_iterations{0};
};

namespace detail {

template <class Grid>
inline IncompressibleState
add_rate(const IncompressibleState &state,
         const common::SeriesQuantity<common::Acceleration> &rate,
         common::ScalarQuantity<common::Time> step, const Grid &grid) {
  (void)grid;
  return StateAccess::make(state.velocity() + rate * step, state.pressure());
}

template <class Grid>
inline IncompressibleState
blend(const IncompressibleState &left, const IncompressibleState &right,
      double left_weight, double right_weight, const Grid &grid) {
  (void)grid;
  return StateAccess::make(
      left.velocity() * left_weight + right.velocity() * right_weight,
      left.pressure() * left_weight + right.pressure() * right_weight);
}

inline common::SeriesQuantity<common::Acceleration>
rate_at(const IncompressibleState &state, const spatial::PeriodicGrid &grid,
        common::ScalarQuantity<common::Time> time,
        const common::ScalarQuantity<Density> &density,
        const common::ScalarQuantity<KinematicViscosity> &viscosity,
        const BodyFn &body, FlowForm form) {
  if (body)
    return mom_rate(state, grid, density, viscosity, body(time, state), form);
  return mom_rate(state, grid, density, viscosity, form);
}

inline common::SeriesQuantity<common::Acceleration>
rate_at(const IncompressibleState &state, const spatial::PeriodicGrid &grid,
        common::ScalarQuantity<common::Time> time,
        const common::SeriesQuantity<Density> &density,
        const common::SeriesQuantity<DynamicViscosity> &viscosity,
        const BodyFn &body, FlowForm form) {
  auto rate = var_rate(state, grid, density, viscosity, form);
  if (!body)
    return rate;
  const auto forcing = body(time, state);
  const Series &value = forcing.si_value();
  require_real_finite(value, "natsci::fluids::advance", "body force");
  if (!same_shape(rate.si_value(), value) ||
      rate.si_value().scalar_type() != value.scalar_type() ||
      rate.si_value().platform() != value.platform())
    throw std::invalid_argument(
        "natsci::fluids::advance: body force must match velocity layout, "
        "dtype, and platform");
  return rate + forcing;
}

inline common::SeriesQuantity<common::Acceleration>
rate_at(const IncompressibleState &state, const spatial::BoxGrid &grid,
        const FlowBounds &bounds, common::ScalarQuantity<common::Time> time,
        const common::ScalarQuantity<Density> &density,
        const common::ScalarQuantity<KinematicViscosity> &viscosity,
        const BodyFn &body, FlowForm form) {
  if (body)
    return mom_rate(state, grid, bounds, density, viscosity, body(time, state),
                    form);
  return mom_rate(state, grid, bounds, density, viscosity, form);
}

inline common::SeriesQuantity<common::Acceleration>
rate_at(const IncompressibleState &state, const spatial::BoxGrid &grid,
        const FlowBounds &bounds, common::ScalarQuantity<common::Time> time,
        const common::SeriesQuantity<Density> &density,
        const common::SeriesQuantity<DynamicViscosity> &viscosity,
        const BodyFn &body, FlowForm form) {
  auto rate = var_rate(state, grid, bounds, density, viscosity, form);
  if (!body)
    return rate;
  const auto forcing = body(time, state);
  const Series &value = forcing.si_value();
  require_real_finite(value, "natsci::fluids::advance", "body force");
  if (!same_shape(rate.si_value(), value) ||
      rate.si_value().scalar_type() != value.scalar_type() ||
      rate.si_value().platform() != value.platform())
    throw std::invalid_argument(
        "natsci::fluids::advance: body force must match velocity layout, "
        "dtype, and platform");
  return rate + forcing;
}

struct DiffuseCore final {
  IncompressibleState state;
  std::int64_t iterations{0};
};

inline std::vector<std::int64_t> vector_axes(const Series &velocity,
                                             std::int64_t grid_dim) {
  const std::int64_t component_axis = velocity.dim() - grid_dim - 1;
  std::vector<std::int64_t> axes;
  axes.reserve(static_cast<std::size_t>(grid_dim + 1));
  for (std::int64_t axis = component_axis; axis < velocity.dim(); ++axis)
    axes.push_back(axis);
  return axes;
}

inline DiffuseCore
diffuse_core(const IncompressibleState &state,
             const spatial::PeriodicGrid &grid,
             const common::ScalarQuantity<KinematicViscosity> &viscosity,
             const common::ScalarQuantity<common::Time> &step,
             const math::krylov::CgOptions &options) {
  if (viscosity.si_value() == 0.0)
    return {state, 0};
  const Series &rhs = state.velocity().si_value();
  const double scale = step.si_value() * viscosity.si_value();
  const auto apply = [&](const Series &value) {
    return value.sub(spatial::laplacian(value, grid).mul(scale));
  };
  auto solved =
      math::krylov::cg(apply, rhs, vector_axes(rhs, grid.dim()), options, rhs);
  if (!solved.converged.all().item<bool>() ||
      solved.breakdown.any().item<bool>())
    throw std::runtime_error(
        "natsci::fluids::advance: implicit diffusion did not converge");
  return {StateAccess::make(common::SeriesQuantity<common::Speed>::from_si(
                                std::move(solved.solution)),
                            state.pressure()),
          solved.iterations};
}

inline DiffuseCore
diffuse_core(const IncompressibleState &state, const spatial::BoxGrid &grid,
             const FlowBounds &bounds,
             const common::ScalarQuantity<KinematicViscosity> &viscosity,
             const common::ScalarQuantity<common::Time> &step,
             const math::krylov::CgOptions &options) {
  if (viscosity.si_value() == 0.0)
    return {state, 0};
  const Series &rhs = state.velocity().si_value();
  const double scale = step.si_value() * viscosity.si_value();
  const auto zero = common::SeriesQuantity<common::Speed>::from_si(
      pluss::series::zeros_like(rhs));
  const Series affine = vel_laplacian(zero, grid, bounds);
  const FlowBounds homogeneous = bounds.zeroed();
  const Series adjusted = rhs.add(affine.mul(scale));
  const auto apply = [&](const Series &value) {
    return value.sub(
        vel_laplacian(common::SeriesQuantity<common::Speed>::from_si(value),
                      grid, homogeneous)
            .mul(scale));
  };
  auto solved = math::krylov::cg(apply, adjusted, vector_axes(rhs, grid.dim()),
                                 options, rhs);
  if (!solved.converged.all().item<bool>() ||
      solved.breakdown.any().item<bool>())
    throw std::runtime_error(
        "natsci::fluids::advance: bounded implicit diffusion did not "
        "converge");
  return {StateAccess::make(common::SeriesQuantity<common::Speed>::from_si(
                                std::move(solved.solution)),
                            state.pressure()),
          solved.iterations};
}

template <class Grid, class Rate, class Diffuse, class Project, class Finalize>
inline StepResult
advance_imex(const IncompressibleState &state, const Grid &grid,
             common::ScalarQuantity<common::Time> time,
             common::ScalarQuantity<common::Time> step, StepLimits limits,
             Rate &&rate, Diffuse &&diffuse, Project &&project,
             Finalize &&finalize) {
  auto predictor = add_rate(state, rate(state, time), step, grid);
  auto diffused = diffuse(predictor);
  auto projected = project(diffused.state);
  auto [checked, report] = finalize(projected);
  return {std::move(checked), std::move(limits), std::move(report), 1, 1,
          diffused.iterations};
}

inline common::StateVector record(const IncompressibleState &state) {
  return common::states(common::field("velocity", state.velocity(), "u"),
                        common::field("pressure", state.pressure(), "p"));
}

inline IncompressibleState state(const common::StateVector &record,
                                 const spatial::PeriodicGrid &grid) {
  return make_state(record.get<common::Speed>("velocity"),
                    record.get<common::Pressure>("pressure"), grid);
}

inline IncompressibleState state(const common::StateVector &record,
                                 const spatial::BoxGrid &grid) {
  return make_state(record.get<common::Speed>("velocity"),
                    record.get<common::Pressure>("pressure"), grid);
}

template <class Grid, class Project, class Rate, class Finalize>
inline StepResult
advance_stages(const IncompressibleState &state, const Grid &grid,
               common::ScalarQuantity<common::Time> time,
               common::ScalarQuantity<common::Time> step, StepLimits limits,
               const StepOptions &options, Project &&project_stage, Rate &&rate,
               Finalize &&finalize) {
  if (options.rule == StepRule::Imex1)
    throw std::logic_error(
        "natsci::fluids::advance: internal IMEX dispatch was not selected");
  IncompressibleState next = state;
  std::int64_t evaluations = 0;
  std::int64_t projections = 0;
  if (options.rule == StepRule::Euler) {
    next = project_stage(add_rate(state, rate(state, time), step, grid));
    evaluations = 1;
    projections = 1;
  } else if (options.rule == StepRule::Rk2) {
    const auto first =
        project_stage(add_rate(state, rate(state, time), step, grid));
    const auto end = common::ScalarQuantity<common::Time>::from_si(
        time.si_value() + step.si_value());
    const auto second = add_rate(first, rate(first, end), step, grid);
    next = project_stage(blend(state, second, 0.5, 0.5, grid));
    evaluations = 2;
    projections = 2;
  } else {
    const auto first =
        project_stage(add_rate(state, rate(state, time), step, grid));
    const auto end = common::ScalarQuantity<common::Time>::from_si(
        time.si_value() + step.si_value());
    const auto first_end = add_rate(first, rate(first, end), step, grid);
    const auto second =
        project_stage(blend(state, first_end, 0.75, 0.25, grid));
    const auto half = common::ScalarQuantity<common::Time>::from_si(
        time.si_value() + 0.5 * step.si_value());
    const auto second_end = add_rate(second, rate(second, half), step, grid);
    next = project_stage(blend(state, second_end, 1.0 / 3.0, 2.0 / 3.0, grid));
    evaluations = 3;
    projections = 3;
  }

  auto [validated, report] = finalize(next);
  return {std::move(validated), std::move(limits), std::move(report),
          evaluations, projections};
}

template <class Grid>
inline StepLimits
prepare_step(const IncompressibleState &state, const Grid &grid,
             const common::ScalarQuantity<Density> &density,
             const common::ScalarQuantity<KinematicViscosity> &viscosity,
             common::ScalarQuantity<common::Time> time,
             common::ScalarQuantity<common::Time> step,
             const StepOptions &options) {
  constexpr const char *operation = "natsci::fluids::advance";
  require_positive(density, operation, "density");
  require_nonnegative(viscosity, operation, "viscosity");
  require_positive(step, operation, "step");
  if (!std::isfinite(time.si_value()))
    throw std::invalid_argument("natsci::fluids::advance: time must be finite");

  StepLimits limits = step_limit_for(state, grid, viscosity, options.guard);
  const Series &allowed = options.rule == StepRule::Imex1
                              ? limits.convection.si_value()
                              : limits.allowed.si_value();
  if (options.guard.enforce &&
      !allowed.ge(step.si_value()).all().template item<bool>())
    throw std::domain_error(
        options.rule == StepRule::Imex1
            ? "natsci::fluids::advance: step exceeds the convective "
              "stability limit"
            : "natsci::fluids::advance: step exceeds a convective or viscous "
              "stability limit");
  return limits;
}

inline StepLimits prepare_step(
    const IncompressibleState &state, const spatial::PeriodicGrid &grid,
    const common::SeriesQuantity<Density> &density,
    const common::SeriesQuantity<DynamicViscosity> &viscosity,
    common::ScalarQuantity<common::Time> time,
    common::ScalarQuantity<common::Time> step, const StepOptions &options) {
  constexpr const char *operation = "natsci::fluids::advance";
  require_positive(step, operation, "step");
  if (!std::isfinite(time.si_value()))
    throw std::invalid_argument("natsci::fluids::advance: time must be finite");
  StepLimits limits =
      step_limit(state, grid, density, viscosity, options.guard);
  if (options.guard.enforce &&
      !limits.allowed.si_value().ge(step.si_value()).all().item<bool>())
    throw std::domain_error(
        "natsci::fluids::advance: step exceeds a convective or viscous "
        "stability limit");
  return limits;
}

inline StepLimits
prepare_step(const IncompressibleState &state, const spatial::BoxGrid &grid,
             const common::SeriesQuantity<Density> &density,
             const common::SeriesQuantity<DynamicViscosity> &viscosity,
             common::ScalarQuantity<common::Time> time,
             common::ScalarQuantity<common::Time> step,
             const StepOptions &options) {
  constexpr const char *operation = "natsci::fluids::advance";
  require_positive(step, operation, "step");
  if (!std::isfinite(time.si_value()))
    throw std::invalid_argument("natsci::fluids::advance: time must be finite");
  StepLimits limits =
      step_limit(state, grid, density, viscosity, options.guard);
  if (options.guard.enforce &&
      !limits.allowed.si_value().ge(step.si_value()).all().item<bool>())
    throw std::domain_error(
        "natsci::fluids::advance: step exceeds a convective or viscous "
        "stability limit");
  return limits;
}

} // namespace detail

[[nodiscard]] inline StepResult
advance(const IncompressibleState &state, const spatial::PeriodicGrid &grid,
        const common::ScalarQuantity<Density> &density,
        const common::ScalarQuantity<KinematicViscosity> &viscosity,
        common::ScalarQuantity<common::Time> time,
        common::ScalarQuantity<common::Time> step,
        const StepOptions &options = {}, const BodyFn &body = {}) {
  StepLimits limits = detail::prepare_step(state, grid, density, viscosity,
                                           time, step, options);

  // Stage projections skip the per-stage divergence reports and state
  // validation (device synchronizations); the finished step is validated and
  // reported once below.
  const auto project_stage = [&](const IncompressibleState &candidate) {
    return detail::project_core(
               candidate, grid, density, step,
               common::SeriesQuantity<common::Frequency>::from_si(
                   spatial::divergence(candidate.velocity().si_value(), grid)),
               options.projection.poisson)
        .state;
  };
  const auto rate = [&](const IncompressibleState &at,
                        common::ScalarQuantity<common::Time> when) {
    return detail::rate_at(at, grid, when, density, viscosity, body,
                           options.form);
  };
  const auto finalize = [&](const IncompressibleState &candidate) {
    auto checked = make_state(candidate.velocity(), candidate.pressure(), grid);
    return std::pair{checked, div_report(checked.velocity(), grid)};
  };
  if (options.rule == StepRule::Imex1) {
    const auto zero_viscosity =
        common::ScalarQuantity<KinematicViscosity>::from_si(0.0);
    const auto explicit_rate = [&](const IncompressibleState &at,
                                   common::ScalarQuantity<common::Time> when) {
      return detail::rate_at(at, grid, when, density, zero_viscosity, body,
                             options.form);
    };
    const auto diffuse = [&](const IncompressibleState &candidate) {
      return detail::diffuse_core(candidate, grid, viscosity, step,
                                  options.implicit);
    };
    return detail::advance_imex(state, grid, time, step, std::move(limits),
                                explicit_rate, diffuse, project_stage,
                                finalize);
  }
  return detail::advance_stages(state, grid, time, step, std::move(limits),
                                options, project_stage, rate, finalize);
}

[[nodiscard]] inline StepResult
advance(const IncompressibleState &state, const spatial::PeriodicGrid &grid,
        const common::SeriesQuantity<Density> &density,
        const common::SeriesQuantity<DynamicViscosity> &viscosity,
        common::ScalarQuantity<common::Time> time,
        common::ScalarQuantity<common::Time> step,
        const StepOptions &options = {}, const BodyFn &body = {}) {
  if (options.rule == StepRule::Imex1)
    throw std::invalid_argument(
        "natsci::fluids::advance: Imex1 currently requires scalar "
        "viscosity");
  StepLimits limits = detail::prepare_step(state, grid, density, viscosity,
                                           time, step, options);
  const auto project_stage = [&](const IncompressibleState &candidate) {
    return detail::project_core(
               candidate, grid, density, step,
               common::SeriesQuantity<common::Frequency>::from_si(
                   spatial::divergence(candidate.velocity().si_value(), grid)),
               options.projection.poisson)
        .state;
  };
  const auto rate = [&](const IncompressibleState &at,
                        common::ScalarQuantity<common::Time> when) {
    return detail::rate_at(at, grid, when, density, viscosity, body,
                           options.form);
  };
  const auto finalize = [&](const IncompressibleState &candidate) {
    auto checked = make_state(candidate.velocity(), candidate.pressure(), grid);
    return std::pair{checked, div_report(checked.velocity(), grid)};
  };
  return detail::advance_stages(state, grid, time, step, std::move(limits),
                                options, project_stage, rate, finalize);
}

[[nodiscard]] inline StepResult
advance(const IncompressibleState &state, const spatial::BoxGrid &grid,
        const FlowBounds &bounds,
        const common::ScalarQuantity<Density> &density,
        const common::ScalarQuantity<KinematicViscosity> &viscosity,
        common::ScalarQuantity<common::Time> time,
        common::ScalarQuantity<common::Time> step,
        const StepOptions &options = {}, const BodyFn &body = {}) {
  if (bounds.dim() != grid.dim())
    throw std::invalid_argument(
        "natsci::fluids::advance: boundary and grid rank mismatch");
  StepLimits limits = detail::prepare_step(state, grid, density, viscosity,
                                           time, step, options);

  const auto project_stage = [&](const IncompressibleState &candidate) {
    return detail::project_core(
               candidate, grid, bounds, density, step,
               common::SeriesQuantity<common::Frequency>::from_si(
                   detail::vel_divergence(candidate.velocity(), grid, bounds)),
               options.projection.poisson)
        .state;
  };
  const auto rate = [&](const IncompressibleState &at,
                        common::ScalarQuantity<common::Time> when) {
    return detail::rate_at(at, grid, bounds, when, density, viscosity, body,
                           options.form);
  };
  const auto finalize = [&](const IncompressibleState &candidate) {
    auto checked = make_state(candidate.velocity(), candidate.pressure(), grid);
    return std::pair{checked, div_report(checked.velocity(), grid, bounds)};
  };
  if (options.rule == StepRule::Imex1) {
    const auto zero_viscosity =
        common::ScalarQuantity<KinematicViscosity>::from_si(0.0);
    const auto explicit_rate = [&](const IncompressibleState &at,
                                   common::ScalarQuantity<common::Time> when) {
      return detail::rate_at(at, grid, bounds, when, density, zero_viscosity,
                             body, options.form);
    };
    const auto diffuse = [&](const IncompressibleState &candidate) {
      return detail::diffuse_core(candidate, grid, bounds, viscosity, step,
                                  options.implicit);
    };
    return detail::advance_imex(state, grid, time, step, std::move(limits),
                                explicit_rate, diffuse, project_stage,
                                finalize);
  }
  return detail::advance_stages(state, grid, time, step, std::move(limits),
                                options, project_stage, rate, finalize);
}

[[nodiscard]] inline StepResult
advance(const IncompressibleState &state, const spatial::BoxGrid &grid,
        const FlowBounds &bounds,
        const common::SeriesQuantity<Density> &density,
        const common::SeriesQuantity<DynamicViscosity> &viscosity,
        common::ScalarQuantity<common::Time> time,
        common::ScalarQuantity<common::Time> step,
        const StepOptions &options = {}, const BodyFn &body = {}) {
  if (bounds.dim() != grid.dim())
    throw std::invalid_argument(
        "natsci::fluids::advance: boundary and grid rank mismatch");
  if (options.rule == StepRule::Imex1)
    throw std::invalid_argument(
        "natsci::fluids::advance: Imex1 currently requires scalar "
        "viscosity");
  StepLimits limits = detail::prepare_step(state, grid, density, viscosity,
                                           time, step, options);

  const auto project_stage = [&](const IncompressibleState &candidate) {
    return detail::project_core(
               candidate, grid, bounds, density, step,
               common::SeriesQuantity<common::Frequency>::from_si(
                   detail::vel_divergence(candidate.velocity(), grid, bounds)),
               options.projection.poisson)
        .state;
  };
  const auto rate = [&](const IncompressibleState &at,
                        common::ScalarQuantity<common::Time> when) {
    return detail::rate_at(at, grid, bounds, when, density, viscosity, body,
                           options.form);
  };
  const auto finalize = [&](const IncompressibleState &candidate) {
    auto checked = make_state(candidate.velocity(), candidate.pressure(), grid);
    return std::pair{checked, div_report(checked.velocity(), grid, bounds)};
  };
  return detail::advance_stages(state, grid, time, step, std::move(limits),
                                options, project_stage, rate, finalize);
}

struct FlowPath final {
  common::Trajectory<common::Time> trajectory;
  std::int64_t steps{0};
  std::int64_t rate_evaluations{0};
  std::int64_t projections{0};
};

struct RunOptions final {
  double safety{0.9};
  common::ScalarQuantity<common::Time> min_step{
      common::ScalarQuantity<common::Time>::from_si(0.0)};
  common::ScalarQuantity<common::Time> max_step{
      common::ScalarQuantity<common::Time>::from_si(
          std::numeric_limits<double>::infinity())};
  std::int64_t max_steps{100'000};
  StepOptions step{};
};

namespace detail {

template <class Advance>
inline FlowPath evolve_steps(const IncompressibleState &initial,
                             common::ScalarQuantity<common::Time> start,
                             common::ScalarQuantity<common::Time> step,
                             std::int64_t steps, Advance &&advance_step) {
  std::int64_t evaluations = 0;
  std::int64_t projections = 0;
  auto marched = common::solve::march(
      [&](common::ScalarQuantity<common::Time> time,
          const common::StateVector &record,
          common::ScalarQuantity<common::Time> width) {
        auto result = advance_step(time, record, width);
        evaluations += result.rate_evaluations;
        projections += result.projections;
        return detail::record(result.state);
      },
      detail::record(initial), start, step, steps);
  return {std::move(marched.trajectory), marched.steps, evaluations,
          projections};
}

inline void validate_run(const RunOptions &options) {
  if (!(options.safety > 0.0) || !(options.safety <= 1.0) ||
      !std::isfinite(options.safety))
    throw std::invalid_argument(
        "natsci::fluids::evolve_to: safety must be finite and in (0, 1]");
  if (!(options.min_step.si_value() >= 0.0) ||
      !std::isfinite(options.min_step.si_value()) ||
      !(options.max_step.si_value() > 0.0) ||
      options.min_step.si_value() > options.max_step.si_value())
    throw std::invalid_argument(
        "natsci::fluids::evolve_to: step bounds are invalid");
  if (options.max_steps <= 0)
    throw std::invalid_argument(
        "natsci::fluids::evolve_to: max_steps must be positive");
}

template <class ReadState, class Limit, class Advance>
inline FlowPath evolve_to_steps(const IncompressibleState &initial,
                                common::ScalarQuantity<common::Time> start,
                                common::ScalarQuantity<common::Time> stop,
                                const RunOptions &options,
                                ReadState &&read_state, Limit &&limit_step,
                                Advance &&advance_step) {
  validate_run(options);
  std::int64_t evaluations = 0;
  std::int64_t projections = 0;
  auto marched = common::solve::march_to(
      [&](common::ScalarQuantity<common::Time> time,
          const common::StateVector &record,
          common::ScalarQuantity<common::Time> width) {
        auto result = advance_step(read_state(record), time, width);
        evaluations += result.rate_evaluations;
        projections += result.projections;
        return detail::record(result.state);
      },
      [&](common::ScalarQuantity<common::Time> time,
          const common::StateVector &record) {
        const auto state = read_state(record);
        const StepLimits limits = limit_step(state);
        const Series &allowed = options.step.rule == StepRule::Imex1
                                    ? limits.convection.si_value()
                                    : limits.allowed.si_value();
        const double stable = allowed.min().template item<double>();
        const double remaining = stop.si_value() - time.si_value();
        const double width =
            std::min(remaining, std::min(options.max_step.si_value(),
                                         options.safety * stable));
        if (!(width > 0.0) || !std::isfinite(width))
          throw std::domain_error(
              "natsci::fluids::evolve_to: stability policy produced no "
              "finite positive step");
        if (width < options.min_step.si_value() &&
            remaining > options.min_step.si_value())
          throw std::domain_error(
              "natsci::fluids::evolve_to: stable step is below min_step");
        return common::ScalarQuantity<common::Time>::from_si(width);
      },
      detail::record(initial), start, stop, options.max_steps);
  return {std::move(marched.trajectory), marched.steps, evaluations,
          projections};
}

} // namespace detail

[[nodiscard]] inline FlowPath
evolve(const IncompressibleState &initial, const spatial::PeriodicGrid &grid,
       const common::ScalarQuantity<Density> &density,
       const common::ScalarQuantity<KinematicViscosity> &viscosity,
       common::ScalarQuantity<common::Time> start,
       common::ScalarQuantity<common::Time> step, std::int64_t steps,
       const StepOptions &options = {}, const BodyFn &body = {}) {
  return detail::evolve_steps(initial, start, step, steps,
                              [&](common::ScalarQuantity<common::Time> time,
                                  const common::StateVector &record,
                                  common::ScalarQuantity<common::Time> width) {
                                return advance(detail::state(record, grid),
                                               grid, density, viscosity, time,
                                               width, options, body);
                              });
}

[[nodiscard]] inline FlowPath
evolve(const IncompressibleState &initial, const spatial::PeriodicGrid &grid,
       const common::SeriesQuantity<Density> &density,
       const common::SeriesQuantity<DynamicViscosity> &viscosity,
       common::ScalarQuantity<common::Time> start,
       common::ScalarQuantity<common::Time> step, std::int64_t steps,
       const StepOptions &options = {}, const BodyFn &body = {}) {
  return detail::evolve_steps(initial, start, step, steps,
                              [&](common::ScalarQuantity<common::Time> time,
                                  const common::StateVector &record,
                                  common::ScalarQuantity<common::Time> width) {
                                return advance(detail::state(record, grid),
                                               grid, density, viscosity, time,
                                               width, options, body);
                              });
}

[[nodiscard]] inline FlowPath
evolve(const IncompressibleState &initial, const spatial::BoxGrid &grid,
       const FlowBounds &bounds, const common::ScalarQuantity<Density> &density,
       const common::ScalarQuantity<KinematicViscosity> &viscosity,
       common::ScalarQuantity<common::Time> start,
       common::ScalarQuantity<common::Time> step, std::int64_t steps,
       const StepOptions &options = {}, const BodyFn &body = {}) {
  return detail::evolve_steps(initial, start, step, steps,
                              [&](common::ScalarQuantity<common::Time> time,
                                  const common::StateVector &record,
                                  common::ScalarQuantity<common::Time> width) {
                                return advance(detail::state(record, grid),
                                               grid, bounds, density, viscosity,
                                               time, width, options, body);
                              });
}

[[nodiscard]] inline FlowPath
evolve(const IncompressibleState &initial, const spatial::BoxGrid &grid,
       const FlowBounds &bounds, const common::SeriesQuantity<Density> &density,
       const common::SeriesQuantity<DynamicViscosity> &viscosity,
       common::ScalarQuantity<common::Time> start,
       common::ScalarQuantity<common::Time> step, std::int64_t steps,
       const StepOptions &options = {}, const BodyFn &body = {}) {
  return detail::evolve_steps(initial, start, step, steps,
                              [&](common::ScalarQuantity<common::Time> time,
                                  const common::StateVector &record,
                                  common::ScalarQuantity<common::Time> width) {
                                return advance(detail::state(record, grid),
                                               grid, bounds, density, viscosity,
                                               time, width, options, body);
                              });
}

[[nodiscard]] inline FlowPath
evolve_to(const IncompressibleState &initial, const spatial::PeriodicGrid &grid,
          const common::ScalarQuantity<Density> &density,
          const common::ScalarQuantity<KinematicViscosity> &viscosity,
          common::ScalarQuantity<common::Time> start,
          common::ScalarQuantity<common::Time> stop,
          const RunOptions &options = {}, const BodyFn &body = {}) {
  return detail::evolve_to_steps(
      initial, start, stop, options,
      [&](const common::StateVector &record) {
        return detail::state(record, grid);
      },
      [&](const IncompressibleState &state) {
        return step_limit(state, grid, viscosity, options.step.guard);
      },
      [&](const IncompressibleState &state,
          common::ScalarQuantity<common::Time> time,
          common::ScalarQuantity<common::Time> width) {
        return advance(state, grid, density, viscosity, time, width,
                       options.step, body);
      });
}

[[nodiscard]] inline FlowPath
evolve_to(const IncompressibleState &initial, const spatial::PeriodicGrid &grid,
          const common::SeriesQuantity<Density> &density,
          const common::SeriesQuantity<DynamicViscosity> &viscosity,
          common::ScalarQuantity<common::Time> start,
          common::ScalarQuantity<common::Time> stop,
          const RunOptions &options = {}, const BodyFn &body = {}) {
  return detail::evolve_to_steps(
      initial, start, stop, options,
      [&](const common::StateVector &record) {
        return detail::state(record, grid);
      },
      [&](const IncompressibleState &state) {
        return step_limit(state, grid, density, viscosity, options.step.guard);
      },
      [&](const IncompressibleState &state,
          common::ScalarQuantity<common::Time> time,
          common::ScalarQuantity<common::Time> width) {
        return advance(state, grid, density, viscosity, time, width,
                       options.step, body);
      });
}

[[nodiscard]] inline FlowPath
evolve_to(const IncompressibleState &initial, const spatial::BoxGrid &grid,
          const FlowBounds &bounds,
          const common::ScalarQuantity<Density> &density,
          const common::ScalarQuantity<KinematicViscosity> &viscosity,
          common::ScalarQuantity<common::Time> start,
          common::ScalarQuantity<common::Time> stop,
          const RunOptions &options = {}, const BodyFn &body = {}) {
  return detail::evolve_to_steps(
      initial, start, stop, options,
      [&](const common::StateVector &record) {
        return detail::state(record, grid);
      },
      [&](const IncompressibleState &state) {
        return step_limit(state, grid, viscosity, options.step.guard);
      },
      [&](const IncompressibleState &state,
          common::ScalarQuantity<common::Time> time,
          common::ScalarQuantity<common::Time> width) {
        return advance(state, grid, bounds, density, viscosity, time, width,
                       options.step, body);
      });
}

[[nodiscard]] inline FlowPath
evolve_to(const IncompressibleState &initial, const spatial::BoxGrid &grid,
          const FlowBounds &bounds,
          const common::SeriesQuantity<Density> &density,
          const common::SeriesQuantity<DynamicViscosity> &viscosity,
          common::ScalarQuantity<common::Time> start,
          common::ScalarQuantity<common::Time> stop,
          const RunOptions &options = {}, const BodyFn &body = {}) {
  return detail::evolve_to_steps(
      initial, start, stop, options,
      [&](const common::StateVector &record) {
        return detail::state(record, grid);
      },
      [&](const IncompressibleState &state) {
        return step_limit(state, grid, density, viscosity, options.step.guard);
      },
      [&](const IncompressibleState &state,
          common::ScalarQuantity<common::Time> time,
          common::ScalarQuantity<common::Time> width) {
        return advance(state, grid, bounds, density, viscosity, time, width,
                       options.step, body);
      });
}

struct SteadyOptions final {
  std::int64_t max_steps{10'000};
  std::int64_t check_interval{10};
  std::int64_t required_checks{2};
  common::ScalarQuantity<common::Acceleration> absolute_tolerance{
      common::ScalarQuantity<common::Acceleration>::from_si(1e-8)};
  double relative_tolerance{1e-6};
  bool require_convergence{true};
  StepOptions step{};
};

struct SteadyResult final {
  IncompressibleState state;
  common::SeriesQuantity<common::Acceleration> residual;
  Series converged;
  common::ScalarQuantity<common::Time> time;
  std::int64_t steps{0};
  std::int64_t rate_evaluations{0};
  std::int64_t projections{0};
};

namespace detail {

inline void validate_steady_options(const SteadyOptions &options) {
  if (options.max_steps <= 0)
    throw std::invalid_argument(
        "natsci::fluids::settle: max_steps must be positive");
  if (options.check_interval <= 0)
    throw std::invalid_argument(
        "natsci::fluids::settle: check_interval must be positive");
  if (options.required_checks <= 0)
    throw std::invalid_argument(
        "natsci::fluids::settle: required_checks must be positive");
  if (!(options.absolute_tolerance.si_value() >= 0.0) ||
      !std::isfinite(options.absolute_tolerance.si_value()) ||
      !(options.relative_tolerance >= 0.0) ||
      !std::isfinite(options.relative_tolerance))
    throw std::invalid_argument(
        "natsci::fluids::settle: tolerances must be finite and "
        "non-negative");
}

template <class Grid>
inline Series steady_residual(const IncompressibleState &before,
                              const IncompressibleState &after,
                              const Grid &grid,
                              common::ScalarQuantity<common::Time> step) {
  const Series &velocity = after.velocity().si_value();
  const std::int64_t component_axis = velocity.dim() - grid.dim() - 1;
  std::vector<std::int64_t> field_axes;
  field_axes.reserve(static_cast<std::size_t>(grid.dim() + 1));
  for (std::int64_t axis = component_axis; axis < velocity.dim(); ++axis)
    field_axes.push_back(axis);
  return velocity.sub(before.velocity().si_value())
      .abs()
      .amax(field_axes, false)
      .div(step.si_value());
}

template <class Grid, class Advance>
inline SteadyResult
settle_steps(const IncompressibleState &initial, const Grid &grid,
             common::ScalarQuantity<common::Time> start,
             common::ScalarQuantity<common::Time> step,
             const SteadyOptions &options, Advance &&advance_step) {
  validate_steady_options(options);
  if (!(step.si_value() > 0.0) || !std::isfinite(step.si_value()))
    throw std::invalid_argument(
        "natsci::fluids::settle: step must be finite and positive");
  if (!std::isfinite(start.si_value()))
    throw std::invalid_argument("natsci::fluids::settle: start must be finite");

  IncompressibleState current = initial;
  const Series &velocity = current.velocity().si_value();
  const auto batches = current.batch_shape(grid);
  Series residual =
      pluss::series::full(batches, std::numeric_limits<double>::infinity(),
                          velocity.scalar_type())
          .to(velocity.platform());
  Series initial_residual = pluss::series::undefined();
  Series streak = pluss::series::zeros(batches, velocity.scalar_type())
                      .to(velocity.platform());
  Series converged = streak.eq(-1.0);
  std::int64_t evaluations = 0;
  std::int64_t projections = 0;
  std::int64_t completed = 0;

  for (; completed < options.max_steps; ++completed) {
    const auto time = common::ScalarQuantity<common::Time>::from_si(
        start.si_value() + static_cast<double>(completed) * step.si_value());
    auto result = advance_step(current, time, step);
    evaluations += result.rate_evaluations;
    projections += result.projections;
    const bool inspect = (completed + 1) % options.check_interval == 0 ||
                         completed + 1 == options.max_steps;
    if (inspect) {
      residual = steady_residual(current, result.state, grid, step);
      if (!initial_residual.defined())
        initial_residual = residual;
      const Series tolerance = initial_residual.mul(options.relative_tolerance)
                                   .add(options.absolute_tolerance.si_value());
      const Series meets = residual.le(tolerance);
      streak = streak.add(1.0).mul(meets.to(streak.scalar_type()));
      converged = streak.ge(static_cast<double>(options.required_checks));
    }
    current = std::move(result.state);
    if (inspect && converged.all().item<bool>()) {
      ++completed;
      break;
    }
  }

  if (options.require_convergence && !converged.all().item<bool>())
    throw std::runtime_error(
        "natsci::fluids::settle: steady residual did not converge within "
        "max_steps");
  return {
      std::move(current),
      common::SeriesQuantity<common::Acceleration>::from_si(
          std::move(residual)),
      std::move(converged),
      common::ScalarQuantity<common::Time>::from_si(
          start.si_value() + static_cast<double>(completed) * step.si_value()),
      completed,
      evaluations,
      projections};
}

} // namespace detail

[[nodiscard]] inline SteadyResult
settle(const IncompressibleState &initial, const spatial::PeriodicGrid &grid,
       const common::ScalarQuantity<Density> &density,
       const common::ScalarQuantity<KinematicViscosity> &viscosity,
       common::ScalarQuantity<common::Time> start,
       common::ScalarQuantity<common::Time> step,
       const SteadyOptions &options = {}, const BodyFn &body = {}) {
  const auto runner = [&](const IncompressibleState &state,
                          common::ScalarQuantity<common::Time> time,
                          common::ScalarQuantity<common::Time> width) {
    return advance(state, grid, density, viscosity, time, width, options.step,
                   body);
  };
  return detail::settle_steps(initial, grid, start, step, options, runner);
}

[[nodiscard]] inline SteadyResult
settle(const IncompressibleState &initial, const spatial::PeriodicGrid &grid,
       const common::SeriesQuantity<Density> &density,
       const common::SeriesQuantity<DynamicViscosity> &viscosity,
       common::ScalarQuantity<common::Time> start,
       common::ScalarQuantity<common::Time> step,
       const SteadyOptions &options = {}, const BodyFn &body = {}) {
  const auto runner = [&](const IncompressibleState &state,
                          common::ScalarQuantity<common::Time> time,
                          common::ScalarQuantity<common::Time> width) {
    return advance(state, grid, density, viscosity, time, width, options.step,
                   body);
  };
  return detail::settle_steps(initial, grid, start, step, options, runner);
}

[[nodiscard]] inline SteadyResult
settle(const IncompressibleState &initial, const spatial::BoxGrid &grid,
       const FlowBounds &bounds, const common::ScalarQuantity<Density> &density,
       const common::ScalarQuantity<KinematicViscosity> &viscosity,
       common::ScalarQuantity<common::Time> start,
       common::ScalarQuantity<common::Time> step,
       const SteadyOptions &options = {}, const BodyFn &body = {}) {
  const auto runner = [&](const IncompressibleState &state,
                          common::ScalarQuantity<common::Time> time,
                          common::ScalarQuantity<common::Time> width) {
    return advance(state, grid, bounds, density, viscosity, time, width,
                   options.step, body);
  };
  return detail::settle_steps(initial, grid, start, step, options, runner);
}

[[nodiscard]] inline SteadyResult
settle(const IncompressibleState &initial, const spatial::BoxGrid &grid,
       const FlowBounds &bounds, const common::SeriesQuantity<Density> &density,
       const common::SeriesQuantity<DynamicViscosity> &viscosity,
       common::ScalarQuantity<common::Time> start,
       common::ScalarQuantity<common::Time> step,
       const SteadyOptions &options = {}, const BodyFn &body = {}) {
  const auto runner = [&](const IncompressibleState &state,
                          common::ScalarQuantity<common::Time> time,
                          common::ScalarQuantity<common::Time> width) {
    return advance(state, grid, bounds, density, viscosity, time, width,
                   options.step, body);
  };
  return detail::settle_steps(initial, grid, start, step, options, runner);
}

} // namespace pluss::subsp::natsci::physics::fluids
