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

struct History {
  std::vector<double> displacement;
  std::vector<double> velocity;
  std::vector<double> acceleration;
};

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
    pluss::Series density = f64({5.0}),
    pluss::ScalarType dtype = pluss::ScalarType::Float64) {
  return {f64({2.0}).to(dtype), f64({3.0}).to(dtype),
          f64({100.0}).to(dtype), density.to(dtype)};
}

structure::Fix axial_fix() {
  return {{0, 0, 0, 1, 1}, {0, 1, 2, 1, 2}, {}};
}

structure::Fix fixed_root() { return {{0, 0, 0}, {0, 1, 2}, {}}; }

pluss::Series history_load(const std::vector<double> &axial,
                           pluss::ScalarType dtype =
                               pluss::ScalarType::Float64) {
  std::vector<double> values(axial.size() * 6, 0.0);
  for (std::size_t sample = 0; sample < axial.size(); ++sample)
    values[sample * 6 + 3] = axial[sample];
  return pluss::series::make(values,
                             {static_cast<std::int64_t>(axial.size()), 2, 3})
      .to(dtype);
}

pluss::Series initial(double x, double y = 0.0,
                      pluss::ScalarType dtype = pluss::ScalarType::Float64) {
  return pluss::series::make({{0.0, 0.0, 0.0}, {x, y, 0.0}}).to(dtype);
}

pluss::Series tip_x(const pluss::Series &value) {
  return value.select(-2, 1).select(-1, 0);
}

History reference(const std::vector<double> &time,
                  const std::vector<double> &force, double u, double v,
                  double mass, double stiffness, double mass_damping,
                  double stiffness_damping, double radius) {
  const double damping = mass_damping * mass + stiffness_damping * stiffness;
  double a = (force.front() - damping * v - stiffness * u) / mass;
  History result{{u}, {v}, {a}};
  const double alpha_m = (2.0 - radius) / (1.0 + radius);
  const double alpha_f = 1.0 / (1.0 + radius);
  const double beta = 0.25 * std::pow(1.0 + alpha_m - alpha_f, 2.0);
  const double gamma = 0.5 + alpha_m - alpha_f;
  for (std::size_t index = 0; index + 1 < time.size(); ++index) {
    const double dt = time[index + 1] - time[index];
    const double cu = beta * alpha_f * dt * dt / alpha_m;
    const double cv = gamma * alpha_f * dt / alpha_m;
    const double pred_u =
        u + alpha_f * dt * v + alpha_f * dt * dt * (0.5 - beta / alpha_m) * a;
    const double pred_v = v + alpha_f * dt * (1.0 - gamma / alpha_m) * a;
    const double applied =
        (1.0 - alpha_f) * force[index] + alpha_f * force[index + 1];
    const double alpha_a = (applied - damping * pred_v - stiffness * pred_u) /
                           (mass + damping * cv + stiffness * cu);
    const double next_u =
        (pred_u + cu * alpha_a - (1.0 - alpha_f) * u) / alpha_f;
    const double next_v =
        (pred_v + cv * alpha_a - (1.0 - alpha_f) * v) / alpha_f;
    const double next_a = (alpha_a - (1.0 - alpha_m) * a) / alpha_m;
    u = next_u;
    v = next_v;
    a = next_a;
    result.displacement.push_back(u);
    result.velocity.push_back(v);
    result.acceleration.push_back(a);
  }
  return result;
}

structure::NonlinearMotionOptions options() {
  structure::NonlinearMotionOptions value;
  value.spectral_radius = 0.6;
  value.mass_damping = 0.02;
  value.stiffness_damping = 0.001;
  value.max_iterations = 12;
  value.relative_tolerance = 1e-10;
  value.absolute_tolerance = 1e-11;
  value.solve.max_iterations = 12;
  value.solve.relative_tolerance = 1e-11;
  value.solve.absolute_tolerance = 1e-12;
  value.solve.check_interval = 1;
  return value;
}

} // namespace

int main() {
  constexpr double mass = 20.0 / 3.0;
  constexpr double stiffness = 100.0;
  const auto beam = structure::frame(one_member(), section());
  const std::vector<double> time{0.0, 0.07, 0.2, 0.31};
  const std::vector<double> force{0.0, 2.0, -1.0, 3.0};
  const auto control = options();
  const auto expected = reference(time, force, 0.2, -0.1, mass, stiffness,
                                  control.mass_damping,
                                  control.stiffness_damping,
                                  control.spectral_radius);
  const auto result = structure::motion(
      beam, history_load(force), pluss::series::make(time), axial_fix(),
      {initial(0.2), initial(-0.1)}, control);
  close(tip_x(result.displacement), expected.displacement, 3e-8);
  close(tip_x(result.velocity), expected.velocity, 3e-8);
  close(tip_x(result.acceleration), expected.acceleration, 3e-7);
  expect(result.converged.all().item<bool>(),
         "nonlinear axial history did not converge");
  expect(result.displacement.sizes() ==
             std::vector<std::int64_t>{4, 2, 3},
         "nonlinear motion history shape is wrong");

  const std::vector<double> short_time{0.0, 0.01, 0.02, 0.03};
  const std::vector<double> zero_force(short_time.size(), 0.0);
  auto geometric_options = options();
  geometric_options.spectral_radius = 1.0;
  geometric_options.mass_damping = 0.0;
  geometric_options.stiffness_damping = 0.0;
  const auto geometric = structure::motion(
      beam, history_load(zero_force), pluss::series::make(short_time),
      fixed_root(), {initial(0.0, 0.25), {}}, geometric_options);
  expect(geometric.converged.all().item<bool>(),
         "large-displacement free motion did not converge");
  expect(geometric.strain_energy.isfinite().all().item<bool>() &&
             geometric.kinetic.isfinite().all().item<bool>(),
         "large-displacement energy became non-finite");
  expect(geometric.displacement.select(-3, 1)
                 .sub(geometric.displacement.select(-3, 0))
                 .abs()
                 .max()
                 .item<double>() > 1e-8,
         "geometric free motion did not evolve");

  const auto density_batch =
      pluss::series::make(std::vector<double>{5.0, 10.0}, {2, 1})
          .to(pluss::ScalarType::Float64);
  const auto batch_beam = structure::frame(one_member(), section(density_batch));
  const auto batch = structure::motion(
      batch_beam, history_load(zero_force), pluss::series::make(short_time),
      axial_fix(), {initial(0.2), {}}, geometric_options);
  expect(batch.displacement.sizes() ==
             std::vector<std::int64_t>{2, 4, 2, 3},
         "nonlinear density batch shape is wrong");
  close(batch.acceleration.select(-3, 0).select(-2, 1).select(-1, 0),
        {-3.0, -1.5}, 3e-8);

  const auto beam32 = structure::frame(
      one_member(pluss::ScalarType::Float32),
      section(f64({5.0}), pluss::ScalarType::Float32));
  auto options32 = control;
  options32.relative_tolerance = 2e-5;
  options32.absolute_tolerance = 1e-6;
  options32.solve.relative_tolerance = 2e-5;
  options32.solve.absolute_tolerance = 1e-6;
  const auto result32 = structure::motion(
      beam32, history_load(force, pluss::ScalarType::Float32),
      pluss::series::make(time).to(pluss::ScalarType::Float32), axial_fix(),
      {initial(0.2, 0.0, pluss::ScalarType::Float32),
       initial(-0.1, 0.0, pluss::ScalarType::Float32)},
      options32);
  expect(result32.displacement.scalar_type() == pluss::ScalarType::Float32,
         "nonlinear motion widened Float32");
  close(tip_x(result32.displacement), expected.displacement, 3e-5);

  auto tracked_force = history_load(force);
  tracked_force.tracks_slope_(true);
  const auto tracked = structure::motion(
      beam, tracked_force, pluss::series::make(time), axial_fix(), {}, control);
  tip_x(tracked.displacement).select(0, 3).reverse();
  expect(tracked_force.sensitivity().isfinite().all().item<bool>(),
         "nonlinear motion load sensitivity is non-finite");
  std::vector<double> expected_sensitivity;
  for (std::size_t sample = 0; sample < time.size(); ++sample) {
    std::vector<double> basis(time.size(), 0.0);
    basis[sample] = 1.0;
    expected_sensitivity.push_back(
        reference(time, basis, 0.0, 0.0, mass, stiffness,
                  control.mass_damping, control.stiffness_damping,
                  control.spectral_radius)
            .displacement.back());
  }
  close(tracked_force.sensitivity().select(-2, 1).select(-1, 0),
        expected_sensitivity, 5e-7);

  auto stopped_options = control;
  stopped_options.solve.max_iterations = 0;
  stopped_options.fail_on_nonconvergence = false;
  const auto stopped = structure::motion(
      beam, history_load({1.0, 1.0}), pluss::series::make({0.0, 0.1}),
      axial_fix(), {}, stopped_options);
  expect(!stopped.converged.all().item<bool>(),
         "zero-budget nonlinear motion did not report nonconvergence");

  expect_error(
      [&] {
        auto moving = axial_fix();
        moving.value = f64({0.0, 0.0, 0.0, 1.0, 0.0});
        (void)structure::motion(beam, history_load({0.0, 0.0}),
                                pluss::series::make({0.0, 0.1}), moving, {},
                                control);
      },
      "moving nonlinear support was accepted");
  expect_error(
      [&] {
        auto invalid = control;
        invalid.armijo = 0.0;
        (void)structure::motion(beam, history_load({0.0, 0.0}),
                                pluss::series::make({0.0, 0.1}), axial_fix(),
                                {}, invalid);
      },
      "invalid nonlinear motion line search was accepted");

  // A stiff rod rotated rigidly about its pin and left unloaded is in equilibrium: the step must converge and the rod must not move,
  // although the rounding noise of its internal forces is far above the absolute tolerance.
  {
    constexpr int pieces = 8;
    constexpr double angle = 1.0;
    std::vector<double> coordinates, start;
    std::vector<std::int64_t> starts, ends;
    for (int i = 0; i <= pieces; ++i) {
      const double x = static_cast<double>(i) / pieces;
      coordinates.insert(coordinates.end(), {0.0, -x});
      start.insert(start.end(), {x * std::sin(angle), x * (1.0 - std::cos(angle)), angle});
    }
    for (int i = 0; i < pieces; ++i) {
      starts.push_back(i);
      ends.push_back(i + 1);
    }
    const auto fill = [&](double value) {
      return pluss::series::make(std::vector<double>(pieces, value))
          .to(pluss::ScalarType::Float64);
    };
    const auto rod = structure::frame(
        {pluss::series::make(coordinates).reshape({pieces + 1, 2}).to(pluss::ScalarType::Float64), starts, ends},
        {fill(4e-4), fill(1.333e-8), fill(200e9), fill(7850.0)});
    structure::MotionState initial;
    initial.displacement = pluss::series::make(start).reshape({pieces + 1, 3}).to(pluss::ScalarType::Float64);
    const auto time = pluss::series::make({0.0, 0.01, 0.02, 0.03}).to(pluss::ScalarType::Float64);
    const auto none = pluss::series::zeros({4, pieces + 1, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
    const auto r = structure::motion(rod, none, time, {{0, 0}, {0, 1}, {}}, initial, structure::NonlinearMotionOptions{});
    expect(r.converged.all().item<bool>() &&
               std::abs(r.displacement.select(-3, 3).select(-2, pieces).select(-1, 2).item<double>() - angle) < 1e-9,
           "an unloaded rigidly rotated rod did not stay in equilibrium");
  }

  std::cout << "E6g nonlinear motion checks passed\n";
}
