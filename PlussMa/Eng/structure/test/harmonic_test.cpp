#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng.h"

#include <cmath>
#include <complex>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace structure = pluss::subsp::eng::structure;

namespace {

using Complex = std::complex<double>;

void expect(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}

void close(const pluss::Series &value, const std::vector<Complex> &expected,
           double tolerance = 1e-8) {
  const auto actual =
      value.to(pluss::ScalarType::Complex128).host<Complex>();
  expect(actual.size() == expected.size(), "result size mismatch");
  for (std::size_t lane = 0; lane < actual.size(); ++lane)
    if (std::abs(actual[lane] - expected[lane]) > tolerance)
      throw std::runtime_error("complex result mismatch at lane " +
                               std::to_string(lane));
}

void close_real(const pluss::Series &value, const std::vector<double> &expected,
                double tolerance = 1e-8) {
  const auto actual = value.host<double>();
  expect(actual.size() == expected.size(), "real result size mismatch");
  for (std::size_t lane = 0; lane < actual.size(); ++lane)
    if (std::abs(actual[lane] - expected[lane]) > tolerance)
      throw std::runtime_error("real result mismatch at lane " +
                               std::to_string(lane));
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
    pluss::Series density = f64({5.0}),
    pluss::ScalarType dtype = pluss::ScalarType::Float64) {
  return {f64({2.0}).to(dtype), f64({3.0}).to(dtype),
          f64({100.0}).to(dtype), density.to(dtype)};
}

structure::Fix fixed_root() { return {{0, 0, 0}, {0, 1, 2}, {}}; }

pluss::Series force(const std::vector<Complex> &tip,
                    pluss::ScalarType dtype = pluss::ScalarType::Complex128) {
  std::vector<Complex> lanes(tip.size() * 6, Complex{});
  for (std::size_t frequency = 0; frequency < tip.size(); ++frequency)
    lanes[frequency * 6 + 3] = tip[frequency];
  return pluss::series::make(lanes,
                             {static_cast<std::int64_t>(tip.size()), 2, 3})
      .to(dtype);
}

pluss::Series tip_x(const pluss::Series &value) {
  return value.select(-2, 1).select(-1, 0);
}

Complex response(Complex applied, double omega, double density,
                 double mass_damping, double stiffness_damping) {
  constexpr double stiffness = 100.0;
  const double mass = density * 4.0 / 3.0;
  const double damping = mass_damping * mass +
                         stiffness_damping * stiffness;
  return applied / Complex{stiffness - omega * omega * mass,
                           omega * damping};
}

} // namespace

int main() {
  const auto beam = structure::frame(one_member(), section());
  const std::vector<double> frequencies{0.0, 2.0, 4.0};
  const std::vector<Complex> applied{{10.0, 5.0}, {10.0, -3.0}, {-4.0, 2.0}};
  structure::HarmonicOptions options;
  options.mass_damping = 0.03;
  options.stiffness_damping = 0.002;
  options.solve.max_iterations = 12;
  options.solve.restart = 6;
  options.solve.relative_tolerance = 1e-11;
  options.solve.absolute_tolerance = 1e-12;
  const auto result = structure::harmonic(
      beam, force(applied), pluss::series::make(frequencies), fixed_root(),
      options);

  std::vector<Complex> expected;
  for (std::size_t index = 0; index < frequencies.size(); ++index)
    expected.push_back(response(applied[index], frequencies[index], 5.0,
                                options.mass_damping,
                                options.stiffness_damping));
  close(tip_x(result.displacement), expected, 2e-9);
  std::vector<Complex> velocity;
  std::vector<Complex> acceleration;
  for (std::size_t index = 0; index < expected.size(); ++index) {
    velocity.push_back(Complex{0.0, frequencies[index]} * expected[index]);
    acceleration.push_back(-frequencies[index] * frequencies[index] *
                           expected[index]);
  }
  close(tip_x(result.velocity), velocity, 2e-9);
  close(tip_x(result.acceleration), acceleration, 2e-9);
  expect(result.displacement.sizes() ==
             std::vector<std::int64_t>{3, 2, 3},
         "frequency response shape is wrong");
  expect(result.converged.all().item<bool>(),
         "analytical frequency sweep did not converge");
  expect(result.reaction.select(-2, 1).abs().lt(1e-9).all().item<bool>(),
         "reaction leaked onto a free node");
  close_real(result.omega, frequencies);

  const auto density =
      pluss::series::make(std::vector<double>{5.0, 10.0}, {2, 1});
  const auto batch_beam = structure::frame(one_member(), section(density));
  const auto batch = structure::harmonic(
      batch_beam, force({{8.0, 0.0}, {8.0, 0.0}}),
      pluss::series::make({1.0, 3.0}), fixed_root(), options);
  expect(batch.displacement.sizes() ==
             std::vector<std::int64_t>{2, 2, 2, 3},
         "material and frequency batches collided");
  close(tip_x(batch.displacement),
        {response({8.0, 0.0}, 1.0, 5.0, options.mass_damping,
                  options.stiffness_damping),
         response({8.0, 0.0}, 3.0, 5.0, options.mass_damping,
                  options.stiffness_damping),
         response({8.0, 0.0}, 1.0, 10.0, options.mass_damping,
                  options.stiffness_damping),
         response({8.0, 0.0}, 3.0, 10.0, options.mass_damping,
                  options.stiffness_damping)},
        3e-9);

  const auto beam32 = structure::frame(
      one_member(pluss::ScalarType::Float32),
      section(f64({5.0}), pluss::ScalarType::Float32));
  structure::HarmonicOptions options32 = options;
  options32.solve.relative_tolerance = 2e-5;
  options32.solve.absolute_tolerance = 1e-6;
  const auto result32 = structure::harmonic(
      beam32, force({{10.0, 2.0}}, pluss::ScalarType::Complex64),
      f64({2.0}).to(pluss::ScalarType::Float32), fixed_root(), options32);
  expect(result32.displacement.scalar_type() == pluss::ScalarType::Complex64,
         "Float32 frame did not retain Complex64 response precision");
  close(tip_x(result32.displacement),
        {response({10.0, 2.0}, 2.0, 5.0, options.mass_damping,
                  options.stiffness_damping)},
        3e-5);

  auto tracked_load = pluss::series::make(
      std::vector<double>{0.0, 0.0, 0.0, 10.0, 0.0, 0.0}, {1, 2, 3});
  tracked_load.tracks_slope_(true);
  const auto tracked = structure::harmonic(
      beam, tracked_load, f64({0.0}), fixed_root(), options);
  tip_x(tracked.displacement).real().sum().reverse();
  close_real(tracked_load.sensitivity().select(-2, 1).select(-1, 0),
             {0.01}, 2e-7);

  structure::HarmonicOptions report = options;
  report.solve.max_iterations = 0;
  report.fail_on_nonconvergence = false;
  const auto stopped = structure::harmonic(
      beam, force({{10.0, 0.0}}), f64({2.0}), fixed_root(), report);
  expect(!stopped.converged.all().item<bool>() && stopped.iterations == 0,
         "zero-budget harmonic solve did not report nonconvergence");

  expect_error(
      [&] {
        (void)structure::harmonic(beam, force({{1.0, 0.0}}), f64({-1.0}),
                                  fixed_root(), options);
      },
      "negative frequency was accepted");
  expect_error(
      [&] {
        const auto no_mass = structure::frame(
            one_member(), {f64({2.0}), f64({3.0}), f64({100.0}), {}});
        (void)structure::harmonic(no_mass, force({{1.0, 0.0}}), f64({1.0}),
                                  fixed_root(), options);
      },
      "missing density was accepted");
  expect_error(
      [&] {
        (void)structure::harmonic(
            beam, force({{1.0, 0.0}, {1.0, 0.0}}), f64({1.0}), fixed_root(),
            options);
      },
      "frequency/load length mismatch was accepted");
  expect_error(
      [&] {
        auto moving = fixed_root();
        moving.value = f64({0.0, 0.0, 1.0});
        (void)structure::harmonic(beam, force({{1.0, 0.0}}), f64({1.0}),
                                  moving, options);
      },
      "nonzero support amplitude was accepted");
  expect_error(
      [&] {
        auto failing = options;
        failing.solve.max_iterations = 0;
        (void)structure::harmonic(beam, force({{1.0, 0.0}}), f64({1.0}),
                                  fixed_root(), failing);
      },
      "required harmonic convergence was not enforced");

  std::cout << "E6e harmonic response checks passed\n";
}
