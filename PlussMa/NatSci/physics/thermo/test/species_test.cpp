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

thermo::SpeciesData species(const char *name, double midpoint, double low_cp,
                            double high_cp) {
  thermo::SpeciesData value;
  value.name = name;
  value.composition = {{name, 1.0}};
  value.t_min = 200.0;
  value.t_mid = midpoint;
  value.t_max = 3000.0;
  value.low = {low_cp, 0.0, 0.0, 0.0, 0.0, 10.0, 2.0};
  value.high = {high_cp, 0.0, 0.0, 0.0, 0.0, 20.0, 3.0};
  return value;
}

thermo::Nasa7 model() {
  return thermo::Nasa7(
      {species("A", 1000.0, 3.0, 4.0), species("B", 1200.0, 5.0, 6.0)},
      pluss::series::make(300.0).to(ScalarType::Float64));
}

void expect_shape(const Series &value,
                  std::initializer_list<std::int64_t> shape,
                  const std::string &message) {
  expect(value.sizes() == std::vector<std::int64_t>(shape), message);
}

void test_high_rank_series_graph() {
  const auto nasa = model();
  const Series rank2 = pluss::series::full({2, 3}, 900.0, ScalarType::Float64);
  const Series rank3 =
      pluss::series::full({2, 1, 3}, 1100.0, ScalarType::Float64);
  const Series rank4 =
      pluss::series::full({1, 2, 1, 3}, 1300.0, ScalarType::Float64);

  const auto p2 = nasa.eval(rank2);
  const auto p3 = nasa.eval(rank3);
  const auto p4 = nasa.eval(rank4);
  expect_shape(p2.cp_r, {2, 3, 2}, "rank-2 field did not append species");
  expect_shape(p3.h_rt, {2, 1, 3, 2}, "rank-3 field did not append species");
  expect_shape(p4.s_r, {1, 2, 1, 3, 2}, "rank-4 field did not append species");
  expect(p2.cp_r.select(-1, 0).eq(3.0).all().item<bool>() &&
             p2.cp_r.select(-1, 1).eq(5.0).all().item<bool>() &&
             p3.cp_r.select(-1, 0).eq(4.0).all().item<bool>() &&
             p3.cp_r.select(-1, 1).eq(5.0).all().item<bool>() &&
             p4.cp_r.select(-1, 0).eq(4.0).all().item<bool>() &&
             p4.cp_r.select(-1, 1).eq(6.0).all().item<bool>(),
         "batched midpoint selection is incorrect");

  const Series strided =
      pluss::series::make({800.0, 900.0, 1000.0, 1100.0, 1200.0, 1300.0})
          .to(ScalarType::Float64)
          .reshape({2, 3})
          .transpose(0, 1);
  const Series strided_cp = nasa.cp_r(strided);
  expect_shape(strided_cp, {3, 2, 2},
               "strided temperature view lost its logical shape");
  expect(
      strided_cp.select(-1, 0)
          .eq(strided.le(1000.0).where(
              pluss::series::full(strided.sizes(), 3.0, ScalarType::Float64),
              pluss::series::full(strided.sizes(), 4.0, ScalarType::Float64)))
          .all()
          .item<bool>(),
      "strided lanes were not evaluated in logical order");
}

void test_typed_properties_and_reverse() {
  const auto nasa = model();
  Series temperature =
      pluss::series::make({500.0, 700.0}).to(ScalarType::Float64);
  temperature.tracks_slope_(true);
  const auto point = common::TemperaturePoint<Series>::from_si(temperature);
  const auto value = thermo::props(nasa, point);

  static_assert(std::same_as<typename decltype(value.cp)::dimension,
                             common::MolarGasConstant>);
  static_assert(
      std::same_as<typename decltype(value.h)::dimension, thermo::MolarEnergy>);
  static_assert(std::same_as<typename decltype(value.s)::dimension,
                             common::MolarGasConstant>);
  expect_shape(value.cp.si_value(), {2, 2},
               "typed molar properties lost lane or species axes");
  expect(
      value.g.si_value().allclose(value.h.si_value().sub(value.s.si_value().mul(
                                      temperature.unsqueeze(-1))),
                                  1e-11, 1e-11),
      "molar Gibbs identity g = h - T s failed");

  value.h.si_value().sum().reverse();
  const double r =
      common::constants::codata2022::molar_gas_constant.measurement.value()
          .si_value();
  expect(
      temperature.sensitivity().allclose(
          pluss::series::full({2}, 8.0 * r, ScalarType::Float64), 1e-11, 1e-11),
      "NASA enthalpy reverse sensitivity does not equal total cp");

  const auto masses = common::SeriesQuantity<thermo::MolarMass>::from_si(
      pluss::series::make({0.028, 0.032}).to(ScalarType::Float64));
  const auto mass = thermo::mass_props(nasa, point, masses);
  static_assert(std::same_as<typename decltype(mass.cp)::dimension,
                             thermo::SpecificHeat>);
  static_assert(std::same_as<typename decltype(mass.h)::dimension,
                             thermo::SpecificEnergy>);
  expect(mass.cp.si_value().allclose(value.cp.si_value().div(masses.si_value()),
                                     1e-12, 1e-12),
         "mass-basis conversion did not broadcast molar masses by species");
}

void test_midpoint_and_failures() {
  const auto nasa = model();
  const Series at_mid =
      pluss::series::make({1000.0, 1200.0}).to(ScalarType::Float64);
  const Series cp = nasa.cp_r(at_mid);
  expect(cp.select(0, 0).select(0, 0).item<double>() == 3.0 &&
             cp.select(0, 1).select(0, 1).item<double>() == 5.0,
         "exact NASA midpoints must select the low interval");

  const auto rejects = [](auto &&fn) {
    try {
      fn();
    } catch (const std::invalid_argument &) {
      return true;
    }
    return false;
  };
  expect(rejects([&] {
           nasa.check(pluss::series::make({199.0}).to(ScalarType::Float64));
         }) &&
             rejects([&] {
               (void)nasa.eval(pluss::series::make(
                   {std::numeric_limits<double>::quiet_NaN()}));
             }) &&
             rejects([&] {
               (void)nasa.eval(
                   pluss::series::make({300.0}).to(ScalarType::Float32));
             }),
         "temperature validation accepted an invalid field");

  auto duplicate = species("A", 1000.0, 3.0, 4.0);
  auto invalid = species("C", 1000.0, 3.0, 4.0);
  invalid.low[2] = std::numeric_limits<double>::infinity();
  const Series like = pluss::series::make(300.0).to(ScalarType::Float64);
  expect(rejects([&] { (void)thermo::Nasa7({duplicate, duplicate}, like); }) &&
             rejects([&] { (void)thermo::Nasa7({invalid}, like); }),
         "NASA model validation accepted duplicate or non-finite metadata");
}

} // namespace

int main() {
  try {
    test_high_rank_series_graph();
    test_typed_properties_and_reverse();
    test_midpoint_and_failures();
    std::cout << "NatSci species thermodynamics checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
