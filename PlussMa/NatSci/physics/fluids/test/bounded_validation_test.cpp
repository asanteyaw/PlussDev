#include "pluss/ma/natsci/physics.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace fluids = pluss::subsp::natsci::physics::fluids;

using pluss::ScalarType;
using pluss::Series;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
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

spatial::BoxGrid channel_grid(std::int64_t ny) {
  return spatial::make_box_grid(
      {3, ny}, {common::ScalarQuantity<common::Length>::from_si(1.0 / 3.0),
                common::ScalarQuantity<common::Length>::from_si(1.0 / ny)});
}

Series stack(const Series &x, const Series &y, std::int64_t axis) {
  return x.unsqueeze(axis).unite({y.unsqueeze(axis)}, axis);
}

fluids::ProjectionOptions strict_projection() {
  fluids::ProjectionOptions options;
  options.poisson.solve.max_iterations = 512;
  options.poisson.solve.absolute_tolerance = 1e-12;
  options.poisson.solve.relative_tolerance = 1e-12;
  options.poisson.solve.check_interval = 1;
  options.poisson.mean_tolerance = 1e-12;
  return options;
}

struct StartUpChannel final {
  fluids::IncompressibleState initial;
  fluids::FlowBounds bounds;
  fluids::BodyFn body;
};

StartUpChannel start_up_channel(const spatial::BoxGrid &grid,
                                const Series &forcing) {
  const std::int64_t nx = grid.shape()[0];
  const std::int64_t ny = grid.shape()[1];
  const std::vector<std::int64_t> batches(forcing.sizes().begin(),
                                          forcing.sizes().end());
  std::vector<std::int64_t> scalar_shape = batches;
  scalar_shape.push_back(nx);
  scalar_shape.push_back(ny);
  std::vector<std::int64_t> force_view = batches;
  force_view.push_back(1);
  force_view.push_back(1);
  const Series ax = forcing.reshape(force_view).expand(scalar_shape, true);
  const Series velocity = stack(pluss::series::zeros_like(ax),
                                pluss::series::zeros_like(ax), forcing.dim());
  const Series acceleration =
      stack(ax, pluss::series::zeros_like(ax), forcing.dim());
  const auto initial = fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(velocity),
      common::SeriesQuantity<common::Pressure>::from_si(
          pluss::series::zeros_like(ax)),
      grid);
  const auto bounds =
      fluids::flow_bounds(grid, {fluids::outlet({0, spatial::Side::Low}),
                                 fluids::outlet({0, spatial::Side::High}),
                                 fluids::no_slip({1, spatial::Side::Low}),
                                 fluids::no_slip({1, spatial::Side::High})});
  const fluids::BodyFn body = [acceleration](
                                  common::ScalarQuantity<common::Time>,
                                  const fluids::IncompressibleState &) {
    return common::SeriesQuantity<common::Acceleration>::from_si(acceleration);
  };
  return {initial, bounds, body};
}

Series transient_poiseuille(const Series &forcing, std::int64_t ny, double nu,
                            double time, std::int64_t terms = 128) {
  const Series y = pluss::series::arange(0, ny, 1, ScalarType::Float64)
                       .add(0.5)
                       .div(static_cast<double>(ny));
  const Series mode =
      pluss::series::arange(1, 2 * terms, 2, ScalarType::Float64);
  const Series wave = mode.unsqueeze(-1).mul(std::numbers::pi);
  const Series decay =
      mode.square()
          .mul(-nu * std::numbers::pi * std::numbers::pi * time)
          .exp()
          .neg()
          .add(1.0)
          .div(mode.pow(3.0));
  const Series shape = wave.mul(y.unsqueeze(0))
                           .sin()
                           .mul(decay.unsqueeze(-1))
                           .sum(0, false)
                           .mul(4.0 / (nu * std::numbers::pi *
                                       std::numbers::pi * std::numbers::pi));
  return forcing.unsqueeze(-1).mul(shape);
}

Series march_channel(const StartUpChannel &setup, const spatial::BoxGrid &grid,
                     double nu, double stop, std::int64_t steps) {
  fluids::StepOptions options;
  options.rule = fluids::StepRule::Rk2;
  options.projection = strict_projection();
  auto state = setup.initial;
  const double dt = stop / static_cast<double>(steps);
  for (std::int64_t step = 0; step < steps; ++step)
    state = fluids::advance(state, grid, setup.bounds, density(), viscosity(nu),
                            seconds(static_cast<double>(step) * dt),
                            seconds(dt), options, setup.body)
                .state;
  return state.velocity().si_value();
}

void test_transient_poiseuille_convergence() {
  constexpr double nu = 0.1;
  constexpr double stop = 0.2;
  const Series forcing = pluss::series::make({0.7, 1.0});
  double previous = 0.0;
  for (const std::int64_t ny : {8, 16, 32}) {
    const auto grid = channel_grid(ny);
    const auto setup = start_up_channel(grid, forcing);
    const double h = 1.0 / static_cast<double>(ny);
    const std::int64_t steps =
        static_cast<std::int64_t>(std::ceil(stop / (0.5 * h * h)));
    const Series velocity = march_channel(setup, grid, nu, stop, steps);
    const Series profile = velocity.select(1, 0).mean(1, false);
    const Series exact = transient_poiseuille(forcing, ny, nu, stop);
    const double error = profile.sub(exact).abs().amax().item<double>();
    if (previous > 0.0)
      expect(previous / error > 3.0,
             "transient Poiseuille profile is not second-order under grid "
             "refinement");
    previous = error;

    expect(velocity.select(1, 1).abs().amax().item<double>() < 1e-12,
           "start-up channel invented cross-stream velocity");
    expect(fluids::div_report(
               common::SeriesQuantity<common::Speed>::from_si(velocity), grid,
               setup.bounds)
                   .max_abs.si_value()
                   .abs()
                   .amax()
                   .item<double>() < 1e-11,
           "start-up channel lost incompressibility");
  }
  expect(previous < 2e-3,
         "finest transient Poiseuille profile exceeds its accuracy gate");
}

struct Cavity final {
  spatial::BoxGrid grid;
  fluids::IncompressibleState initial;
  fluids::FlowBounds bounds;
};

Cavity cavity(std::int64_t n) {
  auto grid = spatial::make_box_grid(
      {n, n}, {common::ScalarQuantity<common::Length>::from_si(1.0 / n),
               common::ScalarQuantity<common::Length>::from_si(1.0 / n)});
  const auto lid = common::SeriesQuantity<common::Speed>::from_si(
      pluss::series::make({1.0, 0.0}));
  auto bounds = fluids::flow_bounds(
      grid, {fluids::no_slip({0, spatial::Side::Low}),
             fluids::no_slip({0, spatial::Side::High}),
             fluids::no_slip({1, spatial::Side::Low}),
             fluids::no_slip({1, spatial::Side::High}, lid)});
  auto initial = fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(
          pluss::series::zeros({2, n, n}, ScalarType::Float64)),
      common::SeriesQuantity<common::Pressure>::from_si(
          pluss::series::zeros({n, n}, ScalarType::Float64)),
      grid);
  return {std::move(grid), std::move(initial), std::move(bounds)};
}

std::pair<double, double> ghia_error(const Series &velocity, std::int64_t n) {
  const std::int64_t lower = n / 2 - 1;
  const std::int64_t upper = n / 2;
  const Series u = velocity.select(0, 0);
  const Series v = velocity.select(0, 1);
  const Series u_center = u.select(0, lower).add(u.select(0, upper)).mul(0.5);
  const Series v_center = v.select(1, lower).add(v.select(1, upper)).mul(0.5);
  const Series coordinate = pluss::series::arange(0, n, 1, ScalarType::Float64)
                                .add(0.5)
                                .div(static_cast<double>(n));
  const Series y = pluss::series::make(
      {0.9766, 0.8516, 0.7344, 0.6172, 0.5000, 0.2813, 0.1719, 0.1016});
  const Series u_reference =
      pluss::series::make({0.84123, 0.23151, 0.00332, -0.13641, -0.20581,
                           -0.15662, -0.10150, -0.06434});
  const Series x = pluss::series::make(
      {0.9688, 0.8594, 0.8047, 0.5000, 0.2344, 0.1563, 0.0938});
  const Series v_reference = pluss::series::make(
      {-0.05906, -0.22445, -0.24533, 0.05454, 0.17527, 0.16077, 0.12317});
  const Series sampled_u = pluss::subsp::math::interp::linear(
      coordinate, u_center, y, pluss::subsp::math::interp::EdgeMode::Linear);
  const Series sampled_v = pluss::subsp::math::interp::linear(
      coordinate, v_center, x, pluss::subsp::math::interp::EdgeMode::Linear);
  return {sampled_u.sub(u_reference).abs().amax().item<double>(),
          sampled_v.sub(v_reference).abs().amax().item<double>()};
}

void test_lid_driven_cavity_mesh_study() {
  double previous_u = 0.0;
  double previous_v = 0.0;
  for (const std::int64_t n : {8, 16}) {
    const auto setup = cavity(n);
    fluids::SteadyOptions options;
    options.max_steps = 3'000;
    options.check_interval = n == 8 ? 25 : 50;
    options.required_checks = 2;
    options.absolute_tolerance =
        common::ScalarQuantity<common::Acceleration>::from_si(2e-4);
    options.relative_tolerance = 1e-4;
    options.require_convergence = true;
    options.step.rule = fluids::StepRule::Rk2;
    options.step.projection = strict_projection();
    const double dt = 0.32 / static_cast<double>(n);
    const auto result =
        fluids::settle(setup.initial, setup.grid, setup.bounds, density(),
                       viscosity(0.01), seconds(0.0), seconds(dt), options);
    const auto [u_error, v_error] =
        ghia_error(result.state.velocity().si_value(), n);
    expect(result.converged.all().item<bool>(),
           "Re=100 cavity did not reach the stated steady residual");
    if (previous_u > 0.0)
      expect(u_error < previous_u && v_error < previous_v,
             "Ghia centerline error did not decrease under cavity grid "
             "refinement");
    previous_u = u_error;
    previous_v = v_error;
  }
  expect(previous_u < 0.06 && previous_v < 0.06,
         "finest Re=100 cavity centerlines exceed the Ghia accuracy gate");
}

} // namespace

int main() {
  try {
    test_transient_poiseuille_convergence();
    test_lid_driven_cavity_mesh_study();
    std::cout << "NatSci bounded fluid validation checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
