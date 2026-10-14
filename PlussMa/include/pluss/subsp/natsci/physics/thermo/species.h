#pragma once

#include "pluss/subsp/natsci/common/thermo_poly.h"
#include "pluss/subsp/natsci/physics/thermo.h"

namespace pluss::subsp::natsci::physics::thermo {

using SpeciesData = common::thermo::SpeciesData;
using Nasa7 = common::thermo::Nasa7;
using MolarEnergy = common::DimensionQuotient<common::Energy, common::Amount>;

struct MolarProps final {
  common::SeriesQuantity<common::MolarGasConstant> cp;
  common::SeriesQuantity<MolarEnergy> h;
  common::SeriesQuantity<common::MolarGasConstant> s;
  common::SeriesQuantity<MolarEnergy> g;
};

struct MassProps final {
  common::SeriesQuantity<SpecificHeat> cp;
  common::SeriesQuantity<SpecificEnergy> h;
  common::SeriesQuantity<SpecificHeat> s;
  common::SeriesQuantity<SpecificEnergy> g;
};

// Molar ideal-gas properties for every species. The temperature field keeps
// all of its axes; a final species axis is appended by the shared evaluator.
[[nodiscard]] inline MolarProps
props(const Nasa7 &model, const common::TemperaturePoint<Series> &temperature) {
  const auto value = model.eval(temperature.si_value());
  const double r =
      common::constants::codata2022::molar_gas_constant.measurement.value()
          .si_value();
  const Series temp = temperature.si_value().unsqueeze(-1);
  const Series cp = value.cp_r.mul(r);
  const Series h = value.h_rt.mul(temp).mul(r);
  const Series s = value.s_r.mul(r);
  const Series g = value.g_rt().mul(temp).mul(r);
  return {
      common::SeriesQuantity<common::MolarGasConstant>::from_si(cp),
      common::SeriesQuantity<MolarEnergy>::from_si(h),
      common::SeriesQuantity<common::MolarGasConstant>::from_si(s),
      common::SeriesQuantity<MolarEnergy>::from_si(g),
  };
}

// Convert the fused molar result to a mass basis. A scalar molar mass applies
// to one species; a [species] Series converts a complete multi-species result.
template <common::QuantityRep MassRep>
[[nodiscard]] inline MassProps
mass_props(const Nasa7 &model,
           const common::TemperaturePoint<Series> &temperature,
           const common::Quantity<MolarMass, MassRep> &molar_mass) {
  constexpr const char *operation = "natsci::thermo::mass_props";
  detail::require_positive(molar_mass.si_value(), operation, "molar mass");
  const auto molar = props(model, temperature);
  return {molar.cp / molar_mass, molar.h / molar_mass, molar.s / molar_mass,
          molar.g / molar_mass};
}

} // namespace pluss::subsp::natsci::physics::thermo
