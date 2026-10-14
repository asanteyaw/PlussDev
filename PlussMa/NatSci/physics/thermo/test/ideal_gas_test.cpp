#include "pluss/ma/natsci/physics.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace thermo = pluss::subsp::natsci::physics::thermo;

using pluss::ScalarType;
using pluss::Series;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

template <common::DimensionType D>
common::ScalarQuantity<D> scalar(double value) {
  return common::ScalarQuantity<D>::from_si(value);
}

common::TemperaturePoint<> kelvin(double value) {
  return common::TemperaturePoint<>::from_si(value);
}

void test_codata_and_molar_state() {
  const auto &constant = common::constants::codata2022::molar_gas_constant;
  expect(constant.exact &&
             std::abs(constant.measurement.value().si_value() -
                      8.31446261815324) < 1e-15 &&
             constant.measurement.standard_uncertainty().si_value() == 0.0,
         "CODATA molar gas constant metadata is incorrect");

  const auto amount = scalar<common::Amount>(1.0);
  const auto temperature = kelvin(273.15);
  const auto pressure = scalar<common::Pressure>(100'000.0);
  const auto volume = thermo::volume(amount, temperature, pressure);
  static_assert(
      std::same_as<typename decltype(volume)::dimension, common::Volume>);
  expect(std::abs(volume.si_value() - 0.022710954641485484) < 1e-15,
         "standard-state molar volume does not match CODATA");

  const auto recovered_pressure = thermo::pressure(amount, temperature, volume);
  const auto recovered_temp = thermo::temp(recovered_pressure, volume, amount);
  const auto recovered_amount =
      thermo::moles(recovered_pressure, volume, temperature);
  expect(std::abs(recovered_pressure.si_value() - 100'000.0) < 1e-10 &&
             std::abs(recovered_temp.si_value() - 273.15) < 1e-12 &&
             std::abs(recovered_amount.si_value() - 1.0) < 1e-12,
         "molar ideal-gas inverse relations do not close");
}

void test_mass_basis_and_caloric_properties() {
  const auto molar_mass = scalar<thermo::MolarMass>(0.02896546);
  const auto gas = thermo::gas_const(molar_mass);
  const auto gamma = scalar<common::Dimensionless>(1.4);
  const auto cv = thermo::cv(gas, gamma);
  const auto cp = thermo::cp(gas, gamma);
  const auto ratio = thermo::heat_ratio(cp, cv);
  const auto temperature = kelvin(288.15);
  const auto pressure = scalar<common::Pressure>(101'325.0);
  const auto density = thermo::density(pressure, temperature, gas);

  static_assert(std::same_as<typename decltype(gas)::dimension,
                             thermo::SpecificGasConstant>);
  static_assert(
      std::same_as<typename decltype(density)::dimension, thermo::MassDensity>);
  static_assert(std::same_as<typename decltype(thermo::energy(
                                 cv, temperature))::dimension,
                             thermo::SpecificEnergy>);

  expect(std::abs(gas.si_value() - 287.04749098247498) < 1e-10,
         "specific gas constant is incorrect");
  expect(std::abs(cp.si_value() - 1004.6662184386624) < 1e-9 &&
             std::abs(cv.si_value() - 717.61872745618746) < 1e-9 &&
             std::abs(ratio.si_value() - 1.4) < 1e-14,
         "calorically perfect heat capacities are inconsistent");
  expect(std::abs(density.si_value() - 1.2250229735472498) < 1e-12,
         "mass-basis ideal-gas density is incorrect");
  expect(std::abs(thermo::pressure(density, temperature, gas).si_value() -
                  pressure.si_value()) < 1e-10,
         "mass-basis pressure and density do not invert");
  expect(std::abs(thermo::sound_speed(gamma, gas, temperature).si_value() -
                  340.29079966587432) < 1e-10,
         "ideal-gas sound speed is incorrect");
  expect(std::abs(thermo::energy(cv, temperature).si_value() -
                  cv.si_value() * 288.15) < 1e-9 &&
             std::abs(thermo::enthalpy(cp, temperature).si_value() -
                      cp.si_value() * 288.15) < 1e-9,
         "caloric energy relations are incorrect");
  expect(
      std::abs(thermo::kappa(pressure).si_value() - 1.0 / 101'325.0) < 1e-18 &&
          std::abs(thermo::beta(temperature).si_value() - 1.0 / 288.15) < 1e-18,
      "ideal-gas response coefficients are incorrect");

  const auto celsius =
      common::TemperaturePoint<>::from(15.0, common::units::celsius);
  expect(std::abs(celsius.si_value() - 288.15) < 1e-12 &&
             std::abs(thermo::density(pressure, celsius, gas).si_value() -
                      density.si_value()) < 1e-12,
         "affine Celsius input was not converted to absolute temperature once");
}

void test_broadcasting_and_reverse_sensitivity() {
  const auto amount = common::SeriesQuantity<common::Amount>::from_si(
      pluss::series::make({1.0, 2.0}).reshape({2, 1}));
  Series temperature_values = pluss::series::make({300.0, 450.0, 600.0});
  temperature_values.tracks_slope_(true);
  const auto temperature =
      common::TemperaturePoint<Series>::from_si(temperature_values);
  const auto volume = scalar<common::Volume>(0.5);
  const auto pressure = thermo::pressure(amount, temperature, volume);
  const double gas =
      common::constants::codata2022::molar_gas_constant.measurement.value()
          .si_value();
  const Series expected =
      amount.si_value().mul(temperature_values).mul(gas / 0.5);

  expect(pressure.si_value().sizes() == std::vector<std::int64_t>{2, 3} &&
             pressure.si_value().allclose(expected, 1e-12, 1e-12),
         "molar pressure did not preserve Series broadcasting");

  pressure.si_value().sum().reverse();
  expect(temperature_values.has_sensitivity() &&
             temperature_values.sensitivity().allclose(
                 pluss::series::full({3}, 3.0 * gas / 0.5, ScalarType::Float64),
                 1e-11, 1e-11),
         "ideal-gas pressure severed reverse sensitivity");

  const Series gamma_values = pluss::series::make({1.4, 1.3}).reshape({2, 1});
  const auto gamma =
      common::SeriesQuantity<common::Dimensionless>::from_si(gamma_values);
  const auto specific_gas =
      scalar<thermo::SpecificGasConstant>(287.04749098247498);
  const auto batch_temperature = common::TemperaturePoint<Series>::from_si(
      pluss::series::make({300.0, 600.0}));
  const auto batch_cp = thermo::cp(specific_gas, gamma);
  const auto batch_sound =
      thermo::sound_speed(gamma, specific_gas, batch_temperature);
  const Series expected_cp =
      gamma_values.div(gamma_values.sub(1.0)).mul(specific_gas.si_value());
  const Series expected_sound = gamma_values.mul(batch_temperature.si_value())
                                    .mul(specific_gas.si_value())
                                    .sqrt();

  expect(batch_cp.si_value().sizes() == std::vector<std::int64_t>{2, 1} &&
             batch_cp.si_value().allclose(expected_cp, 1e-11, 1e-11) &&
             batch_sound.si_value().sizes() ==
                 std::vector<std::int64_t>{2, 2} &&
             batch_sound.si_value().allclose(expected_sound, 1e-11, 1e-11),
         "caloric properties did not preserve multidimensional broadcasting");
}

void test_failures() {
  const auto rejects = [](auto &&fn) {
    try {
      fn();
    } catch (const std::invalid_argument &) {
      return true;
    }
    return false;
  };

  expect(rejects([] {
           (void)thermo::volume(scalar<common::Amount>(1.0), kelvin(0.0),
                                scalar<common::Pressure>(100'000.0));
         }),
         "ideal-gas relation accepted absolute zero");
  expect(
      rejects([] { (void)thermo::gas_const(scalar<thermo::MolarMass>(0.0)); }),
      "specific gas constant accepted zero molar mass");
  expect(rejects([] {
           (void)thermo::cv(scalar<thermo::SpecificGasConstant>(287.0),
                            scalar<common::Dimensionless>(1.0));
         }),
         "caloric relation accepted gamma <= 1");
  expect(rejects([] {
           (void)thermo::heat_ratio(scalar<thermo::SpecificHeat>(700.0),
                                    scalar<thermo::SpecificHeat>(800.0));
         }),
         "heat ratio accepted cp <= cv");
  expect(rejects([] {
           const auto bad =
               common::TemperaturePoint<Series>::from_si(pluss::series::make(
                   {300.0, std::numeric_limits<double>::quiet_NaN()}));
           (void)thermo::beta(bad);
         }),
         "thermodynamic validation accepted a non-finite batch lane");
}

} // namespace

int main() {
  try {
    test_codata_and_molar_state();
    test_mass_basis_and_caloric_properties();
    test_broadcasting_and_reverse_sensitivity();
    test_failures();
    std::cout << "NatSci ideal-gas thermodynamics checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
