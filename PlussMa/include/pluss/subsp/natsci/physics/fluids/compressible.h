#pragma once

#include "pluss/subsp/natsci/common/poisson.h"
#include "pluss/subsp/natsci/common/solve.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::physics::fluids::gas {

namespace common = ::pluss::subsp::natsci::common;
namespace spatial = ::pluss::subsp::natsci::common::spatial;

using Density = common::DimensionQuotient<common::Mass, common::Volume>;
using MomDensity = common::DimensionProduct<Density, common::Speed>;
using EnergyDensity = common::Pressure;

struct Model final {
  double gamma{1.4};
  double rho_floor{1e-12};
  double p_floor{1e-12};
};

enum class Recon : std::uint8_t { Cell = 0, Minmod = 1 };
enum class Rule : std::uint8_t { Euler = 0, Rk2 = 1, Rk3 = 2 };

struct Options final {
  Rule rule{Rule::Rk2};
  Recon recon{Recon::Minmod};
  double courant{0.45};
  bool enforce{true};
};

struct RunOptions final {
  double safety{0.9};
  common::ScalarQuantity<common::Time> min_step{
      common::ScalarQuantity<common::Time>::from_si(0.0)};
  common::ScalarQuantity<common::Time> max_step{
      common::ScalarQuantity<common::Time>::from_si(
          std::numeric_limits<double>::infinity())};
  std::int64_t max_steps{100'000};
  Options step{};
};

namespace detail {

inline bool same_layout(const Series &lhs, const Series &rhs) noexcept {
  if (lhs.dim() != rhs.dim() || lhs.scalar_type() != rhs.scalar_type() ||
      lhs.platform() != rhs.platform())
    return false;
  for (std::int64_t axis = 0; axis < lhs.dim(); ++axis)
    if (lhs.size(axis) != rhs.size(axis))
      return false;
  return true;
}

inline void validate_model(const Model &model, const char *operation) {
  if (!(model.gamma > 1.0) || !std::isfinite(model.gamma))
    throw std::invalid_argument(std::string(operation) +
                                ": gamma must be finite and greater than one");
  if (!(model.rho_floor >= 0.0) || !std::isfinite(model.rho_floor) ||
      !(model.p_floor >= 0.0) || !std::isfinite(model.p_floor))
    throw std::invalid_argument(std::string(operation) +
                                ": floors must be finite and non-negative");
}

inline std::int64_t stages(Rule rule, const char *operation) {
  switch (rule) {
  case Rule::Euler:
    return 1;
  case Rule::Rk2:
    return 2;
  case Rule::Rk3:
    return 3;
  }
  throw std::invalid_argument(std::string(operation) +
                              ": rule is not a recognized integrator");
}

inline void validate_options(const Options &options, const char *operation) {
  (void)stages(options.rule, operation);
  switch (options.recon) {
  case Recon::Cell:
  case Recon::Minmod:
    break;
  default:
    throw std::invalid_argument(std::string(operation) +
                                ": recon is not a recognized reconstruction");
  }
  if (!(options.courant > 0.0) || !(options.courant <= 1.0) ||
      !std::isfinite(options.courant))
    throw std::invalid_argument(std::string(operation) +
                                ": courant must be finite and in (0, 1]");
}

inline void validate_run(const RunOptions &options) {
  constexpr const char *operation = "natsci::fluids::gas::evolve_to";
  validate_options(options.step, operation);
  if (!(options.safety > 0.0) || !(options.safety <= 1.0) ||
      !std::isfinite(options.safety))
    throw std::invalid_argument(
        "natsci::fluids::gas::evolve_to: safety must be finite and in (0, 1]");
  if (!(options.min_step.si_value() >= 0.0) ||
      !std::isfinite(options.min_step.si_value()) ||
      !(options.max_step.si_value() > 0.0) ||
      options.min_step.si_value() > options.max_step.si_value())
    throw std::invalid_argument(
        "natsci::fluids::gas::evolve_to: step bounds are invalid");
  if (options.max_steps <= 0)
    throw std::invalid_argument(
        "natsci::fluids::gas::evolve_to: max_steps must be positive");
}

struct Raw final {
  Series rho;
  Series mom;
  Series energy;
};

inline std::int64_t component_axis(const Raw &state,
                                   const spatial::PeriodicGrid &grid,
                                   const char *operation) {
  spatial::detail::require_scalar_layout(state.rho, grid, operation);
  const std::int64_t axis =
      spatial::detail::require_vector_layout(state.mom, grid, operation);
  spatial::detail::require_scalar_layout(state.energy, grid, operation);
  if (!same_layout(state.rho, state.energy))
    throw std::invalid_argument(std::string(operation) +
                                ": density and energy layouts must match");
  const Series first = state.mom.select(axis, 0);
  if (!same_layout(first, state.rho))
    throw std::invalid_argument(std::string(operation) +
                                ": momentum must add only its component axis");
  return axis;
}

inline Series pressure(const Raw &state, const spatial::PeriodicGrid &grid,
                       const Model &model) {
  const std::int64_t axis =
      component_axis(state, grid, "natsci::fluids::gas::pressure");
  const Series kinetic =
      state.mom.square().sum(axis, false).div(state.rho).mul(0.5);
  return state.energy.sub(kinetic).mul(model.gamma - 1.0);
}

inline Series valid(const Raw &state, const spatial::PeriodicGrid &grid,
                    const Model &model) {
  return state.rho.gt(model.rho_floor)
      .logical_and(pressure(state, grid, model).gt(model.p_floor))
      .logical_and(state.rho.isfinite())
      .logical_and(
          state.mom.isfinite().all(state.mom.dim() - grid.dim() - 1, false))
      .logical_and(state.energy.isfinite());
}

inline void require_valid(const Raw &state, const spatial::PeriodicGrid &grid,
                          const Model &model, const char *operation) {
  validate_model(model, operation);
  component_axis(state, grid, operation);
  if (!valid(state, grid, model).all().item<bool>())
    throw std::domain_error(std::string(operation) +
                            ": every cell must have finite positive density "
                            "and pressure");
}

inline Series minmod(const Series &back, const Series &forward) {
  const Series same_sign = back.mul(forward).gt(0.0);
  return same_sign.where(back.sgn().mul(back.abs().minimum(forward.abs())),
                         pluss::series::zeros_like(back));
}

inline Series slope(const Series &value, const spatial::PeriodicGrid &grid,
                    std::int64_t axis, Recon recon) {
  if (recon == Recon::Cell)
    return pluss::series::zeros_like(value);
  const std::int64_t series_axis = value.dim() - grid.dim() + axis;
  return minmod(value.sub(value.roll(1, series_axis)),
                value.roll(-1, series_axis).sub(value));
}

struct Face final {
  Raw left;
  Raw right;
};

inline Face face(const Raw &state, const spatial::PeriodicGrid &grid,
                 std::int64_t axis, Recon recon, const Model &model) {
  const std::int64_t rho_axis = state.rho.dim() - grid.dim() + axis;
  const std::int64_t mom_axis = state.mom.dim() - grid.dim() + axis;
  Raw left{state.rho.add(slope(state.rho, grid, axis, recon).mul(0.5)),
           state.mom.add(slope(state.mom, grid, axis, recon).mul(0.5)),
           state.energy.add(slope(state.energy, grid, axis, recon).mul(0.5))};
  Raw right{
      state.rho.roll(-1, rho_axis)
          .sub(slope(state.rho, grid, axis, recon).roll(-1, rho_axis).mul(0.5)),
      state.mom.roll(-1, mom_axis)
          .sub(slope(state.mom, grid, axis, recon).roll(-1, mom_axis).mul(0.5)),
      state.energy.roll(-1, rho_axis)
          .sub(slope(state.energy, grid, axis, recon)
                   .roll(-1, rho_axis)
                   .mul(0.5))};

  const Series admissible =
      valid(left, grid, model).logical_and(valid(right, grid, model));
  const std::int64_t component = state.mom.dim() - grid.dim() - 1;
  const Series vector_mask = admissible.unsqueeze(component);
  const Raw first_left{state.rho, state.mom, state.energy};
  const Raw first_right{state.rho.roll(-1, rho_axis),
                        state.mom.roll(-1, mom_axis),
                        state.energy.roll(-1, rho_axis)};
  left = {admissible.where(left.rho, first_left.rho),
          vector_mask.where(left.mom, first_left.mom),
          admissible.where(left.energy, first_left.energy)};
  right = {admissible.where(right.rho, first_right.rho),
           vector_mask.where(right.mom, first_right.mom),
           admissible.where(right.energy, first_right.energy)};
  return {std::move(left), std::move(right)};
}

struct Flux final {
  Series mass;
  Series momentum;
  Series energy;
  Series wave;
};

inline Flux physical_flux(const Raw &state, const spatial::PeriodicGrid &grid,
                          std::int64_t axis, const Model &model) {
  const std::int64_t component = state.mom.dim() - grid.dim() - 1;
  const Series rho_vector = state.rho.unsqueeze(component);
  const Series velocity = state.mom.div(rho_vector);
  const Series normal = velocity.select(component, axis);
  const Series p = pressure(state, grid, model);
  std::vector<Series> momentum;
  momentum.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t row = 0; row < grid.dim(); ++row) {
    Series value = state.mom.select(component, row).mul(normal);
    if (row == axis)
      value = value.add(p);
    momentum.push_back(std::move(value));
  }
  const Series sound = p.mul(model.gamma).div(state.rho).sqrt();
  return {state.mom.select(component, axis),
          spatial::detail::stack_components(momentum, component),
          state.energy.add(p).mul(normal), normal.abs().add(sound)};
}

inline Flux num_flux(const Raw &state, const spatial::PeriodicGrid &grid,
                     std::int64_t axis, const Model &model, Recon recon) {
  Face states = face(state, grid, axis, recon, model);
  Flux left = physical_flux(states.left, grid, axis, model);
  Flux right = physical_flux(states.right, grid, axis, model);
  const Series wave = left.wave.maximum(right.wave);
  const std::int64_t component = state.mom.dim() - grid.dim() - 1;
  return {left.mass.add(right.mass)
              .sub(states.right.rho.sub(states.left.rho).mul(wave))
              .mul(0.5),
          left.momentum.add(right.momentum)
              .sub(states.right.mom.sub(states.left.mom)
                       .mul(wave.unsqueeze(component)))
              .mul(0.5),
          left.energy.add(right.energy)
              .sub(states.right.energy.sub(states.left.energy).mul(wave))
              .mul(0.5),
          std::move(wave)};
}

inline Raw rate(const Raw &state, const spatial::PeriodicGrid &grid,
                const Model &model, Recon recon) {
  Raw result{pluss::series::zeros_like(state.rho),
             pluss::series::zeros_like(state.mom),
             pluss::series::zeros_like(state.energy)};
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    Flux high = num_flux(state, grid, axis, model, recon);
    const std::int64_t scalar_axis = state.rho.dim() - grid.dim() + axis;
    const std::int64_t vector_axis = state.mom.dim() - grid.dim() + axis;
    const double width = grid.spacing(axis).si_value();
    result.rho = result.rho.sub(
        high.mass.sub(high.mass.roll(1, scalar_axis)).div(width));
    result.mom = result.mom.sub(
        high.momentum.sub(high.momentum.roll(1, vector_axis)).div(width));
    result.energy = result.energy.sub(
        high.energy.sub(high.energy.roll(1, scalar_axis)).div(width));
  }
  return result;
}

inline Raw add_rate(const Raw &state, const Raw &rate, double step) {
  return {state.rho.add(rate.rho.mul(step)), state.mom.add(rate.mom.mul(step)),
          state.energy.add(rate.energy.mul(step))};
}

inline Raw blend(const Raw &left, const Raw &right, double left_weight,
                 double right_weight) {
  return {left.rho.mul(left_weight).add(right.rho.mul(right_weight)),
          left.mom.mul(left_weight).add(right.mom.mul(right_weight)),
          left.energy.mul(left_weight).add(right.energy.mul(right_weight))};
}

} // namespace detail

class State final {
public:
  [[nodiscard]] const common::SeriesQuantity<Density> &
  density() const noexcept {
    return density_;
  }
  [[nodiscard]] const common::SeriesQuantity<MomDensity> &
  momentum() const noexcept {
    return momentum_;
  }
  [[nodiscard]] const common::SeriesQuantity<EnergyDensity> &
  energy() const noexcept {
    return energy_;
  }

private:
  State(common::SeriesQuantity<Density> density,
        common::SeriesQuantity<MomDensity> momentum,
        common::SeriesQuantity<EnergyDensity> energy)
      : density_(std::move(density)), momentum_(std::move(momentum)),
        energy_(std::move(energy)) {}

  common::SeriesQuantity<Density> density_;
  common::SeriesQuantity<MomDensity> momentum_;
  common::SeriesQuantity<EnergyDensity> energy_;

  friend State make(const common::SeriesQuantity<Density> &,
                    const common::SeriesQuantity<MomDensity> &,
                    const common::SeriesQuantity<EnergyDensity> &,
                    const spatial::PeriodicGrid &, const Model &);
  friend struct Access;
};

struct Access final {
  static detail::Raw raw(const State &state) {
    return {state.density().si_value(), state.momentum().si_value(),
            state.energy().si_value()};
  }
  static State make(detail::Raw state) {
    return State(
        common::SeriesQuantity<Density>::from_si(std::move(state.rho)),
        common::SeriesQuantity<MomDensity>::from_si(std::move(state.mom)),
        common::SeriesQuantity<EnergyDensity>::from_si(
            std::move(state.energy)));
  }
};

[[nodiscard]] inline State
make(const common::SeriesQuantity<Density> &density,
     const common::SeriesQuantity<MomDensity> &momentum,
     const common::SeriesQuantity<EnergyDensity> &energy,
     const spatial::PeriodicGrid &grid, const Model &model = {}) {
  detail::Raw raw{density.si_value(), momentum.si_value(), energy.si_value()};
  detail::require_valid(raw, grid, model, "natsci::fluids::gas::make");
  return Access::make(std::move(raw));
}

namespace detail {

inline common::StateVector record(const State &state) {
  return common::states(common::field("density", state.density(), "rho"),
                        common::field("momentum", state.momentum(), "rho u"),
                        common::field("energy", state.energy(), "E"));
}

inline State state(const common::StateVector &record,
                   const spatial::PeriodicGrid &grid, const Model &model) {
  return make(record.get<Density>("density"),
              record.get<MomDensity>("momentum"),
              record.get<EnergyDensity>("energy"), grid, model);
}

} // namespace detail

[[nodiscard]] inline common::SeriesQuantity<common::Speed>
velocity(const State &state, const spatial::PeriodicGrid &grid) {
  const auto raw = Access::raw(state);
  const std::int64_t component =
      detail::component_axis(raw, grid, "natsci::fluids::gas::velocity");
  return common::SeriesQuantity<common::Speed>::from_si(
      raw.mom.div(raw.rho.unsqueeze(component)));
}

[[nodiscard]] inline common::SeriesQuantity<common::Pressure>
pressure(const State &state, const spatial::PeriodicGrid &grid,
         const Model &model = {}) {
  detail::validate_model(model, "natsci::fluids::gas::pressure");
  return common::SeriesQuantity<common::Pressure>::from_si(
      detail::pressure(Access::raw(state), grid, model));
}

[[nodiscard]] inline common::SeriesQuantity<common::Speed>
sound(const State &state, const spatial::PeriodicGrid &grid,
      const Model &model = {}) {
  const Series p = pressure(state, grid, model).si_value();
  return common::SeriesQuantity<common::Speed>::from_si(
      p.mul(model.gamma).div(state.density().si_value()).sqrt());
}

[[nodiscard]] inline common::SeriesQuantity<common::Time>
step_limit(const State &state, const spatial::PeriodicGrid &grid,
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
  Series rate = pluss::series::zeros(
                    std::vector<std::int64_t>(
                        state.density().si_value().sizes().begin(),
                        state.density().si_value().sizes().end() - grid.dim()),
                    speed_value.scalar_type())
                    .to(speed_value.platform());
  const auto axes =
      spatial::detail::spatial_axes(state.density().si_value(), grid);
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis)
    rate = rate.add(speed_value.select(component, axis)
                        .abs()
                        .add(sound_value)
                        .amax(axes, false)
                        .div(grid.spacing(axis).si_value()));
  return common::SeriesQuantity<common::Time>::from_si(
      pluss::codem::make_scalar_like(rate, courant).div(rate));
}

struct StepResult final {
  State state;
  common::SeriesQuantity<common::Time> limit;
  std::int64_t flux_evaluations{0};
};

[[nodiscard]] inline StepResult step(const State &state,
                                     const spatial::PeriodicGrid &grid,
                                     common::ScalarQuantity<common::Time> width,
                                     const Model &model = {},
                                     const Options &options = {}) {
  constexpr const char *operation = "natsci::fluids::gas::step";
  detail::validate_model(model, operation);
  detail::validate_options(options, operation);
  if (!(width.si_value() > 0.0) || !std::isfinite(width.si_value()))
    throw std::invalid_argument(
        "natsci::fluids::gas::step: width must be finite and positive");
  const auto limit = step_limit(state, grid, model, options.courant);
  if (options.enforce &&
      !limit.si_value().ge(width.si_value()).all().item<bool>())
    throw std::domain_error(
        "natsci::fluids::gas::step: width exceeds the acoustic CFL limit");

  const detail::Raw initial = Access::raw(state);
  detail::Raw next;
  std::int64_t evaluations = 0;
  if (options.rule == Rule::Euler) {
    next = detail::add_rate(initial,
                            detail::rate(initial, grid, model, options.recon),
                            width.si_value());
    evaluations = 1;
  } else if (options.rule == Rule::Rk2) {
    const detail::Raw first = detail::add_rate(
        initial, detail::rate(initial, grid, model, options.recon),
        width.si_value());
    detail::require_valid(first, grid, model,
                          "natsci::fluids::gas::step stage 1");
    const detail::Raw second =
        detail::add_rate(first, detail::rate(first, grid, model, options.recon),
                         width.si_value());
    next = detail::blend(initial, second, 0.5, 0.5);
    evaluations = 2;
  } else {
    const detail::Raw first = detail::add_rate(
        initial, detail::rate(initial, grid, model, options.recon),
        width.si_value());
    detail::require_valid(first, grid, model,
                          "natsci::fluids::gas::step stage 1");
    const detail::Raw first_end =
        detail::add_rate(first, detail::rate(first, grid, model, options.recon),
                         width.si_value());
    const detail::Raw second = detail::blend(initial, first_end, 0.75, 0.25);
    detail::require_valid(second, grid, model,
                          "natsci::fluids::gas::step stage 2");
    const detail::Raw second_end = detail::add_rate(
        second, detail::rate(second, grid, model, options.recon),
        width.si_value());
    next = detail::blend(initial, second_end, 1.0 / 3.0, 2.0 / 3.0);
    evaluations = 3;
  }
  detail::require_valid(next, grid, model, operation);
  return {Access::make(std::move(next)), limit, evaluations};
}

struct FlowPath final {
  common::Trajectory<common::Time> trajectory;
  std::int64_t steps{0};
  std::int64_t flux_evaluations{0};
};

[[nodiscard]] inline FlowPath
evolve(const State &initial, const spatial::PeriodicGrid &grid,
       common::ScalarQuantity<common::Time> start,
       common::ScalarQuantity<common::Time> width, std::int64_t steps,
       const Model &model = {}, const Options &options = {}) {
  detail::validate_model(model, "natsci::fluids::gas::evolve");
  detail::validate_options(options, "natsci::fluids::gas::evolve");
  std::int64_t evaluations = 0;
  auto marched = common::solve::march(
      [&](common::ScalarQuantity<common::Time>,
          const common::StateVector &record,
          common::ScalarQuantity<common::Time> step_width) {
        auto advanced = step(detail::state(record, grid, model), grid,
                             step_width, model, options);
        evaluations += advanced.flux_evaluations;
        return detail::record(advanced.state);
      },
      detail::record(initial), start, width, steps);
  return {std::move(marched.trajectory), marched.steps, evaluations};
}

[[nodiscard]] inline FlowPath
evolve_to(const State &initial, const spatial::PeriodicGrid &grid,
          common::ScalarQuantity<common::Time> start,
          common::ScalarQuantity<common::Time> stop, const Model &model = {},
          const RunOptions &options = {}) {
  detail::validate_model(model, "natsci::fluids::gas::evolve_to");
  detail::validate_run(options);
  std::int64_t evaluations = 0;
  auto marched = common::solve::march_to(
      [&](common::ScalarQuantity<common::Time>,
          const common::StateVector &record,
          common::ScalarQuantity<common::Time> width) {
        auto advanced = step(detail::state(record, grid, model), grid, width,
                             model, options.step);
        evaluations += advanced.flux_evaluations;
        return detail::record(advanced.state);
      },
      [&](common::ScalarQuantity<common::Time> time,
          const common::StateVector &record) {
        const State at = detail::state(record, grid, model);
        const double stable = step_limit(at, grid, model, options.step.courant)
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
      detail::record(initial), start, stop, options.max_steps);
  return {std::move(marched.trajectory), marched.steps, evaluations};
}

} // namespace pluss::subsp::natsci::physics::fluids::gas
