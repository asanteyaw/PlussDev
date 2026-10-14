#include "pluss/ma/natsci/common.h"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace field = common::spatial;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

bool close(double actual, double expected, double relative = 1e-9) {
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

field::PeriodicGrid grid2(std::int64_t nx = 8, std::int64_t ny = 6) {
  return field::make_periodic_grid(
      {nx, ny}, {common::ScalarQuantity<common::Length>::from_si(
                     2.0 * std::numbers::pi / static_cast<double>(nx)),
                 common::ScalarQuantity<common::Length>::from_si(
                     2.0 * std::numbers::pi / static_cast<double>(ny))});
}

pluss::Series mesh_values(std::int64_t nx, std::int64_t ny, const auto &fn) {
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

void test_grid_and_scalar_operators() {
  constexpr std::int64_t nx = 8;
  constexpr std::int64_t ny = 6;
  const auto grid = grid2(nx, ny);
  expect(grid.dim() == 2 && grid.shape() == std::vector<std::int64_t>{nx, ny},
         "periodic grid lost its spatial metadata");

  const pluss::Series scalar = mesh_values(nx, ny, [](double x, double y) {
    return std::sin(x) + 2.0 * std::cos(y);
  });
  const pluss::Series expected_x =
      mesh_values(nx, ny, [](double x, double) { return std::cos(x); });
  const pluss::Series expected_y =
      mesh_values(nx, ny, [](double, double y) { return -2.0 * std::sin(y); });
  const double x_factor =
      std::sin(grid.spacing(0).si_value()) / grid.spacing(0).si_value();
  const double y_factor =
      std::sin(grid.spacing(1).si_value()) / grid.spacing(1).si_value();

  const pluss::Series gradient = field::gradient(scalar, grid);
  expect_shape(gradient, {2, nx, ny},
               "gradient did not insert the component axis");
  expect(field::back_div(field::fwd_grad(scalar, grid), grid)
             .allclose(field::laplacian(scalar, grid), 1e-12, 1e-12),
         "matched periodic gradient/divergence does not compose to the "
         "Laplacian");
  expect(gradient.select(0, 0).allclose(expected_x.mul(x_factor), 1e-12, 1e-12),
         "centered periodic x derivative is incorrect");
  expect(gradient.select(0, 1).allclose(expected_y.mul(y_factor), 1e-12, 1e-12),
         "centered periodic y derivative is incorrect");

  const double xx_factor = 2.0 * (1.0 - std::cos(grid.spacing(0).si_value())) /
                           std::pow(grid.spacing(0).si_value(), 2.0);
  const double yy_factor = 2.0 * (1.0 - std::cos(grid.spacing(1).si_value())) /
                           std::pow(grid.spacing(1).si_value(), 2.0);
  const pluss::Series expected_laplacian =
      mesh_values(nx, ny, [=](double x, double y) {
        return -xx_factor * std::sin(x) - 2.0 * yy_factor * std::cos(y);
      });
  expect(
      field::laplacian(scalar, grid).allclose(expected_laplacian, 1e-12, 1e-12),
      "periodic Laplacian is incorrect");

  // The wide Laplacian is exactly divergence(gradient(.)) of the centered
  // operators; its discrete symbol is -sin^2(k h) / h^2 per axis.
  const double xw_factor = std::pow(x_factor, 2.0);
  const double yw_factor = std::pow(y_factor, 2.0);
  const pluss::Series expected_wide =
      mesh_values(nx, ny, [=](double x, double y) {
        return -xw_factor * std::sin(x) - 2.0 * yw_factor * std::cos(y);
      });
  expect(field::wide_laplacian(scalar, grid)
             .allclose(field::divergence(field::gradient(scalar, grid), grid),
                       1e-12, 1e-12),
         "wide Laplacian is not divergence(gradient(.))");
  expect(field::wide_laplacian(scalar, grid)
             .allclose(expected_wide, 1e-12, 1e-12),
         "wide Laplacian symbol is incorrect");
  expect(field::wide_laplacian(scalar.transpose(0, 1),
                               field::make_periodic_grid(
                                   {ny, nx}, {grid.spacing(1), grid.spacing(0)}))
             .allclose(expected_wide.transpose(0, 1), 1e-12, 1e-12),
         "wide Laplacian lost transposed storage semantics");
  // Alternating (Nyquist) fields lie in the null space of the wide operator.
  const pluss::Series checker = mesh_values(nx, ny, [=](double x, double y) {
    const double i = std::round(x * nx / (2.0 * std::numbers::pi));
    const double j = std::round(y * ny / (2.0 * std::numbers::pi));
    return std::fmod(i + j, 2.0) == 0.0 ? 1.0 : -1.0;
  });
  expect(field::wide_laplacian(checker, grid).abs().max().item<double>() <
             1e-12,
         "the checkerboard is not annihilated by the wide Laplacian");
  static_assert(std::same_as<
                typename decltype(field::wide_laplacian(
                    common::SeriesQuantity<common::Length>::from_si(scalar),
                    grid))::dimension,
                common::DimensionQuotient<common::Length, common::Area>>);
}

void test_vector_operators_and_batches() {
  constexpr std::int64_t nx = 8;
  constexpr std::int64_t ny = 6;
  const auto grid = grid2(nx, ny);
  const pluss::Series x_component =
      mesh_values(nx, ny, [](double x, double) { return std::sin(x); });
  const pluss::Series y_component =
      mesh_values(nx, ny, [](double, double y) { return std::cos(y); });
  const pluss::Series velocity =
      x_component.unsqueeze(0).unite({y_component.unsqueeze(0)}, 0);

  const double x_factor =
      std::sin(grid.spacing(0).si_value()) / grid.spacing(0).si_value();
  const double y_factor =
      std::sin(grid.spacing(1).si_value()) / grid.spacing(1).si_value();
  const pluss::Series expected_divergence =
      mesh_values(nx, ny, [=](double x, double y) {
        return x_factor * std::cos(x) - y_factor * std::sin(y);
      });
  expect(field::divergence(velocity, grid)
             .allclose(expected_divergence, 1e-12, 1e-12),
         "vector divergence is incorrect");
  expect(field::curl(velocity, grid).abs().max().item<double>() < 1e-12,
         "curl of a separable gradient field is not zero");

  const pluss::Series scalar = mesh_values(
      nx, ny, [](double x, double y) { return std::sin(x) + std::cos(y); });
  const pluss::Series expected_advection =
      x_component.mul(field::partial(scalar, grid, 0))
          .add(y_component.mul(field::partial(scalar, grid, 1)));
  expect(field::advect(velocity, scalar, grid)
             .allclose(expected_advection, 1e-12, 1e-12),
         "scalar advection is incorrect");

  const pluss::Series batches =
      scalar.unsqueeze(0).unite({scalar.mul(2.0).unsqueeze(0)}, 0);
  expect_shape(field::gradient(batches, grid), {2, 2, nx, ny},
               "gradient lost a leading batch axis");
  const pluss::Series batched_velocity =
      velocity.unsqueeze(0).unite({velocity.mul(2.0).unsqueeze(0)}, 0);
  expect_shape(field::vector_advection(batched_velocity, grid), {2, 2, nx, ny},
               "vector advection lost its batch or component axis");
}

void test_one_three_dimensional_and_strided_fields() {
  constexpr std::int64_t n = 8;
  const double spacing = 2.0 * std::numbers::pi / static_cast<double>(n);
  const auto grid1 = field::make_periodic_grid(
      {n}, {common::ScalarQuantity<common::Length>::from_si(spacing)});
  std::vector<double> sine_values;
  std::vector<double> cosine_values;
  sine_values.reserve(n);
  cosine_values.reserve(n);
  for (std::int64_t index = 0; index < n; ++index) {
    const double x = spacing * static_cast<double>(index);
    sine_values.push_back(std::sin(x));
    cosine_values.push_back(std::cos(x));
  }
  const double factor = std::sin(spacing) / spacing;
  expect(field::partial(pluss::series::make(sine_values), grid1, 0)
             .allclose(pluss::series::make(cosine_values).mul(factor), 1e-12,
                       1e-12),
         "one-dimensional centered derivative is incorrect");

  constexpr std::int64_t nx = 4;
  constexpr std::int64_t ny = 3;
  constexpr std::int64_t nz = 3;
  const double hx = 2.0 * std::numbers::pi / static_cast<double>(nx);
  const auto grid3 = field::make_periodic_grid(
      {nx, ny, nz}, {common::ScalarQuantity<common::Length>::from_si(hx),
                     common::ScalarQuantity<common::Length>::from_si(1.0),
                     common::ScalarQuantity<common::Length>::from_si(1.0)});
  std::vector<double> z_values;
  std::vector<double> expected_y;
  z_values.reserve(nx * ny * nz);
  expected_y.reserve(nx * ny * nz);
  const double x_factor = std::sin(hx) / hx;
  for (std::int64_t x = 0; x < nx; ++x) {
    for (std::int64_t y = 0; y < ny; ++y) {
      for (std::int64_t z = 0; z < nz; ++z) {
        (void)y;
        (void)z;
        const double coordinate = hx * static_cast<double>(x);
        z_values.push_back(std::sin(coordinate));
        expected_y.push_back(-x_factor * std::cos(coordinate));
      }
    }
  }
  const pluss::Series z = pluss::series::make(z_values, {nx, ny, nz});
  const pluss::Series zero = pluss::series::zeros_like(z);
  const pluss::Series velocity =
      zero.unsqueeze(0).unite({zero.unsqueeze(0), z.unsqueeze(0)}, 0);
  const pluss::Series curl = field::curl(velocity, grid3);
  expect_shape(curl, {3, nx, ny, nz},
               "three-dimensional curl changed the vector layout");
  expect(curl.select(0, 0).eq(0.0).all().item<bool>() &&
             curl.select(0, 2).eq(0.0).all().item<bool>() &&
             curl.select(0, 1).allclose(
                 pluss::series::make(expected_y, {nx, ny, nz}), 1e-12, 1e-12),
         "three-dimensional curl orientation is incorrect");

  const auto original_grid = grid2();
  const pluss::Series original = mesh_values(
      8, 6, [](double x, double y) { return std::sin(x) + std::cos(y); });
  const pluss::Series strided = original.transpose(0, 1);
  expect(!strided.is_contiguous(),
         "strided field test input became contiguous");
  const auto transposed_grid = field::make_periodic_grid(
      {6, 8}, {original_grid.spacing(1), original_grid.spacing(0)});
  const pluss::Series strided_gradient =
      field::gradient(strided, transposed_grid);
  expect(strided_gradient.select(0, 0).allclose(
             field::partial(original, original_grid, 1).transpose(0, 1), 1e-12,
             1e-12) &&
             strided_gradient.select(0, 1).allclose(
                 field::partial(original, original_grid, 0).transpose(0, 1),
                 1e-12, 1e-12),
         "spatial operators lost non-contiguous axis correspondence");
}

void test_dimensions_and_reverse_sensitivity() {
  const auto grid = grid2();
  pluss::Series scalar = mesh_values(
      8, 6, [](double x, double y) { return std::sin(x) + std::cos(y); });
  scalar.tracks_slope_(true);
  const auto temperature =
      common::SeriesQuantity<common::Temperature>::from_si(scalar);
  const auto temperature_gradient = field::gradient(temperature, grid);
  static_assert(
      std::same_as<
          typename decltype(temperature_gradient)::dimension,
          common::DimensionQuotient<common::Temperature, common::Length>>);
  const auto temperature_laplacian = field::laplacian(temperature, grid);
  static_assert(std::same_as<
                typename decltype(temperature_laplacian)::dimension,
                common::DimensionQuotient<common::Temperature, common::Area>>);

  const pluss::Series velocity_values =
      pluss::series::ones({2, 8, 6}, pluss::Config{}
                                         .with_dtype(scalar.scalar_type())
                                         .with_platform(scalar.platform()));
  const auto velocity =
      common::SeriesQuantity<common::Speed>::from_si(velocity_values);
  const auto transported = field::advect(velocity, temperature, grid);
  static_assert(std::same_as<
                typename decltype(transported)::dimension,
                common::DimensionQuotient<common::Temperature, common::Time>>);

  temperature_gradient.si_value().square().sum().reverse();
  expect(scalar.has_sensitivity(),
         "field operators severed the Series reverse graph");
  expect(scalar.sensitivity().isfinite().all().item<bool>(),
         "field operator reverse sensitivity is non-finite");
}

void test_failures() {
  bool bad_extent = false;
  try {
    (void)field::make_periodic_grid(
        {2}, {common::ScalarQuantity<common::Length>::from_si(1.0)});
  } catch (const std::invalid_argument &) {
    bad_extent = true;
  }
  expect(bad_extent, "periodic grid accepted a degenerate centered stencil");

  const auto grid = grid2();
  bool shape_mismatch = false;
  try {
    (void)field::gradient(pluss::series::zeros({8, 7}), grid);
  } catch (const std::invalid_argument &) {
    shape_mismatch = true;
  }
  expect(shape_mismatch, "gradient accepted a mismatched spatial shape");

  bool component_mismatch = false;
  try {
    (void)field::divergence(pluss::series::zeros({3, 8, 6}), grid);
  } catch (const std::invalid_argument &) {
    component_mismatch = true;
  }
  expect(component_mismatch,
         "divergence accepted the wrong number of vector components");

  bool bad_axis = false;
  try {
    (void)field::partial(pluss::series::zeros({8, 6}), grid, 2);
  } catch (const std::out_of_range &) {
    bad_axis = true;
  }
  expect(bad_axis, "partial derivative accepted an out-of-range axis");
}

} // namespace

int main() {
  try {
    test_grid_and_scalar_operators();
    test_vector_operators_and_batches();
    test_one_three_dimensional_and_strided_fields();
    test_dimensions_and_reverse_sensitivity();
    test_failures();
    std::cout << "NatSci periodic field checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
