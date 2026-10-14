#include "pluss/ma/natsci/common.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;

namespace {

using Field = common::Dimensionless;
using Source = common::DimensionQuotient<Field, common::Area>;

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

spatial::BoxGrid grid1(std::int64_t extent = 4, double spacing = 1.0) {
  return spatial::make_box_grid(
      {extent}, {common::ScalarQuantity<common::Length>::from_si(spacing)});
}

spatial::Bounds<Field> value_bounds(const spatial::BoxGrid &grid, double low,
                                    double high) {
  return spatial::make_bounds<Field>(
      grid, {spatial::value_bc(spatial::Face{0, spatial::Side::Low},
                               common::ScalarQuantity<Field>::from_si(low)),
             spatial::value_bc(spatial::Face{0, spatial::Side::High},
                               common::ScalarQuantity<Field>::from_si(high))});
}

spatial::Bounds<Field> flux_bounds(const spatial::BoxGrid &grid, double low,
                                   double high) {
  using Flux = common::DimensionQuotient<Field, common::Length>;
  return spatial::make_bounds<Field>(
      grid, {spatial::flux_bc(spatial::Face{0, spatial::Side::Low},
                              common::ScalarQuantity<Flux>::from_si(low)),
             spatial::flux_bc(spatial::Face{0, spatial::Side::High},
                              common::ScalarQuantity<Flux>::from_si(high))});
}

spatial::PoissonOptions strict_poisson() {
  spatial::PoissonOptions options;
  options.solve.max_iterations = 32;
  options.solve.relative_tolerance = 1e-12;
  options.solve.absolute_tolerance = 1e-12;
  options.solve.check_interval = 1;
  options.mean_tolerance = 1e-12;
  return options;
}

void test_value_and_flux_ghosts() {
  const auto grid = grid1();
  const auto field = common::SeriesQuantity<Field>::from_si(
      pluss::series::make({0.5, 1.5, 2.5, 3.5}));

  const auto fixed =
      spatial::ghosts(field, grid, value_bounds(grid, 0.0, 4.0), 0);
  expect(fixed.low.si_value().item<double>() == -0.5 &&
             fixed.high.si_value().item<double>() == 4.5,
         "value boundary reconstruction did not reflect about the face");
  expect(spatial::partial(field, grid, value_bounds(grid, 0.0, 4.0), 0)
             .si_value()
             .allclose(field.si_value().mul(0.0).add(1.0), 1e-12, 1e-12),
         "bounded first derivative is wrong at a value face");

  const auto flux =
      spatial::ghosts(field, grid, flux_bounds(grid, -1.0, 1.0), 0);
  expect(flux.low.si_value().item<double>() == -0.5 &&
             flux.high.si_value().item<double>() == 4.5,
         "outward flux signs are inconsistent across opposing faces");
  expect(spatial::laplacian(field, grid, flux_bounds(grid, -1.0, 1.0))
                 .si_value()
                 .abs()
                 .max()
                 .item<double>() < 1e-12,
         "linear field has nonzero bounded Laplacian");
}

void test_profile_batches_and_strides() {
  const auto grid = grid1();
  const pluss::Series values =
      pluss::series::make({{0.5, 1.5, 2.5, 3.5}, {10.5, 11.5, 12.5, 13.5}});
  const auto field = common::SeriesQuantity<Field>::from_si(values);
  const auto low =
      common::SeriesQuantity<Field>::from_si(pluss::series::make({0.0, 10.0}));
  const auto high =
      common::SeriesQuantity<Field>::from_si(pluss::series::make({4.0, 14.0}));
  const auto bounds = spatial::make_bounds<Field>(
      grid, {spatial::value_bc(spatial::Face{0, spatial::Side::Low}, low),
             spatial::value_bc(spatial::Face{0, spatial::Side::High}, high)});
  expect(spatial::partial(field, grid, bounds, 0)
             .si_value()
             .allclose(values.mul(0.0).add(1.0), 1e-12, 1e-12),
         "face profiles did not preserve independent batch lanes");

  const auto grid2 = spatial::make_box_grid(
      {3, 4}, {common::ScalarQuantity<common::Length>::from_si(1.0),
               common::ScalarQuantity<common::Length>::from_si(1.0)});
  const pluss::Series base =
      pluss::series::make(
          {{0.5, 1.5, 2.5}, {0.5, 1.5, 2.5}, {0.5, 1.5, 2.5}, {0.5, 1.5, 2.5}})
          .transpose(0, 1);
  expect(!base.is_contiguous(), "bounded stride fixture became contiguous");
  using Flux = common::DimensionQuotient<Field, common::Length>;
  const auto bounds2 = spatial::make_bounds<Field>(
      grid2, {spatial::flux_bc(spatial::Face{0, spatial::Side::Low},
                               common::ScalarQuantity<Flux>::from_si(-1.0)),
              spatial::flux_bc(spatial::Face{0, spatial::Side::High},
                               common::ScalarQuantity<Flux>::from_si(1.0)),
              spatial::flux_bc(spatial::Face{1, spatial::Side::Low},
                               common::ScalarQuantity<Flux>::from_si(0.0)),
              spatial::flux_bc(spatial::Face{1, spatial::Side::High},
                               common::ScalarQuantity<Flux>::from_si(0.0))});
  expect(spatial::laplacian(common::SeriesQuantity<Field>::from_si(base), grid2,
                            bounds2)
                 .si_value()
                 .abs()
                 .max()
                 .item<double>() < 1e-12,
         "bounded stencil lost non-contiguous axis correspondence");
}

void test_bounded_poisson_and_reverse() {
  const auto grid = grid1();
  const pluss::Series linear = pluss::series::make({0.5, 1.5, 2.5, 3.5});
  const auto fixed = value_bounds(grid, 0.0, 4.0);
  const auto zero_rhs = common::SeriesQuantity<Source>::from_si(
      pluss::series::zeros_like(linear));
  const auto solved =
      spatial::solve_poisson(zero_rhs, grid, fixed, strict_poisson());
  static_assert(
      std::same_as<typename decltype(solved.solution)::dimension, Field>);
  expect(solved.solution.si_value().allclose(linear, 1e-10, 1e-10),
         "value-bounded Poisson solve did not recover the affine solution");

  const auto flux = flux_bounds(grid, -1.0, 1.0);
  const auto gauge =
      spatial::solve_poisson(zero_rhs, grid, flux, strict_poisson());
  expect(gauge.solution.si_value().allclose(linear.sub(linear.mean()), 1e-10,
                                            1e-10),
         "all-flux Poisson solve did not recover its zero-mean gauge");

  pluss::Series tracked = linear.clone();
  tracked.tracks_slope_(true);
  const auto tracked_field = common::SeriesQuantity<Field>::from_si(tracked);
  spatial::laplacian(tracked_field, grid, fixed)
      .si_value()
      .square()
      .sum()
      .reverse();
  expect(tracked.has_sensitivity() &&
             tracked.sensitivity().isfinite().all().item<bool>(),
         "bounded reconstruction severed reverse sensitivity");
}

void test_failures() {
  bool bad_grid = false;
  try {
    (void)grid1(1);
  } catch (const std::invalid_argument &) {
    bad_grid = true;
  }
  expect(bad_grid, "bounded grid accepted an unusable extent");

  const auto grid = grid1();
  bool missing = false;
  try {
    (void)spatial::make_bounds<Field>(
        grid, {spatial::value_bc(spatial::Face{0, spatial::Side::Low},
                                 common::ScalarQuantity<Field>::from_si(0.0))});
  } catch (const std::invalid_argument &) {
    missing = true;
  }
  expect(missing, "boundary set accepted a missing face");

  bool duplicate = false;
  try {
    (void)spatial::make_bounds<Field>(
        grid, {spatial::value_bc(spatial::Face{0, spatial::Side::Low},
                                 common::ScalarQuantity<Field>::from_si(0.0)),
               spatial::value_bc(spatial::Face{0, spatial::Side::Low},
                                 common::ScalarQuantity<Field>::from_si(1.0))});
  } catch (const std::invalid_argument &) {
    duplicate = true;
  }
  expect(duplicate, "boundary set accepted duplicate face rules");

  bool incompatible = false;
  try {
    const auto rhs =
        common::SeriesQuantity<Source>::from_si(pluss::series::ones({4}));
    (void)spatial::solve_poisson(rhs, grid, flux_bounds(grid, 0.0, 0.0),
                                 strict_poisson());
  } catch (const std::domain_error &) {
    incompatible = true;
  }
  expect(incompatible, "all-flux Poisson accepted incompatible source mass");

  bool wide_rejected = false;
  try {
    const auto rhs =
        common::SeriesQuantity<Source>::from_si(pluss::series::zeros({4}));
    auto options = strict_poisson();
    options.stencil = spatial::PoissonStencil::Wide;
    (void)spatial::solve_poisson(rhs, grid, value_bounds(grid, 0.0, 0.0),
                                 options);
  } catch (const std::invalid_argument &) {
    wide_rejected = true;
  }
  expect(wide_rejected, "bounded Poisson accepted the periodic wide stencil");
}

} // namespace

int main() {
  try {
    test_value_and_flux_ghosts();
    test_profile_batches_and_strides();
    test_bounded_poisson_and_reverse();
    test_failures();
    std::cout << "NatSci bounded field checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
