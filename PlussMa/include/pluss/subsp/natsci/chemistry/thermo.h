#pragma once

// Chemistry-facing names for the shared NASA-7 thermodynamic engine. The
// evaluator itself lives in natsci::common so physics and chemistry use the
// same range convention, coefficient cache, validation, and Series graph.

#include "pluss/subsp/natsci/common/constants.h"
#include "pluss/subsp/natsci/common/thermo_poly.h"

namespace pluss::subsp::natsci::chemistry::thermo {

using SpeciesData = ::pluss::subsp::natsci::common::thermo::SpeciesData;
using Nasa7 = ::pluss::subsp::natsci::common::thermo::Nasa7;

inline double gas_constant() {
  return ::pluss::subsp::natsci::common::constants::codata2022::
      molar_gas_constant.measurement.value()
          .si_value();
}

inline constexpr double standard_pressure = 101325.0;

} // namespace pluss::subsp::natsci::chemistry::thermo
