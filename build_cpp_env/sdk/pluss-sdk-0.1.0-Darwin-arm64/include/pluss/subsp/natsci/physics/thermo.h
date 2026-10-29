#pragma once

#include "pluss/subsp/natsci/common/constants.h"
#include "pluss/subsp/natsci/common/point.h"
#include "pluss/subsp/natsci/common/solve.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace pluss::subsp::natsci::physics::thermo {

namespace common = ::pluss::subsp::natsci::common;

using MassDensity = common::DimensionQuotient<common::Mass, common::Volume>;
using MolarMass = common::DimensionQuotient<common::Mass, common::Amount>;
using SpecificEnergy = common::DimensionQuotient<common::Energy, common::Mass>;
using SpecificHeat = common::DimensionQuotient<common::Entropy, common::Mass>;
using SpecificGasConstant = SpecificHeat;
using Compressibility =
    common::DimensionQuotient<common::Dimensionless, common::Pressure>;
using Expansion =
    common::DimensionQuotient<common::Dimensionless, common::Temperature>;

namespace detail {

template <common::QuantityRep Rep>
inline void require_finite(const Rep &value, const char *operation,
                           const char *role) {
  if constexpr (std::same_as<std::remove_cvref_t<Rep>, double>) {
    if (!std::isfinite(value))
      throw std::invalid_argument(std::string(operation) + ": " + role +
                                  " must be finite");
  } else {
    if (!value.defined() || !value.dtype().is_floating() || value.numel() == 0)
      throw std::invalid_argument(std::string(operation) + ": " + role +
                                  " must be a non-empty real floating Series");
    if (!value.isfinite().all().template item<bool>())
      throw std::invalid_argument(std::string(operation) + ": " + role +
                                  " must be finite");
  }
}

template <common::QuantityRep Rep>
inline void require_positive(const Rep &value, const char *operation,
                             const char *role) {
  if constexpr (std::same_as<std::remove_cvref_t<Rep>, double>) {
    if (!(value > 0.0) || !std::isfinite(value))
      throw std::invalid_argument(std::string(operation) + ": " + role +
                                  " must be finite and positive");
  } else {
    if (!value.defined() || !value.dtype().is_floating() || value.numel() == 0)
      throw std::invalid_argument(std::string(operation) + ": " + role +
                                  " must be a non-empty real floating Series");
    if (!value.isfinite().all().template item<bool>() ||
        !value.gt(0.0).all().template item<bool>())
      throw std::invalid_argument(std::string(operation) + ": " + role +
                                  " must be finite and positive");
  }
}

template <common::QuantityRep Rep>
inline void require_above_one(const Rep &value, const char *operation,
                              const char *role) {
  if constexpr (std::same_as<std::remove_cvref_t<Rep>, double>) {
    if (!(value > 1.0) || !std::isfinite(value))
      throw std::invalid_argument(std::string(operation) + ": " + role +
                                  " must be finite and greater than one");
  } else {
    if (!value.defined() || !value.dtype().is_floating() || value.numel() == 0)
      throw std::invalid_argument(std::string(operation) + ": " + role +
                                  " must be a non-empty real floating Series");
    if (!value.isfinite().all().template item<bool>() ||
        !value.gt(1.0).all().template item<bool>())
      throw std::invalid_argument(std::string(operation) + ": " + role +
                                  " must be finite and greater than one");
  }
}

template <common::QuantityRep Rep>
[[nodiscard]] auto
absolute_temp(const common::TemperaturePoint<Rep> &temperature,
              const char *operation) {
  require_positive(temperature.si_value(), operation, "temperature");
  return common::Quantity<common::Temperature, Rep>::from_si(
      temperature.si_value());
}

[[nodiscard]] inline auto molar_r() {
  return common::constants::codata2022::molar_gas_constant.measurement.value();
}

template <common::QuantityRep Rep>
[[nodiscard]] auto one_like(const Rep &value) {
  if constexpr (std::same_as<std::remove_cvref_t<Rep>, double>)
    return 1.0;
  else
    return pluss::codem::make_scalar_like(value, 1.0);
}

} // namespace detail

template <common::QuantityRep AmountRep, common::QuantityRep TemperatureRep,
          common::QuantityRep VolumeRep>
[[nodiscard]] auto
pressure(const common::Quantity<common::Amount, AmountRep> &amount,
         const common::TemperaturePoint<TemperatureRep> &temperature,
         const common::Quantity<common::Volume, VolumeRep> &volume) {
  constexpr const char *operation = "natsci::thermo::pressure";
  detail::require_positive(amount.si_value(), operation, "amount");
  detail::require_positive(volume.si_value(), operation, "volume");
  return amount * detail::molar_r() *
         detail::absolute_temp(temperature, operation) / volume;
}

template <common::QuantityRep AmountRep, common::QuantityRep TemperatureRep,
          common::QuantityRep PressureRep>
[[nodiscard]] auto
volume(const common::Quantity<common::Amount, AmountRep> &amount,
       const common::TemperaturePoint<TemperatureRep> &temperature,
       const common::Quantity<common::Pressure, PressureRep> &pressure_value) {
  constexpr const char *operation = "natsci::thermo::volume";
  detail::require_positive(amount.si_value(), operation, "amount");
  detail::require_positive(pressure_value.si_value(), operation, "pressure");
  return amount * detail::molar_r() *
         detail::absolute_temp(temperature, operation) / pressure_value;
}

template <common::QuantityRep PressureRep, common::QuantityRep VolumeRep,
          common::QuantityRep AmountRep>
[[nodiscard]] auto
temp(const common::Quantity<common::Pressure, PressureRep> &pressure_value,
     const common::Quantity<common::Volume, VolumeRep> &volume_value,
     const common::Quantity<common::Amount, AmountRep> &amount) {
  constexpr const char *operation = "natsci::thermo::temp";
  detail::require_positive(pressure_value.si_value(), operation, "pressure");
  detail::require_positive(volume_value.si_value(), operation, "volume");
  detail::require_positive(amount.si_value(), operation, "amount");
  const auto result =
      pressure_value * volume_value / (amount * detail::molar_r());
  using Result = std::remove_cvref_t<decltype(result)>;
  return common::TemperaturePoint<typename Result::rep_type>::from_si(
      result.si_value());
}

template <common::QuantityRep PressureRep, common::QuantityRep VolumeRep,
          common::QuantityRep TemperatureRep>
[[nodiscard]] auto
moles(const common::Quantity<common::Pressure, PressureRep> &pressure_value,
      const common::Quantity<common::Volume, VolumeRep> &volume_value,
      const common::TemperaturePoint<TemperatureRep> &temperature) {
  constexpr const char *operation = "natsci::thermo::moles";
  detail::require_positive(pressure_value.si_value(), operation, "pressure");
  detail::require_positive(volume_value.si_value(), operation, "volume");
  return pressure_value * volume_value /
         (detail::molar_r() * detail::absolute_temp(temperature, operation));
}

template <common::QuantityRep Rep>
[[nodiscard]] auto
gas_const(const common::Quantity<MolarMass, Rep> &molar_mass) {
  constexpr const char *operation = "natsci::thermo::gas_const";
  detail::require_positive(molar_mass.si_value(), operation, "molar mass");
  return detail::molar_r() / molar_mass;
}

template <common::QuantityRep DensityRep, common::QuantityRep TemperatureRep,
          common::QuantityRep GasRep>
[[nodiscard]] auto
pressure(const common::Quantity<MassDensity, DensityRep> &density_value,
         const common::TemperaturePoint<TemperatureRep> &temperature,
         const common::Quantity<SpecificGasConstant, GasRep> &gas_constant) {
  constexpr const char *operation = "natsci::thermo::pressure";
  detail::require_positive(density_value.si_value(), operation, "density");
  detail::require_positive(gas_constant.si_value(), operation,
                           "specific gas constant");
  return density_value * gas_constant *
         detail::absolute_temp(temperature, operation);
}

template <common::QuantityRep PressureRep, common::QuantityRep TemperatureRep,
          common::QuantityRep GasRep>
[[nodiscard]] auto
density(const common::Quantity<common::Pressure, PressureRep> &pressure_value,
        const common::TemperaturePoint<TemperatureRep> &temperature,
        const common::Quantity<SpecificGasConstant, GasRep> &gas_constant) {
  constexpr const char *operation = "natsci::thermo::density";
  detail::require_positive(pressure_value.si_value(), operation, "pressure");
  detail::require_positive(gas_constant.si_value(), operation,
                           "specific gas constant");
  return pressure_value /
         (gas_constant * detail::absolute_temp(temperature, operation));
}

template <common::QuantityRep GasRep, common::QuantityRep RatioRep>
[[nodiscard]] auto
cv(const common::Quantity<SpecificGasConstant, GasRep> &gas_constant,
   const common::Quantity<common::Dimensionless, RatioRep> &gamma) {
  constexpr const char *operation = "natsci::thermo::cv";
  detail::require_positive(gas_constant.si_value(), operation,
                           "specific gas constant");
  detail::require_above_one(gamma.si_value(), operation, "heat ratio");
  auto denominator_rep = common::detail::sub_rep(gamma.si_value(), 1.0);
  using DenominatorRep = std::remove_cvref_t<decltype(denominator_rep)>;
  const auto denominator =
      common::Quantity<common::Dimensionless, DenominatorRep>::from_si(
          std::move(denominator_rep));
  return gas_constant / denominator;
}

template <common::QuantityRep GasRep, common::QuantityRep RatioRep>
[[nodiscard]] auto
cp(const common::Quantity<SpecificGasConstant, GasRep> &gas_constant,
   const common::Quantity<common::Dimensionless, RatioRep> &gamma) {
  return cv(gas_constant, gamma) + gas_constant;
}

template <common::QuantityRep CpRep, common::QuantityRep CvRep>
[[nodiscard]] auto
heat_ratio(const common::Quantity<SpecificHeat, CpRep> &cp_value,
           const common::Quantity<SpecificHeat, CvRep> &cv_value) {
  constexpr const char *operation = "natsci::thermo::heat_ratio";
  detail::require_positive(cp_value.si_value(), operation, "cp");
  detail::require_positive(cv_value.si_value(), operation, "cv");
  detail::require_positive(
      common::detail::sub_rep(cp_value.si_value(), cv_value.si_value()),
      operation, "cp - cv");
  return cp_value / cv_value;
}

template <common::QuantityRep HeatRep, common::QuantityRep TemperatureRep>
[[nodiscard]] auto
energy(const common::Quantity<SpecificHeat, HeatRep> &cv_value,
       const common::TemperaturePoint<TemperatureRep> &temperature) {
  constexpr const char *operation = "natsci::thermo::energy";
  detail::require_positive(cv_value.si_value(), operation, "cv");
  return cv_value * detail::absolute_temp(temperature, operation);
}

template <common::QuantityRep HeatRep, common::QuantityRep TemperatureRep>
[[nodiscard]] auto
enthalpy(const common::Quantity<SpecificHeat, HeatRep> &cp_value,
         const common::TemperaturePoint<TemperatureRep> &temperature) {
  constexpr const char *operation = "natsci::thermo::enthalpy";
  detail::require_positive(cp_value.si_value(), operation, "cp");
  return cp_value * detail::absolute_temp(temperature, operation);
}

template <common::QuantityRep RatioRep, common::QuantityRep GasRep,
          common::QuantityRep TemperatureRep>
[[nodiscard]] auto
sound_speed(const common::Quantity<common::Dimensionless, RatioRep> &gamma,
            const common::Quantity<SpecificGasConstant, GasRep> &gas_constant,
            const common::TemperaturePoint<TemperatureRep> &temperature) {
  constexpr const char *operation = "natsci::thermo::sound_speed";
  detail::require_above_one(gamma.si_value(), operation, "heat ratio");
  detail::require_positive(gas_constant.si_value(), operation,
                           "specific gas constant");
  const auto squared =
      gamma * gas_constant * detail::absolute_temp(temperature, operation);
  using ResultRep = decltype(common::detail::sqrt_rep(squared.si_value()));
  return common::Quantity<common::Speed, ResultRep>::from_si(
      common::detail::sqrt_rep(squared.si_value()));
}

template <common::QuantityRep Rep>
[[nodiscard]] auto
kappa(const common::Quantity<common::Pressure, Rep> &pressure_value) {
  constexpr const char *operation = "natsci::thermo::kappa";
  detail::require_positive(pressure_value.si_value(), operation, "pressure");
  auto result = common::detail::div_rep(
      detail::one_like(pressure_value.si_value()), pressure_value.si_value());
  using ResultRep = std::remove_cvref_t<decltype(result)>;
  return common::Quantity<Compressibility, ResultRep>::from_si(
      std::move(result));
}

template <common::QuantityRep Rep>
[[nodiscard]] auto beta(const common::TemperaturePoint<Rep> &temperature) {
  constexpr const char *operation = "natsci::thermo::beta";
  detail::require_positive(temperature.si_value(), operation, "temperature");
  auto result = common::detail::div_rep(
      detail::one_like(temperature.si_value()), temperature.si_value());
  using ResultRep = std::remove_cvref_t<decltype(result)>;
  return common::Quantity<Expansion, ResultRep>::from_si(std::move(result));
}

template <common::QuantityRep HeatRep, common::QuantityRep WorkRep>
[[nodiscard]] auto
delta_e(const common::Quantity<common::Energy, HeatRep> &heat,
        const common::Quantity<common::Energy, WorkRep> &work) {
  constexpr const char *operation = "natsci::thermo::delta_e";
  detail::require_finite(heat.si_value(), operation, "heat input");
  detail::require_finite(work.si_value(), operation, "work output");
  return heat - work;
}

template <common::QuantityRep EnergyRep, common::QuantityRep WorkRep>
[[nodiscard]] auto
heat_in(const common::Quantity<common::Energy, EnergyRep> &stored_change,
        const common::Quantity<common::Energy, WorkRep> &work) {
  constexpr const char *operation = "natsci::thermo::heat_in";
  detail::require_finite(stored_change.si_value(), operation,
                         "stored-energy change");
  detail::require_finite(work.si_value(), operation, "work output");
  return stored_change + work;
}

template <common::QuantityRep HeatRep, common::QuantityRep EnergyRep>
[[nodiscard]] auto
work_out(const common::Quantity<common::Energy, HeatRep> &heat,
         const common::Quantity<common::Energy, EnergyRep> &stored_change) {
  constexpr const char *operation = "natsci::thermo::work_out";
  detail::require_finite(heat.si_value(), operation, "heat input");
  detail::require_finite(stored_change.si_value(), operation,
                         "stored-energy change");
  return heat - stored_change;
}

template <common::QuantityRep TemperatureRep, common::QuantityRep WorkRep,
          common::QuantityRep HeatRep, common::QuantityRep EnergyRep>
struct IdealPath final {
  common::TemperaturePoint<TemperatureRep> final_temp;
  common::Quantity<common::Energy, WorkRep> work;
  common::Quantity<common::Energy, HeatRep> heat;
  common::Quantity<common::Energy, EnergyRep> delta_u;
};

template <class PressureFn>
[[nodiscard]] auto
pv_work(PressureFn pressure_fn,
        common::ScalarQuantity<common::Volume> initial_volume,
        common::ScalarQuantity<common::Volume> final_volume,
        const pluss::subsp::math::integ::QuadraOptions &options = {}) {
  constexpr const char *operation = "natsci::thermo::pv_work";
  detail::require_positive(initial_volume.si_value(), operation,
                           "initial volume");
  detail::require_positive(final_volume.si_value(), operation, "final volume");
  return common::solve::quad(std::move(pressure_fn), initial_volume,
                             final_volume, options);
}

template <common::QuantityRep MassRep, common::QuantityRep HeatRep,
          common::QuantityRep InitialRep, common::QuantityRep FinalRep>
[[nodiscard]] auto
iso_v(const common::Quantity<common::Mass, MassRep> &mass,
      const common::Quantity<SpecificHeat, HeatRep> &cv_value,
      const common::TemperaturePoint<InitialRep> &initial_temp,
      const common::TemperaturePoint<FinalRep> &final_temp) {
  constexpr const char *operation = "natsci::thermo::iso_v";
  detail::require_positive(mass.si_value(), operation, "mass");
  detail::require_positive(cv_value.si_value(), operation, "cv");
  (void)detail::absolute_temp(initial_temp, operation);
  (void)detail::absolute_temp(final_temp, operation);
  const auto stored_change = mass * cv_value * (final_temp - initial_temp);
  using EnergyRep = typename decltype(stored_change)::rep_type;
  auto no_work = common::Quantity<common::Energy, EnergyRep>::from_si(
      common::detail::zero_rep(stored_change.si_value()));
  return IdealPath<FinalRep, EnergyRep, EnergyRep, EnergyRep>{
      final_temp, no_work, stored_change, stored_change};
}

template <common::QuantityRep MassRep, common::QuantityRep CpRep,
          common::QuantityRep CvRep, common::QuantityRep InitialRep,
          common::QuantityRep FinalRep>
[[nodiscard]] auto
iso_p(const common::Quantity<common::Mass, MassRep> &mass,
      const common::Quantity<SpecificHeat, CpRep> &cp_value,
      const common::Quantity<SpecificHeat, CvRep> &cv_value,
      const common::TemperaturePoint<InitialRep> &initial_temp,
      const common::TemperaturePoint<FinalRep> &final_temp) {
  constexpr const char *operation = "natsci::thermo::iso_p";
  detail::require_positive(mass.si_value(), operation, "mass");
  (void)heat_ratio(cp_value, cv_value);
  (void)detail::absolute_temp(initial_temp, operation);
  (void)detail::absolute_temp(final_temp, operation);
  const auto temperature_change = final_temp - initial_temp;
  const auto stored_change = mass * cv_value * temperature_change;
  const auto heat = mass * cp_value * temperature_change;
  const auto work = heat - stored_change;
  return IdealPath<FinalRep, typename decltype(work)::rep_type,
                   typename decltype(heat)::rep_type,
                   typename decltype(stored_change)::rep_type>{
      final_temp, work, heat, stored_change};
}

template <common::QuantityRep AmountRep, common::QuantityRep TemperatureRep,
          common::QuantityRep InitialVolumeRep,
          common::QuantityRep FinalVolumeRep>
[[nodiscard]] auto
iso_t(const common::Quantity<common::Amount, AmountRep> &amount,
      const common::TemperaturePoint<TemperatureRep> &temperature,
      const common::Quantity<common::Volume, InitialVolumeRep> &initial_volume,
      const common::Quantity<common::Volume, FinalVolumeRep> &final_volume) {
  constexpr const char *operation = "natsci::thermo::iso_t";
  detail::require_positive(amount.si_value(), operation, "amount");
  detail::require_positive(initial_volume.si_value(), operation,
                           "initial volume");
  detail::require_positive(final_volume.si_value(), operation, "final volume");
  const auto ratio = final_volume / initial_volume;
  auto log_ratio_rep = common::detail::log_rep(ratio.si_value());
  using RatioRep = std::remove_cvref_t<decltype(log_ratio_rep)>;
  const auto log_ratio =
      common::Quantity<common::Dimensionless, RatioRep>::from_si(
          std::move(log_ratio_rep));
  const auto work = amount * detail::molar_r() *
                    detail::absolute_temp(temperature, operation) * log_ratio;
  using EnergyRep = typename decltype(work)::rep_type;
  auto no_change = common::Quantity<common::Energy, EnergyRep>::from_si(
      common::detail::zero_rep(work.si_value()));
  return IdealPath<TemperatureRep, EnergyRep, EnergyRep, EnergyRep>{
      temperature, work, work, no_change};
}

template <common::QuantityRep MassRep, common::QuantityRep HeatRep,
          common::QuantityRep RatioRep, common::QuantityRep TemperatureRep,
          common::QuantityRep InitialVolumeRep,
          common::QuantityRep FinalVolumeRep>
[[nodiscard]] auto
iso_s(const common::Quantity<common::Mass, MassRep> &mass,
      const common::Quantity<SpecificHeat, HeatRep> &cv_value,
      const common::Quantity<common::Dimensionless, RatioRep> &gamma,
      const common::TemperaturePoint<TemperatureRep> &initial_temp,
      const common::Quantity<common::Volume, InitialVolumeRep> &initial_volume,
      const common::Quantity<common::Volume, FinalVolumeRep> &final_volume) {
  constexpr const char *operation = "natsci::thermo::iso_s";
  detail::require_positive(mass.si_value(), operation, "mass");
  detail::require_positive(cv_value.si_value(), operation, "cv");
  detail::require_above_one(gamma.si_value(), operation, "heat ratio");
  detail::require_positive(initial_volume.si_value(), operation,
                           "initial volume");
  detail::require_positive(final_volume.si_value(), operation, "final volume");
  const auto initial_absolute = detail::absolute_temp(initial_temp, operation);
  const auto ratio = initial_volume / final_volume;
  const auto exponent = common::detail::sub_rep(gamma.si_value(), 1.0);
  const auto factor = common::detail::pow_rep(ratio.si_value(), exponent);
  auto final_rep = common::detail::mul_rep(initial_absolute.si_value(), factor);
  using FinalRep = std::remove_cvref_t<decltype(final_rep)>;
  const auto final_temp =
      common::TemperaturePoint<FinalRep>::from_si(std::move(final_rep));
  const auto stored_change = mass * cv_value * (final_temp - initial_temp);
  const auto work = -stored_change;
  using EnergyRep = typename decltype(stored_change)::rep_type;
  auto no_heat = common::Quantity<common::Energy, EnergyRep>::from_si(
      common::detail::zero_rep(stored_change.si_value()));
  return IdealPath<FinalRep, EnergyRep, EnergyRep, EnergyRep>{
      final_temp, work, no_heat, stored_change};
}

} // namespace pluss::subsp::natsci::physics::thermo
