// P3e: convergence, conservation, and sensitivity gates for the periodic
// incompressible stepper. Every case compares against an independent analytic
// solution or a same-grid fine-step reference, never against a pinned number
// produced by the code under test.
#include "pluss/ma/natsci/physics.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace fluids = pluss::subsp::natsci::physics::fluids;

using pluss::ScalarType;
using pluss::Series;

namespace {

constexpr double kTwoPi = 2.0 * std::numbers::pi;

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

std::string number(double value) {
  char text[32];
  std::snprintf(text, sizeof text, "%.3e", value);
  return text;
}

double max_abs(const Series &value) { return value.abs().amax().item<double>(); }

spatial::PeriodicGrid cube_grid(std::int64_t dim, std::int64_t n) {
  const auto h = common::ScalarQuantity<common::Length>::from_si(kTwoPi / n);
  if (dim == 2)
    return spatial::make_periodic_grid({n, n}, {h, h});
  return spatial::make_periodic_grid({n, n, n}, {h, h, h});
}

// Cell-centered coordinate of every cell along `axis`, broadcast to the grid.
Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(static_cast<std::size_t>(grid.dim()), 1);
  view[static_cast<std::size_t>(axis)] = n;
  const double h = grid.spacing(axis).si_value();
  return pluss::series::arange(0, n, 1, ScalarType::Float64)
      .add(0.5)
      .mul(h)
      .reshape(view)
      .expand(grid.shape(), true);
}

Series stack(const std::vector<Series> &components) {
  Series result = components.front().unsqueeze(0);
  std::vector<Series> rest;
  for (std::size_t i = 1; i < components.size(); ++i)
    rest.push_back(components[i].unsqueeze(0));
  return result.unite(rest, 0);
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

fluids::StepOptions options(fluids::StepRule rule) {
  fluids::StepOptions step;
  step.rule = rule;
  step.guard.enforce = false; // convergence runs probe the stable regime directly
  step.projection.poisson.solve.max_iterations = 800;
  step.projection.poisson.solve.absolute_tolerance = 1e-14;
  step.projection.poisson.solve.relative_tolerance = 1e-14;
  return step;
}

fluids::IncompressibleState make(const Series &velocity, const Series &pressure,
                                 const spatial::PeriodicGrid &grid) {
  return fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(velocity),
      common::SeriesQuantity<common::Pressure>::from_si(pressure), grid);
}

// A smooth analytic velocity is divergence free only to O(h^2) in the centered
// discrete sense. The first stage would project that residual away in a
// dt-independent jump that contaminates the measured temporal order, so initial
// data are projected once (the step passed here only scales the pressure
// increment, which is discarded from the velocity).
fluids::IncompressibleState conform(const fluids::IncompressibleState &state,
                                    const spatial::PeriodicGrid &grid) {
  return fluids::project(state, grid, density(), seconds(1.0),
                         options(fluids::StepRule::Rk3).projection)
      .state;
}

fluids::IncompressibleState march(fluids::IncompressibleState state,
                                  const spatial::PeriodicGrid &grid, double nu,
                                  double start, double duration,
                                  std::int64_t steps,
                                  const fluids::StepOptions &step_options,
                                  const fluids::BodyFn &body = {}) {
  const double dt = duration / static_cast<double>(steps);
  for (std::int64_t k = 0; k < steps; ++k)
    state = fluids::advance(state, grid, density(), viscosity(nu),
                            seconds(start + static_cast<double>(k) * dt),
                            seconds(dt), step_options, body)
                .state;
  return state;
}

// ---------------------------------------------------------------- Taylor-Green

struct Vortex {
  Series velocity;
  Series pressure;
};

// u = -cos x sin y e^{-2 nu t}, v = sin x cos y e^{-2 nu t},
// p = -(rho / 4)(cos 2x + cos 2y) e^{-4 nu t}; exact for the 2-D Navier-Stokes
// equations on the periodic box (Taylor and Green 1937).
Vortex taylor_green(const spatial::PeriodicGrid &grid, double nu, double t) {
  const Series x = coordinate(grid, 0);
  const Series y = coordinate(grid, 1);
  const double velocity_decay = std::exp(-2.0 * nu * t);
  const double pressure_decay = std::exp(-4.0 * nu * t);
  const Series u = x.cos().neg().mul(y.sin()).mul(velocity_decay);
  const Series v = x.sin().mul(y.cos()).mul(velocity_decay);
  const Series p =
      x.mul(2.0).cos().add(y.mul(2.0).cos()).mul(-0.25 * pressure_decay);
  return {stack({u, v}), p};
}

void test_taylor_green_spatial_order_and_invariants() {
  constexpr double nu = 0.1;
  constexpr double final_time = 0.4;
  double previous = 0.0;
  for (const std::int64_t n : {8, 16, 32}) {
    const auto grid = cube_grid(2, n);
    const Vortex start = taylor_green(grid, nu, 0.0);
    const auto end = march(make(start.velocity, start.pressure, grid), grid, nu,
                           0.0, final_time, 40, options(fluids::StepRule::Rk3));
    const Vortex exact = taylor_green(grid, nu, final_time);
    const double error = max_abs(end.velocity().si_value().sub(exact.velocity));
    if (previous > 0.0)
      expect(previous / error > 3.4,
             "Taylor-Green velocity error is not second order in space: n=" +
                 std::to_string(n) + " ratio " + number(previous / error));
    previous = error;

    if (n == 32) {
      expect(error < 5e-4, "Taylor-Green velocity error " + number(error) +
                               " exceeds its second-order bound at n=32");
      const auto axes = std::vector<std::int64_t>{1, 2};
      const double decay = end.velocity()
                               .si_value()
                               .square()
                               .sum()
                               .item<double>() /
                           start.velocity.square().sum().item<double>();
      expect(std::abs(decay - std::exp(-4.0 * nu * final_time)) < 2e-3,
             "kinetic energy does not follow exp(-4 nu t): " + number(decay));
      expect(max_abs(end.velocity().si_value().mean(axes, false)) < 1e-13,
             "projection changed the (zero) mean momentum");
      expect(max_abs(spatial::divergence(end.velocity().si_value(), grid)) <
                 1e-11,
             "projected Taylor-Green velocity is not divergence free");
      const Series pressure_error =
          end.pressure().si_value().sub(exact.pressure);
      expect(max_abs(pressure_error.sub(pressure_error.mean())) < 2e-2,
             "Taylor-Green pressure drifted beyond the gauge");
    }
  }
}

// --------------------------------------------------------- temporal convergence

void test_temporal_orders_against_fine_reference() {
  constexpr std::int64_t n = 16;
  constexpr double nu = 0.2;
  constexpr double final_time = 1.0;
  const auto grid = cube_grid(2, n);
  const Vortex start = taylor_green(grid, nu, 0.0);
  const auto initial = make(start.velocity, start.pressure, grid);

  struct Case {
    fluids::StepRule rule;
    double minimum_order;
    const char *name;
  };
  for (const Case &c : {Case{fluids::StepRule::Euler, 0.9, "Euler"},
                        Case{fluids::StepRule::Rk2, 1.9, "RK2"},
                        Case{fluids::StepRule::Rk3, 2.9, "RK3"}}) {
    const auto step_options = options(c.rule);
    const Series reference =
        march(initial, grid, nu, 0.0, final_time, 320, step_options)
            .velocity()
            .si_value();
    double previous = 0.0;
    for (const std::int64_t steps : {10, 20, 40}) {
      const double error = max_abs(
          march(initial, grid, nu, 0.0, final_time, steps, step_options)
              .velocity()
              .si_value()
              .sub(reference));
      if (previous > 0.0) {
        const double order = std::log2(previous / error);
        expect(order > c.minimum_order,
               std::string(c.name) + " temporal order " + number(order) +
                   " is below " + number(c.minimum_order));
      }
      previous = error;
    }
  }
}

// ------------------------------------------------------------ 3-D Beltrami flow

// ABC flow with A = B = C = 1 is a Beltrami field (curl u = u), so the
// convection term is a pure gradient and the exact solution decays as
// e^{-nu t} with p = -|u|^2 / 2 (Arnold 1965; Childress and Gilbert 1995).
Vortex beltrami(const spatial::PeriodicGrid &grid, double nu, double t) {
  const Series x = coordinate(grid, 0);
  const Series y = coordinate(grid, 1);
  const Series z = coordinate(grid, 2);
  const double decay = std::exp(-nu * t);
  const Series u = z.sin().add(y.cos()).mul(decay);
  const Series v = x.sin().add(z.cos()).mul(decay);
  const Series w = y.sin().add(x.cos()).mul(decay);
  const Series p = u.square().add(v.square()).add(w.square()).mul(-0.5);
  return {stack({u, v, w}), p};
}

void test_beltrami_flow_in_three_dimensions() {
  constexpr double nu = 0.1;
  constexpr double final_time = 0.2;
  double previous = 0.0;
  for (const std::int64_t n : {8, 16}) {
    const auto grid = cube_grid(3, n);
    const Vortex start = beltrami(grid, nu, 0.0);
    const auto end = march(make(start.velocity, start.pressure, grid), grid, nu,
                           0.0, final_time, 8, options(fluids::StepRule::Rk3));
    const Vortex exact = beltrami(grid, nu, final_time);
    const double error = max_abs(end.velocity().si_value().sub(exact.velocity));
    if (previous > 0.0)
      expect(previous / error > 3.3,
             "3-D Beltrami velocity error is not second order: ratio " +
                 number(previous / error));
    previous = error;
    expect(max_abs(spatial::divergence(end.velocity().si_value(), grid)) <
               1e-10,
           "3-D projected velocity is not divergence free");
  }
}

// ----------------------------------------------- manufactured forced 2-D flow

struct Mode {
  double k1;
  double k2;
  double amplitude; // c(t) = amplitude * decay(t)
};

// Stream-function modes psi_m = c_m(t) sin(k1 x) sin(k2 y): the velocity
// u = psi_y, v = -psi_x is divergence free for any amplitudes, but is NOT a
// Navier-Stokes solution, so the body force that makes it one exercises the
// full nonlinear operator, the viscous term, and the time dependence.
// c_0(t) = cos t and c_1(t) = 0.5 exp(-t); p = 0.3 cos(x + y) cos t.
struct Manufactured {
  Series x;
  Series y;

  static double amplitude(std::size_t mode, double t) {
    return mode == 0 ? std::cos(t) : 0.5 * std::exp(-t);
  }
  static double amplitude_rate(std::size_t mode, double t) {
    return mode == 0 ? -std::sin(t) : -0.5 * std::exp(-t);
  }

  Series velocity(double t) const {
    Series u = pluss::series::zeros_like(x);
    Series v = pluss::series::zeros_like(x);
    const std::vector<std::pair<double, double>> k{{1.0, 1.0}, {2.0, 1.0}};
    for (std::size_t m = 0; m < k.size(); ++m) {
      const double c = amplitude(m, t);
      u = u.add(x.mul(k[m].first).sin().mul(y.mul(k[m].second).cos())
                    .mul(c * k[m].second));
      v = v.sub(x.mul(k[m].first).cos().mul(y.mul(k[m].second).sin())
                    .mul(c * k[m].first));
    }
    return stack({u, v});
  }

  Series pressure(double t) const {
    return x.add(y).cos().mul(0.3 * std::cos(t));
  }

  Series force(double t, double nu) const {
    Series u = pluss::series::zeros_like(x), v = u, ux = u, uy = u, vx = u,
           vy = u, ut = u, vt = u, lap_u = u, lap_v = u;
    const std::vector<std::pair<double, double>> k{{1.0, 1.0}, {2.0, 1.0}};
    for (std::size_t m = 0; m < k.size(); ++m) {
      const double k1 = k[m].first, k2 = k[m].second;
      const double c = amplitude(m, t), dc = amplitude_rate(m, t);
      const Series sx = x.mul(k1).sin(), cx = x.mul(k1).cos();
      const Series sy = y.mul(k2).sin(), cy = y.mul(k2).cos();
      const Series u_m = sx.mul(cy).mul(c * k2);
      const Series v_m = cx.mul(sy).mul(-c * k1);
      u = u.add(u_m);
      v = v.add(v_m);
      ux = ux.add(cx.mul(cy).mul(c * k1 * k2));
      uy = uy.add(sx.mul(sy).mul(-c * k2 * k2));
      vx = vx.add(sx.mul(sy).mul(c * k1 * k1));
      vy = vy.add(cx.mul(cy).mul(-c * k1 * k2));
      ut = ut.add(sx.mul(cy).mul(dc * k2));
      vt = vt.add(cx.mul(sy).mul(-dc * k1));
      lap_u = lap_u.add(u_m.mul(-(k1 * k1 + k2 * k2)));
      lap_v = lap_v.add(v_m.mul(-(k1 * k1 + k2 * k2)));
    }
    const Series sine = x.add(y).sin().mul(-0.3 * std::cos(t)); // dp/dx = dp/dy
    const Series fu = ut.add(u.mul(ux)).add(v.mul(uy)).add(sine).sub(lap_u.mul(nu));
    const Series fv = vt.add(u.mul(vx)).add(v.mul(vy)).add(sine).sub(lap_v.mul(nu));
    return stack({fu, fv});
  }
};

void test_manufactured_forced_flow() {
  constexpr double nu = 0.05;
  constexpr double final_time = 0.4;

  // Spatial order with a step small enough that time error is negligible.
  double previous = 0.0;
  for (const std::int64_t n : {16, 32}) {
    const auto grid = cube_grid(2, n);
    const Manufactured mms{coordinate(grid, 0), coordinate(grid, 1)};
    const fluids::BodyFn body = [&](common::ScalarQuantity<common::Time> time,
                                    const fluids::IncompressibleState &) {
      return common::SeriesQuantity<common::Acceleration>::from_si(
          mms.force(time.si_value(), nu));
    };
    const auto end = march(conform(make(mms.velocity(0.0), mms.pressure(0.0), grid), grid),
                           grid, nu, 0.0, final_time, 80,
                           options(fluids::StepRule::Rk3), body);
    const double error =
        max_abs(end.velocity().si_value().sub(mms.velocity(final_time)));
    if (previous > 0.0)
      expect(previous / error > 3.3,
             "manufactured-flow velocity error is not second order: ratio " +
                 number(previous / error));
    previous = error;
  }

  // Temporal order of the forced, time-dependent problem against a same-grid
  // fine-step reference.
  const auto grid = cube_grid(2, 16);
  const Manufactured mms{coordinate(grid, 0), coordinate(grid, 1)};
  const fluids::BodyFn body = [&](common::ScalarQuantity<common::Time> time,
                                  const fluids::IncompressibleState &) {
    return common::SeriesQuantity<common::Acceleration>::from_si(
        mms.force(time.si_value(), nu));
  };
  const auto initial =
      conform(make(mms.velocity(0.0), mms.pressure(0.0), grid), grid);
  struct Case {
    fluids::StepRule rule;
    double minimum_order;
    const char *name;
  };
  for (const Case &c : {Case{fluids::StepRule::Euler, 0.9, "Euler"},
                        Case{fluids::StepRule::Rk2, 1.9, "RK2"},
                        Case{fluids::StepRule::Rk3, 2.9, "RK3"}}) {
    const auto step_options = options(c.rule);
    const Series reference =
        march(initial, grid, nu, 0.0, final_time, 320, step_options, body)
            .velocity()
            .si_value();
    double last = 0.0;
    for (const std::int64_t steps : {10, 20, 40}) {
      const double error = max_abs(
          march(initial, grid, nu, 0.0, final_time, steps, step_options, body)
              .velocity()
              .si_value()
              .sub(reference));
      if (last > 0.0) {
        const double order = std::log2(last / error);
        expect(order > c.minimum_order,
               std::string("forced ") + c.name + " temporal order " +
                   number(order) + " is below " + number(c.minimum_order));
      }
      last = error;
    }
  }
}

// ------------------------------------------------------ reverse sensitivity

void test_reverse_sensitivity_matches_finite_difference() {
  constexpr std::int64_t n = 8;
  constexpr double nu = 0.1;
  const auto grid = cube_grid(2, n);
  const Vortex start = taylor_green(grid, nu, 0.0);
  const auto step_options = options(fluids::StepRule::Rk2);

  const auto energy_of = [&](double scale_value, bool track) {
    Series scale = pluss::series::full({}, scale_value, ScalarType::Float64);
    if (track)
      scale.tracks_slope_(true);
    const auto state = make(start.velocity.mul(scale), start.pressure.mul(scale.square()), grid);
    const auto end = march(state, grid, nu, 0.0, 0.1, 2, step_options);
    return std::pair{scale, end.velocity().si_value().square().sum().mul(0.5)};
  };

  auto [scale, energy] = energy_of(1.0, true);
  energy.reverse();
  expect(scale.has_sensitivity(),
         "reverse sensitivity did not reach the initial amplitude");
  const double gradient = scale.sensitivity().item<double>();
  constexpr double step = 1e-5;
  const double plus = energy_of(1.0 + step, false).second.item<double>();
  const double minus = energy_of(1.0 - step, false).second.item<double>();
  const double finite_difference = (plus - minus) / (2.0 * step);
  expect(std::abs(gradient - finite_difference) <
             1e-5 * std::max(1.0, std::abs(finite_difference)),
         "reverse sensitivity " + number(gradient) +
             " disagrees with the finite difference " +
             number(finite_difference));
}

} // namespace

int main() {
  try {
    test_taylor_green_spatial_order_and_invariants();
    test_temporal_orders_against_fine_reference();
    test_beltrami_flow_in_three_dimensions();
    test_manufactured_forced_flow();
    test_reverse_sensitivity_matches_finite_difference();
    std::cout << "NatSci fluid validation checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
