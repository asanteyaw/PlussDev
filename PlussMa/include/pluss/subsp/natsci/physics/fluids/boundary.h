#pragma once

#include "pluss/subsp/natsci/common/boundary.h"

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::physics::fluids {

namespace boundary_common = ::pluss::subsp::natsci::common;
namespace boundary_spatial = ::pluss::subsp::natsci::common::spatial;

enum class FlowKind { NoSlip, FreeSlip, Inlet, Outlet };

class FlowBound final {
public:
  [[nodiscard]] boundary_spatial::Face face() const noexcept { return face_; }
  [[nodiscard]] FlowKind kind() const noexcept { return kind_; }
  [[nodiscard]] const std::optional<
      boundary_common::SeriesQuantity<boundary_common::Speed>> &
  velocity() const noexcept {
    return velocity_;
  }
  [[nodiscard]] const std::optional<
      boundary_spatial::Bound<boundary_common::Pressure>> &
  pressure() const noexcept {
    return pressure_;
  }

  [[nodiscard]] static FlowBound no_slip(boundary_spatial::Face face) {
    return FlowBound(face, FlowKind::NoSlip, std::nullopt, std::nullopt);
  }

  [[nodiscard]] static FlowBound no_slip(
      boundary_spatial::Face face,
      const boundary_common::SeriesQuantity<boundary_common::Speed> &velocity) {
    return FlowBound(face, FlowKind::NoSlip, velocity, std::nullopt);
  }

  [[nodiscard]] static FlowBound free_slip(boundary_spatial::Face face) {
    return FlowBound(face, FlowKind::FreeSlip, std::nullopt, std::nullopt);
  }

  [[nodiscard]] static FlowBound free_slip(
      boundary_spatial::Face face,
      const boundary_common::SeriesQuantity<boundary_common::Speed> &velocity) {
    return FlowBound(face, FlowKind::FreeSlip, velocity, std::nullopt);
  }

  [[nodiscard]] static FlowBound inlet(
      boundary_spatial::Face face,
      const boundary_common::SeriesQuantity<boundary_common::Speed> &velocity) {
    return FlowBound(face, FlowKind::Inlet, velocity, std::nullopt);
  }

  [[nodiscard]] static FlowBound outlet(boundary_spatial::Face face) {
    return FlowBound(face, FlowKind::Outlet, std::nullopt, std::nullopt);
  }

  template <boundary_common::QuantityRep Rep>
  [[nodiscard]] static FlowBound
  outlet(boundary_spatial::Face face,
         const boundary_common::Quantity<boundary_common::Pressure, Rep>
             &pressure) {
    return FlowBound(face, FlowKind::Outlet, std::nullopt,
                     boundary_spatial::Bound<boundary_common::Pressure>::value(
                         face, pressure));
  }

  // The linearized boundary rule used for increments and pressure
  // corrections.  Preserve the physical kind, but remove prescribed data.
  [[nodiscard]] FlowBound zeroed() const {
    return FlowBound(face_, kind_, std::nullopt, std::nullopt);
  }

private:
  FlowBound(
      boundary_spatial::Face face, FlowKind kind,
      std::optional<boundary_common::SeriesQuantity<boundary_common::Speed>>
          velocity,
      std::optional<boundary_spatial::Bound<boundary_common::Pressure>>
          pressure)
      : face_(face), kind_(kind), velocity_(std::move(velocity)),
        pressure_(std::move(pressure)) {}

  boundary_spatial::Face face_;
  FlowKind kind_;
  std::optional<boundary_common::SeriesQuantity<boundary_common::Speed>>
      velocity_;
  std::optional<boundary_spatial::Bound<boundary_common::Pressure>> pressure_;
};

[[nodiscard]] inline FlowBound no_slip(boundary_spatial::Face face) {
  return FlowBound::no_slip(face);
}

[[nodiscard]] inline FlowBound no_slip(
    boundary_spatial::Face face,
    const boundary_common::SeriesQuantity<boundary_common::Speed> &velocity) {
  return FlowBound::no_slip(face, velocity);
}

[[nodiscard]] inline FlowBound free_slip(boundary_spatial::Face face) {
  return FlowBound::free_slip(face);
}

[[nodiscard]] inline FlowBound free_slip(
    boundary_spatial::Face face,
    const boundary_common::SeriesQuantity<boundary_common::Speed> &velocity) {
  return FlowBound::free_slip(face, velocity);
}

[[nodiscard]] inline FlowBound
inlet(boundary_spatial::Face face,
      const boundary_common::SeriesQuantity<boundary_common::Speed> &velocity) {
  return FlowBound::inlet(face, velocity);
}

[[nodiscard]] inline FlowBound outlet(boundary_spatial::Face face) {
  return FlowBound::outlet(face);
}

template <boundary_common::QuantityRep Rep>
[[nodiscard]] inline FlowBound outlet(
    boundary_spatial::Face face,
    const boundary_common::Quantity<boundary_common::Pressure, Rep> &pressure) {
  return FlowBound::outlet(face, pressure);
}

class FlowBounds final {
public:
  [[nodiscard]] const FlowBound &at(std::int64_t axis,
                                    boundary_spatial::Side side) const {
    if (axis < 0 || axis >= dim_)
      throw std::out_of_range(
          "natsci::fluids::FlowBounds::at: face axis is out of range");
    return rules_[boundary_spatial::detail::face_slot({axis, side})];
  }

  [[nodiscard]] std::int64_t dim() const noexcept { return dim_; }

  [[nodiscard]] FlowBounds zeroed() const {
    std::vector<FlowBound> rules;
    rules.reserve(rules_.size());
    for (const auto &rule : rules_)
      rules.push_back(rule.zeroed());
    return FlowBounds(dim_, std::move(rules));
  }

private:
  FlowBounds(std::int64_t dim, std::vector<FlowBound> rules)
      : dim_(dim), rules_(std::move(rules)) {}

  std::int64_t dim_{};
  std::vector<FlowBound> rules_;

  friend FlowBounds flow_bounds(const boundary_spatial::BoxGrid &,
                                std::vector<FlowBound>);
};

[[nodiscard]] inline FlowBounds
flow_bounds(const boundary_spatial::BoxGrid &grid,
            std::vector<FlowBound> rules) {
  constexpr const char *operation = "natsci::fluids::flow_bounds";
  auto ordered = boundary_spatial::detail::order_faces(
      grid, rules, [](const FlowBound &rule) { return rule.face(); },
      operation);
  return FlowBounds(grid.dim(), std::move(ordered));
}

[[nodiscard]] inline FlowBounds
flow_bounds(const boundary_spatial::BoxGrid &grid,
            std::initializer_list<FlowBound> rules) {
  return flow_bounds(grid, std::vector<FlowBound>(rules));
}

struct FlowGhosts final {
  boundary_common::SeriesQuantity<boundary_common::Speed> low;
  boundary_common::SeriesQuantity<boundary_common::Speed> high;
};

namespace detail {

inline Series zero_like(const Series &value) {
  return value.mul(pluss::codem::make_scalar_like(value, 0.0));
}

inline Series target_plane(const FlowBound &rule, const Series &velocity,
                           const boundary_spatial::BoxGrid &grid,
                           std::int64_t spatial_axis,
                           std::int64_t component_axis,
                           boundary_spatial::Side side) {
  const std::int64_t series_axis = velocity.dim() - grid.dim() + spatial_axis;
  const std::int64_t index =
      side == boundary_spatial::Side::Low ? 0 : velocity.size(series_axis) - 1;
  const Series plane = velocity.select(series_axis, index);
  if (!rule.velocity())
    return zero_like(plane);

  Series profile = rule.velocity()->si_value();
  if (profile.scalar_type() != plane.scalar_type())
    throw std::invalid_argument(
        "natsci::fluids::vel_ghosts: velocity profile dtype mismatch");
  if (profile.platform() != plane.platform())
    throw std::invalid_argument(
        "natsci::fluids::vel_ghosts: velocity profile platform mismatch");

  if (profile.dim() == 1 && profile.size(0) == grid.dim()) {
    std::vector<std::int64_t> shape(static_cast<std::size_t>(plane.dim()), 1);
    shape[static_cast<std::size_t>(component_axis)] = grid.dim();
    profile = profile.reshape(shape);
  }

  Series expanded;
  try {
    expanded = zero_like(plane).add(profile);
  } catch (const std::exception &) {
    throw std::invalid_argument(
        "natsci::fluids::vel_ghosts: velocity profile is not broadcastable "
        "to the face plane");
  }
  if (!boundary_spatial::detail::same_shape(expanded, plane))
    throw std::invalid_argument(
        "natsci::fluids::vel_ghosts: velocity profile changed the face shape");
  return expanded;
}

} // namespace detail

[[nodiscard]] inline FlowGhosts vel_ghosts(
    const boundary_common::SeriesQuantity<boundary_common::Speed> &velocity,
    const boundary_spatial::BoxGrid &grid, const FlowBounds &bounds,
    std::int64_t axis) {
  constexpr const char *operation = "natsci::fluids::vel_ghosts";
  const Series &field = velocity.si_value();
  const std::int64_t component_axis =
      boundary_spatial::detail::require_vector_layout(field, grid, operation);
  boundary_spatial::detail::require_axis(grid, axis, operation);
  if (bounds.dim() != grid.dim())
    throw std::invalid_argument(
        "natsci::fluids::vel_ghosts: boundary and grid rank mismatch");

  std::vector<std::optional<Series>> targets(
      static_cast<std::size_t>(2 * grid.dim()));
  for (std::int64_t face_axis = 0; face_axis < grid.dim(); ++face_axis) {
    for (const auto side :
         {boundary_spatial::Side::Low, boundary_spatial::Side::High}) {
      const auto &rule = bounds.at(face_axis, side);
      if (rule.velocity())
        targets[boundary_spatial::detail::face_slot({face_axis, side})] =
            detail::target_plane(rule, field, grid, face_axis, component_axis,
                                 side);
    }
  }

  std::vector<Series> low_components;
  std::vector<Series> high_components;
  low_components.reserve(static_cast<std::size_t>(grid.dim()));
  high_components.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t component = 0; component < grid.dim(); ++component) {
    std::vector<boundary_spatial::Bound<boundary_common::Speed>> rules;
    rules.reserve(static_cast<std::size_t>(2 * grid.dim()));
    for (std::int64_t face_axis = 0; face_axis < grid.dim(); ++face_axis) {
      for (const auto side :
           {boundary_spatial::Side::Low, boundary_spatial::Side::High}) {
        const auto &rule = bounds.at(face_axis, side);
        const auto face = boundary_spatial::Face{face_axis, side};
        const bool zero_flux =
            rule.kind() == FlowKind::Outlet ||
            (rule.kind() == FlowKind::FreeSlip && component != face_axis);
        if (zero_flux) {
          using Flux =
              boundary_common::DimensionQuotient<boundary_common::Speed,
                                                 boundary_common::Length>;
          rules.push_back(boundary_spatial::flux_bc(
              face, boundary_common::ScalarQuantity<Flux>::from_si(0.0)));
          continue;
        }

        const auto &target = targets[boundary_spatial::detail::face_slot(face)];
        if (target) {
          rules.push_back(boundary_spatial::value_bc(
              face,
              boundary_common::SeriesQuantity<boundary_common::Speed>::from_si(
                  target->select(component_axis, component))));
        } else {
          rules.push_back(boundary_spatial::value_bc(
              face,
              boundary_common::ScalarQuantity<boundary_common::Speed>::from_si(
                  0.0)));
        }
      }
    }

    const auto component_field =
        boundary_common::SeriesQuantity<boundary_common::Speed>::from_si(
            field.select(component_axis, component));
    const auto component_ghosts = boundary_spatial::ghosts(
        component_field, grid, boundary_spatial::make_bounds(grid, rules),
        axis);
    low_components.push_back(component_ghosts.low.si_value());
    high_components.push_back(component_ghosts.high.si_value());
  }

  return {boundary_common::SeriesQuantity<boundary_common::Speed>::from_si(
              boundary_spatial::detail::stack_components(low_components,
                                                         component_axis)),
          boundary_common::SeriesQuantity<boundary_common::Speed>::from_si(
              boundary_spatial::detail::stack_components(high_components,
                                                         component_axis))};
}

[[nodiscard]] inline boundary_spatial::Bounds<boundary_common::Pressure>
p_bounds(const boundary_spatial::BoxGrid &grid, const FlowBounds &bounds) {
  if (bounds.dim() != grid.dim())
    throw std::invalid_argument(
        "natsci::fluids::p_bounds: boundary and grid rank mismatch");

  using PressureFlux =
      boundary_common::DimensionQuotient<boundary_common::Pressure,
                                         boundary_common::Length>;
  std::vector<boundary_spatial::Bound<boundary_common::Pressure>> rules;
  rules.reserve(static_cast<std::size_t>(2 * grid.dim()));
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    for (const auto side :
         {boundary_spatial::Side::Low, boundary_spatial::Side::High}) {
      const auto &flow = bounds.at(axis, side);
      if (flow.kind() == FlowKind::Outlet) {
        if (flow.pressure())
          rules.push_back(*flow.pressure());
        else
          rules.push_back(boundary_spatial::value_bc(
              flow.face(), boundary_common::ScalarQuantity<
                               boundary_common::Pressure>::from_si(0.0)));
      } else {
        rules.push_back(boundary_spatial::flux_bc(
            flow.face(),
            boundary_common::ScalarQuantity<PressureFlux>::from_si(0.0)));
      }
    }
  }
  return boundary_spatial::make_bounds(grid, std::move(rules));
}

} // namespace pluss::subsp::natsci::physics::fluids
