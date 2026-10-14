#include "pluss/ma/natsci/physics.h"

#include <cmath>
#include <iostream>
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

void test_first_law_signs() {
  const auto heat = scalar<common::Energy>(500.0);
  const auto work = scalar<common::Energy>(125.0);
  const auto stored = thermo::delta_e(heat, work);
  expect(stored.si_value() == 375.0 &&
             thermo::heat_in(stored, work).si_value() == 500.0 &&
             thermo::work_out(heat, stored).si_value() == 125.0,
         "closed-system first-law sign convention is inconsistent");
}

void test_named_ideal_paths() {
  const auto mass = scalar<common::Mass>(2.0);
  const auto cv = scalar<thermo::SpecificHeat>(718.0);
  const auto cp = scalar<thermo::SpecificHeat>(1005.0);
  const auto cold = kelvin(300.0);
  const auto hot = kelvin(400.0);

  const auto fixed_volume = thermo::iso_v(mass, cv, cold, hot);
  static_assert(std::same_as<typename decltype(fixed_volume.work)::dimension,
                             common::Energy>);
  expect(fixed_volume.work.si_value() == 0.0 &&
             fixed_volume.delta_u.si_value() == 143'600.0 &&
             fixed_volume.heat.si_value() == 143'600.0,
         "constant-volume path does not satisfy the first law");

  const auto fixed_pressure = thermo::iso_p(mass, cp, cv, cold, hot);
  expect(fixed_pressure.delta_u.si_value() == 143'600.0 &&
             fixed_pressure.work.si_value() == 57'400.0 &&
             fixed_pressure.heat.si_value() == 201'000.0,
         "constant-pressure path does not partition heat and work");

  const auto amount = scalar<common::Amount>(2.0);
  const auto small = scalar<common::Volume>(0.01);
  const auto large = scalar<common::Volume>(0.02);
  const auto fixed_temp = thermo::iso_t(amount, cold, small, large);
  const double exact_work = 2.0 * 8.31446261815324 * 300.0 * std::log(2.0);
  expect(std::abs(fixed_temp.work.si_value() - exact_work) < 1e-11 &&
             fixed_temp.heat.si_value() == fixed_temp.work.si_value() &&
             fixed_temp.delta_u.si_value() == 0.0,
         "reversible isothermal expansion is incorrect");
  const auto compression = thermo::iso_t(amount, cold, large, small);
  expect(std::abs(compression.work.si_value() + exact_work) < 1e-11,
         "reversible isothermal compression did not reverse work sign");

  const auto gamma = scalar<common::Dimensionless>(1.4);
  const auto expansion =
      thermo::iso_s(scalar<common::Mass>(1.0), cv, gamma, hot,
                    scalar<common::Volume>(0.5), scalar<common::Volume>(1.0));
  const double final_temp = 400.0 * std::pow(0.5, 0.4);
  const double delta_u = 718.0 * (final_temp - 400.0);
  expect(std::abs(expansion.final_temp.si_value() - final_temp) < 1e-12 &&
             std::abs(expansion.delta_u.si_value() - delta_u) < 1e-9 &&
             std::abs(expansion.work.si_value() + delta_u) < 1e-9 &&
             expansion.heat.si_value() == 0.0,
         "reversible adiabatic path is incorrect");
}

void test_batched_paths_and_work_quadrature() {
  const auto mass = scalar<common::Mass>(1.0);
  const auto cv = scalar<thermo::SpecificHeat>(718.0);
  const auto gamma = common::SeriesQuantity<common::Dimensionless>::from_si(
      pluss::series::make({1.3, 1.4}).reshape({2, 1}));
  const auto initial_temp = common::TemperaturePoint<Series>::from_si(
      pluss::series::make({300.0, 600.0}));
  const auto path =
      thermo::iso_s(mass, cv, gamma, initial_temp, scalar<common::Volume>(1.0),
                    common::SeriesQuantity<common::Volume>::from_si(
                        pluss::series::make({2.0, 4.0})));
  const Series expected_temp =
      initial_temp.si_value().mul(pluss::series::make({2.0, 4.0})
                                      .reciprocal()
                                      .log()
                                      .mul(gamma.si_value().sub(1.0))
                                      .exp());
  expect(path.final_temp.si_value().sizes() ==
                 std::vector<std::int64_t>{2, 2} &&
             path.work.si_value().sizes() == std::vector<std::int64_t>{2, 2} &&
             path.heat.si_value().sizes() == std::vector<std::int64_t>{2, 2} &&
             path.final_temp.si_value().allclose(expected_temp, 1e-12, 1e-12),
         "adiabatic path did not broadcast parameter and state batches");

  Series amount_values = pluss::series::make({1.0, 2.0}).reshape({2, 1});
  amount_values.tracks_slope_(true);
  const auto amounts =
      common::SeriesQuantity<common::Amount>::from_si(amount_values);
  const auto temperature = kelvin(300.0);
  pluss::subsp::math::integ::QuadraOptions options;
  options.rule = pluss::subsp::math::integ::QuadraRule::GaussLegendre;
  options.panels = 4;
  options.gauss_points = 10;
  options.adaptive = false;
  const auto numeric = thermo::pv_work(
      [amounts,
       temperature](const common::SeriesQuantity<common::Volume> &volume) {
        return thermo::pressure(amounts, temperature, volume);
      },
      scalar<common::Volume>(0.01), scalar<common::Volume>(0.02), options);
  static_assert(std::same_as<typename decltype(numeric.value)::dimension,
                             common::Energy>);
  const double unit_work = 8.31446261815324 * 300.0 * std::log(2.0);
  expect(
      numeric.converged &&
          numeric.value.si_value().allclose(
              pluss::series::make({unit_work, 2.0 * unit_work}), 1e-10, 1e-10),
      "batched p dV quadrature disagrees with the isothermal closed form");
  numeric.value.si_value().sum().reverse();
  expect(amount_values.sensitivity().allclose(
             pluss::series::full({2, 1}, unit_work, ScalarType::Float64), 1e-10,
             1e-10),
         "p dV quadrature severed reverse sensitivity");

  const auto detached_amounts = common::SeriesQuantity<common::Amount>::from_si(
      amounts.si_value().detach());
  const auto reversed = thermo::pv_work(
      [amounts = detached_amounts,
       temperature](const common::SeriesQuantity<common::Volume> &volume) {
        return thermo::pressure(amounts, temperature, volume);
      },
      scalar<common::Volume>(0.02), scalar<common::Volume>(0.01), options);
  expect(reversed.value.si_value().allclose(numeric.value.si_value().neg(),
                                            1e-10, 1e-10),
         "p dV quadrature did not preserve orientation");
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
           (void)thermo::iso_t(scalar<common::Amount>(1.0), kelvin(300.0),
                               scalar<common::Volume>(0.0),
                               scalar<common::Volume>(1.0));
         }) &&
             rejects([] {
               (void)thermo::iso_s(scalar<common::Mass>(1.0),
                                   scalar<thermo::SpecificHeat>(718.0),
                                   scalar<common::Dimensionless>(1.0),
                                   kelvin(300.0), scalar<common::Volume>(1.0),
                                   scalar<common::Volume>(2.0));
             }),
         "ideal path validation accepted a non-physical state");
}

} // namespace

int main() {
  try {
    test_first_law_signs();
    test_named_ideal_paths();
    test_batched_paths_and_work_quadrature();
    test_failures();
    std::cout << "NatSci ideal-gas process checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
