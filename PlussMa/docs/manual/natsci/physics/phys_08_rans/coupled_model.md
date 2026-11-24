# Coupling a model to the flow solver

[Back to Physics 8: Reynolds-averaged turbulence](../phys_08_rans.md)

The channel solver is a one-dimensional problem with a built-in mesh. To use the same models in a general flow, the model variables are carried next to the pressure on the flow grid and advanced together with the velocity. This page does that on a periodic box with the Taylor-Green vortex, to show how the transported variables behave and how the starting value of the model changes the flow.

## How it works

`rans::initial_transport(closure, viscosity, state)` makes a `rans::Transport` holding the variables of the chosen model on the grid of `state`: {math}`\tilde\nu` for Spalart-Allmaras, or {math}`k` and {math}`\omega` for SST. The `rans::Closure` has the `model` and a `freestream` value, the initial {math}`\tilde\nu/\nu` (or {math}`\nu_t/\nu` for SST); its default is 3.

`rans::advance(state, transport, grid, density, viscosity, closure, time, step, options, body, wall_distance)` takes one step. It first advances the model variables with the resolved velocity: they are advected upwind, diffused with a variable coefficient, produced by the strain {math}`|S|` and destroyed near walls. It then forms the eddy viscosity from the updated variables,

```{math}
\nu_t = \tilde\nu\, f_{v1}(\chi), \quad f_{v1} = \frac{\chi^3}{\chi^3 + 7.1^3}, \quad \chi = \tilde\nu/\nu \qquad\text{(Spalart-Allmaras)}, \qquad \nu_t = \frac{a_1 k}{\max(a_1 \omega,\ |S| F_2)} \qquad\text{(SST)},
```

and advances the momentum equation with the total viscosity {math}`\nu + \nu_t` held fixed over the stages. The model equations are operator-split from the momentum step, which makes the coupling first order in time. The call returns a `CoupledStep` with the new `flow` (a `StepResult`) and the new `transport`. On a box grid the call takes `FlowBounds` and takes the wall distance from the no-slip faces; on a periodic grid there is no wall, so you supply the distance to one as a field.

## Spalart-Allmaras on a vortex

The program advances a Taylor-Green vortex with the Spalart-Allmaras model for 60 steps of 0.01. The wall distance is set to 1 everywhere, far from any wall, and the molecular viscosity is 0.001. It prints the mean {math}`\tilde\nu/\nu` and the kinetic energy every 20 steps.

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
namespace rans = fluids::rans;

static constexpr double two_pi = 2.0 * std::numbers::pi;

static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(2, 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

static double kinetic_energy(const fluids::IncompressibleState &state) { return 0.5 * state.velocity().si_value().square().sum(0, false).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const std::int64_t n = 16;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const auto x = coordinate(grid, 0), y = coordinate(grid, 1);
  const auto velocity = x.cos().neg().mul(y.sin()).unsqueeze(0).unite(std::vector<pluss::Series>{x.sin().mul(y.cos()).unsqueeze(0)}, 0);
  auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(velocity), common::SeriesQuantity<common::Pressure>::from_si(velocity.select(0, 0).mul(0.0)), grid);
  const auto density = common::ScalarQuantity<fluids::Density>::from_si(1.0);
  const auto viscosity = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(1e-3);
  state = fluids::project(state, grid, density, common::ScalarQuantity<common::Time>::from_si(1.0)).state;

  // the model variable is carried next to the flow: it starts at three times the molecular viscosity, is advected, produced by the strain and destroyed near walls
  rans::Closure closure;  // Spalart-Allmaras, free stream nu~ = 3 nu
  auto transport = rans::initial_transport(closure, viscosity, state);
  fluids::StepOptions options;
  options.rule = fluids::StepRule::Rk3;
  options.guard.enforce = false;
  const auto wall_distance = x.mul(0.0).add(1.0);  // a periodic box has no wall, so the distance to one is supplied: here 1 m everywhere

  out.writeln("   step    nu~ / nu (mean)   kinetic energy");
  for (int k = 0; k <= 60; ++k) {
    if (k % 20 == 0)
      out.writeln("   ", k, "      ", transport.nu_tilde.mean().item<double>() / 1e-3, "        ", 0.5 * state.velocity().si_value().square().sum(0, false).mean().item<double>());
    const auto step = rans::advance(state, transport, grid, density, viscosity, closure, common::ScalarQuantity<common::Time>::from_si(k * 0.01), common::ScalarQuantity<common::Time>::from_si(0.01), options, {}, wall_distance);
    state = step.flow.state;
    transport = step.transport;
  }
}
```

Output

```text
   step    nu~ / nu (mean)   kinetic energy
   0      3.000000        0.250000
   20      3.064444        0.249767
   40      3.132628        0.249528
   60      3.204526        0.249282
```

The model variable starts at the free-stream value `3.0` times the molecular viscosity and grows as the strain produces it: `3.064`, `3.133`, `3.205` after 20, 40 and 60 steps. The kinetic energy falls from `0.250000` to `0.249282` over the run, a loss that is larger than molecular viscosity alone would give (`0.25 (1 - e^{-0.0024}) = 0.0006`, against `0.000718` observed) because the eddy viscosity, about `0.2` times the molecular one at `chi = 3`, is added to it.

## The k-omega SST variables

With `closure.model` set to `KOmegaSST`, the transport holds {math}`k` and {math}`\omega` instead, and {math}`\tilde\nu` is left undefined. The program runs the same vortex for 60 steps and prints the means of {math}`k` and {math}`\omega`.

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
namespace rans = fluids::rans;

static constexpr double two_pi = 2.0 * std::numbers::pi;

static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(2, 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

static double kinetic_energy(const fluids::IncompressibleState &state) { return 0.5 * state.velocity().si_value().square().sum(0, false).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const std::int64_t n = 16;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const auto x = coordinate(grid, 0), y = coordinate(grid, 1);
  const auto velocity = x.cos().neg().mul(y.sin()).unsqueeze(0).unite(std::vector<pluss::Series>{x.sin().mul(y.cos()).unsqueeze(0)}, 0);
  auto start = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(velocity), common::SeriesQuantity<common::Pressure>::from_si(velocity.select(0, 0).mul(0.0)), grid);
  const auto density = common::ScalarQuantity<fluids::Density>::from_si(1.0);
  const auto viscosity = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(1e-3);
  start = fluids::project(start, grid, density, common::ScalarQuantity<common::Time>::from_si(1.0)).state;
  fluids::StepOptions options;
  options.rule = fluids::StepRule::Rk3;
  options.guard.enforce = false;
  const auto wall_distance = x.mul(0.0).add(1.0);

  // the k-omega SST model carries two variables: the turbulent kinetic energy k and the specific dissipation omega
  rans::Closure closure;
  closure.model = rans::Model::KOmegaSST;
  auto state = start;
  auto transport = rans::initial_transport(closure, viscosity, state);
  out.writeln("start: mean k ", transport.k.mean().item<double>(), ", mean omega ", transport.omega.mean().item<double>(), ", so nu_t = k/omega is about ", transport.k.mean().item<double>() / transport.omega.mean().item<double>());
  for (int k = 0; k < 60; ++k) {
    const auto step = rans::advance(state, transport, grid, density, viscosity, closure, common::ScalarQuantity<common::Time>::from_si(k * 0.01), common::ScalarQuantity<common::Time>::from_si(0.01), options, {}, wall_distance);
    state = step.flow.state;
    transport = step.transport;
  }
  out.writeln("after 60 steps: mean k ", transport.k.mean().item<double>(), ", mean omega ", transport.omega.mean().item<double>());
  out.writeln("k stays positive: smallest k ", transport.k.min().item<double>(), "; omega smallest ", transport.omega.min().item<double>());
  out.writeln("the model variable the other family would carry (nu~) is ", transport.nu_tilde.defined() ? "defined" : "left undefined");
}
```

Output

```text
start: mean k 0.030000, mean omega 10.000000, so nu_t = k/omega is about 0.003000
after 60 steps: mean k 0.020756, mean omega 6.867474
k stays positive: smallest k 0.019372; omega smallest 6.683736
the model variable the other family would carry (nu~) is left undefined
```

The starting values are `k = 0.03` and `omega = 10`, chosen by `initial_transport` so that {math}`\nu_t = k/\omega = 0.003`, three times the molecular viscosity. Over 60 steps both decay, to `0.020756` and `6.867474`: the destruction terms, {math}`\beta^* k\omega` and {math}`\beta\omega^2`, win over the production of a weakly strained vortex. Their ratio stays near `0.0030`, so the eddy viscosity changes little. Both stay positive (smallest `0.019372` and `6.683736`): the update is written so that the destruction is implicit and cannot drive a variable negative. The last line confirms that `nu_tilde` is left undefined.

## The free-stream value

The starting level of the model variable decides how turbulent the fluid is. The program starts the Spalart-Allmaras model at four free-stream values, from half the molecular viscosity to thirty times it, runs 40 steps and prints the energy lost.

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
namespace rans = fluids::rans;

static constexpr double two_pi = 2.0 * std::numbers::pi;

static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(2, 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

static double kinetic_energy(const fluids::IncompressibleState &state) { return 0.5 * state.velocity().si_value().square().sum(0, false).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const std::int64_t n = 16;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const auto x = coordinate(grid, 0), y = coordinate(grid, 1);
  const auto velocity = x.cos().neg().mul(y.sin()).unsqueeze(0).unite(std::vector<pluss::Series>{x.sin().mul(y.cos()).unsqueeze(0)}, 0);
  auto start = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(velocity), common::SeriesQuantity<common::Pressure>::from_si(velocity.select(0, 0).mul(0.0)), grid);
  const auto density = common::ScalarQuantity<fluids::Density>::from_si(1.0);
  const auto viscosity = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(1e-3);
  start = fluids::project(start, grid, density, common::ScalarQuantity<common::Time>::from_si(1.0)).state;
  fluids::StepOptions options;
  options.rule = fluids::StepRule::Rk3;
  options.guard.enforce = false;
  const auto wall_distance = x.mul(0.0).add(1.0);

  // the free-stream value sets how turbulent the fluid is at the start, as a multiple of the molecular viscosity
  out.writeln("free stream nu~/nu    mean nu~/nu at the start    after 40 steps    kinetic energy lost");
  const double e0 = 0.5 * start.velocity().si_value().square().sum(0, false).mean().item<double>();
  for (double fs : {0.5, 3.0, 10.0, 30.0}) {
    rans::Closure closure;
    closure.freestream = fs;
    auto state = start;
    auto transport = rans::initial_transport(closure, viscosity, state);
    const double initial = transport.nu_tilde.mean().item<double>() / 1e-3;
    for (int k = 0; k < 40; ++k) {
      const auto step = rans::advance(state, transport, grid, density, viscosity, closure, common::ScalarQuantity<common::Time>::from_si(k * 0.01), common::ScalarQuantity<common::Time>::from_si(0.01), options, {}, wall_distance);
      state = step.flow.state;
      transport = step.transport;
    }
    out.writeln("   ", fs, "              ", initial, "                 ", transport.nu_tilde.mean().item<double>() / 1e-3, "        ", e0 - 0.5 * state.velocity().si_value().square().sum(0, false).mean().item<double>());
  }
}
```

Output

```text
free stream nu~/nu    mean nu~/nu at the start    after 40 steps    kinetic energy lost
   0.500000              0.500000                 0.523002        0.000380
   3.000000              3.000000                 3.132628        0.000472
   10.000000              10.000000                 10.433693        0.003346
   30.000000              30.000000                 30.940705        0.011763
```

The Spalart-Allmaras eddy viscosity is {math}`\nu_t = \tilde\nu f_{v1}`, and {math}`f_{v1}` is small unless {math}`\tilde\nu` is several times the molecular viscosity: it is `0.07` at {math}`\chi = 3`, `0.74` at `10` and `0.99` at `30`. The energy losses show it. At `0.5` and `3.0` the loss, `0.000380` and `0.000472`, is close to what the molecular viscosity alone gives over `t = 0.4`, `0.25 (1 - e^{-0.0016}) = 0.0004`. At `10` the flow is damped by about `8.4` times the molecular viscosity and loses `0.003346`, and at `30` by about `31` times and loses `0.011763`. The mean {math}`\tilde\nu/\nu` itself changes by only a few percent in each run, so the starting value is the main choice.

## Decisions

- Start `freestream` at the default `3` for external aerodynamics: it is low enough not to trip the boundary layer early. Use a larger value only when the incoming flow is known to be turbulent, and say so.
- Use `rans::eddy_viscosity(state, transport, grid, bounds, viscosity, closure)` on a box grid to read the eddy viscosity back for post-processing; on a periodic grid take {math}`\tilde\nu` from the transport directly.
- On a periodic grid supply a `wall_distance` field that matches the physics. A constant is only a placeholder that switches the near-wall terms off.
- Use an explicit time rule: the viscosity varies in space and time and the implicit-explicit rule assumes a single number.

## Pitfalls

The transport must belong to the closure, and the time rule must be explicit. The program tries the implicit-explicit rule, then hands a Spalart-Allmaras transport to the SST closure.

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
namespace rans = fluids::rans;

static constexpr double two_pi = 2.0 * std::numbers::pi;

static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(2, 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

static double kinetic_energy(const fluids::IncompressibleState &state) { return 0.5 * state.velocity().si_value().square().sum(0, false).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const std::int64_t n = 16;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const auto x = coordinate(grid, 0), y = coordinate(grid, 1);
  const auto velocity = x.cos().neg().mul(y.sin()).unsqueeze(0).unite(std::vector<pluss::Series>{x.sin().mul(y.cos()).unsqueeze(0)}, 0);
  auto start = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(velocity), common::SeriesQuantity<common::Pressure>::from_si(velocity.select(0, 0).mul(0.0)), grid);
  const auto density = common::ScalarQuantity<fluids::Density>::from_si(1.0);
  const auto viscosity = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(1e-3);
  start = fluids::project(start, grid, density, common::ScalarQuantity<common::Time>::from_si(1.0)).state;
  fluids::StepOptions options;
  options.rule = fluids::StepRule::Rk3;
  options.guard.enforce = false;
  const auto wall_distance = x.mul(0.0).add(1.0);

  rans::Closure closure;
  const auto transport = rans::initial_transport(closure, viscosity, start);
  const auto zero = common::ScalarQuantity<common::Time>::from_si(0.0), width = common::ScalarQuantity<common::Time>::from_si(0.01);
  // 1. the implicit-explicit rule assumes one viscosity number, which a model that varies in space cannot supply
  fluids::StepOptions imex = options;
  imex.rule = fluids::StepRule::Imex1;
  try {
    (void)rans::advance(start, transport, grid, density, viscosity, closure, zero, width, imex, {}, wall_distance);
  } catch (const pluss::Error &e) {
    out.writeln("Imex1: ", e.message());
  }
  // 2. the transport must belong to the closure: nu~ for Spalart-Allmaras, k and omega for SST. Handing nu~ to the SST model leaves k and omega undefined
  rans::Closure sst;
  sst.model = rans::Model::KOmegaSST;
  try {
    (void)rans::advance(start, transport, grid, density, viscosity, sst, zero, width, options, {}, wall_distance);
    out.writeln("an SA transport accepted by the SST model");
  } catch (const pluss::Error &e) {
    out.writeln("SA transport with the SST closure: ", e.message());
  } catch (const std::exception &e) {
    out.writeln("SA transport with the SST closure raised a ", "non-pluss exception");
  }
}
```

Output

```text
Imex1: natsci::fluids::rans::advance: Imex1 needs scalar viscosity; use an explicit rule with a closure
SA transport with the SST closure: natsci::fluids::rans::advance: the k-omega SST closure needs a transport that carries k and omega (make it with rans::initial_transport using the same closure)
```

`Imex1` is refused with the message that it needs a scalar viscosity. The mismatched transport is refused with a message that says the SST closure needs a transport carrying {math}`k` and {math}`\omega`, and that you make one with `rans::initial_transport` using the same closure.

## Where next

To model only the small scales of a resolved flow instead, read [Physics 7](../phys_07_les.md); the two families share the incompressible solver of [the Taylor-Green vortex](../phys_02_fluids/taylor_green_vortex.md). For compressible flows, see [Physics 6](../phys_06_gas_dynamics.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="turbulent_channel.html">
    <span class="label">Previous</span>
    <span class="title">The turbulent channel</span>
  </a>
</div>
<!-- apiref:nav:end -->
