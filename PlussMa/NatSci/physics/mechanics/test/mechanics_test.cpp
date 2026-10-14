#include "pluss/ma/natsci/physics.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace mechanics = pluss::subsp::natsci::physics::mechanics;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

bool close(double actual, double expected, double relative = 1e-10) {
  const double scale = std::max({1.0, std::abs(actual), std::abs(expected)});
  return std::abs(actual - expected) <= relative * scale;
}

void expect_shape(const pluss::Series &value,
                  std::initializer_list<std::int64_t> expected,
                  const std::string &message) {
  expect(value.dim() == static_cast<std::int64_t>(expected.size()), message);
  std::size_t axis = 0;
  for (const auto extent : expected)
    expect(value.size(static_cast<std::int64_t>(axis++)) == extent, message);
}

auto unit_gravity() {
  return common::ScalarQuantity<common::GravitationalConstant>::from_si(1.0);
}

mechanics::Bodies two_body() {
  return mechanics::make(
      common::SeriesQuantity<common::Mass>::from_si(
          pluss::series::make({2.0, 3.0})),
      common::SeriesQuantity<common::Length>::from_si(pluss::series::make(
          {{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}})),
      common::SeriesQuantity<common::Speed>::from_si(pluss::series::make(
          {{0.0, 1.5, 0.0}, {0.0, -1.0, 0.0}})));
}

void test_state_and_invariants() {
  const auto bodies = two_body();
  expect(bodies.count() == 2 && bodies.axes() == 3,
         "body state lost its semantic axes");
  expect(close(mechanics::total_mass(bodies).si_value().item<double>(), 5.0),
         "total mass is incorrect");
  expect(mechanics::center(bodies).si_value().allclose(
             pluss::series::make({1.2, 0.0, 0.0}), 1e-12, 1e-12),
         "center of mass is incorrect");
  expect(mechanics::center_speed(bodies).si_value().allclose(
             pluss::series::make({0.0, 0.0, 0.0}), 1e-12, 1e-12),
         "center velocity is incorrect");
  expect(mechanics::total_momentum(bodies).si_value().allclose(
             pluss::series::make({0.0, 0.0, 0.0}), 1e-12, 1e-12),
         "total momentum is incorrect");
  expect(close(mechanics::total_kinetic(bodies).si_value().item<double>(),
               3.75),
         "kinetic energy is incorrect");
  expect(close(mechanics::potential_energy(bodies, unit_gravity())
                   .si_value()
                   .item<double>(),
               -3.0),
         "pair potential energy is incorrect");
  expect(close(mechanics::total_energy(bodies, unit_gravity())
                   .si_value()
                   .item<double>(),
               0.75),
         "total mechanical energy is incorrect");
  expect(mechanics::angular_momentum(bodies).si_value().allclose(
             pluss::series::make({0.0, 0.0, -6.0}), 1e-12, 1e-12),
         "angular momentum is incorrect");

  const auto centered = mechanics::shift_to_center(bodies);
  expect(mechanics::center(centered).si_value().abs().max().item<double>() <
             1e-12,
         "center-frame shift left a position offset");
  expect(mechanics::center_speed(centered)
                 .si_value()
                 .abs()
                 .max()
                 .item<double>() < 1e-12,
         "center-frame shift left a velocity offset");
}

void test_direct_gravity() {
  const auto bodies = two_body();
  const auto acceleration = mechanics::gravity(bodies, unit_gravity());
  expect(acceleration.si_value().allclose(
             pluss::series::make(
                 {{0.75, 0.0, 0.0}, {-0.5, 0.0, 0.0}}),
             1e-12, 1e-12),
         "direct gravity disagrees with the pairwise reference");

  const auto net_force =
      acceleration.si_value().mul(bodies.mass().si_value().unsqueeze(-1)).sum(
          -2, false);
  expect(net_force.abs().max().item<double>() < 1e-12,
         "pairwise gravity violated equal-and-opposite force balance");

  const auto softened = mechanics::gravity(
      bodies, unit_gravity(),
      common::ScalarQuantity<common::Length>::from_si(2.0));
  const double expected = 6.0 / std::pow(8.0, 1.5);
  expect(close(softened.si_value().select(0, 0).select(0, 0).item<double>(),
               expected),
         "Plummer softening used the wrong r-squared convention");
}

void test_batches_views_and_autodiff() {
  const auto base = pluss::series::make(
      std::vector<double>{0, 2, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0},
      {2, 3, 2});
  auto position = base.transpose(1, 2);
  expect(!position.is_contiguous(), "test position must be a strided view");
  position.tracks_slope_(true);
  const auto bodies = mechanics::make(
      common::SeriesQuantity<common::Mass>::from_si(
          pluss::series::make({2.0, 3.0})),
      common::SeriesQuantity<common::Length>::from_si(position),
      common::SeriesQuantity<common::Speed>::from_si(
          pluss::series::zeros_like(position)));
  expect_shape(bodies.mass().si_value(), {2, 2},
               "mass did not broadcast across batch axes");
  expect_shape(mechanics::gravity(bodies, unit_gravity()).si_value(),
               {2, 2, 3}, "batched gravity changed state shape");

  mechanics::potential_energy(bodies, unit_gravity()).si_value().sum().reverse();
  expect(position.has_sensitivity(),
         "pairwise potential energy severed reverse history");
  expect_shape(position.sensitivity(), {2, 2, 3},
               "potential sensitivity changed strided input shape");
}

void test_leapfrog() {
  const auto body = mechanics::make(
      common::SeriesQuantity<common::Mass>::from_si(pluss::series::make(1.0)),
      common::SeriesQuantity<common::Length>::from_si(
          pluss::series::make(std::vector<double>{1.0}, {1, 1})),
      common::SeriesQuantity<common::Speed>::from_si(
          pluss::series::make(std::vector<double>{0.0}, {1, 1})));
  using InvTime2 = common::DimensionPower<common::Time, -2>;
  const auto omega2 = common::ScalarQuantity<InvTime2>::from_si(1.0);
  const auto next = mechanics::leapfrog(
      body, common::ScalarQuantity<common::Time>::from_si(0.1),
      [&](const mechanics::Bodies &state) {
        return -(state.position() * omega2);
      });
  expect(close(next.position().si_value().item<double>(), 0.995),
         "drift-kick-drift position is incorrect");
  expect(close(next.velocity().si_value().item<double>(), -0.1),
         "drift-kick-drift velocity is incorrect");
}

void test_failures() {
  bool negative_mass = false;
  try {
    (void)mechanics::make(
        common::SeriesQuantity<common::Mass>::from_si(
            pluss::series::make({1.0, -1.0})),
        common::SeriesQuantity<common::Length>::from_si(
            pluss::series::zeros({2, 3}, pluss::ScalarType::Float64)),
        common::SeriesQuantity<common::Speed>::from_si(
            pluss::series::zeros({2, 3}, pluss::ScalarType::Float64)));
  } catch (const std::invalid_argument &) {
    negative_mass = true;
  }
  expect(negative_mass, "negative body mass was accepted");

  bool collision = false;
  try {
    const auto coincident = mechanics::make(
        common::SeriesQuantity<common::Mass>::from_si(
            pluss::series::make({1.0, 1.0})),
        common::SeriesQuantity<common::Length>::from_si(
            pluss::series::zeros({2, 3}, pluss::ScalarType::Float64)),
        common::SeriesQuantity<common::Speed>::from_si(
            pluss::series::zeros({2, 3}, pluss::ScalarType::Float64)));
    (void)mechanics::gravity(coincident, unit_gravity());
  } catch (const std::domain_error &) {
    collision = true;
  }
  expect(collision, "unsoftened coincident bodies were accepted");

  bool wrong_axes = false;
  try {
    const auto planar = mechanics::make(
        common::SeriesQuantity<common::Mass>::from_si(
            pluss::series::make({1.0, 1.0})),
        common::SeriesQuantity<common::Length>::from_si(
            pluss::series::zeros({2, 2}, pluss::ScalarType::Float64)),
        common::SeriesQuantity<common::Speed>::from_si(
            pluss::series::zeros({2, 2}, pluss::ScalarType::Float64)));
    (void)mechanics::angular_momentum(planar);
  } catch (const std::invalid_argument &) {
    wrong_axes = true;
  }
  expect(wrong_axes, "3-D angular momentum accepted a planar state");
}

} // namespace

int main() {
  try {
    test_state_and_invariants();
    test_direct_gravity();
    test_batches_views_and_autodiff();
    test_leapfrog();
    test_failures();
    std::cout << "NatSci mechanics foundation checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}

