#include "pluss/ma/natsci/physics.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace gas = pluss::subsp::natsci::physics::fluids::gas;

using pluss::ScalarType;
using pluss::Series;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

template <class Fn> void expect_failure(Fn &&fn, const std::string &message) {
  try {
    std::forward<Fn>(fn)();
  } catch (const std::exception &) {
    return;
  }
  throw std::runtime_error(message);
}

common::ScalarQuantity<common::Time> seconds(double value) {
  return common::ScalarQuantity<common::Time>::from_si(value);
}

spatial::PeriodicGrid grid1(std::int64_t cells, double width = 1.0) {
  return spatial::make_periodic_grid(
      {cells}, {common::ScalarQuantity<common::Length>::from_si(width)});
}

spatial::PeriodicGrid grid2(std::int64_t nx, std::int64_t ny, double dx,
                            double dy) {
  return spatial::make_periodic_grid(
      {nx, ny}, {common::ScalarQuantity<common::Length>::from_si(dx),
                 common::ScalarQuantity<common::Length>::from_si(dy)});
}

gas::State primitive(const Series &rho, const Series &velocity,
                     const Series &pressure, const spatial::PeriodicGrid &grid,
                     const gas::Model &model = {}) {
  const std::int64_t component = rho.dim() - grid.dim();
  const Series momentum = velocity.mul(rho.unsqueeze(component));
  const Series kinetic =
      velocity.square().sum(component, false).mul(rho).mul(0.5);
  const Series energy = pressure.div(model.gamma - 1.0).add(kinetic);
  return gas::make(common::SeriesQuantity<gas::Density>::from_si(rho),
                   common::SeriesQuantity<gas::MomDensity>::from_si(momentum),
                   common::SeriesQuantity<gas::EnergyDensity>::from_si(energy),
                   grid, model);
}

gas::State final_state(const gas::FlowPath &path,
                       const spatial::PeriodicGrid &grid,
                       const gas::Model &model = {}) {
  const auto &record = path.trajectory.state();
  return gas::make(
      common::SeriesQuantity<gas::Density>::from_si(
          record.get<gas::Density>("density").si_value().select(0, -1)),
      common::SeriesQuantity<gas::MomDensity>::from_si(
          record.get<gas::MomDensity>("momentum").si_value().select(0, -1)),
      common::SeriesQuantity<gas::EnergyDensity>::from_si(
          record.get<gas::EnergyDensity>("energy").si_value().select(0, -1)),
      grid, model);
}

std::vector<std::int64_t> spatial_axes(const Series &value,
                                       const spatial::PeriodicGrid &grid) {
  std::vector<std::int64_t> axes;
  for (std::int64_t axis = value.dim() - grid.dim(); axis < value.dim(); ++axis)
    axes.push_back(axis);
  return axes;
}

void test_constant_batch_and_cfl() {
  constexpr std::int64_t nx = 7;
  constexpr std::int64_t ny = 5;
  constexpr double dx = 0.2;
  constexpr double dy = 0.3;
  const auto grid = grid2(nx, ny, dx, dy);

  const Series rho =
      pluss::series::full({2, nx, ny}, 1.25, ScalarType::Float64);
  const Series pressure =
      pluss::series::full({2, nx, ny}, 0.9, ScalarType::Float64);
  Series u = pluss::series::full({2, nx, ny}, 0.4, ScalarType::Float64);
  Series v = pluss::series::full({2, nx, ny}, -0.15, ScalarType::Float64);
  const Series velocity = u.unsqueeze(1).unite({v.unsqueeze(1)}, 1);
  const auto initial = primitive(rho, velocity, pressure, grid);

  constexpr double courant = 0.45;
  const double sound = std::sqrt(1.4 * 0.9 / 1.25);
  const double expected = courant / ((0.4 + sound) / dx + (0.15 + sound) / dy);
  expect(gas::step_limit(initial, grid, {}, courant)
             .si_value()
             .allclose(pluss::series::full({2}, expected, ScalarType::Float64),
                       1e-13, 1e-13),
         "compressible acoustic CFL is not the multidimensional unsplit limit");

  gas::Options options;
  options.rule = gas::Rule::Rk3;
  const auto advanced =
      gas::step(initial, grid, seconds(0.25 * expected), {}, options);
  expect(advanced.flux_evaluations == 3,
         "RK3 reported the wrong flux evaluation count");
  expect(
      advanced.state.density().si_value().equal(rho) &&
          advanced.state.momentum().si_value().equal(
              initial.momentum().si_value()) &&
          advanced.state.energy().si_value().equal(initial.energy().si_value()),
      "a uniform batched two-dimensional gas state did not remain exact");
}

void test_periodic_conservation_and_fixed_path() {
  constexpr std::int64_t n = 64;
  constexpr double dx = 1.0 / static_cast<double>(n);
  const auto grid = grid1(n, dx);
  const Series x =
      pluss::series::arange(0, n, 1, ScalarType::Float64).add(0.5).mul(dx);
  const Series rho = x.mul(6.283185307179586).sin().mul(0.1).add(1.0);
  const Series speed = x.mul(6.283185307179586).cos().mul(0.05).add(0.2);
  const Series pressure = x.mul(12.566370614359172).sin().mul(0.05).add(1.0);
  const auto initial = primitive(rho, speed.unsqueeze(0), pressure, grid);

  gas::Options options;
  options.rule = gas::Rule::Rk2;
  const double width =
      gas::step_limit(initial, grid, {}, 0.4).si_value().item<double>() * 0.5;
  const auto path =
      gas::evolve(initial, grid, seconds(0.0), seconds(width), 4, {}, options);
  const auto end = final_state(path, grid);
  expect(path.steps == 4 && path.flux_evaluations == 8 &&
             path.trajectory.coordinate().si_value().sizes() ==
                 std::vector<std::int64_t>{5},
         "fixed gas evolution reported the wrong trajectory accounting");

  const auto rho_axes = spatial_axes(rho, grid);
  const auto mom_axes = spatial_axes(initial.momentum().si_value(), grid);
  expect(end.density()
             .si_value()
             .sum(rho_axes, false)
             .allclose(initial.density().si_value().sum(rho_axes, false), 2e-12,
                       2e-12),
         "periodic finite-volume evolution did not conserve mass");
  expect(end.momentum()
             .si_value()
             .sum(mom_axes, false)
             .allclose(initial.momentum().si_value().sum(mom_axes, false),
                       2e-12, 2e-12),
         "periodic finite-volume evolution did not conserve momentum");
  expect(end.energy()
             .si_value()
             .sum(rho_axes, false)
             .allclose(initial.energy().si_value().sum(rho_axes, false), 2e-12,
                       2e-12),
         "periodic finite-volume evolution did not conserve total energy");
}

void test_sod_shock_tube() {
  constexpr std::int64_t n = 200;
  constexpr double dx = 1.0 / static_cast<double>(n);
  const auto grid = grid1(n, dx);
  const Series x =
      pluss::series::arange(0, n, 1, ScalarType::Float64).add(0.5).mul(dx);
  const Series left = x.lt(0.5);
  const Series rho = left.where(pluss::series::full_like(x, 1.0),
                                pluss::series::full_like(x, 0.125));
  const Series pressure = left.where(pluss::series::full_like(x, 1.0),
                                     pluss::series::full_like(x, 0.1));
  const Series speed = pluss::series::zeros_like(x);
  const auto initial = primitive(rho, speed.unsqueeze(0), pressure, grid);

  gas::RunOptions options;
  options.safety = 0.95;
  options.step.rule = gas::Rule::Rk3;
  options.step.recon = gas::Recon::Minmod;
  options.step.courant = 0.45;
  const auto path =
      gas::evolve_to(initial, grid, seconds(0.0), seconds(0.1), {}, options);
  const auto end = final_state(path, grid);
  const Series end_rho = end.density().si_value();
  const Series end_u = gas::velocity(end, grid).si_value().select(0, 0);
  const Series end_p = gas::pressure(end, grid).si_value();

  expect(path.steps > 1 && std::abs(path.trajectory.coordinate()
                                        .si_value()
                                        .select(0, -1)
                                        .item<double>() -
                                    0.1) < 1e-15,
         "adaptive gas evolution did not terminate exactly at the stop time");
  expect(end_rho.gt(0.0).all().item<bool>() &&
             end_p.gt(0.0).all().item<bool>() &&
             end_rho.isfinite().all().item<bool>() &&
             end_p.isfinite().all().item<bool>(),
         "Sod evolution lost positivity or finiteness");

  // At x/t = 0 the exact Sod solution lies in the uniform left-star region.
  // Sod's gamma=1.4 reference values are rho=0.426319, u=0.927453,
  // p=0.303130. Average several cells away from the fan tail and contact so
  // this checks the Riemann solution rather than one grid-alignment choice.
  const Series sample = pluss::series::arange(101, 111, 1, ScalarType::Int64);
  expect(std::abs(end_rho.index_select(0, sample).mean().item<double>() -
                  0.426319) < 0.035,
         "Sod left-star density is outside the finite-volume tolerance");
  expect(std::abs(end_u.index_select(0, sample).mean().item<double>() -
                  0.927453) < 0.055,
         "Sod left-star velocity is outside the finite-volume tolerance");
  expect(std::abs(end_p.index_select(0, sample).mean().item<double>() -
                  0.303130) < 0.035,
         "Sod left-star pressure is outside the finite-volume tolerance");
}

void test_failures_and_reverse_sensitivity() {
  const auto grid = grid1(8, 0.125);
  const Series ones = pluss::series::ones({8}, ScalarType::Float64);
  const Series zero_velocity =
      pluss::series::zeros({1, 8}, ScalarType::Float64);
  expect_failure(
      [&] {
        gas::Model invalid;
        invalid.gamma = 1.0;
        (void)primitive(ones, zero_velocity, ones, grid, invalid);
      },
      "gamma <= 1 was accepted");
  expect_failure(
      [&] { (void)primitive(ones.neg(), zero_velocity, ones, grid); },
      "negative density was accepted");
  expect_failure(
      [&] {
        gas::Options invalid;
        invalid.recon = static_cast<gas::Recon>(99);
        (void)gas::step(primitive(ones, zero_velocity, ones, grid), grid,
                        seconds(1e-3), {}, invalid);
      },
      "an unknown reconstruction rule was accepted");

  const auto still = primitive(ones, zero_velocity, ones, grid);
  expect_failure([&] { (void)gas::step(still, grid, seconds(1.0)); },
                 "a step beyond the acoustic CFL limit was accepted");

  Series scale = pluss::series::full({}, 1.2, ScalarType::Float64);
  scale.tracks_slope_(true);
  const Series rho = ones.mul(scale);
  const Series speed = pluss::series::full({1, 8}, 0.1, ScalarType::Float64);
  const auto tracked = primitive(rho, speed, ones, grid);
  gas::Options options;
  options.rule = gas::Rule::Euler;
  options.recon = gas::Recon::Cell;
  const auto advanced = gas::step(tracked, grid, seconds(1e-3), {}, options);
  advanced.state.density().si_value().sum().reverse();
  expect(scale.has_sensitivity() &&
             std::abs(scale.sensitivity().item<double>() - 8.0) < 1e-11,
         "compressible conservative evolution severed reverse sensitivity");
}

} // namespace

int main() {
  try {
    test_constant_batch_and_cfl();
    test_periodic_conservation_and_fixed_path();
    test_sod_shock_tube();
    test_failures_and_reverse_sensitivity();
    std::cout << "NatSci compressible-flow checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
