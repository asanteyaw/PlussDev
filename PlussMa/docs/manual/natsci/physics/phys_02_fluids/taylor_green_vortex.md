# The Taylor-Green vortex

[Back to Physics 2: Fluids](../phys_02_fluids.md)

A flow of water or air is described by its velocity at every point and by a pressure that keeps the fluid from compressing. This page runs an incompressible flow in a periodic box, in which an exact solution is known, and measures how the error falls as the grid is refined.

## How it works

A periodic grid is made with `spatial::make_periodic_grid(shape, spacing)`, and a flow state with `fluids::make_state(velocity, pressure, grid)`. `fluids::advance(state, grid, density, viscosity, time, dt, options)` takes one time step: it moves the velocity under advection and viscosity, then `fluids::project` removes the divergence by solving for the pressure that makes the velocity field divergence free. `options.rule` selects the time-stepping rule; `Rk3` is a third-order Runge-Kutta rule. The starting field is projected once before the run so that the first step does not start from a small divergence left by the grid.

## Error against the exact solution

The Taylor-Green vortex, `u = -cos x sin y e^{-2 nu t}`, `v = sin x cos y e^{-2 nu t}`, is an exact solution of the two-dimensional equations. The program starts it with viscosity `0.1`, runs to time `0.4` on grids of 8, 16 and 32 cells per side, and compares the velocity with the exact one and the kinetic energy with `exp(-4 nu t)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace fluids = natsci::physics::fluids;

static constexpr double two_pi = 2.0 * std::numbers::pi;

// cell-centred coordinate of every cell along one axis of a periodic 2-D grid
static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(2, 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

// the Taylor-Green vortex, an exact solution of the 2-D equations for a fluid of viscosity nu
static pluss::Series velocity_at(const spatial::PeriodicGrid &grid, double nu, double t) {
  const auto x = coordinate(grid, 0), y = coordinate(grid, 1);
  const double decay = std::exp(-2.0 * nu * t);
  const auto u = x.cos().neg().mul(y.sin()).mul(decay);
  const auto v = x.sin().mul(y.cos()).mul(decay);
  return u.unsqueeze(0).unite(std::vector<pluss::Series>{v.unsqueeze(0)}, 0);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const double nu = 0.1, end_time = 0.4;
  const auto density = common::ScalarQuantity<fluids::Density>::from_si(1.0);
  const auto viscosity = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(nu);

  fluids::StepOptions options;
  options.rule = fluids::StepRule::Rk3;     // third-order Runge-Kutta in time
  options.guard.enforce = false;            // this run picks its own step, far below the stability limit

  double previous_error = 0.0;
  for (std::int64_t n : {8, 16, 32}) {
    const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
    const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
    const auto start = velocity_at(grid, nu, 0.0);
    const auto pressure = start.select(0, 0).mul(0.0);
    auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(start),
                                    common::SeriesQuantity<common::Pressure>::from_si(pressure), grid);
    // the grid's discrete divergence of the starting field is small but not zero: project it once
    state = fluids::project(state, grid, density, common::ScalarQuantity<common::Time>::from_si(1.0), options.projection).state;

    const int steps = 40;
    const double dt = end_time / steps;
    for (int k = 0; k < steps; ++k)
      state = fluids::advance(state, grid, density, viscosity, common::ScalarQuantity<common::Time>::from_si(k * dt),
                              common::ScalarQuantity<common::Time>::from_si(dt), options).state;

    const double error = state.velocity().si_value().sub(velocity_at(grid, nu, end_time)).abs().amax().item<double>();
    const double energy_ratio = state.velocity().si_value().square().sum().item<double>() / start.square().sum().item<double>();
    out.writeln(n, " x ", n, " cells: largest velocity error = ", error, ", kinetic energy ratio = ", energy_ratio);
    if (previous_error > 0.0) out.writeln("    the error shrank by a factor of ", previous_error / error, " on the finer grid");
    previous_error = error;
  }
  out.writeln("exact energy ratio exp(-4 nu t) = ", std::exp(-4.0 * nu * end_time));
}
```

Output

```text
8 x 8 cells: largest velocity error = 0.003181, kinetic energy ratio = 0.859038
16 x 16 cells: largest velocity error = 0.000909, kinetic energy ratio = 0.853889
    the error shrank by a factor of 3.500295 on the finer grid
32 x 32 cells: largest velocity error = 0.000235, kinetic energy ratio = 0.852581
    the error shrank by a factor of 3.871622 on the finer grid
exact energy ratio exp(-4 nu t) = 0.852144
```

The largest velocity error falls from `0.003181` to `0.000909` to `0.000235` as the grid is refined, a factor of `3.50` and then `3.87` for each doubling: close to four, the factor expected from second-order accuracy in space. The kinetic energy ratio approaches the exact `0.852144` as the grid is refined (`0.859038`, `0.853889`, `0.852581`).

<!-- addex:start -->
## The effect of viscosity

Viscosity damps the vortex, and the kinetic energy decays as {math}`e^{-4\nu t}`. The program runs the vortex on a `16 x 16` grid with three viscosities and compares the energy ratio at time `0.4` with the exact law.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace fluids = natsci::physics::fluids;

static constexpr double two_pi = 2.0 * std::numbers::pi;

// cell-centred coordinate of every cell along one axis of a periodic 2-D grid
static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(2, 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

// the Taylor-Green vortex, an exact solution of the 2-D equations for a fluid of viscosity nu
static pluss::Series velocity_at(const spatial::PeriodicGrid &grid, double nu, double t) {
  const auto x = coordinate(grid, 0), y = coordinate(grid, 1);
  const double decay = std::exp(-2.0 * nu * t);
  const auto u = x.cos().neg().mul(y.sin()).mul(decay);
  const auto v = x.sin().mul(y.cos()).mul(decay);
  return u.unsqueeze(0).unite(std::vector<pluss::Series>{v.unsqueeze(0)}, 0);
}


// advance a Taylor-Green vortex of viscosity nu on an n x n grid with the given rule and number of steps; returns (largest velocity error, kinetic energy ratio)
static std::pair<double, double> run(std::int64_t n, double nu, double end_time, fluids::StepRule rule, int steps) {
  const auto density = common::ScalarQuantity<fluids::Density>::from_si(1.0);
  const auto viscosity = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(nu);
  fluids::StepOptions options;
  options.rule = rule;
  options.guard.enforce = false;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const auto start = velocity_at(grid, nu, 0.0);
  auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(start), common::SeriesQuantity<common::Pressure>::from_si(start.select(0, 0).mul(0.0)), grid);
  state = fluids::project(state, grid, density, common::ScalarQuantity<common::Time>::from_si(1.0), options.projection).state;
  const double dt = end_time / steps;
  for (int k = 0; k < steps; ++k)
    state = fluids::advance(state, grid, density, viscosity, common::ScalarQuantity<common::Time>::from_si(k * dt), common::ScalarQuantity<common::Time>::from_si(dt), options).state;
  const double error = state.velocity().si_value().sub(velocity_at(grid, nu, end_time)).abs().amax().item<double>();
  return {error, state.velocity().si_value().square().sum().item<double>() / start.square().sum().item<double>()};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  // the viscosity sets how fast the vortex decays: kinetic energy falls as exp(-4 nu t)
  for (double nu : {0.02, 0.1, 0.5}) {
    const auto r = run(16, nu, 0.4, fluids::StepRule::Rk3, 40);
    out.writeln("viscosity ", nu, ": energy ratio at t = 0.4 is ", r.second, ", exact exp(-4 nu t) = ", std::exp(-4.0 * nu * 0.4), ", largest velocity error ", r.first);
  }
}
```

Output

```text
viscosity 0.02000: energy ratio at t = 0.4 is 0.96890, exact exp(-4 nu t) = 0.96851, largest velocity error 0.00019
viscosity 0.10000: energy ratio at t = 0.4 is 0.85389, exact exp(-4 nu t) = 0.85214, largest velocity error 0.00091
viscosity 0.50000: energy ratio at t = 0.4 is 0.45395, exact exp(-4 nu t) = 0.44933, largest velocity error 0.00331
```

At a viscosity of `0.02` the energy has fallen to `0.96890` (exact `0.96851`); at `0.1` to `0.85389` (`0.85214`); and at `0.5` to `0.45395` (`0.44933`). The computed decay agrees with the law in each case, and the error grows with the viscosity because the field then changes faster in time.

## Time-stepping rules

The step rule sets the accuracy in time. `StepOptions::rule` selects Euler, a second-order or a third-order Runge-Kutta rule. To measure the time stepping alone the program compares each rule, at three step counts, with a very accurate run on the same `32 x 32` grid, so that the error of the grid does not enter.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace fluids = natsci::physics::fluids;

static constexpr double two_pi = 2.0 * std::numbers::pi;

// cell-centred coordinate of every cell along one axis of a periodic 2-D grid
static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(2, 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

// the Taylor-Green vortex, an exact solution of the 2-D equations for a fluid of viscosity nu
static pluss::Series velocity_at(const spatial::PeriodicGrid &grid, double nu, double t) {
  const auto x = coordinate(grid, 0), y = coordinate(grid, 1);
  const double decay = std::exp(-2.0 * nu * t);
  const auto u = x.cos().neg().mul(y.sin()).mul(decay);
  const auto v = x.sin().mul(y.cos()).mul(decay);
  return u.unsqueeze(0).unite(std::vector<pluss::Series>{v.unsqueeze(0)}, 0);
}


// advance a Taylor-Green vortex of viscosity nu on an n x n grid with the given rule and number of steps; returns (largest velocity error, kinetic energy ratio)
static pluss::Series run_velocity(std::int64_t n, double nu, double end_time, fluids::StepRule rule, int steps) {
  const auto density = common::ScalarQuantity<fluids::Density>::from_si(1.0);
  const auto viscosity = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(nu);
  fluids::StepOptions options;
  options.rule = rule;
  options.guard.enforce = false;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const auto start = velocity_at(grid, nu, 0.0);
  auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(start), common::SeriesQuantity<common::Pressure>::from_si(start.select(0, 0).mul(0.0)), grid);
  state = fluids::project(state, grid, density, common::ScalarQuantity<common::Time>::from_si(1.0), options.projection).state;
  const double dt = end_time / steps;
  for (int k = 0; k < steps; ++k)
    state = fluids::advance(state, grid, density, viscosity, common::ScalarQuantity<common::Time>::from_si(k * dt), common::ScalarQuantity<common::Time>::from_si(dt), options).state;
  return state.velocity().si_value();
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10).with_sep(""));
  // time stepping on its own: compare each rule with a very accurate run on the same 32 x 32 grid (third-order rule, 320 steps),
  // so that only the error of the time stepping is measured
  const auto reference = run_velocity(32, 0.1, 0.4, fluids::StepRule::Rk3, 320);
  const std::vector<std::pair<const char *, fluids::StepRule>> rules = {{"Euler", fluids::StepRule::Euler}, {"Rk2  ", fluids::StepRule::Rk2}, {"Rk3  ", fluids::StepRule::Rk3}};
  for (const auto &rule : rules) {
    std::vector<double> errors;
    for (int steps : {10, 20, 40}) errors.push_back(run_velocity(32, 0.1, 0.4, rule.second, steps).sub(reference).abs().amax().item<double>());
    out.writeln(rule.first, ": error with 10, 20, 40 steps = ", errors, ", ratios ", errors[0] / errors[1], ", ", errors[1] / errors[2]);
  }
}
```

Output

```text
Euler: error with 10, 20, 40 steps = [0.0002922678, 0.0001457557, 0.0000727837], ratios 2.0051893401, 2.0025866209
Rk2  : error with 10, 20, 40 steps = [0.0000007775, 0.0000001938, 0.0000000484], ratios 4.0119831102, 4.0059823931
Rk3  : error with 10, 20, 40 steps = [0.0000000016, 0.0000000002, 0.0000000000], ratios 8.0278287590, 8.0305528501
```

Halving the step halves the error of the Euler rule (the ratios are `2.005` and `2.003`), quarters it for the second-order rule (`4.012`, `4.006`) and divides it by eight for the third-order rule (`8.028`, `8.031`): the observed orders are one, two and three.

<!-- addex:end -->

## Decisions

- Choose the step rule by the accuracy you need: `Rk3` for a smooth flow with few steps, `Euler` only for rough checks; halving the step cuts the error by 2, 4 or 8.
- Refine the grid to reduce the spatial error and the step to reduce the temporal one; the error is the sum of both.
- Leave `options.guard.enforce` on. Turn it off only for a run whose step you have chosen, as the pages here do for fixed small steps.
- Project the starting velocity once with `project` so that its divergence is zero on the grid.

## Pitfalls

The step guard refuses a step beyond the convective or viscous stability limit. The program advances a vortex to time 4 in four steps of size 1, with the guard on, then without it with fewer and fewer steps, and with 400 small steps for comparison.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace fluids = natsci::physics::fluids;

static constexpr double two_pi = 2.0 * std::numbers::pi;

// cell-centred coordinate of every cell along one axis of a periodic 2-D grid
static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(2, 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

// the Taylor-Green vortex, an exact solution of the 2-D equations for a fluid of viscosity nu
static pluss::Series velocity_at(const spatial::PeriodicGrid &grid, double nu, double t) {
  const auto x = coordinate(grid, 0), y = coordinate(grid, 1);
  const double decay = std::exp(-2.0 * nu * t);
  const auto u = x.cos().neg().mul(y.sin()).mul(decay);
  const auto v = x.sin().mul(y.cos()).mul(decay);
  return u.unsqueeze(0).unite(std::vector<pluss::Series>{v.unsqueeze(0)}, 0);
}


// advance a Taylor-Green vortex of viscosity nu on an n x n grid with the given rule and number of steps; returns (largest velocity error, kinetic energy ratio)
static pluss::Series run_velocity(std::int64_t n, double nu, double end_time, fluids::StepRule rule, int steps, bool enforce) {
  const auto density = common::ScalarQuantity<fluids::Density>::from_si(1.0);
  const auto viscosity = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(nu);
  fluids::StepOptions options;
  options.rule = rule;
  options.guard.enforce = enforce;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const auto start = velocity_at(grid, nu, 0.0);
  auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(start), common::SeriesQuantity<common::Pressure>::from_si(start.select(0, 0).mul(0.0)), grid);
  state = fluids::project(state, grid, density, common::ScalarQuantity<common::Time>::from_si(1.0), options.projection).state;
  const double dt = end_time / steps;
  for (int k = 0; k < steps; ++k)
    state = fluids::advance(state, grid, density, viscosity, common::ScalarQuantity<common::Time>::from_si(k * dt), common::ScalarQuantity<common::Time>::from_si(dt), options).state;
  return state.velocity().si_value();
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a 32 x 32 vortex of viscosity 0.1 advanced to time 4 with 4 steps of size 1: far above the stable step for this grid
  try {
    const auto guarded = run_velocity(32, 0.1, 4.0, fluids::StepRule::Rk3, 4, true);
    out.writeln("guard on : largest speed ", guarded.abs().amax().item<double>());
  } catch (const pluss::Error &e) {
    out.writeln("guard on : ", e.message());
  }
  for (int steps : {4, 2, 1}) {
    const auto unguarded = run_velocity(32, 0.1, 4.0, fluids::StepRule::Rk3, steps, false);
    out.writeln("guard off, ", steps, " steps of size ", 4.0 / steps, ": largest speed ", unguarded.abs().amax().item<double>());
  }
  // the same vortex with 400 steps of size 0.01 follows the exact solution, whose largest speed at time 4 is exp(-2 nu t) = 0.4493
  const auto fine = run_velocity(32, 0.1, 4.0, fluids::StepRule::Rk3, 400, true);
  out.writeln("400 steps, guard on: largest speed ", fine.abs().amax().item<double>());
}
```

Output

```text
guard on : natsci::fluids::advance: step exceeds a convective or viscous stability limit
guard off, 4 steps of size 1.0000: largest speed 0.4460
guard off, 2 steps of size 2.0000: largest speed 0.4449
guard off, 1 steps of size 4.0000: largest speed 0.4318
400 steps, guard on: largest speed 0.4462
```

With the guard on, the first step stops with the message shown. Without it the program runs on and the largest speed drifts from the converged `0.4462` to `0.4460`, `0.4449` and `0.4318` as the step grows from 1 to 4. For this smooth vortex the damage looks small, but the guard cannot know that: a rougher flow would not be so forgiving. Reduce the step rather than turn the guard off.

## Where next

Return to [Physics 2: Fluids](../phys_02_fluids.md) for the other fluid pages. [Symplectic integration](../phys_05_mechanics/symplectic_integration.md) discusses time stepping for ordinary differential equations in the same way.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
