#include "pluss/codem/series_factory.h"
#include "pluss/ma/natsci/common.h"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <iostream>
#include <stdexcept>
#include <string>

namespace common = pluss::ma::natsci::common;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool close(double actual, double expected, double relative = 1e-12) {
  const double scale = std::max({1.0, std::abs(actual), std::abs(expected)});
  return std::abs(actual - expected) <= relative * scale;
}

template <class Lhs, class Rhs>
concept Addable = requires(const Lhs &lhs, const Rhs &rhs) { lhs + rhs; };

using LengthQuantity = common::ScalarQuantity<common::Length>;
using TimeQuantity = common::ScalarQuantity<common::Time>;

static_assert(Addable<LengthQuantity, LengthQuantity>);
static_assert(!Addable<LengthQuantity, TimeQuantity>);
static_assert(
    std::same_as<common::DimensionQuotient<common::Length, common::Time>,
                 common::Speed>);

void test_scalar_quantities() {
  const auto distance = LengthQuantity::from(1.25, common::units::kilometre);
  const auto elapsed = TimeQuantity::from(2.0, common::units::minute);
  const auto speed = distance / elapsed;

  expect(close(distance.in(common::units::metre), 1250.0),
         "kilometre-to-metre conversion failed");
  expect(close(elapsed.in(common::units::second), 120.0),
         "minute-to-second conversion failed");
  expect(close(speed.in(common::units::metre_per_second), 1250.0 / 120.0),
         "derived speed dimension/value failed");

  bool rejected = false;
  try {
    [[maybe_unused]] common::Unit<common::Length> invalid(
        0.0, "bad", "invalid length unit");
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  expect(rejected, "non-positive unit scale was not rejected");
}

void test_series_quantities() {
  const auto kilometres = common::SeriesQuantity<common::Length>::from(
      pluss::series::make({1.0, 2.5, 0.125}), common::units::kilometre);
  const auto seconds =
      common::ScalarQuantity<common::Time>::from(10.0, common::units::second);
  const auto speeds = kilometres / seconds;
  const auto metres = kilometres.in(common::units::metre);
  const auto metres_per_second = speeds.in(common::units::metre_per_second);

  expect(close(metres.select(0, 0).item<double>(), 1000.0),
         "Series quantity conversion lane 0 failed");
  expect(close(metres.select(0, 1).item<double>(), 2500.0),
         "Series quantity conversion lane 1 failed");
  expect(close(metres.select(0, 2).item<double>(), 125.0),
         "Series quantity conversion lane 2 failed");
  expect(close(metres_per_second.select(0, 0).item<double>(), 100.0),
         "mixed Series/scalar quantity division failed");
}

void test_constants() {
  using namespace common::constants::codata2022;

  expect(speed_of_light.exact, "speed of light must be marked exact");
  expect(speed_of_light.measurement.value().in(
             common::units::metre_per_second) == 299792458.0,
         "speed of light value drifted");
  expect(planck_constant.measurement.value().in(common::units::joule_second) ==
             6.62607015e-34,
         "Planck constant value drifted");
  expect(!newtonian_gravitation.exact, "G must not be marked exact");
  expect(newtonian_gravitation.measurement.standard_uncertainty().in(
             common::units::gravitational_si) == 1.5e-15,
         "G standard uncertainty drifted");
  expect(newtonian_gravitation.revision == "CODATA 2022",
         "constant revision metadata missing");
  expect(newtonian_gravitation.doi == "10.1063/5.0279860",
         "constant DOI metadata missing");
}

void test_independent_uncertainty() {
  const common::Measurement<common::Length> width(
      LengthQuantity::from(5.0, common::units::metre),
      LengthQuantity::from(0.05, common::units::metre));
  const common::Measurement<common::Length> height(
      LengthQuantity::from(10.0, common::units::metre),
      LengthQuantity::from(0.1, common::units::metre));
  const auto area = width * height;

  expect(close(area.value().in(common::units::square_metre), 50.0),
         "measurement product value failed");
  expect(close(area.standard_uncertainty().in(common::units::square_metre),
               std::sqrt(0.5), 1e-12),
         "measurement product uncertainty failed");

  bool rejected = false;
  try {
    [[maybe_unused]] common::Measurement<common::Length> invalid(
        LengthQuantity::from(1.0, common::units::metre),
        LengthQuantity::from(-0.1, common::units::metre));
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  expect(rejected, "negative standard uncertainty was not rejected");
}

} // namespace

int main() {
  try {
    test_scalar_quantities();
    test_series_quantities();
    test_constants();
    test_independent_uncertainty();
    std::cout << "NatSci common foundation: all checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "NatSci common foundation failure: " << error.what() << '\n';
    return 1;
  }
}
