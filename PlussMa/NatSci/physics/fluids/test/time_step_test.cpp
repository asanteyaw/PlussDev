#include "pluss/ma/natsci/physics.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace fluids = pluss::subsp::natsci::physics::fluids;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

spatial::PeriodicGrid grid2() {
  return spatial::make_periodic_grid(
      {6, 4}, {common::ScalarQuantity<common::Length>::from_si(0.5),
               common::ScalarQuantity<common::Length>::from_si(0.25)});
}

fluids::IncompressibleState still(const spatial::PeriodicGrid &grid,
                                  std::int64_t batches = 0) {
  std::vector<std::int64_t> velocity_shape;
  std::vector<std::int64_t> pressure_shape;
  if (batches > 0) {
    velocity_shape.push_back(batches);
    pressure_shape.push_back(batches);
  }
  velocity_shape.insert(velocity_shape.end(), {2, 6, 4});
  pressure_shape.insert(pressure_shape.end(), {6, 4});
  return fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(
          pluss::series::zeros(velocity_shape, pluss::ScalarType::Float64)),
      common::SeriesQuantity<common::Pressure>::from_si(
          pluss::series::zeros(pressure_shape, pluss::ScalarType::Float64)),
      grid);
}

common::ScalarQuantity<fluids::Density> density() {
  return common::ScalarQuantity<fluids::Density>::from_si(1.0);
}

common::ScalarQuantity<fluids::KinematicViscosity> viscosity(double value) {
  return common::ScalarQuantity<fluids::KinematicViscosity>::from_si(value);
}

common::ScalarQuantity<common::Time> seconds(double value) {
  return common::ScalarQuantity<common::Time>::from_si(value);
}

fluids::ProjectionOptions quick_projection() {
  fluids::ProjectionOptions options;
  options.poisson.solve.max_iterations = 64;
  options.poisson.solve.absolute_tolerance = 1e-12;
  options.poisson.solve.relative_tolerance = 1e-12;
  return options;
}

void test_limits_are_batched_and_exact() {
  const auto grid = grid2();
  pluss::Series velocity =
      pluss::series::zeros({2, 2, 6, 4}, pluss::ScalarType::Float64);
  velocity.index_fill_(1, pluss::series::make({0}, pluss::ScalarType::Int64),
                       2.0);
  velocity.index_fill_(1, pluss::series::make({1}, pluss::ScalarType::Int64),
                       1.0);
  const auto state = fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(velocity),
      common::SeriesQuantity<common::Pressure>::from_si(
          pluss::series::zeros({2, 6, 4}, pluss::ScalarType::Float64)),
      grid);
  const auto limits = fluids::step_limit(state, grid, viscosity(0.1));
  expect(limits.allowed.si_value().sizes() == std::vector<std::int64_t>{2},
         "step limits collapsed batch lanes");
  expect(limits.convection.si_value().allclose(
             pluss::series::full({2}, 0.0625, pluss::ScalarType::Float64),
             1e-12, 1e-12),
         "convective CFL limit is incorrect");
  expect(limits.diffusion.si_value().allclose(
             pluss::series::full({2}, 0.25, pluss::ScalarType::Float64), 1e-12,
             1e-12),
         "viscous stability limit is incorrect");
}

void test_all_step_rules_and_body_force() {
  const auto grid = grid2();
  const auto initial = still(grid, 2);
  const pluss::Series body_values =
      pluss::series::full({2, 2, 6, 4}, 0.0, pluss::ScalarType::Float64)
          .index_fill(1, pluss::series::make({0}, pluss::ScalarType::Int64),
                      2.0)
          .index_fill(1, pluss::series::make({1}, pluss::ScalarType::Int64),
                      -1.0);
  const fluids::BodyFn body = [body_values](
                                  common::ScalarQuantity<common::Time>,
                                  const fluids::IncompressibleState &) {
    return common::SeriesQuantity<common::Acceleration>::from_si(body_values);
  };

  for (const auto rule : {fluids::StepRule::Euler, fluids::StepRule::Rk2,
                          fluids::StepRule::Rk3}) {
    fluids::StepOptions options;
    options.rule = rule;
    options.projection = quick_projection();
    const auto result =
        fluids::advance(initial, grid, density(), viscosity(0.0), seconds(1.5),
                        seconds(0.05), options, body);
    const pluss::Series expected = body_values.mul(0.05);
    expect(result.state.velocity().si_value().allclose(expected, 1e-11, 1e-11),
           "constant body acceleration was not integrated exactly");
    expect(result.state.pressure().si_value().eq(0.0).all().item<bool>(),
           "uniform body acceleration invented pressure");
    const std::int64_t stages = rule == fluids::StepRule::Euler
                                    ? 1
                                    : (rule == fluids::StepRule::Rk2 ? 2 : 3);
    expect(result.rate_evaluations == stages && result.projections == stages,
           "step rule reported the wrong stage count");
    expect(result.divergence.max_abs.si_value().eq(0.0).all().item<bool>(),
           "projected uniform acceleration is not divergence free");
  }
}

void test_time_dependent_body_uses_stage_clocks() {
  const auto grid = grid2();
  const auto initial = still(grid);
  const fluids::BodyFn body = [](
                                  common::ScalarQuantity<common::Time> time,
                                  const fluids::IncompressibleState &state) {
    return common::SeriesQuantity<common::Acceleration>::from_si(
        pluss::series::ones_like(state.velocity().si_value()) *
        time.si_value());
  };

  for (const auto [rule, expected] :
       {std::pair{fluids::StepRule::Euler, 0.2},
        std::pair{fluids::StepRule::Rk2, 0.22},
        std::pair{fluids::StepRule::Rk3, 0.22}}) {
    fluids::StepOptions options;
    options.rule = rule;
    options.projection = quick_projection();
    const auto result =
        fluids::advance(initial, grid, density(), viscosity(0.0), seconds(1.0),
                        seconds(0.2), options, body);
    expect(result.state.velocity().si_value().allclose(
               pluss::series::full_like(initial.velocity().si_value(),
                                        expected),
               1e-11, 1e-11),
           "body force was evaluated at the wrong Runge-Kutta stage time");
  }
}

void test_guard_rejection_and_trajectory() {
  const auto grid = grid2();
  pluss::Series velocity =
      pluss::series::ones({2, 6, 4}, pluss::ScalarType::Float64);
  const auto moving = fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(velocity),
      common::SeriesQuantity<common::Pressure>::from_si(
          pluss::series::zeros({6, 4}, pluss::ScalarType::Float64)),
      grid);
  bool rejected = false;
  try {
    (void)fluids::advance(moving, grid, density(), viscosity(0.0), seconds(0.0),
                          seconds(1.0));
  } catch (const std::domain_error &) {
    rejected = true;
  }
  expect(rejected, "advance accepted a step above its CFL bound");

  fluids::StepOptions options;
  options.rule = fluids::StepRule::Rk2;
  options.projection = quick_projection();
  const auto path = fluids::evolve(still(grid), grid, density(), viscosity(0.0),
                                   seconds(2.0), seconds(0.1), 3, options);
  expect(path.steps == 3 && path.rate_evaluations == 6 && path.projections == 6,
         "evolve lost scheduler or stage diagnostics");
  expect(path.trajectory.coordinate().si_value().allclose(
             pluss::series::make({2.0, 2.1, 2.2, 2.3}), 1e-12, 1e-12),
         "C3 march constructed the wrong physical coordinate");
  expect(path.trajectory.state()
                 .get<common::Speed>("velocity")
                 .si_value()
                 .sizes() == std::vector<std::int64_t>{4, 2, 6, 4},
         "C3 march did not preserve the velocity field layout");
  expect(path.trajectory.state()
                 .get<common::Pressure>("pressure")
                 .si_value()
                 .sizes() == std::vector<std::int64_t>{4, 6, 4},
         "C3 march did not preserve the pressure field layout");
}

void test_reverse_sensitivity_through_stages() {
  const auto grid = grid2();
  pluss::Series force =
      pluss::series::full({2, 6, 4}, 0.25, pluss::ScalarType::Float64);
  force.tracks_slope_(true);
  const fluids::BodyFn body = [&force](common::ScalarQuantity<common::Time>,
                                       const fluids::IncompressibleState &) {
    return common::SeriesQuantity<common::Acceleration>::from_si(force);
  };
  fluids::StepOptions options;
  options.rule = fluids::StepRule::Rk3;
  options.projection = quick_projection();
  const auto result =
      fluids::advance(still(grid), grid, density(), viscosity(0.0),
                      seconds(0.0), seconds(0.05), options, body);
  result.state.velocity().si_value().sum().reverse();
  expect(
      force.has_sensitivity() &&
          force.sensitivity().allclose(
              pluss::series::full({2, 6, 4}, 0.05, pluss::ScalarType::Float64),
              1e-10, 1e-10),
      "SSPRK3 projection severed body-force sensitivity");
}

} // namespace

int main() {
  try {
    test_limits_are_batched_and_exact();
    test_all_step_rules_and_body_force();
    test_time_dependent_body_uses_stage_clocks();
    test_guard_rejection_and_trajectory();
    test_reverse_sensitivity_through_stages();
    std::cout << "NatSci fluid time-step checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
