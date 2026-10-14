#include "pluss/ma/natsci/physics.h"

#include <cmath>
#include <cstdint>
#include <iostream>
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

spatial::BoxGrid grid2(std::int64_t nx = 8, std::int64_t ny = 12) {
  return spatial::make_box_grid(
      {nx, ny}, {common::ScalarQuantity<common::Length>::from_si(1.0 / nx),
                 common::ScalarQuantity<common::Length>::from_si(1.0 / ny)});
}

Series stack(const Series &x, const Series &y, std::int64_t axis = 0) {
  return x.unsqueeze(axis).unite({y.unsqueeze(axis)}, axis);
}

fluids::FlowBounds walls(const spatial::BoxGrid &grid) {
  return fluids::flow_bounds(grid, {fluids::no_slip({0, spatial::Side::Low}),
                                    fluids::no_slip({0, spatial::Side::High}),
                                    fluids::no_slip({1, spatial::Side::Low}),
                                    fluids::no_slip({1, spatial::Side::High})});
}

fluids::ProjectionOptions strict_projection() {
  fluids::ProjectionOptions options;
  options.poisson.solve.max_iterations = 800;
  options.poisson.solve.absolute_tolerance = 1e-12;
  options.poisson.solve.relative_tolerance = 1e-12;
  options.poisson.solve.check_interval = 1;
  options.poisson.mean_tolerance = 1e-12;
  return options;
}

void test_matched_bounded_projection() {
  const auto grid = grid2();
  const Series x = pluss::series::arange(0, 8, 1, ScalarType::Float64)
                       .add(0.5)
                       .div(8.0)
                       .reshape({8, 1})
                       .expand({8, 12}, true);
  const Series y = pluss::series::arange(0, 12, 1, ScalarType::Float64)
                       .add(0.5)
                       .div(12.0)
                       .reshape({1, 12})
                       .expand({8, 12}, true);
  const Series velocity = stack(x.mul(y), x.square().sub(y));
  const auto state = fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(velocity),
      common::SeriesQuantity<common::Pressure>::from_si(
          pluss::series::zeros({8, 12}, ScalarType::Float64)),
      grid);

  const auto result = fluids::project(state, grid, walls(grid), density(),
                                      seconds(0.1), strict_projection());
  expect(result.before.max_abs.si_value().item<double>() > 1.0,
         "bounded projection test started from a nearly solenoidal field");
  expect(result.after.max_abs.si_value().item<double>() < 2e-10,
         "matched bounded projection did not remove discrete divergence");
  expect(result.converged.all().item<bool>() &&
             !result.breakdown.any().item<bool>(),
         "matched bounded projection did not report convergence");
}

void test_closed_flux_compatibility() {
  const auto grid = grid2(6, 8);
  const Series inlet_profile =
      stack(pluss::series::ones({8}, ScalarType::Float64),
            pluss::series::zeros({8}, ScalarType::Float64));
  const auto bounds = fluids::flow_bounds(
      grid, {fluids::inlet(
                 {0, spatial::Side::Low},
                 common::SeriesQuantity<common::Speed>::from_si(inlet_profile)),
             fluids::no_slip({0, spatial::Side::High}),
             fluids::no_slip({1, spatial::Side::Low}),
             fluids::no_slip({1, spatial::Side::High})});
  const auto state = fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(
          pluss::series::zeros({2, 6, 8}, ScalarType::Float64)),
      common::SeriesQuantity<common::Pressure>::from_si(
          pluss::series::zeros({6, 8}, ScalarType::Float64)),
      grid);

  bool rejected = false;
  try {
    (void)fluids::project(state, grid, bounds, density(), seconds(0.1),
                          strict_projection());
  } catch (const std::domain_error &) {
    rejected = true;
  }
  expect(rejected,
         "closed bounded projection accepted a non-zero net boundary flux");
}

struct Channel final {
  fluids::IncompressibleState state;
  fluids::FlowBounds bounds;
  fluids::BodyFn body;
  Series profile;
};

Channel channel(const spatial::BoxGrid &grid, const Series &forcing,
                double nu) {
  const std::int64_t nx = grid.shape()[0];
  const std::int64_t ny = grid.shape()[1];
  const double h = grid.spacing(1).si_value();
  const double mid = 0.5 * static_cast<double>(ny - 1);
  const double curve = h * h / (2.0 * nu);
  const double peak = 0.5 * curve * ((mid + 1.0) * (mid + 1.0) + mid * mid);
  const Series j = pluss::series::arange(0, ny, 1, ScalarType::Float64);
  const Series shape = pluss::codem::make_scalar_like(j, peak).sub(
      j.sub(mid).square().mul(curve));
  const Series profile = forcing.unsqueeze(-1).mul(shape);
  const std::vector<std::int64_t> batches(forcing.sizes().begin(),
                                          forcing.sizes().end());
  const Series zero_profile = pluss::series::zeros_like(profile);
  const Series inlet_profile = stack(profile, zero_profile, forcing.dim());

  std::vector<std::int64_t> x_view = batches;
  x_view.push_back(1);
  x_view.push_back(ny);
  std::vector<std::int64_t> scalar_shape = batches;
  scalar_shape.push_back(nx);
  scalar_shape.push_back(ny);
  const Series ux = profile.reshape(x_view).expand(scalar_shape, true);
  const Series uy = pluss::series::zeros_like(ux);
  const Series velocity = stack(ux, uy, forcing.dim());
  const auto state = fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(velocity),
      common::SeriesQuantity<common::Pressure>::from_si(
          pluss::series::zeros_like(ux)),
      grid);
  const auto bounds = fluids::flow_bounds(
      grid, {fluids::inlet(
                 {0, spatial::Side::Low},
                 common::SeriesQuantity<common::Speed>::from_si(inlet_profile)),
             fluids::outlet({0, spatial::Side::High}),
             fluids::no_slip({1, spatial::Side::Low}),
             fluids::no_slip({1, spatial::Side::High})});

  std::vector<std::int64_t> force_view = batches;
  force_view.push_back(1);
  force_view.push_back(1);
  const Series ax = forcing.reshape(force_view).expand(scalar_shape, true);
  const Series acceleration =
      stack(ax, pluss::series::zeros_like(ax), forcing.dim());
  const fluids::BodyFn body = [acceleration](
                                  common::ScalarQuantity<common::Time>,
                                  const fluids::IncompressibleState &) {
    return common::SeriesQuantity<common::Acceleration>::from_si(acceleration);
  };
  return {state, bounds, body, profile};
}

void test_poiseuille_equilibrium_batches_and_path() {
  const auto grid = grid2(10, 16);
  const Series forcing = pluss::series::make({0.8, 1.2});
  const auto setup = channel(grid, forcing, 0.2);
  const auto rate =
      fluids::mom_rate(setup.state, grid, setup.bounds, density(),
                       viscosity(0.2), setup.body(seconds(0.0), setup.state));
  expect(rate.si_value().abs().amax().item<double>() < 2e-12,
         "discrete Poiseuille profile is not a momentum equilibrium");
  expect(fluids::div_report(setup.state.velocity(), grid, setup.bounds)
             .max_abs.si_value()
             .eq(0.0)
             .all()
             .item<bool>(),
         "plane Poiseuille profile is not discretely divergence free");

  fluids::StepOptions options;
  options.rule = fluids::StepRule::Rk3;
  options.projection = strict_projection();
  const auto next = fluids::advance(setup.state, grid, setup.bounds, density(),
                                    viscosity(0.2), seconds(0.0),
                                    seconds(0.001), options, setup.body);
  expect(next.state.velocity().si_value().allclose(
             setup.state.velocity().si_value(), 2e-11, 2e-11),
         "bounded SSPRK3 did not preserve steady Poiseuille flow");
  expect(next.limits.allowed.si_value().sizes() == std::vector<std::int64_t>{2},
         "bounded step limits collapsed batch lanes");
  expect(next.rate_evaluations == 3 && next.projections == 3,
         "bounded SSPRK3 reported the wrong stage count");

  const auto path =
      fluids::evolve(setup.state, grid, setup.bounds, density(), viscosity(0.2),
                     seconds(0.0), seconds(0.001), 2, options, setup.body);
  expect(path.steps == 2 && path.rate_evaluations == 6 && path.projections == 6,
         "bounded trajectory lost scheduler or stage diagnostics");
  expect(path.trajectory.state()
                 .get<common::Speed>("velocity")
                 .si_value()
                 .sizes() == std::vector<std::int64_t>{3, 2, 2, 10, 16},
         "bounded trajectory did not preserve time, batch, and field axes");

  const Series y =
      pluss::series::arange(0, 16, 1, ScalarType::Float64).add(0.5).div(16.0);
  const Series continuum = y.mul(pluss::codem::make_scalar_like(y, 1.0).sub(y))
                               .mul(0.8 / (2.0 * 0.2));
  expect(setup.profile.select(0, 0).allclose(continuum, 2e-3, 2e-3),
         "discrete channel equilibrium does not approach the analytic "
         "Poiseuille parabola");
}

void test_reverse_sensitivity() {
  const auto grid = grid2(6, 10);
  Series forcing = pluss::series::make({0.7, 1.1});
  forcing.tracks_slope_(true);
  const auto setup = channel(grid, forcing, 0.25);
  fluids::StepOptions options;
  options.rule = fluids::StepRule::Rk2;
  options.projection = strict_projection();
  const auto result = fluids::advance(setup.state, grid, setup.bounds,
                                      density(), viscosity(0.25), seconds(0.0),
                                      seconds(0.005), options, setup.body);
  result.state.velocity().si_value().sum().reverse();
  expect(forcing.has_sensitivity() &&
             forcing.sensitivity().isfinite().all().item<bool>() &&
             forcing.sensitivity().gt(0.0).all().item<bool>(),
         "bounded stepper severed body/profile reverse sensitivity");
}

void test_steady_controller() {
  const auto grid = grid2(6, 10);
  const auto setup = channel(grid, pluss::series::make({0.6, 1.0}), 0.25);
  fluids::SteadyOptions options;
  options.max_steps = 4;
  options.check_interval = 1;
  options.required_checks = 2;
  options.absolute_tolerance =
      common::ScalarQuantity<common::Acceleration>::from_si(1e-10);
  options.relative_tolerance = 0.0;
  options.step.rule = fluids::StepRule::Rk2;
  options.step.projection = strict_projection();
  const auto steady = fluids::settle(setup.state, grid, setup.bounds, density(),
                                     viscosity(0.25), seconds(0.0),
                                     seconds(0.002), options, setup.body);
  expect(steady.steps == 2 && steady.rate_evaluations == 4 &&
             steady.projections == 4,
         "settle ignored its consecutive-check or stage-count contract");
  expect(steady.converged.sizes() == std::vector<std::int64_t>{2} &&
             steady.converged.all().item<bool>(),
         "settle collapsed or failed a batched steady decision");
  expect(steady.residual.si_value().le(1e-10).all().item<bool>(),
         "settle reported a residual above its absolute tolerance");
  expect(std::abs(steady.time.si_value() - 0.004) < 1e-15,
         "settle reported the wrong physical completion time");

  fluids::SteadyOptions limited = options;
  limited.max_steps = 1;
  limited.required_checks = 2;
  limited.require_convergence = false;
  const auto incomplete = fluids::settle(
      setup.state, grid, setup.bounds, density(), viscosity(0.25), seconds(0.0),
      seconds(0.002), limited, setup.body);
  expect(!incomplete.converged.any().item<bool>() && incomplete.steps == 1,
         "settle did not report an exhausted convergence budget");

  const auto rejects = [&](const fluids::SteadyOptions &invalid,
                           common::ScalarQuantity<common::Time> step) {
    try {
      (void)fluids::settle(setup.state, grid, setup.bounds, density(),
                           viscosity(0.25), seconds(0.0), step, invalid,
                           setup.body);
    } catch (const std::invalid_argument &) {
      return true;
    }
    return false;
  };

  fluids::SteadyOptions invalid = options;
  invalid.max_steps = 0;
  expect(rejects(invalid, seconds(0.002)),
         "settle accepted a non-positive step budget");
  invalid = options;
  invalid.check_interval = 0;
  expect(rejects(invalid, seconds(0.002)),
         "settle accepted a non-positive inspection interval");
  invalid = options;
  invalid.required_checks = 0;
  expect(rejects(invalid, seconds(0.002)),
         "settle accepted a non-positive consecutive-check count");
  invalid = options;
  invalid.absolute_tolerance =
      common::ScalarQuantity<common::Acceleration>::from_si(-1.0);
  expect(rejects(invalid, seconds(0.002)),
         "settle accepted a negative absolute tolerance");
  invalid = options;
  invalid.relative_tolerance = -1.0;
  expect(rejects(invalid, seconds(0.002)),
         "settle accepted a negative relative tolerance");
  expect(rejects(options, seconds(0.0)),
         "settle accepted a non-positive physical time step");
}

void test_mps_projection_when_available() {
  if (!pluss::codem::mps_backend_available())
    return;

  const auto grid = grid2(4, 4);
  const Series x = pluss::series::arange(0, 4, 1, ScalarType::Float32)
                       .add(0.5)
                       .reshape({4, 1})
                       .expand({4, 4}, true);
  const Series y = pluss::series::arange(0, 4, 1, ScalarType::Float32)
                       .add(0.5)
                       .reshape({1, 4})
                       .expand({4, 4}, true);
  const auto state = fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(
          stack(x.mul(y), x.sub(y)).to(pluss::pMPS)),
      common::SeriesQuantity<common::Pressure>::from_si(
          pluss::series::zeros({4, 4}, ScalarType::Float32).to(pluss::pMPS)),
      grid);
  fluids::ProjectionOptions options;
  options.poisson.solve.max_iterations = 128;
  options.poisson.solve.absolute_tolerance = 1e-5;
  options.poisson.solve.relative_tolerance = 1e-5;
  options.poisson.solve.check_interval = 1;
  options.poisson.mean_tolerance = 1e-5;
  const auto result = fluids::project(state, grid, walls(grid), density(),
                                      seconds(0.1), options);
  expect(result.state.velocity().si_value().platform() == pluss::pMPS,
         "bounded projection moved an MPS field to the host");
  expect(result.after.max_abs.si_value().item<float>() < 2e-4F,
         "MPS bounded projection did not satisfy divergence tolerance");
}

} // namespace

int main() {
  try {
    test_matched_bounded_projection();
    test_closed_flux_compatibility();
    test_poiseuille_equilibrium_batches_and_path();
    test_reverse_sensitivity();
    test_steady_controller();
    test_mps_projection_when_available();
    std::cout << "NatSci bounded fluid step checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
