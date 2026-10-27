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

struct ScalarHistory {
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

pluss::Series
history_load(const std::vector<double> &axial,
             pluss::ScalarType dtype = pluss::ScalarType::Float64) {
  std::vector<double> values(axial.size() * 6, 0.0);
  for (std::size_t sample = 0; sample < axial.size(); ++sample)
    values[sample * 6 + 3] = axial[sample];
  return pluss::series::make(values,
                             {static_cast<std::int64_t>(axial.size()), 2, 3})
      .to(dtype);
}

pluss::Series
initial_axial(double value,
              pluss::ScalarType dtype = pluss::ScalarType::Float64) {
  return pluss::series::make({{0.0, 0.0, 0.0}, {value, 0.0, 0.0}}).to(dtype);
}

ScalarHistory scalar_response(const std::vector<double> &time,
                              const std::vector<double> &force, double u,
                              double v, double mass, double stiffness,
                              double mass_damping, double stiffness_damping,
                              double radius) {
  const double damping = mass_damping * mass + stiffness_damping * stiffness;
  double a = (force.front() - damping * v - stiffness * u) / mass;
  ScalarHistory out{{u}, {v}, {a}};
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
    out.displacement.push_back(u);
    out.velocity.push_back(v);
    out.acceleration.push_back(a);
  }
  return out;
}

pluss::Series tip_axial(const pluss::Series &history) {
  return history.select(-2, 1).select(-1, 0);
}

} // namespace

int main() {
  constexpr double mass = 20.0 / 3.0;
  constexpr double stiffness = 100.0;
  const auto beam = structure::frame(one_member(), section());

  const std::vector<double> short_time{0.0, 0.1, 0.2};
  const std::vector<double> zero_force(short_time.size(), 0.0);
  const auto free_reference = scalar_response(short_time, zero_force, 1.0, 0.0,
                                              mass, stiffness, 0.0, 0.0, 1.0);
  const auto free = structure::motion(beam, history_load(zero_force),
                                      pluss::series::make(short_time),
                                      fixed_root(), {initial_axial(1.0), {}});
  close(tip_axial(free.displacement), free_reference.displacement, 2e-9);
  close(tip_axial(free.velocity), free_reference.velocity, 2e-9);
  close(tip_axial(free.acceleration), free_reference.acceleration, 2e-8);
  close(free.kinetic.add(free.strain_energy), {50.0, 50.0, 50.0}, 2e-8);
  expect(free.displacement.sizes() == std::vector<std::int64_t>{3, 2, 3},
         "motion history shape is wrong");
  expect(free.converged.all().item<bool>(), "free-vibration solve failed");
  expect(free.reaction.select(-2, 1).eq(0.0).all().item<bool>(),
         "reaction leaked onto a free node");

  const std::vector<double> varied_time{0.0, 0.07, 0.2, 0.31};
  const std::vector<double> varied_force{0.0, 2.0, -1.0, 3.0};
  structure::MotionOptions damped_options;
  damped_options.spectral_radius = 0.4;
  damped_options.mass_damping = 0.03;
  damped_options.stiffness_damping = 0.002;
  const auto damped_reference = scalar_response(
      varied_time, varied_force, 0.2, -0.1, mass, stiffness,
      damped_options.mass_damping, damped_options.stiffness_damping,
      damped_options.spectral_radius);
  const auto damped = structure::motion(
      beam, history_load(varied_force), pluss::series::make(varied_time),
      fixed_root(), {initial_axial(0.2), initial_axial(-0.1)}, damped_options);
  close(tip_axial(damped.displacement), damped_reference.displacement, 2e-9);
  close(tip_axial(damped.velocity), damped_reference.velocity, 2e-9);
  close(tip_axial(damped.acceleration), damped_reference.acceleration, 2e-8);

  const auto density_batch =
      pluss::series::make(std::vector<double>{5.0, 10.0}, {2, 1})
          .to(pluss::ScalarType::Float64);
  const auto batch_beam =
      structure::frame(one_member(), section(density_batch));
  const auto batch = structure::motion(batch_beam, history_load(zero_force),
                                       pluss::series::make(short_time),
                                       fixed_root(), {initial_axial(1.0), {}});
  expect(batch.displacement.sizes() == std::vector<std::int64_t>{2, 3, 2, 3},
         "batched motion history shape is wrong");
  close(batch.acceleration.select(-3, 0).select(-2, 1).select(-1, 0),
        {-15.0, -7.5}, 2e-8);

  const auto beam32 = structure::frame(
      {one_member().position.to(pluss::ScalarType::Float32), {0}, {1}},
      {section().area.to(pluss::ScalarType::Float32),
       section().inertia.to(pluss::ScalarType::Float32),
       section().modulus.to(pluss::ScalarType::Float32),
       section().density.to(pluss::ScalarType::Float32)});
  structure::MotionOptions loose;
  loose.solve.relative_tolerance = 2e-5;
  loose.solve.absolute_tolerance = 1e-6;
  const auto motion32 = structure::motion(
      beam32, history_load(zero_force, pluss::ScalarType::Float32),
      pluss::series::make(short_time).to(pluss::ScalarType::Float32),
      fixed_root(), {initial_axial(1.0, pluss::ScalarType::Float32), {}},
      loose);
  expect(motion32.displacement.scalar_type() == pluss::ScalarType::Float32,
         "motion widened Float32");
  close(tip_axial(motion32.displacement), free_reference.displacement, 2e-5);

  auto tracked_initial = initial_axial(1.0);
  tracked_initial.tracks_slope_(true);
  const auto tracked = structure::motion(beam, history_load(zero_force),
                                         pluss::series::make(short_time),
                                         fixed_root(), {tracked_initial, {}});
  tip_axial(tracked.displacement).select(0, 2).reverse();
  close(tracked_initial.sensitivity().select(-2, 1).select(-1, 0),
        {free_reference.displacement.back()}, 3e-7);

  auto tracked_force = history_load(varied_force);
  tracked_force.tracks_slope_(true);
  const auto forced =
      structure::motion(beam, tracked_force, pluss::series::make(varied_time),
                        fixed_root(), {}, damped_options);
  tip_axial(forced.displacement).select(0, 3).reverse();
  std::vector<double> force_sensitivity;
  for (std::size_t sample = 0; sample < varied_time.size(); ++sample) {
    std::vector<double> basis(varied_time.size(), 0.0);
    basis[sample] = 1.0;
    force_sensitivity.push_back(
        scalar_response(varied_time, basis, 0.0, 0.0, mass, stiffness,
                        damped_options.mass_damping,
                        damped_options.stiffness_damping,
                        damped_options.spectral_radius)
            .displacement.back());
  }
  close(tip_axial(forced.displacement),
        scalar_response(varied_time, varied_force, 0.0, 0.0, mass, stiffness,
                        damped_options.mass_damping,
                        damped_options.stiffness_damping,
                        damped_options.spectral_radius)
            .displacement,
        2e-9);
  close(tracked_force.sensitivity().select(-2, 1).select(-1, 0),
        force_sensitivity, 3e-7);

  auto tracked_time = pluss::series::make(short_time);
  tracked_time.tracks_slope_(true);
  const auto timed =
      structure::motion(beam, history_load(zero_force), tracked_time,
                        fixed_root(), {initial_axial(1.0), {}});
  tip_axial(timed.displacement).select(0, 2).reverse();
  std::vector<double> time_sensitivity;
  constexpr double time_step = 1e-6;
  for (std::size_t sample = 0; sample < short_time.size(); ++sample) {
    auto lower = short_time;
    auto upper = short_time;
    lower[sample] -= time_step;
    upper[sample] += time_step;
    const double lower_value = scalar_response(lower, zero_force, 1.0, 0.0,
                                               mass, stiffness, 0.0, 0.0, 1.0)
                                   .displacement.back();
    const double upper_value = scalar_response(upper, zero_force, 1.0, 0.0,
                                               mass, stiffness, 0.0, 0.0, 1.0)
                                   .displacement.back();
    time_sensitivity.push_back((upper_value - lower_value) / (2.0 * time_step));
  }
  close(tracked_time.sensitivity(), time_sensitivity, 2e-5);

  std::vector<double> long_time(31);
  for (std::size_t index = 0; index < long_time.size(); ++index)
    long_time[index] = 0.1 * static_cast<double>(index);
  const std::vector<double> long_force(long_time.size(), 0.0);
  const auto undamped = structure::motion(
      beam, history_load(long_force), pluss::series::make(long_time),
      fixed_root(), {initial_axial(1.0), {}});
  structure::MotionOptions filtered_options;
  filtered_options.spectral_radius = 0.0;
  const auto filtered = structure::motion(
      beam, history_load(long_force), pluss::series::make(long_time),
      fixed_root(), {initial_axial(1.0), {}}, filtered_options);
  const double undamped_final = undamped.kinetic.add(undamped.strain_energy)
                                    .select(-1, -1)
                                    .item<double>();
  const double filtered_final = filtered.kinetic.add(filtered.strain_energy)
                                    .select(-1, -1)
                                    .item<double>();
  expect(std::abs(undamped_final - 50.0) < 2e-7,
         "average-acceleration energy drifted");
  expect(filtered_final < undamped_final * 0.8,
         "generalized-alpha did not damp the high-frequency response");

  expect_error(
      [&] {
        const auto no_mass = structure::frame(
            one_member(), {f64({2.0}), f64({3.0}), f64({100.0}), {}});
        (void)structure::motion(no_mass, history_load(zero_force),
                                pluss::series::make(short_time), fixed_root());
      },
      "motion accepted a frame without density");
  expect_error(
      [&] {
        (void)structure::motion(beam, history_load(zero_force),
                                f64({0.0, 0.1, 0.1}), fixed_root());
      },
      "motion accepted repeated time samples");
  expect_error(
      [&] {
        (void)structure::motion(beam, history_load(zero_force),
                                pluss::series::make(short_time),
                                {{0, 0, 0}, {0, 1, 2}, f64({0.0, 0.0, 0.1})});
      },
      "motion accepted a moving support");
  expect_error(
      [&] {
        structure::MotionOptions invalid;
        invalid.spectral_radius = 1.1;
        (void)structure::motion(beam, history_load(zero_force),
                                pluss::series::make(short_time), fixed_root(),
                                {}, invalid);
      },
      "motion accepted an invalid spectral radius");
  expect_error(
      [&] {
        structure::MotionOptions invalid;
        invalid.mass_damping = -0.1;
        (void)structure::motion(beam, history_load(zero_force),
                                pluss::series::make(short_time), fixed_root(),
                                {}, invalid);
      },
      "motion accepted negative damping");
  expect_error(
      [&] {
        (void)structure::motion(beam, history_load(zero_force),
                                pluss::series::make(short_time), fixed_root(),
                                {initial_axial(1.0).add(1.0), {}});
      },
      "motion accepted nonzero initial support motion");

  structure::MotionOptions diagnostic;
  diagnostic.solve.max_iterations = 0;
  diagnostic.fail_on_nonconvergence = false;
  const auto unresolved = structure::motion(
      beam, history_load(zero_force), pluss::series::make(short_time),
      fixed_root(), {initial_axial(1.0), {}}, diagnostic);
  expect(!unresolved.converged.all().item<bool>(),
         "diagnostic motion hid a nonconverged solve");

  std::cout << "eng structure motion checks passed\n";
  return 0;
}
