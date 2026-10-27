#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng.h"

#include <cmath>
#include <functional>
#include <iostream>
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
           double tolerance = 1e-7) {
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

structure::Mesh one_member() {
  return {pluss::series::make({{0.0, 0.0}, {2.0, 0.0}})
              .to(pluss::ScalarType::Float64),
          {0},
          {1}};
}

structure::Sections section(pluss::Series density = f64({5.0})) {
  return {f64({2.0}), f64({3.0}), f64({100.0}), std::move(density)};
}

structure::Fix fixed_root() { return {{0, 0, 0}, {0, 1, 2}, {}}; }

pluss::Series compression(double value = 1.0) {
  return pluss::series::make({{0.0, 0.0, 0.0}, {-value, 0.0, 0.0}})
      .to(pluss::ScalarType::Float64);
}

} // namespace

int main() {
  // The canonical Series operator solver is verified independently of frames.
  const auto initial = pluss::series::make({{1.0, 1.0}, {1.0, -1.0}})
                           .to(pluss::ScalarType::Float64);
  const auto diagonal_a = f64({2.0, 8.0});
  const auto diagonal_b = f64({1.0, 2.0});
  const auto generic = initial.linearalg.gen_eigh(
      [&](const pluss::Series &x) { return x.mul(diagonal_a); },
      [&](const pluss::Series &x) { return x.mul(diagonal_b); },
      [&](const pluss::Series &rhs) { return rhs.div(diagonal_a); });
  close(generic.values, {2.0, 4.0}, 1e-10);
  expect(generic.converged.all().item<bool>(),
         "generic generalized eigensolver did not converge");

  const auto beam = structure::frame(one_member(), section());
  structure::EigOptions three;
  three.count = 3;
  const auto modal = structure::modes(beam, fixed_root(), three);
  const double root = std::sqrt(80870400.0);
  const double bend_low = (9180.0 - root) / 8.0;
  const double bend_high = (9180.0 + root) / 8.0;
  close(modal.omega,
        {std::sqrt(15.0), std::sqrt(bend_low), std::sqrt(bend_high)}, 2e-6);
  close(modal.hertz,
        {std::sqrt(15.0) / (2.0 * std::acos(-1.0)),
         std::sqrt(bend_low) / (2.0 * std::acos(-1.0)),
         std::sqrt(bend_high) / (2.0 * std::acos(-1.0))},
        4e-7);
  expect(modal.shape.sizes() == std::vector<std::int64_t>{3, 2, 3},
         "modal shape layout is wrong");
  expect(modal.converged.all().item<bool>(), "modal solve did not converge");
  const auto modal_mass = structure::mass(beam, modal.shape.reshape({3, 2, 3}));
  close(modal.shape.mul(modal_mass).sum({-2, -1}, false), {1.0, 1.0, 1.0},
        2e-7);

  const auto density_batch =
      pluss::series::make(std::vector<double>{5.0, 20.0}, {2, 1})
          .to(pluss::ScalarType::Float64);
  const auto batch_beam =
      structure::frame(one_member(), section(density_batch));
  const auto batch_modes = structure::modes(batch_beam, fixed_root());
  close(batch_modes.omega, {std::sqrt(15.0), std::sqrt(15.0) / 2.0}, 2e-6);
  expect(batch_modes.shape.sizes() == std::vector<std::int64_t>{2, 1, 2, 3},
         "batched modal shape layout is wrong");

  const auto buck = structure::buckling(beam, compression(), fixed_root());
  const double buck_root = std::sqrt(111600.0);
  const double expected_factor = (390.0 - buck_root) / 0.3;
  close(buck.factor, {expected_factor}, 3e-5);
  expect(buck.shape.sizes() == std::vector<std::int64_t>{1, 2, 3},
         "buckling shape layout is wrong");
  expect(buck.converged.all().item<bool>(), "buckling solve did not converge");
  close(structure::buckling(beam, compression(2.0), fixed_root()).factor,
        {expected_factor / 2.0}, 3e-5);

  const auto beam32 = structure::frame(
      {one_member().position.to(pluss::ScalarType::Float32), {0}, {1}},
      {section().area.to(pluss::ScalarType::Float32),
       section().inertia.to(pluss::ScalarType::Float32),
       section().modulus.to(pluss::ScalarType::Float32),
       section().density.to(pluss::ScalarType::Float32)});
  structure::EigOptions loose;
  loose.relative_tolerance = 2e-5;
  loose.absolute_tolerance = 1e-6;
  const auto modal32 = structure::modes(beam32, fixed_root(), loose);
  close(modal32.omega, {std::sqrt(15.0)}, 2e-3);
  expect(modal32.omega.scalar_type() == pluss::ScalarType::Float32,
         "modal analysis widened Float32");

  auto tracked_density = f64({5.0});
  tracked_density.tracks_slope_(true);
  const auto tracked_beam =
      structure::frame(one_member(), section(tracked_density));
  structure::modes(tracked_beam, fixed_root()).omega.sum().reverse();
  close(tracked_density.sensitivity(), {-std::sqrt(15.0) / 10.0}, 2e-5);

  auto tracked_load = compression();
  tracked_load.tracks_slope_(true);
  structure::buckling(beam, tracked_load, fixed_root()).factor.sum().reverse();
  close(tracked_load.sensitivity(), {0.0, 0.0, 0.0, expected_factor, 0.0, 0.0},
        2e-3);

  expect_error(
      [&] {
        const auto no_mass = structure::frame(
            one_member(), {f64({2.0}), f64({3.0}), f64({100.0}), {}});
        (void)structure::modes(no_mass, fixed_root());
      },
      "modal analysis accepted a frame without density");
  expect_error(
      [&] {
        structure::EigOptions too_many;
        too_many.count = 4;
        (void)structure::modes(beam, fixed_root(), too_many);
      },
      "modal analysis accepted more modes than free degrees");
  expect_error(
      [&] {
        (void)structure::modes(beam,
                               {{0, 0, 0}, {0, 1, 2}, f64({0.0, 0.0, 0.1})});
      },
      "modal analysis accepted a moving support");
  expect_error(
      [&] { (void)structure::buckling(beam, compression(-1.0), fixed_root()); },
      "buckling accepted a purely tensile reference state");

  std::cout << "eng structure spectral checks passed\n";
  return 0;
}
