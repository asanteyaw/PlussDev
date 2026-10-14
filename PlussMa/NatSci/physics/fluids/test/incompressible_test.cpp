#include "pluss/ma/natsci/physics.h"
#include "pluss/subsp/math/krylov.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace fluids = pluss::subsp::natsci::physics::fluids;
namespace krylov = pluss::subsp::math::krylov;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

void expect_shape(const pluss::Series &value,
                  std::initializer_list<std::int64_t> expected,
                  const std::string &message) {
  expect(value.dim() == static_cast<std::int64_t>(expected.size()), message);
  std::size_t axis = 0;
  for (const auto extent : expected)
    expect(value.size(static_cast<std::int64_t>(axis++)) == extent, message);
}

spatial::PeriodicGrid grid2(std::int64_t nx = 8, std::int64_t ny = 6) {
  return spatial::make_periodic_grid(
      {nx, ny}, {common::ScalarQuantity<common::Length>::from_si(
                     2.0 * std::numbers::pi / static_cast<double>(nx)),
                 common::ScalarQuantity<common::Length>::from_si(
                     2.0 * std::numbers::pi / static_cast<double>(ny))});
}

pluss::Series mesh(std::int64_t nx, std::int64_t ny, const auto &fn) {
  std::vector<double> values;
  values.reserve(static_cast<std::size_t>(nx * ny));
  for (std::int64_t x = 0; x < nx; ++x) {
    for (std::int64_t y = 0; y < ny; ++y) {
      values.push_back(fn(2.0 * std::numbers::pi * static_cast<double>(x) /
                              static_cast<double>(nx),
                          2.0 * std::numbers::pi * static_cast<double>(y) /
                              static_cast<double>(ny)));
    }
  }
  return pluss::series::make(values, {nx, ny});
}

spatial::PoissonOptions strict_poisson(std::int64_t iterations = 64) {
  spatial::PoissonOptions options;
  options.solve.max_iterations = iterations;
  options.solve.relative_tolerance = 1e-11;
  options.solve.absolute_tolerance = 1e-12;
  options.mean_tolerance = 1e-12;
  options.require_convergence = true;
  return options;
}

void test_batched_krylov_contract() {
  const pluss::Series rhs =
      pluss::series::make({{2.0, 4.0, 6.0}, {3.0, 6.0, 9.0}});
  const auto solved = krylov::cg(
      [](const pluss::Series &value) { return value.mul(2.0); }, rhs, {1},
      krylov::CgOptions{.max_iterations = 4,
                        .relative_tolerance = 1e-12,
                        .absolute_tolerance = 1e-12});
  expect(solved.solution.allclose(rhs.mul(0.5), 1e-12, 1e-12),
         "batched matrix-free CG returned the wrong solution");
  expect_shape(solved.residual_norm, {2, 1},
               "CG collapsed independent batch diagnostics");
  expect(solved.converged.all().item<bool>() &&
             !solved.breakdown.any().item<bool>(),
         "CG did not report batched convergence");

  const auto failed = krylov::cg(
      [](const pluss::Series &value) { return value.neg(); },
      pluss::series::ones({2, 3}), {1}, krylov::CgOptions{.max_iterations = 2});
  expect(failed.breakdown.all().item<bool>() &&
             !failed.converged.any().item<bool>(),
         "CG did not expose non-positive operator curvature");

  const pluss::Series diagonal = pluss::series::make({1.0, 100.0, 10000.0});
  const pluss::Series scaled_rhs =
      pluss::series::make({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
  const krylov::CgOptions pc_options{.max_iterations = 8,
                                     .relative_tolerance = 1e-13,
                                     .absolute_tolerance = 1e-13,
                                     .check_interval = 1};
  const auto unscaled = krylov::cg(
      [&](const pluss::Series &value) { return value.mul(diagonal); },
      scaled_rhs, {1}, pc_options);
  const auto scaled = krylov::pcg(
      [&](const pluss::Series &value) { return value.mul(diagonal); },
      [&](const pluss::Series &residual) { return residual.div(diagonal); },
      scaled_rhs, {1}, pc_options);
  expect(scaled.solution.allclose(scaled_rhs.div(diagonal), 1e-12, 1e-12) &&
             scaled.converged.all().item<bool>(),
         "batched PCG returned the wrong diagonally scaled solution");
  expect(scaled.iterations == 1 && scaled.iterations < unscaled.iterations,
         "the exact diagonal preconditioner did not collapse the diagonal "
         "system to one iteration");
}

void test_periodic_poisson_batches_and_strides() {
  constexpr std::int64_t nx = 8;
  constexpr std::int64_t ny = 6;
  const auto grid = grid2(nx, ny);
  const pluss::Series mode = mesh(nx, ny, [](double x, double y) {
    return std::sin(x) + 0.5 * std::cos(2.0 * y);
  });
  const pluss::Series batched =
      mode.unsqueeze(0).unite({mode.mul(-2.0).unsqueeze(0)}, 0);
  const pluss::Series rhs = spatial::laplacian(batched, grid);
  const auto solved = spatial::solve_poisson(rhs, grid, strict_poisson());
  expect(solved.solution.allclose(batched, 1e-9, 1e-9),
         "periodic Poisson solve did not recover the zero-mean field");
  expect_shape(solved.residual_norm, {2, 1, 1},
               "Poisson diagnostics lost batch lanes");

  const pluss::Series transposed = mode.transpose(0, 1);
  expect(!transposed.is_contiguous(),
         "Poisson stride test input unexpectedly became contiguous");
  const auto transposed_grid =
      spatial::make_periodic_grid({ny, nx}, {grid.spacing(1), grid.spacing(0)});
  const auto strided =
      spatial::solve_poisson(spatial::laplacian(transposed, transposed_grid),
                             transposed_grid, strict_poisson());
  expect(strided.solution.allclose(transposed, 1e-9, 1e-9),
         "periodic Poisson solve lost transposed storage semantics");

  const auto zero = spatial::solve_poisson(pluss::series::zeros_like(mode),
                                           grid, strict_poisson(2));
  expect(zero.solution.eq(0.0).all().item<bool>() &&
             zero.converged.all().item<bool>(),
         "zero Poisson rhs did not return the zero gauge solution");

  bool incompatible = false;
  try {
    (void)spatial::solve_poisson(pluss::series::ones_like(mode), grid,
                                 strict_poisson());
  } catch (const std::domain_error &) {
    incompatible = true;
  }
  expect(incompatible, "periodic Poisson solve accepted a nonzero-mean rhs");
}

void test_wide_poisson_null_space() {
  for (const auto extents : {std::pair<std::int64_t, std::int64_t>{8, 6},
                             std::pair<std::int64_t, std::int64_t>{7, 5}}) {
    const auto [nx, ny] = extents;
    const auto grid = grid2(nx, ny);
    const pluss::Series mode = mesh(nx, ny, [](double x, double y) {
      return std::sin(x) + 0.5 * std::cos(y);
    });
    const pluss::Series rhs = spatial::wide_laplacian(mode, grid);
    spatial::PoissonOptions options = strict_poisson(128);
    options.stencil = spatial::PoissonStencil::Wide;
    const auto solved = spatial::solve_poisson(rhs, grid, options);
    expect(spatial::wide_laplacian(solved.solution, grid)
               .allclose(rhs, 1e-8, 1e-8),
           "wide Poisson solve does not invert the wide operator");
    expect(solved.solution.allclose(mode, 1e-8, 1e-8),
           "wide Poisson solve did not recover the smooth gauge-fixed field");

    // A checkerboard component has no preimage under the wide operator on an
    // even axis, so a right-hand side carrying one is incompatible.
    const pluss::Series alternating = mesh(nx, ny, [=](double x, double y) {
      const double i =
          std::round(x * static_cast<double>(nx) / (2.0 * std::numbers::pi));
      const double j =
          std::round(y * static_cast<double>(ny) / (2.0 * std::numbers::pi));
      return std::fmod(i + j, 2.0) == 0.0 ? 1.0 : -1.0;
    });
    const bool even_axes = nx % 2 == 0 && ny % 2 == 0;
    bool rejected = false;
    try {
      (void)spatial::solve_poisson(alternating, grid, options);
    } catch (const std::domain_error &) {
      rejected = true;
    }
    // On odd extents the alternating pattern wraps and is not a null mode, so
    // only even extents promise a rejection.
    if (even_axes)
      expect(rejected,
             "wide Poisson solve accepted a checkerboard right-hand side");
  }
}

void test_state_projection_and_diagnostics() {
  constexpr std::int64_t nx = 8;
  constexpr std::int64_t ny = 6;
  const auto grid = grid2(nx, ny);
  const pluss::Series potential = mesh(
      nx, ny, [](double x, double y) { return std::sin(x) + std::cos(y); });
  const pluss::Series gradient = spatial::gradient(potential, grid);
  const pluss::Series velocity =
      gradient.unsqueeze(0).unite({gradient.mul(0.5).unsqueeze(0)}, 0);
  const pluss::Series pressure = pluss::series::zeros(
      {2, nx, ny}, pluss::Config{}
                       .with_dtype(velocity.scalar_type())
                       .with_platform(velocity.platform()));
  const auto state = fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(velocity),
      common::SeriesQuantity<common::Pressure>::from_si(pressure), grid);
  expect(state.batch_shape(grid) == std::vector<std::int64_t>{2},
         "incompressible state lost its batch shape");

  fluids::ProjectionOptions options;
  options.poisson = strict_poisson();
  const auto projected = fluids::project(
      state, grid, common::ScalarQuantity<fluids::Density>::from_si(1.0),
      common::ScalarQuantity<common::Time>::from_si(1.0), options);
  expect(projected.state.velocity().si_value().abs().max().item<double>() <
             1e-9,
         "pressure correction did not remove a pure gradient velocity");
  expect(projected.after.max_abs.si_value().max().item<double>() < 1e-9,
         "pressure projection did not enforce the centered divergence");
  expect(projected.pressure_correction.si_value().allclose(
             potential.unsqueeze(0).unite({potential.mul(0.5).unsqueeze(0)}, 0),
             1e-9, 1e-9),
         "pressure projection returned the wrong gauge-fixed correction");
  expect_shape(projected.before.rms.si_value(), {2},
               "divergence report collapsed batch lanes");

  const auto report = fluids::div_report(state.velocity(), grid);
  expect_shape(report.field.si_value(), {2, nx, ny},
               "divergence changed the scalar-field layout");
}

void test_momentum_rate_and_reverse_sensitivity() {
  constexpr std::int64_t nx = 8;
  constexpr std::int64_t ny = 6;
  const auto grid = grid2(nx, ny);
  const pluss::Series zero_velocity = pluss::series::zeros({2, nx, ny});
  const pluss::Series zero_pressure = pluss::series::zeros({nx, ny});
  const auto still = fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(zero_velocity),
      common::SeriesQuantity<common::Pressure>::from_si(zero_pressure), grid);
  const auto density = common::ScalarQuantity<fluids::Density>::from_si(1.25);
  const auto viscosity =
      common::ScalarQuantity<fluids::KinematicViscosity>::from_si(0.01);
  const auto rate = fluids::mom_rate(still, grid, density, viscosity);
  static_assert(
      std::same_as<typename decltype(rate)::dimension, common::Acceleration>);
  expect(rate.si_value().eq(0.0).all().item<bool>(),
         "quiescent uniform-pressure flow has nonzero momentum rate");

  const auto body = common::SeriesQuantity<common::Acceleration>::from_si(
      pluss::series::ones_like(zero_velocity).mul(3.0));
  expect(fluids::mom_rate(still, grid, density, viscosity, body)
             .si_value()
             .equal(body.si_value()),
         "body acceleration was not added to the momentum rate");

  pluss::Series potential = mesh(nx, ny, [](double x, double y) {
    return std::sin(x) + 0.25 * std::cos(y);
  });
  potential.tracks_slope_(true);
  const auto solved = spatial::solve_poisson(
      spatial::laplacian(potential, grid), grid, strict_poisson(8));
  solved.solution.square().sum().reverse();
  expect(potential.has_sensitivity() &&
             potential.sensitivity().isfinite().all().item<bool>(),
         "matrix-free Poisson solve severed reverse sensitivity");
}

void test_state_and_projection_failures() {
  const auto grid = grid2();
  bool bad_pressure = false;
  try {
    (void)fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(
                                 pluss::series::zeros({2, 8, 6})),
                             common::SeriesQuantity<common::Pressure>::from_si(
                                 pluss::series::zeros({8, 5})),
                             grid);
  } catch (const std::invalid_argument &) {
    bad_pressure = true;
  }
  expect(bad_pressure, "fluid state accepted mismatched pressure storage");

  const auto state =
      fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(
                             pluss::series::zeros({2, 8, 6})),
                         common::SeriesQuantity<common::Pressure>::from_si(
                             pluss::series::zeros({8, 6})),
                         grid);
  bool bad_density = false;
  try {
    (void)fluids::project(state, grid,
                          common::ScalarQuantity<fluids::Density>::from_si(0.0),
                          common::ScalarQuantity<common::Time>::from_si(0.1));
  } catch (const std::invalid_argument &) {
    bad_density = true;
  }
  expect(bad_density, "pressure projection accepted zero density");
}

} // namespace

int main() {
  try {
    test_batched_krylov_contract();
    test_periodic_poisson_batches_and_strides();
    test_wide_poisson_null_space();
    test_state_projection_and_diagnostics();
    test_momentum_rate_and_reverse_sensitivity();
    test_state_and_projection_failures();
    std::cout << "NatSci incompressible flow checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
