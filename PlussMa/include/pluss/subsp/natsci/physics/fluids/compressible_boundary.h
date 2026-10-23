#pragma once

#include "pluss/subsp/natsci/physics/fluids/compressible.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::physics::fluids::gas {

enum class BoundKind : std::uint8_t {
  Slip = 0,
  Fixed = 1,
  Outlet = 2,
  Far = 3,
  Wall = 4,
  Inlet = 5,
  Blend = 6,
};

enum class WallHeat : std::uint8_t {
  Adiabatic = 0,
  Temperature = 1,
  Flux = 2,
};

struct Primitive final {
  common::SeriesQuantity<Density> density;
  common::SeriesQuantity<common::Speed> velocity;
  common::SeriesQuantity<common::Pressure> pressure;
};

struct WallData final {
  WallHeat heat{WallHeat::Adiabatic};
  std::optional<common::SeriesQuantity<common::Speed>> velocity;
  std::optional<common::SeriesQuantity<common::Temperature>> temperature;
  std::optional<common::SeriesQuantity<HeatFlux>> flux;
};

enum class InletKind : std::uint8_t { Total = 0, Flow = 1 };

struct InletData final {
  InletKind kind{InletKind::Total};
  Series direction;
  std::optional<common::SeriesQuantity<common::Temperature>> total_temperature;
  std::optional<common::SeriesQuantity<common::Pressure>> total_pressure;
  std::optional<common::SeriesQuantity<Density>> density;
  std::optional<common::SeriesQuantity<common::Speed>> speed;
};

[[nodiscard]] inline Primitive
primitive(const common::SeriesQuantity<Density> &density,
          const common::SeriesQuantity<common::Speed> &velocity,
          const common::SeriesQuantity<common::Pressure> &pressure) {
  return {density, velocity, pressure};
}

class Bound final {
public:
  [[nodiscard]] spatial::Face face() const noexcept { return face_; }
  [[nodiscard]] BoundKind kind() const noexcept { return kind_; }
  [[nodiscard]] const std::optional<Primitive> &state() const noexcept {
    return state_;
  }
  [[nodiscard]] const std::optional<common::SeriesQuantity<common::Pressure>> &
  pressure() const noexcept {
    return pressure_;
  }
  [[nodiscard]] const std::optional<WallData> &wall() const noexcept {
    return wall_;
  }
  [[nodiscard]] const std::optional<InletData> &inlet() const noexcept {
    return inlet_;
  }
  [[nodiscard]] const std::optional<Series> &mask() const noexcept {
    return mask_;
  }
  [[nodiscard]] const std::shared_ptr<const Bound> &when_true() const noexcept {
    return when_true_;
  }
  [[nodiscard]] const std::shared_ptr<const Bound> &when_false() const noexcept {
    return when_false_;
  }

  [[nodiscard]] static Bound slip(spatial::Face face) {
    return Bound(face, BoundKind::Slip, std::nullopt, std::nullopt,
                 std::nullopt, std::nullopt);
  }
  [[nodiscard]] static Bound fixed(spatial::Face face, Primitive state) {
    return Bound(face, BoundKind::Fixed, std::move(state), std::nullopt,
                 std::nullopt, std::nullopt);
  }
  [[nodiscard]] static Bound far(spatial::Face face, Primitive state) {
    return Bound(face, BoundKind::Far, std::move(state), std::nullopt,
                 std::nullopt, std::nullopt);
  }
  [[nodiscard]] static Bound outlet(spatial::Face face) {
    return Bound(face, BoundKind::Outlet, std::nullopt, std::nullopt,
                 std::nullopt, std::nullopt);
  }
  [[nodiscard]] static Bound
  outlet(spatial::Face face,
         common::SeriesQuantity<common::Pressure> pressure) {
    return Bound(face, BoundKind::Outlet, std::nullopt, std::move(pressure),
                 std::nullopt, std::nullopt);
  }
  [[nodiscard]] static Bound wall(spatial::Face face, WallData data = {}) {
    return Bound(face, BoundKind::Wall, std::nullopt, std::nullopt,
                 std::move(data), std::nullopt);
  }
  [[nodiscard]] static Bound inlet(spatial::Face face, InletData data) {
    return Bound(face, BoundKind::Inlet, std::nullopt, std::nullopt,
                 std::nullopt, std::move(data));
  }
  [[nodiscard]] static Bound blend(Series mask, Bound when_true,
                                   Bound when_false) {
    const spatial::Face face = when_true.face();
    if (face.axis != when_false.face().axis ||
        face.side != when_false.face().side)
      throw std::invalid_argument(
          "natsci::fluids::gas::blend: both rules must target the same face");
    if (mask.scalar_type() != ScalarType::Bool)
      throw std::invalid_argument(
          "natsci::fluids::gas::blend: mask must have Bool dtype");
    Bound result(face, BoundKind::Blend, std::nullopt, std::nullopt,
                 std::nullopt, std::nullopt);
    result.mask_ = std::move(mask);
    result.when_true_ =
        std::make_shared<const Bound>(std::move(when_true));
    result.when_false_ =
        std::make_shared<const Bound>(std::move(when_false));
    return result;
  }

private:
  Bound(spatial::Face face, BoundKind kind, std::optional<Primitive> state,
        std::optional<common::SeriesQuantity<common::Pressure>> pressure,
        std::optional<WallData> wall, std::optional<InletData> inlet)
      : face_(face), kind_(kind), state_(std::move(state)),
        pressure_(std::move(pressure)), wall_(std::move(wall)),
        inlet_(std::move(inlet)) {}

  spatial::Face face_{};
  BoundKind kind_{};
  std::optional<Primitive> state_;
  std::optional<common::SeriesQuantity<common::Pressure>> pressure_;
  std::optional<WallData> wall_;
  std::optional<InletData> inlet_;
  std::optional<Series> mask_;
  std::shared_ptr<const Bound> when_true_;
  std::shared_ptr<const Bound> when_false_;
};

[[nodiscard]] inline Bound slip(spatial::Face face) {
  return Bound::slip(face);
}
[[nodiscard]] inline Bound fixed(spatial::Face face, Primitive state) {
  return Bound::fixed(face, std::move(state));
}
[[nodiscard]] inline Bound far(spatial::Face face, Primitive state) {
  return Bound::far(face, std::move(state));
}
[[nodiscard]] inline Bound outlet(spatial::Face face) {
  return Bound::outlet(face);
}
[[nodiscard]] inline Bound
outlet(spatial::Face face, common::SeriesQuantity<common::Pressure> pressure) {
  return Bound::outlet(face, std::move(pressure));
}
[[nodiscard]] inline Bound wall(spatial::Face face) {
  return Bound::wall(face);
}
[[nodiscard]] inline Bound
wall(spatial::Face face, common::SeriesQuantity<common::Speed> velocity) {
  WallData data;
  data.velocity = std::move(velocity);
  return Bound::wall(face, std::move(data));
}
[[nodiscard]] inline Bound
wall_temp(spatial::Face face,
          common::SeriesQuantity<common::Temperature> temperature) {
  WallData data;
  data.heat = WallHeat::Temperature;
  data.temperature = std::move(temperature);
  return Bound::wall(face, std::move(data));
}
[[nodiscard]] inline Bound
wall_temp(spatial::Face face,
          common::SeriesQuantity<common::Temperature> temperature,
          common::SeriesQuantity<common::Speed> velocity) {
  WallData data;
  data.heat = WallHeat::Temperature;
  data.velocity = std::move(velocity);
  data.temperature = std::move(temperature);
  return Bound::wall(face, std::move(data));
}
[[nodiscard]] inline Bound wall_heat(spatial::Face face,
                                     common::SeriesQuantity<HeatFlux> flux) {
  WallData data;
  data.heat = WallHeat::Flux;
  data.flux = std::move(flux);
  return Bound::wall(face, std::move(data));
}
[[nodiscard]] inline Bound
wall_heat(spatial::Face face, common::SeriesQuantity<HeatFlux> flux,
          common::SeriesQuantity<common::Speed> velocity) {
  WallData data;
  data.heat = WallHeat::Flux;
  data.velocity = std::move(velocity);
  data.flux = std::move(flux);
  return Bound::wall(face, std::move(data));
}
[[nodiscard]] inline Bound
inlet(spatial::Face face,
      common::SeriesQuantity<common::Temperature> total_temperature,
      common::SeriesQuantity<common::Pressure> total_pressure,
      Series direction) {
  InletData data;
  data.kind = InletKind::Total;
  data.direction = std::move(direction);
  data.total_temperature = std::move(total_temperature);
  data.total_pressure = std::move(total_pressure);
  return Bound::inlet(face, std::move(data));
}
[[nodiscard]] inline Bound inlet(spatial::Face face,
                                 common::SeriesQuantity<Density> density,
                                 common::SeriesQuantity<common::Speed> speed,
                                 Series direction) {
  InletData data;
  data.kind = InletKind::Flow;
  data.direction = std::move(direction);
  data.density = std::move(density);
  data.speed = std::move(speed);
  return Bound::inlet(face, std::move(data));
}

[[nodiscard]] inline Bound blend(Series mask, Bound when_true,
                                 Bound when_false) {
  return Bound::blend(std::move(mask), std::move(when_true),
                      std::move(when_false));
}

class Bounds final {
public:
  [[nodiscard]] const Bound &at(std::int64_t axis, spatial::Side side) const {
    if (axis < 0 || axis >= dim_)
      throw std::out_of_range(
          "natsci::fluids::gas::Bounds::at: face axis is out of range");
    return rules_[spatial::detail::face_slot({axis, side})];
  }
  [[nodiscard]] std::int64_t dim() const noexcept { return dim_; }

private:
  Bounds(std::int64_t dim, std::vector<Bound> rules)
      : dim_(dim), rules_(std::move(rules)) {}

  std::int64_t dim_{};
  std::vector<Bound> rules_;

  friend Bounds bounds(const spatial::BoxGrid &, std::vector<Bound>);
};

using BoundFn =
    std::function<Bounds(common::ScalarQuantity<common::Time>)>;

[[nodiscard]] inline Bounds bounds(const spatial::BoxGrid &grid,
                                   std::vector<Bound> rules) {
  auto ordered = spatial::detail::order_faces(
      grid, rules, [](const Bound &rule) { return rule.face(); },
      "natsci::fluids::gas::bounds");
  return Bounds(grid.dim(), std::move(ordered));
}

[[nodiscard]] inline Bounds bounds(const spatial::BoxGrid &grid,
                                   std::initializer_list<Bound> rules) {
  return bounds(grid, std::vector<Bound>(rules));
}

namespace boundary_detail {

inline void validate_options(const Options &options, const char *operation) {
  detail::validate_options(options, operation);
}

inline Series expand(const Series &value, const Series &prototype,
                     std::int64_t vector_axis, std::int64_t components,
                     const char *operation, const char *role) {
  if (value.scalar_type() != prototype.scalar_type() ||
      value.platform() != prototype.platform())
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " must match the state dtype and platform");
  Series shaped = value;
  if (components > 0 && value.dim() == 1 && value.size(0) == components) {
    std::vector<std::int64_t> shape(static_cast<std::size_t>(prototype.dim()),
                                    1);
    shape[static_cast<std::size_t>(vector_axis)] = components;
    shaped = value.reshape(shape);
  }
  try {
    Series result = pluss::series::zeros_like(prototype).add(shaped);
    if (!detail::same_layout(result, prototype))
      throw std::invalid_argument("shape changed");
    return result;
  } catch (const std::exception &) {
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " is not broadcastable to the boundary face");
  }
}

inline Series expand_mask(const Series &value, const Series &prototype,
                          const char *operation) {
  if (value.scalar_type() != ScalarType::Bool ||
      value.platform() != prototype.platform())
    throw std::invalid_argument(std::string(operation) +
                                ": boundary mask must have Bool dtype and "
                                "match the state platform");
  try {
    const Series bool_prototype =
        pluss::series::zeros_like(prototype).eq(0.0);
    Series result = bool_prototype.logical_and(value);
    if (!detail::same_layout(result, bool_prototype))
      throw std::invalid_argument("shape changed");
    return result;
  } catch (const std::exception &) {
    throw std::invalid_argument(std::string(operation) +
                                ": boundary mask is not broadcastable to "
                                "the boundary face");
  }
}

inline detail::Raw plane(const detail::Raw &state, const spatial::BoxGrid &grid,
                         std::int64_t axis, spatial::Side side) {
  const std::int64_t scalar_axis = state.rho.dim() - grid.dim() + axis;
  const std::int64_t vector_axis = state.mom.dim() - grid.dim() + axis;
  const std::int64_t scalar_index =
      side == spatial::Side::Low ? 0 : state.rho.size(scalar_axis) - 1;
  const std::int64_t vector_index =
      side == spatial::Side::Low ? 0 : state.mom.size(vector_axis) - 1;
  return {state.rho.select(scalar_axis, scalar_index),
          state.mom.select(vector_axis, vector_index),
          state.energy.select(scalar_axis, scalar_index)};
}

inline detail::Raw plane_at(const detail::Raw &state,
                            const spatial::BoxGrid &grid, std::int64_t axis,
                            std::int64_t index) {
  const std::int64_t scalar_axis = state.rho.dim() - grid.dim() + axis;
  const std::int64_t vector_axis = state.mom.dim() - grid.dim() + axis;
  return {state.rho.select(scalar_axis, index),
          state.mom.select(vector_axis, index),
          state.energy.select(scalar_axis, index)};
}

inline Series plane_pressure(const detail::Raw &state, std::int64_t component,
                             const Model &model) {
  return detail::face_pressure(state, component, model);
}

inline detail::Raw from_primitive(const Primitive &source,
                                  const detail::Raw &prototype,
                                  std::int64_t component, std::int64_t dim,
                                  const Model &model, const char *operation) {
  const Series rho = expand(source.density.si_value(), prototype.rho, 0, 0,
                            operation, "boundary density");
  const Series velocity =
      expand(source.velocity.si_value(), prototype.mom, component, dim,
             operation, "boundary velocity");
  const Series pressure = expand(source.pressure.si_value(), prototype.rho, 0,
                                 0, operation, "boundary pressure");
  const Series momentum = velocity.mul(rho.unsqueeze(component));
  const Series kinetic =
      velocity.square().sum(component, false).mul(rho).mul(0.5);
  detail::Raw result{rho, momentum,
                     pressure.div(model.gamma - 1.0).add(kinetic)};
  const Series valid =
      result.rho.gt(model.rho_floor)
          .logical_and(
              plane_pressure(result, component, model).gt(model.p_floor))
          .logical_and(result.rho.isfinite())
          .logical_and(result.mom.isfinite().all(component, false))
          .logical_and(result.energy.isfinite());
  if (!valid.all().item<bool>())
    throw std::domain_error(std::string(operation) +
                            ": prescribed boundary state must have finite "
                            "positive density and pressure");
  return result;
}

inline detail::Raw choose(const Series &mask, const detail::Raw &yes,
                          const detail::Raw &no, std::int64_t component) {
  return {mask.where(yes.rho, no.rho),
          mask.unsqueeze(component).where(yes.mom, no.mom),
          mask.where(yes.energy, no.energy)};
}

inline detail::Raw characteristic_far(const detail::Raw &inside,
                                      const detail::Raw &ambient,
                                      const Bound &rule, const Model &model,
                                      std::int64_t component) {
  const double sign = rule.face().side == spatial::Side::Low ? -1.0 : 1.0;
  const std::int64_t axis = rule.face().axis;
  const Series inside_pressure = plane_pressure(inside, component, model);
  const Series ambient_pressure = plane_pressure(ambient, component, model);
  const Series inside_velocity =
      inside.mom.div(inside.rho.unsqueeze(component));
  const Series ambient_velocity =
      ambient.mom.div(ambient.rho.unsqueeze(component));
  const Series inside_normal =
      inside_velocity.select(component, axis).mul(sign);
  const Series ambient_normal =
      ambient_velocity.select(component, axis).mul(sign);
  const Series inside_sound =
      inside_pressure.mul(model.gamma).div(inside.rho).sqrt();
  const Series ambient_sound =
      ambient_pressure.mul(model.gamma).div(ambient.rho).sqrt();

  const Series supersonic_out = inside_normal.ge(inside_sound);
  const Series supersonic_in = inside_normal.le(inside_sound.neg());
  const Series subsonic =
      supersonic_out.logical_or(supersonic_in).logical_not();
  const Series outgoing =
      inside_normal.add(inside_sound.mul(2.0 / (model.gamma - 1.0)));
  const Series incoming =
      ambient_normal.sub(ambient_sound.mul(2.0 / (model.gamma - 1.0)));
  const Series normal = outgoing.add(incoming).mul(0.5);
  const Series sound = outgoing.sub(incoming).mul(0.25 * (model.gamma - 1.0));
  const Series valid_sound = sound.gt(0.0).logical_and(sound.isfinite());
  if (!subsonic.logical_not().logical_or(valid_sound).all().item<bool>())
    throw std::domain_error(
        "natsci::fluids::gas::boundary: characteristic far field produced "
        "a non-positive sound speed");

  const Series flow_out = normal.ge(0.0);
  const Series inside_entropy =
      inside_pressure.div(inside.rho.pow(model.gamma));
  const Series ambient_entropy =
      ambient_pressure.div(ambient.rho.pow(model.gamma));
  const Series entropy = flow_out.where(inside_entropy, ambient_entropy);
  const Series safe_sound = valid_sound.where(sound, inside_sound);
  const Series rho = safe_sound.square()
                         .div(entropy.mul(model.gamma))
                         .pow(1.0 / (model.gamma - 1.0));
  const Series pressure = entropy.mul(rho.pow(model.gamma));
  std::vector<Series> components;
  components.reserve(static_cast<std::size_t>(inside_velocity.size(component)));
  for (std::int64_t index = 0; index < inside_velocity.size(component);
       ++index) {
    if (index == axis) {
      components.push_back(normal.mul(sign));
    } else {
      components.push_back(
          flow_out.where(inside_velocity.select(component, index),
                         ambient_velocity.select(component, index)));
    }
  }
  const Series velocity =
      spatial::detail::stack_components(components, component);
  const Series momentum = velocity.mul(rho.unsqueeze(component));
  const Series kinetic =
      velocity.square().sum(component, false).mul(rho).mul(0.5);
  const detail::Raw sub{rho, momentum,
                        pressure.div(model.gamma - 1.0).add(kinetic)};
  return choose(supersonic_out, inside,
                choose(supersonic_in, ambient, sub, component), component);
}

inline detail::Raw pressure_outlet(const detail::Raw &inside,
                                   const Series &target, const Bound &rule,
                                   const Model &model, std::int64_t component) {
  const double sign = rule.face().side == spatial::Side::Low ? -1.0 : 1.0;
  const std::int64_t axis = rule.face().axis;
  const Series pressure = plane_pressure(inside, component, model);
  const Series velocity = inside.mom.div(inside.rho.unsqueeze(component));
  const Series normal = velocity.select(component, axis).mul(sign);
  const Series sound = pressure.mul(model.gamma).div(inside.rho).sqrt();
  const Series supersonic = normal.ge(sound);
  if (!supersonic.logical_or(normal.ge(0.0)).all().item<bool>())
    throw std::domain_error(
        "natsci::fluids::gas::boundary: pressure outlet encountered "
        "backflow; use a characteristic far field or prescribed state");

  const Series entropy = pressure.div(inside.rho.pow(model.gamma));
  const Series rho = target.div(entropy).pow(1.0 / model.gamma);
  const Series boundary_sound = target.mul(model.gamma).div(rho).sqrt();
  const Series boundary_normal =
      normal.add(sound.mul(2.0 / (model.gamma - 1.0)))
          .sub(boundary_sound.mul(2.0 / (model.gamma - 1.0)));
  if (!supersonic.logical_or(boundary_normal.ge(0.0)).all().item<bool>())
    throw std::domain_error(
        "natsci::fluids::gas::boundary: target outlet pressure reverses "
        "the boundary flow");
  std::vector<Series> components;
  components.reserve(static_cast<std::size_t>(velocity.size(component)));
  for (std::int64_t index = 0; index < velocity.size(component); ++index)
    components.push_back(index == axis ? boundary_normal.mul(sign)
                                       : velocity.select(component, index));
  const Series boundary_velocity =
      spatial::detail::stack_components(components, component);
  const Series momentum = boundary_velocity.mul(rho.unsqueeze(component));
  const Series kinetic =
      boundary_velocity.square().sum(component, false).mul(rho).mul(0.5);
  const detail::Raw sub{rho, momentum,
                        target.div(model.gamma - 1.0).add(kinetic)};
  return choose(supersonic, inside, sub, component);
}

inline detail::Raw characteristic_inlet(const detail::Raw &inside,
                                        const spatial::BoxGrid &grid,
                                        const Bound &rule, const Model &model,
                                        std::int64_t component) {
  constexpr const char *operation = "natsci::fluids::gas::inlet";
  const auto &data = *rule.inlet();
  const Series inside_velocity =
      inside.mom.div(inside.rho.unsqueeze(component));
  Series direction = expand(data.direction, inside_velocity, component,
                            grid.dim(), operation, "inlet direction");
  const Series norm = direction.square().sum(component, false).sqrt();
  const double tolerance =
      direction.scalar_type() == ScalarType::Float32 ? 2e-5 : 2e-12;
  if (!norm.sub(1.0).abs().le(tolerance).all().item<bool>())
    throw std::domain_error(
        "natsci::fluids::gas::inlet: direction must be a unit vector");
  const double sign = rule.face().side == spatial::Side::Low ? -1.0 : 1.0;
  const Series normal_direction =
      direction.select(component, rule.face().axis).mul(sign);
  if (!normal_direction.lt(0.0).all().item<bool>())
    throw std::domain_error(
        "natsci::fluids::gas::inlet: direction must point into the domain");

  const Series inside_pressure = plane_pressure(inside, component, model);
  const Series inside_normal =
      inside_velocity.select(component, rule.face().axis).mul(sign);
  const Series inside_sound =
      inside_pressure.mul(model.gamma).div(inside.rho).sqrt();
  const Series outgoing =
      inside_normal.add(inside_sound.mul(2.0 / (model.gamma - 1.0)));

  Series rho;
  Series pressure;
  Series speed;
  if (data.kind == InletKind::Total) {
    const Series total_temperature =
        expand(data.total_temperature->si_value(), inside.rho, 0, 0, operation,
               "total temperature");
    const Series total_pressure =
        expand(data.total_pressure->si_value(), inside.rho, 0, 0, operation,
               "total pressure");
    if (!total_temperature.gt(0.0)
             .logical_and(total_temperature.isfinite())
             .logical_and(total_pressure.gt(model.p_floor))
             .logical_and(total_pressure.isfinite())
             .all()
             .item<bool>())
      throw std::domain_error(
          "natsci::fluids::gas::inlet: total temperature and pressure must "
          "be finite and positive");
    const Series sound_total =
        total_temperature.mul(model.gamma * model.gas_constant.si_value())
            .sqrt();
    const double expansion = 0.5 * (model.gamma - 1.0);
    const double invariant = 2.0 / (model.gamma - 1.0);
    const auto residual = [&](const Series &mach) {
      return normal_direction.mul(mach)
          .add(invariant)
          .mul(sound_total)
          .div(mach.square().mul(expansion).add(1.0).sqrt())
          .sub(outgoing);
    };
    Series low = pluss::series::zeros_like(inside.rho);
    Series high = pluss::series::ones_like(inside.rho);
    const Series low_residual = residual(low);
    const Series high_residual = residual(high);
    if (!low_residual.ge(0.0)
             .logical_and(high_residual.le(0.0))
             .all()
             .item<bool>())
      throw std::domain_error(
          "natsci::fluids::gas::inlet: total conditions and outgoing "
          "characteristic have no subsonic inward solution");
    for (std::int64_t iteration = 0; iteration < 48; ++iteration) {
      const Series middle = low.add(high).mul(0.5);
      const Series positive = residual(middle).gt(0.0);
      low = positive.where(middle, low);
      high = positive.where(high, middle);
    }
    const Series mach = low.add(high).mul(0.5);
    const Series ratio = mach.square().mul(expansion).add(1.0);
    const Series static_temperature = total_temperature.div(ratio);
    pressure = total_pressure.div(ratio.pow(model.gamma / (model.gamma - 1.0)));
    const Series sound =
        static_temperature.mul(model.gamma * model.gas_constant.si_value())
            .sqrt();
    speed = mach.mul(sound);
    rho = pressure.div(static_temperature).div(model.gas_constant.si_value());
  } else {
    rho = expand(data.density->si_value(), inside.rho, 0, 0, operation,
                 "inlet density");
    speed = expand(data.speed->si_value(), inside.rho, 0, 0, operation,
                   "inlet speed");
    if (!rho.gt(model.rho_floor)
             .logical_and(rho.isfinite())
             .logical_and(speed.gt(0.0))
             .logical_and(speed.isfinite())
             .all()
             .item<bool>())
      throw std::domain_error(
          "natsci::fluids::gas::inlet: density and speed must be finite and "
          "positive");
    const Series normal = normal_direction.mul(speed);
    const Series sound = outgoing.sub(normal).mul(0.5 * (model.gamma - 1.0));
    pressure = sound.square().mul(rho).div(model.gamma);
    if (!sound.gt(0.0)
             .logical_and(speed.lt(sound))
             .logical_and(pressure.gt(model.p_floor))
             .all()
             .item<bool>())
      throw std::domain_error(
          "natsci::fluids::gas::inlet: density-speed data and outgoing "
          "characteristic do not define a subsonic inlet");
  }
  const Series velocity = direction.mul(speed.unsqueeze(component));
  const Series momentum = velocity.mul(rho.unsqueeze(component));
  const Series kinetic =
      velocity.square().sum(component, false).mul(rho).mul(0.5);
  return {rho, momentum, pressure.div(model.gamma - 1.0).add(kinetic)};
}

inline detail::Raw exterior(const detail::Raw &inside,
                            const spatial::BoxGrid &grid, const Bound &rule,
                            const Model &model) {
  constexpr const char *operation = "natsci::fluids::gas::boundary";
  const std::int64_t component = inside.mom.dim() - inside.rho.dim();
  if (component != 1)
    throw std::invalid_argument(
        "natsci::fluids::gas::boundary: momentum layout is invalid");
  const std::int64_t component_axis = inside.rho.dim() - grid.dim() + 1;

  if (rule.kind() == BoundKind::Blend) {
    const Series mask = expand_mask(*rule.mask(), inside.rho, operation);
    const auto when_true =
        exterior(inside, grid, *rule.when_true(), model);
    const auto when_false =
        exterior(inside, grid, *rule.when_false(), model);
    return choose(mask, when_true, when_false, component_axis);
  }

  if (rule.kind() == BoundKind::Fixed)
    return from_primitive(*rule.state(), inside, component_axis, grid.dim(),
                          model, operation);
  if (rule.kind() == BoundKind::Far) {
    const auto ambient = from_primitive(*rule.state(), inside, component_axis,
                                        grid.dim(), model, operation);
    return characteristic_far(inside, ambient, rule, model, component_axis);
  }
  if (rule.kind() == BoundKind::Inlet)
    return characteristic_inlet(inside, grid, rule, model, component_axis);

  const Series rho = inside.rho;
  const Series p = plane_pressure(inside, component_axis, model);
  Series velocity = inside.mom.div(rho.unsqueeze(component_axis));
  if (rule.kind() == BoundKind::Slip || rule.kind() == BoundKind::Wall) {
    std::vector<Series> parts;
    parts.reserve(static_cast<std::size_t>(grid.dim()));
    for (std::int64_t index = 0; index < grid.dim(); ++index) {
      Series value = velocity.select(component_axis, index);
      if (index == rule.face().axis)
        value = value.neg();
      parts.push_back(std::move(value));
    }
    velocity = spatial::detail::stack_components(parts, component_axis);
  }

  Series exterior_pressure = p;
  if (rule.kind() == BoundKind::Outlet && rule.pressure()) {
    const Series target = expand(rule.pressure()->si_value(), p, 0, 0,
                                 operation, "outlet pressure");
    if (!target.gt(model.p_floor).all().item<bool>())
      throw std::domain_error(
          "natsci::fluids::gas::boundary: target outlet pressure must be "
          "finite and positive");
    return pressure_outlet(inside, target, rule, model, component_axis);
  }
  const Series momentum = velocity.mul(rho.unsqueeze(component_axis));
  const Series kinetic =
      velocity.square().sum(component_axis, false).mul(rho).mul(0.5);
  detail::Raw result{rho, momentum,
                     exterior_pressure.div(model.gamma - 1.0).add(kinetic)};
  if (!exterior_pressure.gt(model.p_floor).all().item<bool>())
    throw std::domain_error(
        "natsci::fluids::gas::boundary: pressure outlet reconstruction "
        "produced non-positive exterior pressure");
  return result;
}

inline detail::Flux face_flux(const detail::Raw &left, const detail::Raw &right,
                              std::int64_t component, std::int64_t axis,
                              const Model &model, FluxScheme scheme,
                              const RoePolicy &roe,
                              const Series *wave_floor = nullptr) {
  return detail::numerical_flux(left, right, component, axis, model, scheme,
                                roe, wave_floor);
}

inline Series edge_neighbor(const Series &value, const spatial::BoxGrid &grid,
                            std::int64_t axis, bool upper) {
  const std::int64_t series_axis = value.dim() - grid.dim() + axis;
  const std::int64_t extent = value.size(series_axis);
  const Series edge =
      value.select(series_axis, upper ? extent - 1 : 0).unsqueeze(series_axis);
  if (upper)
    return value.slice(series_axis, 1, extent).unite({edge}, series_axis);
  return edge.unite({value.slice(series_axis, 0, extent - 1)}, series_axis);
}

inline Series slope(const Series &value, const spatial::BoxGrid &grid,
                    std::int64_t axis, Recon recon) {
  const Series lower = edge_neighbor(value, grid, axis, false);
  const Series upper = edge_neighbor(value, grid, axis, true);
  return detail::limited_slope(value.sub(lower), upper.sub(value), recon);
}

inline Series stack_planes(const std::vector<Series> &planes,
                           std::int64_t axis) {
  Series result = planes.front().unsqueeze(axis);
  for (std::size_t index = 1; index < planes.size(); ++index)
    result = result.unite({planes[index].unsqueeze(axis)}, axis);
  return result;
}

inline detail::Raw ghost_band(const detail::Raw &state,
                              const spatial::BoxGrid &grid, std::int64_t axis,
                              spatial::Side side, const Bound &rule,
                              const Model &model) {
  const std::int64_t scalar_axis = state.rho.dim() - grid.dim() + axis;
  const std::int64_t vector_axis = state.mom.dim() - grid.dim() + axis;
  const std::int64_t extent = state.rho.size(scalar_axis);
  std::vector<Series> rho;
  std::vector<Series> mom;
  std::vector<Series> energy;
  rho.reserve(3);
  mom.reserve(3);
  energy.reserve(3);
  if (rule.kind() == BoundKind::Blend) {
    const auto yes = ghost_band(state, grid, axis, side, *rule.when_true(), model);
    const auto no =
        ghost_band(state, grid, axis, side, *rule.when_false(), model);
    const auto face = plane(state, grid, axis, side);
    const Series face_mask = expand_mask(*rule.mask(), face.rho,
                                         "natsci::fluids::gas::boundary");
    const Series band_mask = face_mask.unsqueeze(scalar_axis);
    const std::int64_t component_axis = band_mask.dim() - grid.dim();
    return choose(band_mask, yes, no, component_axis);
  }
  if (rule.kind() != BoundKind::Slip && rule.kind() != BoundKind::Wall) {
    const std::int64_t edge = side == spatial::Side::Low ? 0 : extent - 1;
    const auto outside =
        exterior(plane_at(state, grid, axis, edge), grid, rule, model);
    for (std::int64_t layer = 0; layer < 3; ++layer) {
      rho.push_back(outside.rho);
      mom.push_back(outside.mom);
      energy.push_back(outside.energy);
    }
    return {stack_planes(rho, scalar_axis), stack_planes(mom, vector_axis),
            stack_planes(energy, scalar_axis)};
  }
  for (std::int64_t layer = 0; layer < 3; ++layer) {
    const std::int64_t index =
        side == spatial::Side::Low ? 2 - layer : extent - 1 - layer;
    const auto outside =
        exterior(plane_at(state, grid, axis, index), grid, rule, model);
    rho.push_back(outside.rho);
    mom.push_back(outside.mom);
    energy.push_back(outside.energy);
  }
  return {stack_planes(rho, scalar_axis), stack_planes(mom, vector_axis),
          stack_planes(energy, scalar_axis)};
}

inline std::pair<Series, Series>
bounded_weno5_faces(const Series &value, const Series &low, const Series &high,
                    const spatial::BoxGrid &grid, std::int64_t axis) {
  const std::int64_t series_axis = value.dim() - grid.dim() + axis;
  const std::int64_t extent = value.size(series_axis);
  const Series extended = low.unite({value, high}, series_axis);
  const Series m2 = extended.slice(series_axis, 1, extent + 1);
  const Series m1 = extended.slice(series_axis, 2, extent + 2);
  const Series center = extended.slice(series_axis, 3, extent + 3);
  const Series p1 = extended.slice(series_axis, 4, extent + 4);
  const Series p2 = extended.slice(series_axis, 5, extent + 5);
  const Series p3 = extended.slice(series_axis, 6, extent + 6);
  return {detail::weno5_left(m2, m1, center, p1, p2),
          detail::weno5_left(p3, p2, p1, center, m1)};
}

inline Series wall_velocity(const Bound &rule, const Series &inside,
                            std::int64_t component, std::int64_t dim) {
  if (rule.kind() != BoundKind::Wall || !rule.wall())
    throw std::invalid_argument(
        "natsci::fluids::gas::wall: expected a wall boundary");
  Series value =
      rule.wall()->velocity
          ? expand(rule.wall()->velocity->si_value(), inside, component, dim,
                   "natsci::fluids::gas::wall", "wall velocity")
          : pluss::series::zeros_like(inside);
  if (!value.isfinite().all().item<bool>())
    throw std::domain_error(
        "natsci::fluids::gas::wall: velocity must be finite");
  if (!value.select(component, rule.face().axis).eq(0.0).all().item<bool>())
    throw std::domain_error(
        "natsci::fluids::gas::wall: normal wall velocity requires a moving-"
        "mesh formulation");
  return value;
}

inline Series fixed_velocity(const Bound &rule, const detail::Raw &inside,
                             std::int64_t component, std::int64_t dim,
                             const Model &model) {
  const auto prescribed =
      from_primitive(*rule.state(), inside, component, dim, model,
                     "natsci::fluids::gas::viscous_boundary");
  return prescribed.mom.div(prescribed.rho.unsqueeze(component));
}

inline Series velocity_ghost(const detail::Raw &inside,
                             const spatial::BoxGrid &grid, const Bound &rule,
                             const Model &model) {
  const std::int64_t component_axis = inside.rho.dim() - grid.dim() + 1;
  const Series velocity = inside.mom.div(inside.rho.unsqueeze(component_axis));
  if (rule.kind() == BoundKind::Blend) {
    const Series mask =
        expand_mask(*rule.mask(), inside.rho,
                    "natsci::fluids::gas::viscous_boundary");
    return mask.unsqueeze(component_axis)
        .where(velocity_ghost(inside, grid, *rule.when_true(), model),
               velocity_ghost(inside, grid, *rule.when_false(), model));
  }
  if (rule.kind() == BoundKind::Wall)
    return wall_velocity(rule, velocity, component_axis, grid.dim())
        .mul(2.0)
        .sub(velocity);
  if (rule.kind() == BoundKind::Fixed)
    return fixed_velocity(rule, inside, component_axis, grid.dim(), model)
        .mul(2.0)
        .sub(velocity);
  if (rule.kind() == BoundKind::Inlet) {
    const auto prescribed = exterior(inside, grid, rule, model);
    return prescribed.mom.div(prescribed.rho.unsqueeze(component_axis))
        .mul(2.0)
        .sub(velocity);
  }
  if (rule.kind() != BoundKind::Slip)
    return velocity;

  std::vector<Series> parts;
  parts.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t index = 0; index < grid.dim(); ++index) {
    Series part = velocity.select(component_axis, index);
    if (index == rule.face().axis)
      part = part.neg();
    parts.push_back(std::move(part));
  }
  return spatial::detail::stack_components(parts, component_axis);
}

inline std::pair<Series, Series> neighbors(const Series &value,
                                           const Series &low_ghost,
                                           const Series &high_ghost,
                                           const spatial::BoxGrid &grid,
                                           std::int64_t axis) {
  const std::int64_t series_axis = value.dim() - grid.dim() + axis;
  const std::int64_t extent = value.size(series_axis);
  return {low_ghost.unsqueeze(series_axis)
              .unite({value.slice(series_axis, 0, extent - 1)}, series_axis),
          value.slice(series_axis, 1, extent)
              .unite({high_ghost.unsqueeze(series_axis)}, series_axis)};
}

struct Kinematics final {
  std::vector<std::vector<Series>> derivative;
  Series divergence;
  std::int64_t component{};
};

inline Kinematics bounded_kinematics(const detail::Raw &state,
                                     const spatial::BoxGrid &grid,
                                     const Bounds &rules, const Model &model) {
  const std::int64_t component = state.mom.dim() - grid.dim() - 1;
  const Series velocity = state.mom.div(state.rho.unsqueeze(component));
  std::vector<std::vector<Series>> derivatives(
      static_cast<std::size_t>(grid.dim()));
  for (std::int64_t row = 0; row < grid.dim(); ++row)
    derivatives[static_cast<std::size_t>(row)].reserve(
        static_cast<std::size_t>(grid.dim()));

  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const auto low_inside = plane(state, grid, axis, spatial::Side::Low);
    const auto high_inside = plane(state, grid, axis, spatial::Side::High);
    const Series low_ghost = velocity_ghost(
        low_inside, grid, rules.at(axis, spatial::Side::Low), model);
    const Series high_ghost = velocity_ghost(
        high_inside, grid, rules.at(axis, spatial::Side::High), model);
    auto [lower, upper] =
        neighbors(velocity, low_ghost, high_ghost, grid, axis);
    const Series derivative =
        upper.sub(lower).div(2.0 * grid.spacing(axis).si_value());
    for (std::int64_t row = 0; row < grid.dim(); ++row)
      derivatives[static_cast<std::size_t>(row)].push_back(
          derivative.select(component, row));
  }

  Series divergence = derivatives[0][0];
  for (std::int64_t axis = 1; axis < grid.dim(); ++axis)
    divergence = divergence.add(derivatives[static_cast<std::size_t>(axis)]
                                           [static_cast<std::size_t>(axis)]);
  return {std::move(derivatives), std::move(divergence), component};
}

inline Series bounded_stress(const detail::Raw &state,
                             const spatial::BoxGrid &grid, const Bounds &rules,
                             const Model &model, const Transport &transport) {
  auto kinematics = bounded_kinematics(state, grid, rules, model);
  const double mu = transport.shear.si_value();
  const double diagonal = transport.bulk.si_value() - (2.0 / 3.0) * mu;
  std::vector<Series> rows;
  rows.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t row = 0; row < grid.dim(); ++row) {
    std::vector<Series> columns;
    columns.reserve(static_cast<std::size_t>(grid.dim()));
    for (std::int64_t column = 0; column < grid.dim(); ++column) {
      Series entry =
          kinematics
              .derivative[static_cast<std::size_t>(row)]
                         [static_cast<std::size_t>(column)]
              .add(kinematics.derivative[static_cast<std::size_t>(column)]
                                        [static_cast<std::size_t>(row)])
              .mul(mu);
      if (row == column)
        entry = entry.add(kinematics.divergence.mul(diagonal));
      columns.push_back(std::move(entry));
    }
    rows.push_back(
        spatial::detail::stack_components(columns, kinematics.component));
  }
  return spatial::detail::stack_components(rows, kinematics.component);
}

inline Series bounded_stress(const detail::Raw &state,
                             const spatial::BoxGrid &grid, const Bounds &rules,
                             const Model &model,
                             const TransportField &transport) {
  detail::validate_transport_field(transport, state,
                                   "natsci::fluids::gas::stress");
  auto kinematics = bounded_kinematics(state, grid, rules, model);
  const Series &mu = transport.shear.si_value();
  const Series diagonal = transport.bulk.si_value().sub(mu.mul(2.0 / 3.0));
  std::vector<Series> rows;
  rows.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t row = 0; row < grid.dim(); ++row) {
    std::vector<Series> columns;
    columns.reserve(static_cast<std::size_t>(grid.dim()));
    for (std::int64_t column = 0; column < grid.dim(); ++column) {
      Series entry =
          kinematics
              .derivative[static_cast<std::size_t>(row)]
                         [static_cast<std::size_t>(column)]
              .add(kinematics.derivative[static_cast<std::size_t>(column)]
                                        [static_cast<std::size_t>(row)])
              .mul(mu);
      if (row == column)
        entry = entry.add(kinematics.divergence.mul(diagonal));
      columns.push_back(std::move(entry));
    }
    rows.push_back(
        spatial::detail::stack_components(columns, kinematics.component));
  }
  return spatial::detail::stack_components(rows, kinematics.component);
}

inline Series face_value(const Series &value, const spatial::BoxGrid &grid,
                         std::int64_t axis, spatial::Side side) {
  const std::int64_t series_axis = value.dim() - grid.dim() + axis;
  const std::int64_t index =
      side == spatial::Side::Low ? 0 : value.size(series_axis) - 1;
  return value.select(series_axis, index);
}

inline Series replace_high(const Series &interior, const Series &boundary,
                           const spatial::BoxGrid &grid, std::int64_t axis);
inline Series make_low(const Series &high, const Series &boundary,
                       const spatial::BoxGrid &grid, std::int64_t axis);

inline Series boundary_velocity(const detail::Raw &inside,
                                const spatial::BoxGrid &grid, const Bound &rule,
                                const Model &model) {
  const std::int64_t component = inside.rho.dim() - grid.dim() + 1;
  const Series velocity = inside.mom.div(inside.rho.unsqueeze(component));
  if (rule.kind() == BoundKind::Blend) {
    const Series mask =
        expand_mask(*rule.mask(), inside.rho,
                    "natsci::fluids::gas::viscous_boundary");
    return mask.unsqueeze(component)
        .where(boundary_velocity(inside, grid, *rule.when_true(), model),
               boundary_velocity(inside, grid, *rule.when_false(), model));
  }
  if (rule.kind() == BoundKind::Wall)
    return wall_velocity(rule, velocity, component, grid.dim());
  if (rule.kind() == BoundKind::Fixed)
    return fixed_velocity(rule, inside, component, grid.dim(), model);
  if (rule.kind() == BoundKind::Inlet) {
    const auto prescribed = exterior(inside, grid, rule, model);
    return prescribed.mom.div(prescribed.rho.unsqueeze(component));
  }
  if (rule.kind() != BoundKind::Slip)
    return velocity;

  std::vector<Series> parts;
  parts.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t index = 0; index < grid.dim(); ++index)
    parts.push_back(
        index == rule.face().axis
            ? pluss::series::zeros_like(velocity.select(component, index))
            : velocity.select(component, index));
  return spatial::detail::stack_components(parts, component);
}

inline std::optional<Series> boundary_temperature(const detail::Raw &inside,
                                                  const spatial::BoxGrid &grid,
                                                  const Bound &rule,
                                                  const Model &model) {
  const std::int64_t component = inside.rho.dim() - grid.dim() + 1;
  const Series inside_temperature = plane_pressure(inside, component, model)
                                        .div(inside.rho)
                                        .div(model.gas_constant.si_value());
  if (rule.kind() == BoundKind::Wall &&
      rule.wall()->heat == WallHeat::Temperature) {
    const Series target =
        expand(rule.wall()->temperature->si_value(), inside_temperature, 0, 0,
               "natsci::fluids::gas::wall", "wall temperature");
    if (!target.gt(0.0).logical_and(target.isfinite()).all().item<bool>())
      throw std::domain_error(
          "natsci::fluids::gas::wall: temperature must be finite and "
          "positive");
    return target;
  }
  if (rule.kind() == BoundKind::Fixed || rule.kind() == BoundKind::Inlet) {
    const auto prescribed =
        rule.kind() == BoundKind::Fixed
            ? from_primitive(*rule.state(), inside, component, grid.dim(),
                             model, "natsci::fluids::gas::viscous_boundary")
            : exterior(inside, grid, rule, model);
    const Series target = plane_pressure(prescribed, component, model)
                              .div(prescribed.rho)
                              .div(model.gas_constant.si_value());
    return target;
  }
  return std::nullopt;
}

inline Series boundary_conduction(
    const detail::Raw &inside, const spatial::BoxGrid &grid, const Bound &rule,
    const Model &model, const Series &conductivity,
    const std::optional<SutherlandLaw> &sutherland = std::nullopt) {
  const std::int64_t component = inside.rho.dim() - grid.dim() + 1;
  const Series inside_temperature = plane_pressure(inside, component, model)
                                        .div(inside.rho)
                                        .div(model.gas_constant.si_value());
  if (rule.kind() == BoundKind::Blend) {
    const Series mask =
        expand_mask(*rule.mask(), inside.rho,
                    "natsci::fluids::gas::viscous_boundary");
    return mask.where(boundary_conduction(inside, grid, *rule.when_true(),
                                          model, conductivity, sutherland),
                      boundary_conduction(inside, grid, *rule.when_false(),
                                          model, conductivity, sutherland));
  }
  const double sign = rule.face().side == spatial::Side::Low ? -1.0 : 1.0;
  if (rule.kind() == BoundKind::Wall && rule.wall()->heat == WallHeat::Flux) {
    const Series outward =
        expand(rule.wall()->flux->si_value(), inside_temperature, 0, 0,
               "natsci::fluids::gas::wall", "wall heat flux");
    if (!outward.isfinite().all().item<bool>())
      throw std::domain_error(
          "natsci::fluids::gas::wall: heat flux must be finite");
    return outward.mul(-sign);
  }
  const auto target = boundary_temperature(inside, grid, rule, model);
  if (!target)
    return pluss::series::zeros_like(inside_temperature);
  const Series wall_conductivity =
      sutherland ? detail::sutherland_transport(*target, model, *sutherland)
                       .heat.si_value()
                 : conductivity;
  const Series sum = conductivity.add(wall_conductivity);
  const Series active = sum.gt(0.0);
  const Series face_conductivity =
      active.where(conductivity.mul(wall_conductivity)
                       .mul(2.0)
                       .div(active.where(sum, pluss::series::ones_like(sum))),
                   pluss::series::zeros_like(sum));
  return target->sub(inside_temperature)
      .mul(face_conductivity)
      .mul(sign * 2.0 / grid.spacing(rule.face().axis).si_value());
}

inline Series
boundary_viscosity(const detail::Raw &inside, const spatial::BoxGrid &grid,
                   const Bound &rule, const Model &model,
                   const Series &viscosity,
                   const std::optional<SutherlandLaw> &sutherland) {
  if (rule.kind() == BoundKind::Blend) {
    const Series mask =
        expand_mask(*rule.mask(), inside.rho,
                    "natsci::fluids::gas::viscous_boundary");
    return mask.where(boundary_viscosity(inside, grid, *rule.when_true(), model,
                                         viscosity, sutherland),
                      boundary_viscosity(inside, grid, *rule.when_false(),
                                         model, viscosity, sutherland));
  }
  if (!sutherland)
    return viscosity;
  const auto target = boundary_temperature(inside, grid, rule, model);
  return target ? detail::sutherland_transport(*target, model, *sutherland)
                      .shear.si_value()
                : viscosity;
}

inline detail::Raw
visc_rate_impl(const detail::Raw &state, const spatial::BoxGrid &grid,
               const Bounds &rules, const Model &model, const Series &tau,
               const Series &viscosity, const Series &conductivity,
               const std::optional<SutherlandLaw> &sutherland) {
  detail::Raw result{pluss::series::zeros_like(state.rho),
                     pluss::series::zeros_like(state.mom),
                     pluss::series::zeros_like(state.energy)};
  const std::int64_t component = state.mom.dim() - grid.dim() - 1;
  const Series velocity = state.mom.div(state.rho.unsqueeze(component));
  const Series temperature = detail::temperature(state, grid, model);
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const auto &low_rule = rules.at(axis, spatial::Side::Low);
    const auto &high_rule = rules.at(axis, spatial::Side::High);
    const auto low_inside = plane(state, grid, axis, spatial::Side::Low);
    const auto high_inside = plane(state, grid, axis, spatial::Side::High);
    const double width = grid.spacing(axis).si_value();

    const Series tau_axis = tau.select(component + 1, axis);
    Series tau_high =
        tau_axis.add(edge_neighbor(tau_axis, grid, axis, true)).mul(0.5);
    const Series low_mu = face_value(viscosity, grid, axis, spatial::Side::Low);
    const Series high_mu =
        face_value(viscosity, grid, axis, spatial::Side::High);
    Series low_tau = face_value(tau_axis, grid, axis, spatial::Side::Low);
    Series high_tau = face_value(tau_axis, grid, axis, spatial::Side::High);
    if (sutherland) {
      low_tau = low_tau.mul(boundary_viscosity(low_inside, grid, low_rule,
                                               model, low_mu, sutherland)
                                .div(low_mu)
                                .unsqueeze(component));
      high_tau = high_tau.mul(boundary_viscosity(high_inside, grid, high_rule,
                                                 model, high_mu, sutherland)
                                  .div(high_mu)
                                  .unsqueeze(component));
    }
    tau_high = replace_high(tau_high, high_tau, grid, axis);
    const Series tau_low = make_low(tau_high, low_tau, grid, axis);
    result.mom = result.mom.add(tau_high.sub(tau_low).div(width));

    Series velocity_high =
        velocity.add(edge_neighbor(velocity, grid, axis, true)).mul(0.5);
    const Series low_velocity =
        boundary_velocity(low_inside, grid, low_rule, model);
    const Series high_velocity =
        boundary_velocity(high_inside, grid, high_rule, model);
    velocity_high = replace_high(velocity_high, high_velocity, grid, axis);
    const Series velocity_low =
        make_low(velocity_high, low_velocity, grid, axis);
    Series work_high = tau_high.mul(velocity_high).sum(component, false);
    const Series work_low = tau_low.mul(velocity_low).sum(component, false);

    const Series next_conductivity =
        edge_neighbor(conductivity, grid, axis, true);
    const Series conductivity_sum = conductivity.add(next_conductivity);
    const Series conducting = conductivity_sum.gt(0.0);
    const Series conductivity_face = conducting.where(
        conductivity.mul(next_conductivity)
            .mul(2.0)
            .div(conducting.where(conductivity_sum,
                                  pluss::series::ones_like(conductivity_sum))),
        pluss::series::zeros_like(conductivity_sum));
    Series heat_high = edge_neighbor(temperature, grid, axis, true)
                           .sub(temperature)
                           .div(width)
                           .mul(conductivity_face);
    const Series low_heat = boundary_conduction(
        low_inside, grid, low_rule, model,
        face_value(conductivity, grid, axis, spatial::Side::Low), sutherland);
    const Series high_heat = boundary_conduction(
        high_inside, grid, high_rule, model,
        face_value(conductivity, grid, axis, spatial::Side::High), sutherland);
    heat_high = replace_high(heat_high, high_heat, grid, axis);
    const Series heat_low = make_low(heat_high, low_heat, grid, axis);
    work_high = work_high.add(heat_high);
    const Series energy_low = work_low.add(heat_low);
    result.energy = result.energy.add(work_high.sub(energy_low).div(width));
  }
  return result;
}

inline detail::Raw visc_rate(const detail::Raw &state,
                             const spatial::BoxGrid &grid, const Bounds &rules,
                             const Model &model, const Transport &transport) {
  detail::Raw result{pluss::series::zeros_like(state.rho),
                     pluss::series::zeros_like(state.mom),
                     pluss::series::zeros_like(state.energy)};
  if (transport.shear.si_value() == 0.0 && transport.bulk.si_value() == 0.0 &&
      transport.heat.si_value() == 0.0)
    return result;
  const Series tau = bounded_stress(state, grid, rules, model, transport);
  const Series viscosity =
      pluss::series::full_like(state.rho, transport.shear.si_value());
  const Series conductivity =
      pluss::series::full_like(state.rho, transport.heat.si_value());
  return visc_rate_impl(state, grid, rules, model, tau, viscosity, conductivity,
                        std::nullopt);
}

inline detail::Raw visc_rate(const detail::Raw &state,
                             const spatial::BoxGrid &grid, const Bounds &rules,
                             const Model &model, const SutherlandLaw &law) {
  const TransportField field =
      detail::sutherland_transport(state, grid, model, law);
  const Series tau = bounded_stress(state, grid, rules, model, field);
  return visc_rate_impl(state, grid, rules, model, tau, field.shear.si_value(),
                        field.heat.si_value(), law);
}

inline detail::Face interior_states(const detail::Raw &state,
                                    const spatial::BoxGrid &grid,
                                    const Bounds &rules, std::int64_t axis,
                                    const Model &model, Recon recon) {
  if (recon == Recon::Weno5) {
    const auto low = ghost_band(state, grid, axis, spatial::Side::Low,
                                rules.at(axis, spatial::Side::Low), model);
    const auto high = ghost_band(state, grid, axis, spatial::Side::High,
                                 rules.at(axis, spatial::Side::High), model);
    auto rho = bounded_weno5_faces(state.rho, low.rho, high.rho, grid, axis);
    auto mom = bounded_weno5_faces(state.mom, low.mom, high.mom, grid, axis);
    auto energy =
        bounded_weno5_faces(state.energy, low.energy, high.energy, grid, axis);
    detail::Raw left{std::move(rho.first), std::move(mom.first),
                     std::move(energy.first)};
    detail::Raw right{std::move(rho.second), std::move(mom.second),
                      std::move(energy.second)};
    const detail::Raw first_left{state.rho, state.mom, state.energy};
    const detail::Raw first_right{
        edge_neighbor(state.rho, grid, axis, true),
        edge_neighbor(state.mom, grid, axis, true),
        edge_neighbor(state.energy, grid, axis, true)};
    left = detail::limit_toward(first_left, left, grid, model);
    right = detail::limit_toward(first_right, right, grid, model);
    return {std::move(left), std::move(right)};
  }
  const Series rho_slope = slope(state.rho, grid, axis, recon);
  const Series mom_slope = slope(state.mom, grid, axis, recon);
  const Series energy_slope = slope(state.energy, grid, axis, recon);
  const Series next_rho = edge_neighbor(state.rho, grid, axis, true);
  const Series next_mom = edge_neighbor(state.mom, grid, axis, true);
  const Series next_energy = edge_neighbor(state.energy, grid, axis, true);
  detail::Raw left{state.rho.add(rho_slope.mul(0.5)),
                   state.mom.add(mom_slope.mul(0.5)),
                   state.energy.add(energy_slope.mul(0.5))};
  detail::Raw right{
      next_rho.sub(edge_neighbor(rho_slope, grid, axis, true).mul(0.5)),
      next_mom.sub(edge_neighbor(mom_slope, grid, axis, true).mul(0.5)),
      next_energy.sub(edge_neighbor(energy_slope, grid, axis, true).mul(0.5))};
  const detail::Raw first_left{state.rho, state.mom, state.energy};
  const detail::Raw first_right{next_rho, next_mom, next_energy};
  left = detail::limit_toward(first_left, left, grid, model);
  right = detail::limit_toward(first_right, right, grid, model);
  return {std::move(left), std::move(right)};
}

inline detail::Flux interior_high(const detail::Face &states,
                                  const spatial::BoxGrid &grid,
                                  std::int64_t axis, const Model &model,
                                  FluxScheme scheme, const RoePolicy &roe,
                                  const Series *wave_floor = nullptr) {
  return face_flux(states.left, states.right,
                   states.left.mom.dim() - grid.dim() - 1, axis, model, scheme,
                   roe, wave_floor);
}

inline Series h_floor(const std::vector<Series> &eta,
                      const spatial::BoxGrid &grid, std::int64_t axis) {
  Series result = eta[static_cast<std::size_t>(axis)];
  for (std::int64_t transverse = 0; transverse < grid.dim(); ++transverse) {
    if (transverse == axis)
      continue;
    const Series &cross = eta[static_cast<std::size_t>(transverse)];
    const Series along = edge_neighbor(cross, grid, axis, true);
    result = result.maximum(cross)
                 .maximum(edge_neighbor(cross, grid, transverse, false))
                 .maximum(along)
                 .maximum(edge_neighbor(along, grid, transverse, false));
  }
  return result;
}

inline Series replace_high(const Series &interior, const Series &boundary,
                           const spatial::BoxGrid &grid, std::int64_t axis) {
  const std::int64_t series_axis = interior.dim() - grid.dim() + axis;
  const std::int64_t extent = interior.size(series_axis);
  return interior.slice(series_axis, 0, extent - 1)
      .unite({boundary.unsqueeze(series_axis)}, series_axis);
}

inline Series make_low(const Series &high, const Series &boundary,
                       const spatial::BoxGrid &grid, std::int64_t axis) {
  const std::int64_t series_axis = high.dim() - grid.dim() + axis;
  const std::int64_t extent = high.size(series_axis);
  return boundary.unsqueeze(series_axis)
      .unite({high.slice(series_axis, 0, extent - 1)}, series_axis);
}

inline detail::Raw rate(const detail::Raw &state, const spatial::BoxGrid &grid,
                        const Bounds &rules, const Model &model, Recon recon,
                        FluxScheme scheme, const RoePolicy &roe,
                        const Transport &transport,
                        const std::optional<SutherlandLaw> &sutherland) {
  if (rules.dim() != grid.dim())
    throw std::invalid_argument(
        "natsci::fluids::gas::rate: boundary and grid rank mismatch");
  if (recon == Recon::Weno5)
    for (std::int64_t axis = 0; axis < grid.dim(); ++axis)
      if (grid.shape()[static_cast<std::size_t>(axis)] < 5)
        throw std::invalid_argument(
            "natsci::fluids::gas::rate: bounded WENO5 requires at least "
            "five cells on every spatial axis");
  const std::int64_t component = state.mom.dim() - grid.dim() - 1;
  detail::Raw result{pluss::series::zeros_like(state.rho),
                     pluss::series::zeros_like(state.mom),
                     pluss::series::zeros_like(state.energy)};
  const bool h_correction = scheme == FluxScheme::Roe &&
                            roe.shock == RoeShock::HFix && grid.dim() > 1;
  std::vector<detail::Face> states;
  std::vector<Series> eta;
  states.reserve(static_cast<std::size_t>(grid.dim()));
  if (h_correction)
    eta.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    states.push_back(interior_states(state, grid, rules, axis, model, recon));
    if (h_correction)
      eta.push_back(detail::roe_eta(states.back(), component, axis, model));
  }
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    std::optional<Series> floor;
    if (h_correction)
      floor = h_floor(eta, grid, axis);
    detail::Flux high =
        interior_high(states[static_cast<std::size_t>(axis)], grid, axis, model,
                      scheme, roe, floor ? &*floor : nullptr);
    const detail::Raw low_inside = plane(state, grid, axis, spatial::Side::Low);
    const detail::Raw high_inside =
        plane(state, grid, axis, spatial::Side::High);
    const detail::Raw low_out =
        exterior(low_inside, grid, rules.at(axis, spatial::Side::Low), model);
    const detail::Raw high_out =
        exterior(high_inside, grid, rules.at(axis, spatial::Side::High), model);
    std::optional<Series> low_floor;
    std::optional<Series> high_floor;
    if (floor) {
      const std::int64_t floor_axis = floor->dim() - grid.dim() + axis;
      low_floor = floor->select(floor_axis, 0);
      high_floor = floor->select(floor_axis, floor->size(floor_axis) - 1);
    }
    const detail::Flux low_face =
        face_flux(low_out, low_inside, component, axis, model, scheme, roe,
                  low_floor ? &*low_floor : nullptr);
    const detail::Flux high_face =
        face_flux(high_inside, high_out, component, axis, model, scheme, roe,
                  high_floor ? &*high_floor : nullptr);

    high.mass = replace_high(high.mass, high_face.mass, grid, axis);
    high.momentum = replace_high(high.momentum, high_face.momentum, grid, axis);
    high.energy = replace_high(high.energy, high_face.energy, grid, axis);
    const Series low_mass = make_low(high.mass, low_face.mass, grid, axis);
    const Series low_momentum =
        make_low(high.momentum, low_face.momentum, grid, axis);
    const Series low_energy =
        make_low(high.energy, low_face.energy, grid, axis);
    const double width = grid.spacing(axis).si_value();
    result.rho = result.rho.sub(high.mass.sub(low_mass).div(width));
    result.mom = result.mom.sub(high.momentum.sub(low_momentum).div(width));
    result.energy = result.energy.sub(high.energy.sub(low_energy).div(width));
  }
  const auto diffusive = sutherland
                             ? visc_rate(state, grid, rules, model, *sutherland)
                             : visc_rate(state, grid, rules, model, transport);
  result.mom = result.mom.add(diffusive.mom);
  result.energy = result.energy.add(diffusive.energy);
  return result;
}

inline common::StateVector record(const State &state) {
  return detail::record(state);
}

inline State state(const common::StateVector &record,
                   const spatial::BoxGrid &grid, const Model &model) {
  return make(record.get<Density>("density"),
              record.get<MomDensity>("momentum"),
              record.get<EnergyDensity>("energy"), grid, model);
}

} // namespace boundary_detail

[[nodiscard]] inline TransportField transport(const State &state,
                                              const spatial::BoxGrid &grid,
                                              const Model &model,
                                              const SutherlandLaw &law) {
  constexpr const char *operation = "natsci::fluids::gas::transport";
  detail::validate_model(model, operation);
  detail::validate_sutherland(law, operation);
  const auto raw = Access::raw(state);
  detail::require_valid(raw, grid, model, operation);
  return detail::sutherland_transport(raw, grid, model, law);
}

[[nodiscard]] inline Rate rate(const State &state, const spatial::BoxGrid &grid,
                               const Bounds &rules, const Model &model = {},
                               const Options &options = {}) {
  constexpr const char *operation = "natsci::fluids::gas::rate";
  detail::validate_model(model, operation);
  boundary_detail::validate_options(options, operation);
  const auto raw = Access::raw(state);
  detail::require_valid(raw, grid, model, operation);
  auto value = boundary_detail::rate(raw, grid, rules, model, options.recon,
                                     options.flux, options.roe,
                                     options.transport, options.sutherland);
  return {common::SeriesQuantity<DensityRate>::from_si(std::move(value.rho)),
          common::SeriesQuantity<MomRate>::from_si(std::move(value.mom)),
          common::SeriesQuantity<EnergyRate>::from_si(std::move(value.energy))};
}

[[nodiscard]] inline common::SeriesQuantity<common::Time>
step_limit(const State &state, const spatial::BoxGrid &grid,
           const Model &model = {}, double courant = 0.45) {
  constexpr const char *operation = "natsci::fluids::gas::step_limit";
  detail::validate_model(model, operation);
  if (!(courant > 0.0) || !(courant <= 1.0) || !std::isfinite(courant))
    throw std::invalid_argument(
        "natsci::fluids::gas::step_limit: courant must be finite and in (0, "
        "1]");
  const Series speed_value = velocity(state, grid).si_value();
  const Series sound_value = sound(state, grid, model).si_value();
  const std::int64_t component = speed_value.dim() - grid.dim() - 1;
  const Series &rho = state.density().si_value();
  const std::vector<std::int64_t> batches(rho.sizes().begin(),
                                          rho.sizes().end() - grid.dim());
  Series wave_rate =
      pluss::series::zeros(batches, rho.scalar_type()).to(rho.platform());
  const auto axes = spatial::detail::spatial_axes(rho, grid);
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis)
    wave_rate = wave_rate.add(speed_value.select(component, axis)
                                  .abs()
                                  .add(sound_value)
                                  .amax(axes, false)
                                  .div(grid.spacing(axis).si_value()));
  return common::SeriesQuantity<common::Time>::from_si(
      pluss::codem::make_scalar_like(wave_rate, courant).div(wave_rate));
}

[[nodiscard]] inline common::SeriesQuantity<common::Time>
step_limit(const State &state, const spatial::BoxGrid &grid, const Model &model,
           const Transport &transport, double courant = 0.45,
           double diffusion = 0.5) {
  constexpr const char *operation = "natsci::fluids::gas::step_limit";
  detail::validate_transport(transport, operation);
  if (!(diffusion > 0.0) || !(diffusion <= 0.5) || !std::isfinite(diffusion))
    throw std::invalid_argument(
        "natsci::fluids::gas::step_limit: diffusion must be finite and in "
        "(0, 0.5]");
  const Series acoustic = step_limit(state, grid, model, courant).si_value();
  const Series &rho = state.density().si_value();
  const auto axes = spatial::detail::spatial_axes(rho, grid);
  double inverse_h2 = 0.0;
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const double width = grid.spacing(axis).si_value();
    inverse_h2 += 1.0 / (width * width);
  }
  const double cv = model.gas_constant.si_value() / (model.gamma - 1.0);
  const Series momentum_diff = rho.reciprocal().mul(
      (4.0 / 3.0) * transport.shear.si_value() + transport.bulk.si_value());
  const Series thermal_diff =
      rho.reciprocal().mul(transport.heat.si_value() / cv);
  const Series rate =
      momentum_diff.maximum(thermal_diff).amax(axes, false).mul(inverse_h2);
  const Series infinite =
      pluss::series::full_like(rate, std::numeric_limits<double>::infinity());
  const Series viscous = rate.gt(0.0).where(
      pluss::codem::make_scalar_like(rate, diffusion)
          .div(rate.add(rate.eq(0.0).to(rate.scalar_type()))),
      infinite);
  return common::SeriesQuantity<common::Time>::from_si(
      acoustic.minimum(viscous));
}

[[nodiscard]] inline common::SeriesQuantity<common::Time>
step_limit(const State &state, const spatial::BoxGrid &grid, const Model &model,
           const SutherlandLaw &law, double courant = 0.45,
           double diffusion = 0.5) {
  constexpr const char *operation = "natsci::fluids::gas::step_limit";
  detail::validate_sutherland(law, operation);
  if (!(diffusion > 0.0) || !(diffusion <= 0.5) || !std::isfinite(diffusion))
    throw std::invalid_argument(
        "natsci::fluids::gas::step_limit: diffusion must be finite and in "
        "(0, 0.5]");
  const Series acoustic = step_limit(state, grid, model, courant).si_value();
  const auto field = transport(state, grid, model, law);
  const Series &rho = state.density().si_value();
  const auto axes = spatial::detail::spatial_axes(rho, grid);
  double inverse_h2 = 0.0;
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const double width = grid.spacing(axis).si_value();
    inverse_h2 += 1.0 / (width * width);
  }
  const double cv = model.gas_constant.si_value() / (model.gamma - 1.0);
  const Series momentum_diff =
      field.shear.si_value().mul(4.0 / 3.0).add(field.bulk.si_value()).div(rho);
  const Series thermal_diff = field.heat.si_value().div(rho).div(cv);
  const Series rate =
      momentum_diff.maximum(thermal_diff).amax(axes, false).mul(inverse_h2);
  const Series viscous =
      pluss::codem::make_scalar_like(rate, diffusion).div(rate);
  return common::SeriesQuantity<common::Time>::from_si(
      acoustic.minimum(viscous));
}

namespace boundary_detail {

template <class RulesAt>
[[nodiscard]] inline StepResult
step_at(const State &state, const spatial::BoxGrid &grid, RulesAt &&rules_at,
        common::ScalarQuantity<common::Time> start,
        common::ScalarQuantity<common::Time> width, const Model &model,
        const Options &options) {
  constexpr const char *operation = "natsci::fluids::gas::step";
  detail::validate_model(model, operation);
  boundary_detail::validate_options(options, operation);
  if (!(width.si_value() > 0.0) || !std::isfinite(width.si_value()))
    throw std::invalid_argument(
        "natsci::fluids::gas::step: width must be finite and positive");
  const auto limit = options.sutherland
                         ? step_limit(state, grid, model, *options.sutherland,
                                      options.courant, options.diffusion)
                         : step_limit(state, grid, model, options.transport,
                                      options.courant, options.diffusion);
  if (options.enforce &&
      !limit.si_value().ge(width.si_value()).all().item<bool>())
    throw std::domain_error(
        "natsci::fluids::gas::step: width exceeds the acoustic CFL limit");

  const detail::Raw initial = Access::raw(state);
  std::int64_t evaluations = 0;
  const auto advance = [&](const detail::Raw &at, double time) {
    const auto &rules = rules_at(
        common::ScalarQuantity<common::Time>::from_si(time));
    ++evaluations;
    const detail::Raw high =
        rate(at, grid, rules, model, options.recon, options.flux, options.roe,
             options.transport, options.sutherland);
    return detail::admissible_advance(
        at, high, width.si_value(), grid, model,
        [&] {
          ++evaluations;
          return rate(at, grid, rules, model, Recon::Cell,
                      FluxScheme::Rusanov, RoePolicy{}, options.transport,
                      options.sutherland);
        },
        "natsci::fluids::gas::step positivity fallback");
  };
  const double t0 = start.si_value();
  const double dt = width.si_value();
  detail::Raw next;
  if (options.rule == Rule::Euler) {
    next = advance(initial, t0);
  } else if (options.rule == Rule::Rk2) {
    const auto first = advance(initial, t0);
    const auto second = advance(first, t0 + dt);
    next = detail::blend(initial, second, 0.5, 0.5);
  } else {
    const auto first = advance(initial, t0);
    const auto first_end = advance(first, t0 + dt);
    const auto second = detail::blend(initial, first_end, 0.75, 0.25);
    detail::require_valid(second, grid, model,
                          "natsci::fluids::gas::step stage 2");
    const auto second_end = advance(second, t0 + 0.5 * dt);
    next = detail::blend(initial, second_end, 1.0 / 3.0, 2.0 / 3.0);
  }
  detail::require_valid(next, grid, model, operation);
  return {Access::make(std::move(next)), limit, evaluations};
}

} // namespace boundary_detail

[[nodiscard]] inline StepResult
step(const State &state, const spatial::BoxGrid &grid, const Bounds &rules,
     common::ScalarQuantity<common::Time> width, const Model &model = {},
     const Options &options = {}) {
  return boundary_detail::step_at(
      state, grid, [&](common::ScalarQuantity<common::Time>) -> const Bounds & {
        return rules;
      },
      common::ScalarQuantity<common::Time>::from_si(0.0), width, model,
      options);
}

[[nodiscard]] inline StepResult
step(const State &state, const spatial::BoxGrid &grid, const BoundFn &rules,
     common::ScalarQuantity<common::Time> start,
     common::ScalarQuantity<common::Time> width, const Model &model = {},
     const Options &options = {}) {
  if (!rules)
    throw std::invalid_argument(
        "natsci::fluids::gas::step: boundary function is empty");
  return boundary_detail::step_at(state, grid, rules, start, width, model,
                                  options);
}

[[nodiscard]] inline FlowPath
evolve(const State &initial, const spatial::BoxGrid &grid, const Bounds &rules,
       common::ScalarQuantity<common::Time> start,
       common::ScalarQuantity<common::Time> width, std::int64_t steps,
       const Model &model = {}, const Options &options = {}) {
  boundary_detail::validate_options(options, "natsci::fluids::gas::evolve");
  std::int64_t evaluations = 0;
  auto marched = common::solve::march(
      [&](common::ScalarQuantity<common::Time>,
          const common::StateVector &record,
          common::ScalarQuantity<common::Time> step_width) {
        auto advanced = step(boundary_detail::state(record, grid, model), grid,
                             rules, step_width, model, options);
        evaluations += advanced.flux_evaluations;
        return boundary_detail::record(advanced.state);
      },
      boundary_detail::record(initial), start, width, steps);
  return {std::move(marched.trajectory), marched.steps, evaluations};
}

[[nodiscard]] inline FlowPath
evolve(const State &initial, const spatial::BoxGrid &grid, const BoundFn &rules,
       common::ScalarQuantity<common::Time> start,
       common::ScalarQuantity<common::Time> width, std::int64_t steps,
       const Model &model = {}, const Options &options = {}) {
  if (!rules)
    throw std::invalid_argument(
        "natsci::fluids::gas::evolve: boundary function is empty");
  boundary_detail::validate_options(options, "natsci::fluids::gas::evolve");
  std::int64_t evaluations = 0;
  auto marched = common::solve::march(
      [&](common::ScalarQuantity<common::Time> time,
          const common::StateVector &record,
          common::ScalarQuantity<common::Time> step_width) {
        auto advanced = step(boundary_detail::state(record, grid, model), grid,
                             rules, time, step_width, model, options);
        evaluations += advanced.flux_evaluations;
        return boundary_detail::record(advanced.state);
      },
      boundary_detail::record(initial), start, width, steps);
  return {std::move(marched.trajectory), marched.steps, evaluations};
}

[[nodiscard]] inline FlowPath
evolve_to(const State &initial, const spatial::BoxGrid &grid,
          const Bounds &rules, common::ScalarQuantity<common::Time> start,
          common::ScalarQuantity<common::Time> stop, const Model &model = {},
          const RunOptions &options = {}) {
  detail::validate_run(options);
  boundary_detail::validate_options(options.step,
                                    "natsci::fluids::gas::evolve_to");
  std::int64_t evaluations = 0;
  auto marched = common::solve::march_to(
      [&](common::ScalarQuantity<common::Time>,
          const common::StateVector &record,
          common::ScalarQuantity<common::Time> width) {
        auto advanced = step(boundary_detail::state(record, grid, model), grid,
                             rules, width, model, options.step);
        evaluations += advanced.flux_evaluations;
        return boundary_detail::record(advanced.state);
      },
      [&](common::ScalarQuantity<common::Time> time,
          const common::StateVector &record) {
        const State at = boundary_detail::state(record, grid, model);
        const double stable =
            (options.step.sutherland
                 ? step_limit(at, grid, model, *options.step.sutherland,
                              options.step.courant, options.step.diffusion)
                 : step_limit(at, grid, model, options.step.transport,
                              options.step.courant, options.step.diffusion))
                .si_value()
                .min()
                .template item<double>();
        const double remaining = stop.si_value() - time.si_value();
        const double width =
            std::min(remaining, std::min(options.max_step.si_value(),
                                         options.safety * stable));
        if (!(width > 0.0) || !std::isfinite(width))
          throw std::domain_error(
              "natsci::fluids::gas::evolve_to: stability policy produced no "
              "finite positive step");
        if (width < options.min_step.si_value() &&
            remaining > options.min_step.si_value())
          throw std::domain_error(
              "natsci::fluids::gas::evolve_to: stable step is below min_step");
        return common::ScalarQuantity<common::Time>::from_si(width);
      },
      boundary_detail::record(initial), start, stop, options.max_steps);
  return {std::move(marched.trajectory), marched.steps, evaluations};
}

[[nodiscard]] inline FlowPath
evolve_to(const State &initial, const spatial::BoxGrid &grid,
          const BoundFn &rules, common::ScalarQuantity<common::Time> start,
          common::ScalarQuantity<common::Time> stop, const Model &model = {},
          const RunOptions &options = {}) {
  if (!rules)
    throw std::invalid_argument(
        "natsci::fluids::gas::evolve_to: boundary function is empty");
  detail::validate_run(options);
  boundary_detail::validate_options(options.step,
                                    "natsci::fluids::gas::evolve_to");
  std::int64_t evaluations = 0;
  auto marched = common::solve::march_to(
      [&](common::ScalarQuantity<common::Time> time,
          const common::StateVector &record,
          common::ScalarQuantity<common::Time> width) {
        auto advanced = step(boundary_detail::state(record, grid, model), grid,
                             rules, time, width, model, options.step);
        evaluations += advanced.flux_evaluations;
        return boundary_detail::record(advanced.state);
      },
      [&](common::ScalarQuantity<common::Time> time,
          const common::StateVector &record) {
        const State at = boundary_detail::state(record, grid, model);
        const double stable =
            (options.step.sutherland
                 ? step_limit(at, grid, model, *options.step.sutherland,
                              options.step.courant, options.step.diffusion)
                 : step_limit(at, grid, model, options.step.transport,
                              options.step.courant, options.step.diffusion))
                .si_value()
                .min()
                .template item<double>();
        const double remaining = stop.si_value() - time.si_value();
        const double width =
            std::min(remaining, std::min(options.max_step.si_value(),
                                         options.safety * stable));
        if (!(width > 0.0) || !std::isfinite(width))
          throw std::domain_error(
              "natsci::fluids::gas::evolve_to: stability policy produced no "
              "finite positive step");
        if (width < options.min_step.si_value() &&
            remaining > options.min_step.si_value())
          throw std::domain_error(
              "natsci::fluids::gas::evolve_to: stable step is below min_step");
        return common::ScalarQuantity<common::Time>::from_si(width);
      },
      boundary_detail::record(initial), start, stop, options.max_steps);
  return {std::move(marched.trajectory), marched.steps, evaluations};
}

} // namespace pluss::subsp::natsci::physics::fluids::gas
