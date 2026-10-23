#pragma once

// Conservative thermochemical state and homogeneous finite-rate source step.
// This is the chemistry half of reacting-flow operator splitting. It does not
// reuse the calorically-perfect gas State: total energy here includes NASA-7
// formation energy and temperature is recovered from e(rho,E,rhoY).
//
// For a closed adiabatic cell, chemistry changes rho*Y through M_k*omega_k
// while rho, rho*u, and rho*E remain invariant. The stiff integration is
// delegated to the shared chemistry reactor, then projected back onto the
// exact conserved density before the EOS recovers temperature. This mirrors
// Cantera's IdealGasReactor source convention and prepares a conservative
// Strang split with the spatial reacting-flux operator.

#include "pluss/subsp/natsci/chemistry/ideal_gas.h"
#include "pluss/subsp/natsci/chemistry/reactor.h"
#include "pluss/subsp/natsci/physics/fluids/compressible_species.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

namespace pluss::subsp::natsci::physics::fluids::gas::reacting {

namespace chemistry = ::pluss::subsp::natsci::chemistry;
namespace stiff = ::pluss::subsp::natsci::common::stiff;

class Model final {
public:
  explicit Model(chemistry::kinetics::Mechanism mechanism,
                 double density_floor = 1e-12, double pressure_floor = 1e-12)
      : mechanism_(std::move(mechanism)), eos_(mechanism_),
        density_floor_(density_floor), pressure_floor_(pressure_floor) {
    if (!(density_floor_ >= 0.0) || !std::isfinite(density_floor_) ||
        !(pressure_floor_ >= 0.0) || !std::isfinite(pressure_floor_))
      throw std::invalid_argument(
          "natsci::fluids::gas::reacting: density and pressure floors must "
          "be finite and non-negative");
    const Series imbalance =
        mechanism_.stoichiometry().matmul(eos_.molar_masses());
    const Series scale = mechanism_.stoichiometry()
                             .abs()
                             .matmul(eos_.molar_masses())
                             .clamp(1e-30, 1e300);
    const double tolerance =
        imbalance.scalar_type() == ScalarType::Float32 ? 2e-5 : 2e-12;
    if (!imbalance.abs().le(scale.mul(tolerance)).all().item<bool>())
      throw std::invalid_argument(
          "natsci::fluids::gas::reacting: mechanism reactions must conserve "
          "mass with the supplied molar masses");
  }

  [[nodiscard]] const chemistry::kinetics::Mechanism &mechanism() const {
    return mechanism_;
  }
  [[nodiscard]] const chemistry::thermo::IdealGas &eos() const { return eos_; }
  [[nodiscard]] double density_floor() const noexcept { return density_floor_; }
  [[nodiscard]] double pressure_floor() const noexcept {
    return pressure_floor_;
  }

private:
  chemistry::kinetics::Mechanism mechanism_;
  chemistry::thermo::IdealGas eos_;
  double density_floor_{1e-12};
  double pressure_floor_{1e-12};
};

namespace detail {

struct Raw final {
  Series rho;
  Series mom;
  Series energy;
  Series partial;
};

inline std::int64_t component_axis(const Raw &state,
                                   const spatial::PeriodicGrid &grid,
                                   const char *operation) {
  return gas::detail::component_axis({state.rho, state.mom, state.energy}, grid,
                                     operation);
}

inline Series mass_fractions(const Raw &state,
                             const spatial::PeriodicGrid &grid) {
  return species::detail::fractions(state.partial, state.rho, grid);
}

inline Series specific_internal_energy(const Raw &state,
                                       const spatial::PeriodicGrid &grid,
                                       const char *operation) {
  const std::int64_t component = component_axis(state, grid, operation);
  const Series kinetic =
      state.mom.square().sum(component, false).div(state.rho).mul(0.5);
  return state.energy.sub(kinetic).div(state.rho);
}

inline Series temperature(const Raw &state, const spatial::PeriodicGrid &grid,
                          const Model &model, const char *operation) {
  const Series y =
      species::detail::to_trailing_species(mass_fractions(state, grid), grid);
  return model.eos().temperature(
      specific_internal_energy(state, grid, operation), y);
}

struct ThermoState final {
  Series mass_fractions;
  Series temperature;
  Series pressure;
};

inline ThermoState checked_thermo(const Raw &state,
                                  const spatial::PeriodicGrid &grid,
                                  const Model &model, const char *operation) {
  component_axis(state, grid, operation);
  species::detail::require_partial(state.partial, state.rho, grid, operation);
  if (!state.rho.gt(model.density_floor()).all().item<bool>())
    throw std::domain_error(std::string(operation) +
                            ": density must exceed the model floor");
  const Series y =
      species::detail::to_trailing_species(mass_fractions(state, grid), grid);
  const Series t = model.eos().temperature(
      specific_internal_energy(state, grid, operation), y);
  const Series p = model.eos().pressure(state.rho, t, y);
  if (!p.gt(model.pressure_floor()).all().item<bool>())
    throw std::domain_error(std::string(operation) +
                            ": pressure must exceed the model floor");
  return {y, t, p};
}

inline void require_valid(const Raw &state, const spatial::PeriodicGrid &grid,
                          const Model &model, const char *operation) {
  (void)checked_thermo(state, grid, model, operation);
}

} // namespace detail

class State final {
public:
  [[nodiscard]] const common::SeriesQuantity<Density> &density() const {
    return density_;
  }
  [[nodiscard]] const common::SeriesQuantity<MomDensity> &momentum() const {
    return momentum_;
  }
  [[nodiscard]] const common::SeriesQuantity<EnergyDensity> &energy() const {
    return energy_;
  }
  [[nodiscard]] const common::SeriesQuantity<Density> &partial_density() const {
    return partial_density_;
  }

private:
  State(common::SeriesQuantity<Density> density,
        common::SeriesQuantity<MomDensity> momentum,
        common::SeriesQuantity<EnergyDensity> energy,
        common::SeriesQuantity<Density> partial_density)
      : density_(std::move(density)), momentum_(std::move(momentum)),
        energy_(std::move(energy)),
        partial_density_(std::move(partial_density)) {}

  common::SeriesQuantity<Density> density_;
  common::SeriesQuantity<MomDensity> momentum_;
  common::SeriesQuantity<EnergyDensity> energy_;
  common::SeriesQuantity<Density> partial_density_;

  friend struct Access;
};

struct Access final {
  static detail::Raw raw(const State &state) {
    return {state.density().si_value(), state.momentum().si_value(),
            state.energy().si_value(), state.partial_density().si_value()};
  }
  static State make(detail::Raw state) {
    return State(
        common::SeriesQuantity<Density>::from_si(std::move(state.rho)),
        common::SeriesQuantity<MomDensity>::from_si(std::move(state.mom)),
        common::SeriesQuantity<EnergyDensity>::from_si(std::move(state.energy)),
        common::SeriesQuantity<Density>::from_si(std::move(state.partial)));
  }
};

[[nodiscard]] inline State
from_conserved(const common::SeriesQuantity<Density> &density,
               const common::SeriesQuantity<MomDensity> &momentum,
               const common::SeriesQuantity<EnergyDensity> &energy,
               const common::SeriesQuantity<Density> &partial_density,
               const spatial::PeriodicGrid &grid, const Model &model) {
  detail::Raw raw{density.si_value(), momentum.si_value(), energy.si_value(),
                  partial_density.si_value()};
  detail::require_valid(raw, grid, model,
                        "natsci::fluids::gas::reacting::from_conserved");
  return Access::make(std::move(raw));
}

[[nodiscard]] inline State
primitive(const common::SeriesQuantity<Density> &density,
          const common::SeriesQuantity<common::Speed> &velocity,
          const common::SeriesQuantity<common::Temperature> &temperature,
          const Series &mass_fractions, const spatial::PeriodicGrid &grid,
          const Model &model) {
  constexpr const char *operation = "natsci::fluids::gas::reacting::primitive";
  const Series &rho = density.si_value();
  const Series &speed = velocity.si_value();
  const Series &temp = temperature.si_value();
  const std::int64_t component =
      spatial::detail::require_vector_layout(speed, grid, operation);
  spatial::detail::require_scalar_layout(rho, grid, operation);
  spatial::detail::require_scalar_layout(temp, grid, operation);
  if (!gas::detail::same_layout(speed.select(component, 0), rho) ||
      !gas::detail::same_layout(temp, rho))
    throw std::invalid_argument(std::string(operation) +
                                ": primitive fields must share one layout");
  species::detail::require_layout(mass_fractions, rho, grid, operation);

  const Series y = species::detail::to_trailing_species(mass_fractions, grid);
  const Series internal = model.eos().internal_energy(temp, y);
  const Series kinetic = speed.square().sum(component, false).mul(0.5);
  detail::Raw raw{rho, speed.mul(rho.unsqueeze(component)),
                  rho.mul(internal.add(kinetic)),
                  mass_fractions.mul(rho.unsqueeze(
                      species::detail::species_axis(mass_fractions, grid)))};
  detail::require_valid(raw, grid, model, operation);
  return Access::make(std::move(raw));
}

[[nodiscard]] inline Series mass_fractions(const State &state,
                                           const spatial::PeriodicGrid &grid) {
  return detail::mass_fractions(Access::raw(state), grid);
}

[[nodiscard]] inline common::SeriesQuantity<common::Speed>
velocity(const State &state, const spatial::PeriodicGrid &grid) {
  const auto raw = Access::raw(state);
  const std::int64_t component = detail::component_axis(
      raw, grid, "natsci::fluids::gas::reacting::velocity");
  return common::SeriesQuantity<common::Speed>::from_si(
      raw.mom.div(raw.rho.unsqueeze(component)));
}

[[nodiscard]] inline common::SeriesQuantity<common::Temperature>
temperature(const State &state, const spatial::PeriodicGrid &grid,
            const Model &model) {
  const auto raw = Access::raw(state);
  auto thermo = detail::checked_thermo(
      raw, grid, model, "natsci::fluids::gas::reacting::temperature");
  return common::SeriesQuantity<common::Temperature>::from_si(
      std::move(thermo.temperature));
}

[[nodiscard]] inline common::SeriesQuantity<common::Pressure>
pressure(const State &state, const spatial::PeriodicGrid &grid,
         const Model &model) {
  const auto raw = Access::raw(state);
  auto thermo = detail::checked_thermo(
      raw, grid, model, "natsci::fluids::gas::reacting::pressure");
  return common::SeriesQuantity<common::Pressure>::from_si(
      std::move(thermo.pressure));
}

[[nodiscard]] inline common::SeriesQuantity<common::Speed>
sound(const State &state, const spatial::PeriodicGrid &grid,
      const Model &model) {
  const auto raw = Access::raw(state);
  const auto thermo = detail::checked_thermo(
      raw, grid, model, "natsci::fluids::gas::reacting::sound");
  return common::SeriesQuantity<common::Speed>::from_si(
      model.eos().sound_speed(thermo.temperature, thermo.mass_fractions));
}

struct SourceRate final {
  common::SeriesQuantity<DensityRate> density;
  common::SeriesQuantity<MomRate> momentum;
  common::SeriesQuantity<EnergyRate> energy;
  common::SeriesQuantity<DensityRate> partial_density;
};

[[nodiscard]] inline SourceRate source_rate(const State &state,
                                            const spatial::PeriodicGrid &grid,
                                            const Model &model) {
  const auto raw = Access::raw(state);
  const auto thermo = detail::checked_thermo(
      raw, grid, model, "natsci::fluids::gas::reacting::source_rate");
  const Series c = model.eos().concentrations(raw.rho, thermo.mass_fractions);
  const Series mass = model.mechanism()
                          .net_production_rates(thermo.temperature, c)
                          .mul(model.eos().molar_masses());
  return {common::SeriesQuantity<DensityRate>::from_si(
              pluss::series::zeros_like(raw.rho)),
          common::SeriesQuantity<MomRate>::from_si(
              pluss::series::zeros_like(raw.mom)),
          common::SeriesQuantity<EnergyRate>::from_si(
              pluss::series::zeros_like(raw.energy)),
          common::SeriesQuantity<DensityRate>::from_si(
              species::detail::from_trailing_species(mass, grid))};
}

struct ReactionResult final {
  State state;
  std::int64_t steps{0};
  std::int64_t rejected{0};
  std::int64_t rhs_evaluations{0};
  std::int64_t jacobian_evaluations{0};
};

[[nodiscard]] inline ReactionResult
react(const State &state, const spatial::PeriodicGrid &grid,
      common::ScalarQuantity<common::Time> width, const Model &model,
      const stiff::Options &options = {}) {
  constexpr const char *operation = "natsci::fluids::gas::reacting::react";
  if (!(width.si_value() > 0.0) || !std::isfinite(width.si_value()))
    throw std::invalid_argument(std::string(operation) +
                                ": width must be finite and positive");
  const auto raw = Access::raw(state);
  const auto thermo = detail::checked_thermo(raw, grid, model, operation);
  const Series c = model.eos().concentrations(raw.rho, thermo.mass_fractions);
  const auto integrated = chemistry::reactor::integrate(
      model.mechanism(), chemistry::reactor::make_state(thermo.temperature, c),
      width.si_value(), true, options);
  if (!integrated.converged)
    throw std::runtime_error(std::string(operation) +
                             ": stiff chemistry integration did not "
                             "converge within the step budget");

  const Series final_c = chemistry::reactor::concentrations(integrated.y);
  const Series raw_mass =
      final_c.mul(model.eos().molar_masses()).clamp(0.0, 1e300);
  const Series total_mass = raw_mass.sum(-1, true);
  if (!total_mass.gt(0.0).all().item<bool>())
    throw std::domain_error(std::string(operation) +
                            ": chemistry produced no positive mixture mass");
  const Series final_y = raw_mass.div(total_mass);
  detail::Raw next{raw.rho, raw.mom, raw.energy,
                   species::detail::from_trailing_species(
                       final_y.mul(raw.rho.unsqueeze(-1)), grid)};
  detail::require_valid(next, grid, model, operation);
  return {Access::make(std::move(next)), integrated.steps, integrated.rejected,
          integrated.rhs_evaluations, integrated.jacobian_evaluations};
}

} // namespace pluss::subsp::natsci::physics::fluids::gas::reacting
