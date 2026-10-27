#include "h2o2_mechanism.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace chem = pluss::subsp::natsci::chemistry;
namespace kin = chem::kinetics;
namespace rea = chem::reactor;
namespace th = chem::thermo;
using pluss::Series;

namespace {

constexpr auto F64 = pluss::ScalarType::Float64;

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

void expect_failure(const std::function<void()> &call,
                    const std::string &message) {
  bool failed = false;
  try {
    call();
  } catch (const std::invalid_argument &) {
    failed = true;
  }
  expect(failed, message);
}

Series vec(std::initializer_list<double> values) {
  return pluss::series::make(values).to(F64);
}

kin::Mechanism mechanism() {
  return {h2o2::species(), h2o2::reactions(), vec({300.0}),
          kin::Units::cgs_cal()};
}

Series mixtures() {
  // H2/O2/Ar and wet nitrogen, in the h2o2 mechanism's species order.
  return pluss::series::make({0.2, 0.0, 0.0, 0.1, 0.0, 0.0, 0.0, 0.0, 0.7, 0.0,
                              0.0, 0.0, 0.0, 0.0, 0.0, 0.3, 0.0, 0.0, 0.0, 0.7},
                             {2, 10})
      .to(F64);
}

void test_ideal_gas_identities() {
  const auto mechanism = ::mechanism();
  const th::IdealGas gas(mechanism);
  const Series y = mixtures();
  const Series temperature = vec({1100.0, 700.0});
  const Series density = vec({0.35, 0.8});
  const auto p = gas.properties(temperature, y);
  const Series mixture_r =
      p.mean_molar_mass.reciprocal().mul(th::gas_constant());

  expect(p.enthalpy.sub(p.internal_energy)
             .allclose(mixture_r.mul(temperature), 2e-12, 2e-12),
         "h - e did not equal the mixture gas constant times temperature");
  expect(p.cp.sub(p.cv).allclose(mixture_r, 2e-12, 2e-12),
         "cp - cv did not equal the mixture gas constant");
  expect(p.cv.gt(0.0).all().item<bool>() && p.cp.gt(p.cv).all().item<bool>() &&
             p.gamma().gt(1.0).all().item<bool>(),
         "ideal-gas heat capacities or gamma were not admissible");

  const Series pressure = gas.pressure(density, temperature, y);
  expect(gas.density(pressure, temperature, y).allclose(density, 2e-12, 2e-12),
         "density-pressure round trip failed");
  const Series concentrations = gas.concentrations(density, y);
  expect(concentrations.mul(gas.molar_masses())
             .sum(-1, false)
             .allclose(density, 2e-12, 2e-12),
         "molar concentrations did not reconstruct mixture density");
  expect(gas.sound_speed(temperature, y)
             .square()
             .allclose(p.gamma().mul(pressure).div(density), 3e-12, 3e-12),
         "sound speed did not satisfy a^2 = gamma p/rho");

  // Independent scalar ideal-gas evaluation for the H2/O2/Ar lane.
  const double mean_mass =
      1.0 / (0.2 / 2.016e-3 + 0.1 / 31.998e-3 + 0.7 / 39.95e-3);
  const double expected_pressure =
      0.35 * th::gas_constant() * 1100.0 / mean_mass;
  expect(std::abs(pressure.select(0, 0).item<double>() / expected_pressure -
                  1.0) < 2e-13,
         "mixture pressure disagreed with the scalar ideal-gas reference");
}

void test_energy_temperature_round_trip() {
  const auto mechanism = ::mechanism();
  const th::IdealGas gas(mechanism);
  const Series base_y = mixtures();
  const Series y = base_y.unsqueeze(1).unite({base_y.unsqueeze(1)}, 1);
  const Series temperature =
      pluss::series::make({300.0, 999.0, 1001.0, 3400.0}, {2, 2}).to(F64);
  const Series energy = gas.internal_energy(temperature, y);

  expect(gas.temperature(energy, y).allclose(temperature, 3e-11, 3e-11),
         "energy-to-temperature inversion failed across NASA intervals");
  const Series poor_guess = pluss::series::full_like(temperature, 310.0);
  expect(gas.temperature(energy, y, poor_guess)
             .allclose(temperature, 3e-11, 3e-11),
         "bracketed temperature recovery failed from a poor guess");
  expect(energy.sizes() == std::vector<std::int64_t>({2, 2}),
         "thermodynamic evaluation did not preserve batch shape");

  // The reactor and mixture EOS use the same partial-molar internal energy.
  const Series density = vec({0.4, 0.7});
  const Series t = vec({1100.0, 1600.0});
  const Series c = gas.concentrations(density, base_y);
  const Series reactor_state = rea::make_state(t, c);
  expect(rea::internal_energy_density(mechanism, reactor_state)
             .div(density)
             .allclose(gas.internal_energy(t, base_y), 2e-12, 2e-12),
         "reactor and ideal-gas closures disagreed on internal energy");
}

void test_kinetic_mass_source_and_reverse() {
  const auto mechanism = ::mechanism();
  const th::IdealGas gas(mechanism);
  const Series y = mixtures().select(0, 0).unsqueeze(0);
  const Series density = vec({0.35});
  Series temperature = vec({1250.0});
  temperature.tracks_slope_(true);

  const Series concentration = gas.concentrations(density, y);
  const Series omega =
      mechanism.net_production_rates(temperature, concentration);
  const Series mass_source = omega.mul(gas.molar_masses());
  const double source_scale = mass_source.abs().max().item<double>();
  expect(std::abs(mass_source.sum(-1, false).host<double>().front()) <=
             std::max(1e-11, source_scale * 2e-12),
         "reaction source did not conserve total mass");

  const Series energy = gas.internal_energy(temperature, y);
  const Series recovered = gas.temperature(energy, y);
  recovered.sum().reverse();
  expect(temperature.has_sensitivity() &&
             std::abs(temperature.sensitivity().host<double>().front() - 1.0) < 2e-9,
         "temperature inversion severed or distorted reverse sensitivity");
}

void test_failures() {
  const auto mechanism = ::mechanism();
  const th::IdealGas gas(mechanism);
  const Series y = mixtures();
  const Series temperature = vec({700.0, 900.0});

  expect_failure([&] { (void)gas.properties(temperature, y.mul(0.5)); },
                 "non-normalized mass fractions were accepted");
  expect_failure([&] { (void)gas.properties(vec({700.0}), y); },
                 "temperature with the wrong lane shape was accepted");
  expect_failure(
      [&] {
        const Series below =
            gas.internal_energy(pluss::series::full({2}, gas.t_min(), F64), y);
        (void)gas.temperature(below.sub(1e9), y);
      },
      "out-of-range internal energy was accepted");

  auto no_mass = h2o2::species();
  for (auto &species : no_mass)
    species.molar_mass = 0.0;
  const kin::Mechanism incomplete(no_mass, h2o2::reactions(), vec({300.0}),
                                  kin::Units::cgs_cal());
  expect_failure([&] { (void)th::IdealGas(incomplete); },
                 "ideal-gas closure accepted a mechanism without masses");
}

} // namespace

int main() {
  try {
    test_ideal_gas_identities();
    test_energy_temperature_round_trip();
    test_kinetic_mass_source_and_reverse();
    test_failures();
    std::cout << "NatSci reacting ideal-gas checks passed\n";
    return EXIT_SUCCESS;
  } catch (const std::exception &error) {
    std::cerr << "reacting_ideal_gas_test: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
