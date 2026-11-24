# Decaying flow with a closure

[Back to Physics 7: Large-eddy simulation](../phys_07_les.md)

A closure matters when it is put into the momentum equation and the flow is advanced. This page runs a decaying vortex with and without a subgrid model, checks that the energy the model removes is the subgrid dissipation it reports, sweeps the constant, and looks at what the dynamic model does on flows with more structure.

## How it works

`les::advance(state, grid, density, viscosity, closure, time, step, options)` takes the same arguments as the plain incompressible `fluids::advance`, plus a `les::Closure`, and returns a `StepResult`. At every stage of the Runge-Kutta step it evaluates {math}`\nu_t` from the stage's velocity and adds it to the molecular viscosity, so the model is part of the momentum rate and the stages see it consistently. The stress divergence is in conservative form, which makes the closure drain kinetic energy at the rate

```{math}
\varepsilon_{\text{sgs}} = \left\langle 2\,\nu_t\, S_{ij} S_{ij} \right\rangle = \left\langle \nu_t\, |S|^2 \right\rangle,
```

which `les::sgs_dissipation(state, grid, closure)` returns. The energy budget of the resolved flow is then {math}`\mathrm{d}E/\mathrm{d}t = -\varepsilon_{\text{mol}} - \varepsilon_{\text{sgs}}`, and you can check any closure run against it. The closure needs an explicit time rule: the implicit-explicit rule `Imex1` assumes a single viscosity number.

## The energy budget

A Taylor-Green vortex of molecular viscosity 0.001 on a `32 x 32` grid is run for 50 steps of 0.01 twice, once with the plain solver and once with the Smagorinsky closure. The program adds up `les::sgs_dissipation` over the steps of the modelled run and compares it with the extra energy the modelled run lost.

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
namespace les = fluids::les;

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
  const std::int64_t n = 32;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const auto x = coordinate(grid, 0), y = coordinate(grid, 1);
  const auto velocity = x.cos().neg().mul(y.sin()).unsqueeze(0).unite(std::vector<pluss::Series>{x.sin().mul(y.cos()).unsqueeze(0)}, 0);
  auto start = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(velocity), common::SeriesQuantity<common::Pressure>::from_si(velocity.select(0, 0).mul(0.0)), grid);
  start = fluids::project(start, grid, common::ScalarQuantity<fluids::Density>::from_si(1.0), common::ScalarQuantity<common::Time>::from_si(1.0)).state;

  const auto density = common::ScalarQuantity<fluids::Density>::from_si(1.0);
  const auto viscosity = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(1e-3);  // a thin fluid: the resolved flow loses almost nothing to viscosity
  fluids::StepOptions options;
  options.rule = fluids::StepRule::Rk3;
  options.guard.enforce = false;
  les::Closure closure;
  closure.model = les::Model::Smagorinsky;

  const int steps = 50;
  const double dt = 0.01;
  auto plain = start, modelled = start;
  double integrated = 0.0;  // the energy the closure takes out, added up step by step from the subgrid dissipation
  for (int k = 0; k < steps; ++k) {
    const auto when = common::ScalarQuantity<common::Time>::from_si(k * dt), width = common::ScalarQuantity<common::Time>::from_si(dt);
    integrated += les::sgs_dissipation(modelled, grid, closure).reshape({}).item<double>() * dt;
    plain = fluids::advance(plain, grid, density, viscosity, when, width, options).state;
    modelled = les::advance(modelled, grid, density, viscosity, closure, when, width, options).state;
  }
  const double e0 = kinetic_energy(start), plain_loss = e0 - kinetic_energy(plain), model_loss = e0 - kinetic_energy(modelled);
  out.writeln("kinetic energy at the start ", e0);
  out.writeln("energy lost with molecular viscosity alone (exact ", e0 * (1.0 - std::exp(-4.0 * 1e-3 * steps * dt)), "): ", plain_loss);
  out.writeln("energy lost with the closure added:                          ", model_loss);
  out.writeln("the subgrid dissipation summed over the run:                  ", integrated);
  out.writeln("loss beyond the molecular part, and the subgrid dissipation:   ", model_loss - plain_loss, " vs ", integrated);
  out.writeln("mean eddy viscosity at the end ", les::eddy_viscosity(modelled, grid, closure).si_value().mean().item<double>(), " against a molecular viscosity of 0.001");
}
```

Output

```text
kinetic energy at the start 0.250000
energy lost with molecular viscosity alone (exact 0.000500): 0.000498
energy lost with the closure added:                          0.001276
the subgrid dissipation summed over the run:                  0.000784
loss beyond the molecular part, and the subgrid dissipation:   0.000779 vs 0.000784
mean eddy viscosity at the end 0.000898 against a molecular viscosity of 0.001
```

With molecular viscosity alone the energy falls by `0.000498`, against the exact `0.000500`: the plain solver reproduces the decay law. With the closure it falls by `0.001276`. The difference, `0.000779`, is the energy the model removed, and the subgrid dissipation summed over the steps is `0.000784`: the two agree to better than one percent, which is the budget closing. The mean eddy viscosity at the end, `0.000898`, is nearly the size of the molecular viscosity `0.001`, so on this coarse grid the closure doubles the damping.

## How strongly the constant acts

The extra energy loss of a closure scales with {math}`C_s^2`. The program repeats the run of the first example for four constants and prints the energy lost and the loss beyond that of the first row.

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
namespace les = fluids::les;

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
  const std::int64_t n = 32;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const auto x = coordinate(grid, 0), y = coordinate(grid, 1);
  const auto velocity = x.cos().neg().mul(y.sin()).unsqueeze(0).unite(std::vector<pluss::Series>{x.sin().mul(y.cos()).unsqueeze(0)}, 0);
  auto start = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(velocity), common::SeriesQuantity<common::Pressure>::from_si(velocity.select(0, 0).mul(0.0)), grid);
  const auto density = common::ScalarQuantity<fluids::Density>::from_si(1.0);
  start = fluids::project(start, grid, density, common::ScalarQuantity<common::Time>::from_si(1.0)).state;
  const auto viscosity = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(1e-3);
  fluids::StepOptions options;
  options.rule = fluids::StepRule::Rk3;
  options.guard.enforce = false;

  // how much more energy a stronger closure removes: the extra loss is the subgrid dissipation, which grows with the square of the constant
  out.writeln("constant   energy lost   extra over the first row");
  double first = 0.0;
  for (double cs : {0.05, 0.1, 0.17, 0.25}) {
    les::Closure closure;
    closure.model = les::Model::Smagorinsky;
    closure.constant = cs;
    auto state = start;
    for (int k = 0; k < 50; ++k)
      state = les::advance(state, grid, density, viscosity, closure, common::ScalarQuantity<common::Time>::from_si(k * 0.01), common::ScalarQuantity<common::Time>::from_si(0.01), options).state;
    const double loss = kinetic_energy(start) - kinetic_energy(state);
    if (first == 0.0) first = loss;
    out.writeln(cs, "      ", loss, "      ", loss - first);
  }
}
```

Output

```text
constant   energy lost   extra over the first row
0.050000      0.000561      0.000000
0.100000      0.000765      0.000204
0.170000      0.001276      0.000715
0.250000      0.002182      0.001621
```

The total loss rises from `0.000561` at `C_s = 0.05` to `0.002182` at `0.25`. The extra over the first row is `0.000204`, `0.000715` and `0.001621`, and the ratios track the differences of squares: the extra losses stand in the ratio {math}`1 : 3.5 : 7.9`, and the differences of squares {math}`(0.1^2 - 0.05^2) : (0.17^2 - 0.05^2) : (0.25^2 - 0.05^2)` stand in the ratio {math}`1 : 3.5 : 8.0`. The constant is a tuning knob with a quadratic effect, so a modest change in it matters.

## What the dynamic model chooses

The dynamic model finds {math}`C_s^2` from the flow. The program builds a deterministic flow from a stream function with modes up to wavenumber `kmax` in each direction, amplitudes falling as {math}`k^{-1.5}` and fixed phases, and prints the dynamic {math}`C_s^2` with and without the clip at zero, to compare with the fixed `0.17^2 = 0.0289`.

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
namespace les = fluids::les;

static constexpr double two_pi = 2.0 * std::numbers::pi;

static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(2, 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

// a deterministic stream function with modes up to wavenumber kmax in each direction: amplitudes fall as k^-slope, phases come from a small generator
static pluss::Series field(const pluss::Series &x, const pluss::Series &y, int kmax, double slope, bool want_u) {
  std::uint64_t seed = 12345;
  const auto next = [&seed] { seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; return static_cast<double>(seed >> 11) / 9007199254740992.0; };
  auto total = x.mul(0.0);
  for (int kx = 0; kx <= kmax; ++kx)
    for (int ky = 1; ky <= kmax; ++ky) {
      const double k = std::sqrt(static_cast<double>(kx * kx + ky * ky)), phase = two_pi * next();
      const auto angle = x.mul(static_cast<double>(kx)).add(y.mul(static_cast<double>(ky))).add(phase);
      // u = d psi / d y, v = -d psi / d x for psi = k^-slope cos(angle)
      const double amplitude = std::pow(k, -slope);
      total = total.add((want_u ? angle.sin().mul(-static_cast<double>(ky)) : angle.sin().mul(static_cast<double>(kx))).mul(amplitude));
    }
  return total;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(3).with_sep(""));
  const std::int64_t n = 32;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const auto x = coordinate(grid, 0), y = coordinate(grid, 1);
  out.writeln("a flow with modes up to wavenumber kmax: C_s^2 the dynamic model finds with and without the clip at zero, against the fixed 0.17^2 = 0.0289");
  for (int kmax : {1, 3, 6, 10}) {
    const auto u = field(x, y, kmax, 1.5, true), v = field(x, y, kmax, 1.5, false);
    const auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(u.unsqueeze(0).unite(std::vector<pluss::Series>{v.unsqueeze(0)}, 0)),
                                          common::SeriesQuantity<common::Pressure>::from_si(u.mul(0.0)), grid);
    les::Closure clipped, free;
    clipped.model = free.model = les::Model::DynamicSmagorinsky;
    free.clip = false;  // allow a negative C_s^2: energy flowing from the small scales back to the large ones (backscatter)
    les::Closure fixed;
    fixed.model = les::Model::Smagorinsky;
    // for a field-wide constant, nu_t = C_s^2 Delta^2 |S|, and the fixed model with C_s = 1 gives Delta^2 |S|, so the ratio of the means is C_s^2
    fixed.constant = 1.0;
    const double unit = les::eddy_viscosity(state, grid, fixed).si_value().mean().item<double>();
    out.writeln("kmax ", kmax, ": C_s^2 clipped ", les::eddy_viscosity(state, grid, clipped).si_value().mean().item<double>() / unit, ", unclipped ", les::eddy_viscosity(state, grid, free).si_value().mean().item<double>() / unit);
  }
}
```

Output

```text
a flow with modes up to wavenumber kmax: C_s^2 the dynamic model finds with and without the clip at zero, against the fixed 0.17^2 = 0.0289
kmax 1: C_s^2 clipped 0.000e+00, unclipped -5.305e-18
kmax 3: C_s^2 clipped 0.000e+00, unclipped -8.823e-07
kmax 6: C_s^2 clipped 3.595e-04, unclipped 3.595e-04
kmax 10: C_s^2 clipped 0.000e+00, unclipped -3.399e-03
```

The dynamic coefficient is small and variable. For `kmax = 1` there is nothing to model and the unclipped value is round-off, `-5.3e-18`. For `kmax = 3` it is `-8.8e-07`, still essentially zero, and for `kmax = 6` it is `3.6e-04`, the same with and without the clip. For `kmax = 10` the clipped value is zero while the unclipped value is `-3.4e-03`. These are two-dimensional flows, in which energy tends to move to larger scales (backscatter), and the identity returns a negative {math}`C_s^2` where that happens; the clip at zero removes it, as the default `clip = true` does. In every case the coefficient is far below the fixed `0.0289`: a closure tuned for three-dimensional turbulence is much too dissipative for these flows, which the dynamic model finds without being told.

## Decisions

- Check the budget on a simple case before trusting a closure on a complicated one: add `les::sgs_dissipation` over the steps and compare it with the extra loss, as the first example does.
- Run the closure with an explicit time rule (`Rk2` or `Rk3`) and the stability guard on in production runs; the viscosity adds a diffusive step limit.
- Treat the constant as part of the model definition and report it with the results: two runs with different constants are different models.
- For three-dimensional decaying turbulence the fixed constant `0.17` and the dynamic model agree; in two dimensions they do not, and a fixed constant is not meaningful.

## Pitfalls

A closure needs an explicit time rule, and the constant is validated. The program tries the implicit-explicit rule with a closure, a constant below `-1`, and a zero constant.

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
namespace les = fluids::les;

static constexpr double two_pi = 2.0 * std::numbers::pi;

static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(2, 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const std::int64_t n = 16;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const auto x = coordinate(grid, 0), y = coordinate(grid, 1);
  const auto velocity = x.cos().neg().mul(y.sin()).unsqueeze(0).unite(std::vector<pluss::Series>{x.sin().mul(y.cos()).unsqueeze(0)}, 0);
  const auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(velocity), common::SeriesQuantity<common::Pressure>::from_si(velocity.select(0, 0).mul(0.0)), grid);
  const auto density = common::ScalarQuantity<fluids::Density>::from_si(1.0);
  const auto viscosity = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(1e-3);
  const auto zero = common::ScalarQuantity<common::Time>::from_si(0.0), width = common::ScalarQuantity<common::Time>::from_si(0.01);

  // the implicit-explicit rule treats the viscosity as a single number, which a closure that varies in space cannot be
  les::Closure closure;
  fluids::StepOptions options;
  options.rule = fluids::StepRule::Imex1;
  try {
    (void)les::advance(state, grid, density, viscosity, closure, zero, width, options);
  } catch (const pluss::Error &e) {
    out.writeln("Imex1 with a closure: ", e.message());
  }
  // a constant below -1 is not the 'use the default' sentinel: it is refused
  closure.constant = -2.0;
  try {
    (void)les::eddy_viscosity(state, grid, closure);
  } catch (const pluss::Error &e) {
    out.writeln("a negative constant: ", e.message());
  }
  // a zero constant is allowed and switches the model off: the eddy viscosity is exactly zero
  closure.constant = 0.0;
  out.writeln("a zero constant gives a largest eddy viscosity of ", les::eddy_viscosity(state, grid, closure).si_value().max().item<double>());
}
```

Output

```text
Imex1 with a closure: natsci::fluids::les::advance: Imex1 needs scalar viscosity; use an explicit rule with a closure
a negative constant: natsci::fluids::les: closure constant, width and test ratio must be finite (width >= 0, ratio > 1)
a zero constant gives a largest eddy viscosity of 0.000000
```

`Imex1` is refused with the message that it needs a scalar viscosity and an explicit rule is needed with a closure. A constant below `-1` is refused; any negative value from `-1` upwards means "use the model default". A constant of exactly zero is allowed and turns the model off: the largest eddy viscosity is `0.000000`.

## Where next

For models of the whole turbulence in the mean rather than of the small scales, read [Physics 8](../phys_08_rans.md). For the compressible counterpart with viscosity, see [A viscous gas](../phys_06_gas_dynamics/viscous_gas.md). The flow solver that carries the closure is described in [the Taylor-Green vortex](../phys_02_fluids/taylor_green_vortex.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eddy_viscosity.html">
    <span class="label">Previous</span>
    <span class="title">Eddy-viscosity closures</span>
  </a>
</div>
<!-- apiref:nav:end -->
