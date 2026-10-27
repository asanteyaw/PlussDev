#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

namespace structure = pluss::subsp::eng::structure;

namespace {

void expect(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}

void close(const pluss::Series &value, const std::vector<double> &expected,
           double tolerance = 1e-8) {
  const auto actual = value.host<double>();
  expect(actual.size() == expected.size(), "result size mismatch");
  for (std::size_t lane = 0; lane < actual.size(); ++lane)
    if (std::abs(actual[lane] - expected[lane]) > tolerance)
      throw std::runtime_error("result mismatch at lane " +
                               std::to_string(lane) + ": got " +
                               std::to_string(actual[lane]) + ", expected " +
                               std::to_string(expected[lane]));
}

void expect_error(const std::function<void()> &call,
                  const std::string &message) {
  try {
    call();
  } catch (const std::invalid_argument &) {
    return;
  } catch (const std::runtime_error &) {
    return;
  }
  throw std::runtime_error(message);
}

pluss::Series f64(std::initializer_list<double> values) {
  return pluss::series::make(values).to(pluss::ScalarType::Float64);
}

structure::Mesh one_member(
    pluss::ScalarType dtype = pluss::ScalarType::Float64) {
  return {pluss::series::make({{0.0, 0.0}, {2.0, 0.0}}).to(dtype), {0}, {1}};
}

structure::Sections section(
    pluss::Series area = f64({2.0}),
    pluss::ScalarType dtype = pluss::ScalarType::Float64) {
  return {area.to(dtype), f64({3.0}).to(dtype), f64({100.0}).to(dtype),
          f64({5.0}).to(dtype)};
}

structure::Fix axial_fix() {
  return {{0, 0, 0, 1, 1}, {0, 1, 2, 1, 2}, {}};
}

pluss::Series axial_load(double value,
                         pluss::ScalarType dtype = pluss::ScalarType::Float64) {
  return pluss::series::make({{0.0, 0.0, 0.0}, {value, 0.0, 0.0}}).to(dtype);
}

pluss::Series lateral_load(double value) {
  return pluss::series::make({{0.0, 0.0, 0.0}, {0.0, value, 0.0}})
      .to(pluss::ScalarType::Float64);
}

pluss::Series tip_x(const pluss::Series &value) {
  return value.select(-2, 1).select(-1, 0);
}

pluss::Series tip_y(const pluss::Series &value) {
  return value.select(-2, 1).select(-1, 1);
}

structure::NonlinearOptions options() {
  structure::NonlinearOptions value;
  value.load_steps = 3;
  value.max_iterations = 10;
  value.relative_tolerance = 1e-11;
  value.absolute_tolerance = 1e-12;
  value.solve.max_iterations = 12;
  value.solve.relative_tolerance = 1e-11;
  value.solve.absolute_tolerance = 1e-12;
  value.solve.check_interval = 1;
  return value;
}

} // namespace

int main() {
  const auto beam = structure::frame(one_member(), section());
  const auto axial = structure::solve(beam, axial_load(20.0), axial_fix(),
                                      options());
  close(tip_x(axial.displacement), {0.2}, 2e-9);
  close(axial.end_force, {-20.0, 0.0, 0.0, 20.0, 0.0, 0.0}, 2e-8);
  close(axial.energy, {2.0}, 2e-9);
  expect(axial.converged.all().item<bool>() && axial.load_factor == 1.0,
         "axial continuation did not reach full equilibrium");
  expect(axial.reaction.select(-2, 1).eq(0.0).all().item<bool>(),
         "reaction leaked onto the free axial degree of freedom");

  const auto bent = structure::solve(beam, lateral_load(50.0),
                                     {{0, 0, 0}, {0, 1, 2}, {}}, options());
  expect(bent.converged.all().item<bool>() &&
             bent.residual_norm.max().item<double>() < 1e-7,
         "large-displacement cantilever did not equilibrate");
  expect(tip_y(bent.displacement).item<double>() > 0.1 &&
             tip_x(bent.displacement).abs().item<double>() > 1e-3,
         "cantilever response did not include geometric axial motion");

  constexpr double angle = std::numbers::pi / 3.0;
  const double ux = 2.0 * std::cos(angle) - 2.0;
  const double uy = 2.0 * std::sin(angle);
  const structure::Fix rigid_fix{
      {0, 0, 0, 1, 1, 1},
      {0, 1, 2, 0, 1, 2},
      f64({0.0, 0.0, angle, ux, uy, angle})};
  const auto rigid = structure::solve(
      beam, pluss::series::zeros({2, 3},
                                 pluss::Config{}.with_dtype(
                                     pluss::ScalarType::Float64)),
      rigid_fix, options());
  expect(rigid.end_force.abs().max().item<double>() < 2e-10,
         "finite rigid rotation generated element force");
  expect(rigid.energy.abs().item<double>() < 2e-12,
         "finite rigid rotation generated strain energy");

  const auto batched_area =
      pluss::series::make(std::vector<double>{2.0, 4.0}, {2, 1})
          .to(pluss::ScalarType::Float64);
  const auto batch_beam = structure::frame(one_member(), section(batched_area));
  const auto batch = structure::solve(batch_beam, axial_load(20.0), axial_fix(),
                                      options());
  expect(batch.displacement.sizes() == std::vector<std::int64_t>{2, 2, 3},
         "nonlinear section batch shape is wrong");
  close(tip_x(batch.displacement), {0.2, 0.1}, 2e-9);

  const auto beam32 = structure::frame(
      one_member(pluss::ScalarType::Float32),
      section(f64({2.0}), pluss::ScalarType::Float32));
  auto options32 = options();
  options32.relative_tolerance = 2e-5;
  options32.absolute_tolerance = 1e-6;
  options32.solve.relative_tolerance = 2e-5;
  options32.solve.absolute_tolerance = 1e-6;
  const auto result32 =
      structure::solve(beam32, axial_load(20.0, pluss::ScalarType::Float32),
                       axial_fix(), options32);
  expect(result32.displacement.scalar_type() == pluss::ScalarType::Float32,
         "nonlinear solve widened Float32");
  close(tip_x(result32.displacement), {0.2}, 2e-5);

  auto tracked_load = axial_load(20.0);
  tracked_load.tracks_slope_(true);
  const auto tracked =
      structure::solve(beam, tracked_load, axial_fix(), options());
  tip_x(tracked.displacement).reverse();
  close(tip_x(tracked_load.sensitivity()), {0.01}, 3e-7);

  auto zero_load = axial_load(0.0);
  zero_load.tracks_slope_(true);
  const auto zero = structure::solve(beam, zero_load, axial_fix(), options());
  tip_x(zero.displacement).reverse();
  close(tip_x(zero_load.sensitivity()), {0.01}, 3e-7);

  auto stopped_options = options();
  stopped_options.solve.max_iterations = 0;
  stopped_options.max_cutbacks = 2;
  stopped_options.fail_on_nonconvergence = false;
  const auto stopped = structure::solve(beam, axial_load(20.0), axial_fix(),
                                        stopped_options);
  expect(stopped.load_factor == 0.0 && stopped.cutbacks == 3 &&
             !stopped.converged.all().item<bool>(),
         "failed continuation did not report its cutbacks");

  expect_error(
      [&] {
        auto invalid = options();
        invalid.line_search_shrink = 1.0;
        (void)structure::solve(beam, axial_load(20.0), axial_fix(), invalid);
      },
      "invalid line-search contraction was accepted");
  expect_error(
      [&] {
        auto invalid = options();
        invalid.load_steps = 0;
        (void)structure::solve(beam, axial_load(20.0), axial_fix(), invalid);
      },
      "zero load-step count was accepted");

  std::cout << "E6f nonlinear static checks passed\n";
}
