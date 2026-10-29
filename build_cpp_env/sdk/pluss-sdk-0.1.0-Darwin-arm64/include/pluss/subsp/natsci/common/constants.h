#pragma once

#include "pluss/subsp/natsci/common/measurement.h"

#include <string_view>

namespace pluss::subsp::natsci::common {

template <DimensionType D> struct PhysicalConstant final {
  std::string_view symbol;
  std::string_view name;
  Measurement<D> measurement;
  std::string_view source;
  std::string_view revision;
  std::string_view doi;
  bool exact;
};

namespace constants::codata2022 {

inline constexpr std::string_view source =
    "CODATA recommended values of the fundamental physical constants: 2022";
inline constexpr std::string_view revision = "CODATA 2022";
inline constexpr std::string_view doi = "10.1063/5.0279860";

inline const PhysicalConstant<Speed> speed_of_light{
    "c",
    "speed of light in vacuum",
    Measurement<Speed>(ScalarQuantity<Speed>::from_si(299792458.0),
                       ScalarQuantity<Speed>::from_si(0.0)),
    source,
    revision,
    doi,
    true};

inline const PhysicalConstant<Action> planck_constant{
    "h",
    "Planck constant",
    Measurement<Action>(ScalarQuantity<Action>::from_si(6.62607015e-34),
                        ScalarQuantity<Action>::from_si(0.0)),
    source,
    revision,
    doi,
    true};

inline const PhysicalConstant<ElectricCharge> elementary_charge{
    "e",
    "elementary charge",
    Measurement<ElectricCharge>(
        ScalarQuantity<ElectricCharge>::from_si(1.602176634e-19),
        ScalarQuantity<ElectricCharge>::from_si(0.0)),
    source,
    revision,
    doi,
    true};

inline const PhysicalConstant<Entropy> boltzmann_constant{
    "k",
    "Boltzmann constant",
    Measurement<Entropy>(ScalarQuantity<Entropy>::from_si(1.380649e-23),
                         ScalarQuantity<Entropy>::from_si(0.0)),
    source,
    revision,
    doi,
    true};

inline const PhysicalConstant<MolarInverse> avogadro_constant{
    "N_A",
    "Avogadro constant",
    Measurement<MolarInverse>(
        ScalarQuantity<MolarInverse>::from_si(6.02214076e23),
        ScalarQuantity<MolarInverse>::from_si(0.0)),
    source,
    revision,
    doi,
    true};

inline const PhysicalConstant<MolarGasConstant> molar_gas_constant{
    "R",
    "molar gas constant",
    Measurement<MolarGasConstant>(
        ScalarQuantity<MolarGasConstant>::from_si(8.31446261815324),
        ScalarQuantity<MolarGasConstant>::from_si(0.0)),
    source,
    revision,
    doi,
    true};

inline const PhysicalConstant<GravitationalConstant> newtonian_gravitation{
    "G",
    "Newtonian constant of gravitation",
    Measurement<GravitationalConstant>(
        ScalarQuantity<GravitationalConstant>::from_si(6.67430e-11),
        ScalarQuantity<GravitationalConstant>::from_si(1.5e-15)),
    source,
    revision,
    doi,
    false};

} // namespace constants::codata2022
} // namespace pluss::subsp::natsci::common
