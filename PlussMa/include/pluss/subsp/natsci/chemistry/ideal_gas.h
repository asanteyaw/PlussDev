#pragma once

// Composition-dependent ideal-gas closure for reacting mixtures. The model
// keeps the species axis trailing and evaluates every batch/spatial lane with
// Series operations. Internal energy includes species formation energy from
// the NASA-7 polynomials, so an adiabatic reaction may hold conserved energy
// fixed and recover its temperature after the composition changes.
//
// The formulas follow Cantera's IdealGasPhase and PelePhysics' Fuego EOS:
//   1/M = sum_k Y_k/M_k
//   p = rho R T / M
//   e = sum_k Y_k (R T/M_k) (h_k/RT - 1)
//   cv = sum_k Y_k (R/M_k) (cp_k/R - 1)
//   a = sqrt(gamma p/rho), gamma = cp/cv.
// Temperature recovery uses a bracketed Newton iteration over the common
// NASA validity interval. It is lane-wise and differentiable through Series;
// no cell or species is copied to host memory.

#include "pluss/subsp/natsci/chemistry/kinetics.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace pluss::subsp::natsci::chemistry::thermo {

class IdealGas final {
public:
  struct Properties final {
    Series mean_molar_mass = pluss::series::undefined(); // kg/mol
    Series internal_energy = pluss::series::undefined(); // J/kg
    Series enthalpy = pluss::series::undefined();        // J/kg
    Series cv = pluss::series::undefined();              // J/(kg K)
    Series cp = pluss::series::undefined();              // J/(kg K)

    [[nodiscard]] Series gamma() const { return cp.div(cv); }
  };

  explicit IdealGas(const kinetics::Mechanism &mechanism)
      : nasa_(mechanism.thermo()), masses_(mechanism.molar_masses()),
        names_(mechanism.names()) {
    require(masses_.defined() && masses_.dim() == 1 &&
                masses_.size(0) == nasa_.species(),
            "mechanism molar masses must match its NASA species");
    require(masses_.isfinite().logical_and(masses_.gt(0.0)).all().item<bool>(),
            "molar masses must be finite and positive");
    inv_masses_ = masses_.reciprocal();
  }

  [[nodiscard]] std::int64_t species() const noexcept {
    return nasa_.species();
  }
  [[nodiscard]] const std::vector<std::string> &names() const noexcept {
    return names_;
  }
  [[nodiscard]] const Series &molar_masses() const noexcept { return masses_; }
  [[nodiscard]] double t_min() const noexcept { return nasa_.t_min(); }
  [[nodiscard]] double t_max() const noexcept { return nasa_.t_max(); }

  [[nodiscard]] Series mean_molar_mass(const Series &mass_fractions) const {
    check_fractions(mass_fractions);
    return mass_fractions.mul(inv_masses_).sum(-1, false).reciprocal();
  }

  [[nodiscard]] Series concentrations(const Series &density,
                                      const Series &mass_fractions) const {
    check_fractions(mass_fractions);
    check_field(density, mass_fractions, "density", true);
    return mass_fractions.mul(density.unsqueeze(-1)).mul(inv_masses_);
  }

  [[nodiscard]] Series density(const Series &pressure,
                               const Series &temperature,
                               const Series &mass_fractions) const {
    check_fractions(mass_fractions);
    check_field(pressure, mass_fractions, "pressure", true);
    check_temperature(temperature, mass_fractions);
    return pressure.mul(mean_molar_mass_unchecked(mass_fractions))
        .div(temperature.mul(gas_constant()));
  }

  [[nodiscard]] Series pressure(const Series &density,
                                const Series &temperature,
                                const Series &mass_fractions) const {
    check_fractions(mass_fractions);
    check_field(density, mass_fractions, "density", true);
    check_temperature(temperature, mass_fractions);
    return density.mul(temperature)
        .mul(gas_constant())
        .div(mean_molar_mass_unchecked(mass_fractions));
  }

  [[nodiscard]] Properties properties(const Series &temperature,
                                      const Series &mass_fractions) const {
    check_fractions(mass_fractions);
    check_temperature(temperature, mass_fractions);
    return properties_unchecked(temperature, mass_fractions);
  }

  [[nodiscard]] Series internal_energy(const Series &temperature,
                                       const Series &mass_fractions) const {
    return properties(temperature, mass_fractions).internal_energy;
  }

  [[nodiscard]] Series enthalpy(const Series &temperature,
                                const Series &mass_fractions) const {
    return properties(temperature, mass_fractions).enthalpy;
  }

  [[nodiscard]] Series sound_speed(const Series &temperature,
                                   const Series &mass_fractions) const {
    const auto p = properties(temperature, mass_fractions);
    return p.gamma()
        .mul(temperature)
        .mul(gas_constant())
        .div(p.mean_molar_mass)
        .sqrt();
  }

  // Invert e(T,Y) on the mechanism's common NASA interval. A supplied guess
  // is useful after a conservative update; otherwise a secant interpolation
  // between the two endpoint energies initializes every lane.
  [[nodiscard]] Series
  temperature(const Series &specific_internal_energy,
              const Series &mass_fractions,
              const Series &guess = pluss::series::undefined()) const {
    check_fractions(mass_fractions);
    check_field(specific_internal_energy, mass_fractions,
                "specific internal energy", false);

    const Series lower0 =
        pluss::series::zeros_like(specific_internal_energy).add(t_min());
    const Series upper0 =
        pluss::series::zeros_like(specific_internal_energy).add(t_max());
    const Series e_lower =
        properties_unchecked(lower0, mass_fractions).internal_energy;
    const Series e_upper =
        properties_unchecked(upper0, mass_fractions).internal_energy;
    require(e_upper.gt(e_lower).all().item<bool>(),
            "internal energy must increase over the NASA validity interval");

    const double range_tolerance =
        specific_internal_energy.scalar_type() == ScalarType::Float32 ? 3e-5
                                                                      : 3e-12;
    const Series scale = specific_internal_energy.abs().clamp(1.0, 1e300);
    const Series slack = scale.mul(range_tolerance);
    const Series in_range =
        specific_internal_energy.ge(e_lower.sub(slack))
            .logical_and(specific_internal_energy.le(e_upper.add(slack)));
    require(in_range.all().item<bool>(),
            "specific internal energy is outside the recoverable NASA "
            "temperature interval");

    Series lower = lower0;
    Series upper = upper0;
    Series current;
    if (guess.defined()) {
      check_temperature_layout(guess, mass_fractions, "temperature guess");
      require(guess.isfinite().all().item<bool>(),
              "temperature guess must be finite");
      current = guess.clamp(t_min(), t_max());
    } else {
      const Series fraction =
          specific_internal_energy.sub(e_lower).div(e_upper.sub(e_lower));
      current = lower.add(upper.sub(lower).mul(fraction.clamp(0.0, 1.0)));
    }

    const double relative_tolerance =
        specific_internal_energy.scalar_type() == ScalarType::Float32 ? 2e-5
                                                                      : 2e-12;
    const Series energy_tolerance = scale.mul(relative_tolerance);
    const int iterations =
        specific_internal_energy.scalar_type() == ScalarType::Float32 ? 18 : 28;
    for (int iteration = 0; iteration < iterations; ++iteration) {
      const auto state = properties_unchecked(current, mass_fractions);
      const Series residual =
          state.internal_energy.sub(specific_internal_energy);
      const Series done = residual.abs().le(energy_tolerance);
      const Series below = residual.lt(0.0);
      lower = done.where(current, below.where(current, lower));
      upper = done.where(current, below.where(upper, current));

      const Series midpoint = lower.add(upper).mul(0.5);
      const Series newton = current.sub(residual.div(state.cv));
      const Series valid = newton.isfinite()
                               .logical_and(newton.ge(lower))
                               .logical_and(newton.le(upper));
      const Series candidate = valid.where(newton, midpoint);
      current = done.where(current, candidate);
    }

    const Series residual = properties_unchecked(current, mass_fractions)
                                .internal_energy.sub(specific_internal_energy);
    require(residual.abs().le(energy_tolerance.mul(2.0)).all().item<bool>(),
            "temperature recovery did not converge");
    return current;
  }

private:
  [[nodiscard]] Series
  mean_molar_mass_unchecked(const Series &mass_fractions) const {
    return mass_fractions.mul(inv_masses_).sum(-1, false).reciprocal();
  }

  [[nodiscard]] Properties
  properties_unchecked(const Series &temperature,
                       const Series &mass_fractions) const {
    const auto nasa = nasa_.eval(temperature);
    const double r = gas_constant();
    const Series species_r = inv_masses_.mul(r);
    const Series temp = temperature.unsqueeze(-1);
    const Series species_internal = nasa.h_rt.sub(1.0).mul(species_r).mul(temp);
    const Series species_enthalpy = nasa.h_rt.mul(species_r).mul(temp);
    const Series species_cv = nasa.cp_r.sub(1.0).mul(species_r);
    const Series species_cp = nasa.cp_r.mul(species_r);
    return {mean_molar_mass_unchecked(mass_fractions),
            mass_fractions.mul(species_internal).sum(-1, false),
            mass_fractions.mul(species_enthalpy).sum(-1, false),
            mass_fractions.mul(species_cv).sum(-1, false),
            mass_fractions.mul(species_cp).sum(-1, false)};
  }

  void check_fractions(const Series &mass_fractions) const {
    require(mass_fractions.defined() && mass_fractions.dim() > 0 &&
                mass_fractions.size(-1) == species(),
            "mass fractions need a trailing species axis matching the model");
    require(mass_fractions.dtype().is_floating() &&
                mass_fractions.scalar_type() == masses_.scalar_type() &&
                mass_fractions.platform() == masses_.platform(),
            "mass fractions must match the model floating dtype and platform");
    require(mass_fractions.isfinite()
                .logical_and(mass_fractions.ge(0.0))
                .all()
                .item<bool>(),
            "mass fractions must be finite and non-negative");
    const double tolerance =
        mass_fractions.scalar_type() == ScalarType::Float32 ? 2e-5 : 2e-12;
    const Series total = mass_fractions.sum(-1, false);
    require(
        total.allclose(pluss::series::ones_like(total), tolerance, tolerance),
        "mass fractions must sum to one in every lane");
  }

  void check_field(const Series &field, const Series &mass_fractions,
                   const char *name, bool positive) const {
    require(field.defined() && field.dtype().is_floating() &&
                field.scalar_type() == masses_.scalar_type() &&
                field.platform() == masses_.platform(),
            std::string(name) +
                " must match the model floating dtype and platform");
    const auto sizes = mass_fractions.sizes();
    std::vector<std::int64_t> shape(sizes.begin(), sizes.end());
    shape.pop_back();
    require(field.sizes() == shape,
            std::string(name) +
                " shape must match every non-species mass-fraction axis");
    Series valid = field.isfinite();
    if (positive)
      valid = valid.logical_and(field.gt(0.0));
    require(valid.all().item<bool>(),
            std::string(name) + " must be " +
                (positive ? "finite and positive" : "finite"));
  }

  void check_temperature_layout(const Series &temperature,
                                const Series &mass_fractions,
                                const char *name) const {
    check_field(temperature, mass_fractions, name, false);
  }

  void check_temperature(const Series &temperature,
                         const Series &mass_fractions) const {
    check_temperature_layout(temperature, mass_fractions, "temperature");
    nasa_.check(temperature);
  }

  static void require(bool condition, const std::string &message) {
    if (!condition)
      throw std::invalid_argument("natsci::chemistry::ideal_gas: " + message);
  }

  Nasa7 nasa_;
  Series masses_ = pluss::series::undefined();
  Series inv_masses_ = pluss::series::undefined();
  std::vector<std::string> names_;
};

} // namespace pluss::subsp::natsci::chemistry::thermo
