#include "pluss/ma/natsci/physics.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace fluids = pluss::subsp::natsci::physics::fluids;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

spatial::BoxGrid grid2() {
  return spatial::make_box_grid(
      {2, 3}, {common::ScalarQuantity<common::Length>::from_si(1.0),
               common::ScalarQuantity<common::Length>::from_si(1.0)});
}

pluss::Series velocity_values() {
  return pluss::series::make(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0,
                                                 10.0, 11.0, 12.0, 13.0, 14.0,
                                                 15.0},
                             {2, 2, 3});
}

fluids::FlowBounds mixed_bounds(const spatial::BoxGrid &grid) {
  const auto inlet = common::SeriesQuantity<common::Speed>::from_si(
      pluss::series::make({5.0, 6.0}));
  return fluids::flow_bounds(
      grid,
      {fluids::no_slip({0, spatial::Side::Low}),
       fluids::inlet({0, spatial::Side::High}, inlet),
       fluids::free_slip({1, spatial::Side::Low}),
       fluids::outlet({1, spatial::Side::High},
                      common::ScalarQuantity<common::Pressure>::from_si(2.0))});
}

void test_flow_reconstruction() {
  const auto grid = grid2();
  const auto velocity =
      common::SeriesQuantity<common::Speed>::from_si(velocity_values());
  const auto bounds = mixed_bounds(grid);

  const auto x = fluids::vel_ghosts(velocity, grid, bounds, 0);
  expect(x.low.si_value().allclose(
             pluss::series::make({{-1.0, -2.0, -3.0}, {-10.0, -11.0, -12.0}}),
             1e-12, 1e-12),
         "stationary no-slip wall did not reverse every velocity component");
  expect(x.high.si_value().allclose(
             pluss::series::make({{6.0, 5.0, 4.0}, {-1.0, -2.0, -3.0}}), 1e-12,
             1e-12),
         "inlet profile did not impose the prescribed face velocity");

  const auto y = fluids::vel_ghosts(velocity, grid, bounds, 1);
  expect(y.low.si_value().allclose(
             pluss::series::make({{1.0, 4.0}, {-10.0, -13.0}}), 1e-12, 1e-12),
         "free-slip wall did not reflect normal and copy tangential flow");
  expect(y.high.si_value().allclose(
             pluss::series::make({{3.0, 6.0}, {12.0, 15.0}}), 1e-12, 1e-12),
         "outlet did not apply zero normal velocity gradient");
}

void test_plane_profile_and_pressure_policy() {
  const auto grid = grid2();
  const auto velocity =
      common::SeriesQuantity<common::Speed>::from_si(velocity_values());
  const auto wall = common::SeriesQuantity<common::Speed>::from_si(
      pluss::series::make({{2.0, 3.0, 4.0}, {7.0, 8.0, 9.0}}));
  const auto bounds = fluids::flow_bounds(
      grid, {fluids::no_slip({0, spatial::Side::Low}, wall),
             fluids::inlet({0, spatial::Side::High},
                           common::SeriesQuantity<common::Speed>::from_si(
                               pluss::series::make({5.0, 6.0}))),
             fluids::free_slip({1, spatial::Side::Low}),
             fluids::outlet({1, spatial::Side::High})});
  const auto ghost = fluids::vel_ghosts(velocity, grid, bounds, 0);
  expect(ghost.low.si_value().allclose(
             pluss::series::make({{3.0, 4.0, 5.0}, {4.0, 5.0, 6.0}}), 1e-12,
             1e-12),
         "face-shaped moving-wall profile was not applied lane-wise");

  const auto pressure = fluids::p_bounds(grid, mixed_bounds(grid));
  expect(pressure.at(0, spatial::Side::Low).kind() ==
                 spatial::BoundKind::Flux &&
             pressure.at(0, spatial::Side::High).kind() ==
                 spatial::BoundKind::Flux &&
             pressure.at(1, spatial::Side::Low).kind() ==
                 spatial::BoundKind::Flux &&
             pressure.at(1, spatial::Side::High).kind() ==
                 spatial::BoundKind::Value &&
             !pressure.all_flux(),
         "flow faces were not mapped to the bounded pressure contract");

  using PressureSource =
      common::DimensionQuotient<common::Pressure, common::Area>;
  spatial::PoissonOptions options;
  options.solve.max_iterations = 64;
  options.solve.relative_tolerance = 1e-12;
  options.solve.absolute_tolerance = 1e-12;
  options.solve.check_interval = 1;
  const auto solved = spatial::solve_poisson(
      common::SeriesQuantity<PressureSource>::from_si(
          pluss::series::zeros({2, 3}, pluss::ScalarType::Float64)),
      grid, pressure, options);
  expect(solved.solution.si_value().allclose(
             pluss::series::full({2, 3}, 2.0, pluss::ScalarType::Float64),
             1e-10, 1e-10),
         "outlet pressure and wall flux rules did not form a valid bounded "
         "Poisson problem");
}

void test_batches_and_three_dimensions() {
  const auto grid = grid2();
  const pluss::Series base = velocity_values();
  const pluss::Series batched =
      base.unsqueeze(0).unite({base.mul(2.0).unsqueeze(0)}, 0);
  const auto velocity = common::SeriesQuantity<common::Speed>::from_si(batched);
  const auto x = fluids::vel_ghosts(velocity, grid, mixed_bounds(grid), 0);
  expect(x.low.si_value().equal(batched.select(2, 0).neg()),
         "no-slip reconstruction did not preserve leading batch lanes");

  const auto grid3 = spatial::make_box_grid(
      {2, 2, 2}, {common::ScalarQuantity<common::Length>::from_si(1.0),
                  common::ScalarQuantity<common::Length>::from_si(1.0),
                  common::ScalarQuantity<common::Length>::from_si(1.0)});
  std::vector<double> raw(24);
  for (std::size_t index = 0; index < raw.size(); ++index)
    raw[index] = static_cast<double>(index + 1);
  const pluss::Series values = pluss::series::make(raw, {3, 2, 2, 2});
  const auto bounds3 =
      fluids::flow_bounds(grid3, {fluids::no_slip({0, spatial::Side::Low}),
                                  fluids::no_slip({0, spatial::Side::High}),
                                  fluids::no_slip({1, spatial::Side::Low}),
                                  fluids::no_slip({1, spatial::Side::High}),
                                  fluids::free_slip({2, spatial::Side::Low}),
                                  fluids::outlet({2, spatial::Side::High})});
  const auto z =
      fluids::vel_ghosts(common::SeriesQuantity<common::Speed>::from_si(values),
                         grid3, bounds3, 2);
  const pluss::Series low = values.select(3, 0);
  const pluss::Series high = values.select(3, 1);
  expect(z.low.si_value().select(0, 0).equal(low.select(0, 0)) &&
             z.low.si_value().select(0, 1).equal(low.select(0, 1)) &&
             z.low.si_value().select(0, 2).equal(low.select(0, 2).neg()),
         "three-dimensional free-slip reconstruction confused normal and "
         "tangential components");
  expect(z.high.si_value().equal(high),
         "three-dimensional outlet changed a velocity component");
}

void test_reverse_and_failures() {
  const auto grid = grid2();
  pluss::Series tracked = velocity_values();
  tracked.tracks_slope_(true);
  const auto velocity = common::SeriesQuantity<common::Speed>::from_si(tracked);
  fluids::vel_ghosts(velocity, grid, mixed_bounds(grid), 0)
      .high.si_value()
      .square()
      .sum()
      .reverse();
  expect(tracked.has_sensitivity() &&
             tracked.sensitivity().isfinite().all().item<bool>(),
         "fluid ghost reconstruction severed reverse sensitivity");

  bool missing = false;
  try {
    (void)fluids::flow_bounds(grid, {fluids::no_slip({0, spatial::Side::Low})});
  } catch (const std::invalid_argument &) {
    missing = true;
  }
  expect(missing, "flow boundary set accepted missing faces");

  bool bad_profile = false;
  try {
    const auto bad = fluids::flow_bounds(
        grid, {fluids::no_slip({0, spatial::Side::Low}),
               fluids::inlet({0, spatial::Side::High},
                             common::SeriesQuantity<common::Speed>::from_si(
                                 pluss::series::make({1.0, 2.0, 3.0, 4.0}))),
               fluids::free_slip({1, spatial::Side::Low}),
               fluids::outlet({1, spatial::Side::High})});
    (void)fluids::vel_ghosts(
        common::SeriesQuantity<common::Speed>::from_si(velocity_values()), grid,
        bad, 0);
  } catch (const std::invalid_argument &) {
    bad_profile = true;
  }
  expect(bad_profile, "velocity reconstruction accepted a malformed profile");
}

} // namespace

int main() {
  try {
    test_flow_reconstruction();
    test_plane_profile_and_pressure_policy();
    test_batches_and_three_dimensions();
    test_reverse_and_failures();
    std::cout << "NatSci fluid boundary checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
