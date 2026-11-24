# A shock tube

[Back to Physics 6: Gas dynamics](../phys_06_gas_dynamics.md)

A tube holds gas at high pressure on one side of a diaphragm and low pressure on the other. When the diaphragm breaks, a shock runs into the low-pressure gas, a rarefaction fan runs the other way, and a contact surface between the two parcels of gas drifts along behind the shock. This page runs that experiment, the Sod problem, with the compressible solver and checks every feature of the result against the exact solution.

## How it works

A compressible gas is described by three conserved fields: the density {math}`\rho`, the momentum density {math}`\rho u` and the total energy density {math}`E`. They obey the Euler equations,

```{math}
\partial_t \begin{pmatrix} \rho \\ \rho u \\ E \end{pmatrix} + \partial_x \begin{pmatrix} \rho u \\ \rho u^2 + p \\ (E + p)\,u \end{pmatrix} = 0, \qquad p = (\gamma - 1)\left(E - \tfrac12 \rho u^2\right).
```

The solver is a finite-volume method: each cell holds an average, the flux through each cell face comes from a Riemann solver, and the cell averages are advanced in time. Because every change to a cell is a flux through a face, mass, momentum and energy are conserved exactly.

You build a state with `gas::make(density, momentum, energy, grid)`, where the momentum has a leading component axis (`[1, n]` for a tube), and advance it with `gas::evolve_to(state, grid, rules, start, stop, model, options)`. The grid is made by `spatial::make_box_grid({n}, {cell_width})`. The boundary rules come from `gas::bounds(grid, {...})` and need one rule for every face of the box; a `gas::slip` rule is a closed, frictionless end. `gas::RunOptions` chooses the numerics: `step.flux` is the Riemann solver (`Rusanov`, `Hllc` or `Roe`), `step.recon` is how the cell values are extended to the faces (`Cell`, `Minmod`, `Mc`, `VanLeer` or `Weno5`) and `step.rule` is the time-stepping rule (`Euler`, `Rk2` or `Rk3`). `evolve_to` picks each time step itself, as a safety factor times the largest step the acoustic Courant condition allows, and lands exactly on the stop time.

## The Sod problem

The program fills a tube of unit length with 200 cells. The left half holds gas at density 1 and pressure 1, the right half gas at density 0.125 and pressure 0.1, both at rest. It runs to time 0.2 with an HLLC flux, minmod reconstruction and a third-order time rule, and prints the state at a few points together with the total mass.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace gas = natsci::physics::fluids::gas;

static common::ScalarQuantity<common::Length> metres(double v) { return common::ScalarQuantity<common::Length>::from_si(v); }
static common::ScalarQuantity<common::Time> seconds(double v) { return common::ScalarQuantity<common::Time>::from_si(v); }

// the Sod tube: gas at rest, density 1 and pressure 1 on the left half, density 0.125 and pressure 0.1 on the right half
static gas::State sod_state(const spatial::BoxGrid &grid, std::int64_t n) {
  const auto x = pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).div(static_cast<double>(n));
  const auto left = x.lt(0.5);
  const auto rho = pluss::series::where(left, pluss::series::full_like(x, 1.0), pluss::series::full_like(x, 0.125));
  const auto p = pluss::series::where(left, pluss::series::full_like(x, 1.0), pluss::series::full_like(x, 0.1));
  const auto momentum = pluss::series::zeros({1, n}, pluss::ScalarType::Float64);  // [component, cell]
  return gas::make(common::SeriesQuantity<gas::Density>::from_si(rho), common::SeriesQuantity<gas::MomDensity>::from_si(momentum),
                   common::SeriesQuantity<gas::EnergyDensity>::from_si(p.div(0.4)), grid);  // E = p / (gamma - 1) for a gas at rest
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const std::int64_t n = 200;
  const auto grid = spatial::make_box_grid({n}, {metres(1.0 / n)});  // the grid takes the width of one cell, not the length of the tube
  const auto state = sod_state(grid, n);
  // closed ends: a slip wall reflects the gas but lets it slide, which is all a one-dimensional tube needs
  const auto rules = gas::bounds(grid, {gas::slip({0, spatial::Side::Low}), gas::slip({0, spatial::Side::High})});

  gas::RunOptions run;
  run.step.rule = gas::Rule::Rk3;
  run.step.recon = gas::Recon::Minmod;
  run.step.flux = gas::FluxScheme::Hllc;
  const auto path = gas::evolve_to(state, grid, rules, seconds(0.0), seconds(0.2), {}, run);
  const auto &record = path.trajectory.state();
  const auto rho = record.get<gas::Density>("density").si_value().select(0, -1);
  const auto end = gas::make(common::SeriesQuantity<gas::Density>::from_si(rho), common::SeriesQuantity<gas::MomDensity>::from_si(record.get<gas::MomDensity>("momentum").si_value().select(0, -1)),
                             common::SeriesQuantity<gas::EnergyDensity>::from_si(record.get<gas::EnergyDensity>("energy").si_value().select(0, -1)), grid);
  const auto u = gas::velocity(end, grid).si_value().select(0, 0);
  const auto p = gas::pressure(end, grid).si_value();

  out.writeln("steps taken: ", path.steps, ", flux evaluations: ", path.flux_evaluations);
  out.writeln("mass at the start ", state.density().si_value().sum().item<double>() / n, ", mass at t = 0.2 ", rho.sum().item<double>() / n);
  out.writeln("   x     density  velocity  pressure");
  for (std::int64_t i : {20, 60, 100, 110, 130, 150, 170, 190}) {
    out.writeln(" ", (static_cast<double>(i) + 0.5) / n, "  ", rho.select(0, i).item<double>(), "  ", u.select(0, i).item<double>(), "  ", p.select(0, i).item<double>());
  }
}
```

Output

```text
steps taken: 215, flux evaluations: 645
mass at the start 0.5625, mass at t = 0.2 0.5625
   x     density  velocity  pressure
 0.1025  1.0000  0.0000  1.0000
 0.3025  0.8671  0.1663  0.8190
 0.5025  0.4254  0.9296  0.3023
 0.5525  0.4260  0.9279  0.3030
 0.6525  0.4209  0.9275  0.3031
 0.7525  0.2655  0.9275  0.3031
 0.8525  0.1911  0.5172  0.1886
 0.9525  0.1250  0.0000  0.1000
```

The profile has the structure the theory predicts. At `x = 0.3025` the rarefaction fan is still passing (density `0.8671`, between the undisturbed `1` and the plateau); at `x = 0.5025` and `x = 0.5525` the gas sits on the plateau between the fan and the contact, at density about `0.426`, speed `0.93` and pressure `0.303`. At `x = 0.7525` the contact has been passed, so the pressure and speed are the same (`0.3031`, `0.9275`) but the density has dropped to `0.2655`: pressure and velocity are continuous across a contact, density is not. At `x = 0.8525` the shock is passing, and at `x = 0.9525` the gas has not yet been reached. The mass `0.5625` is unchanged, as the closed ends and the conservative form require.

## Comparing the schemes with the exact solution

To say how good a scheme is you need the exact answer. The program below contains an exact Sod solver: it finds the pressure {math}`p_*` between the waves from {math}`f_L(p_*) + f_R(p_*) = 0`, where {math}`f_K` is the jump relation for a shock if {math}`p_* > p_K` and the relation for a rarefaction otherwise, and then reads off the wave positions. It then runs six combinations of flux and reconstruction on 200 cells and prints the mean absolute density error, summed over the cells.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace gas = natsci::physics::fluids::gas;

static common::ScalarQuantity<common::Length> metres(double v) { return common::ScalarQuantity<common::Length>::from_si(v); }
static common::ScalarQuantity<common::Time> seconds(double v) { return common::ScalarQuantity<common::Time>::from_si(v); }

static gas::State sod_state(const spatial::BoxGrid &grid, std::int64_t n) {
  const auto x = pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).div(static_cast<double>(n));
  const auto left = x.lt(0.5);
  const auto rho = pluss::series::where(left, pluss::series::full_like(x, 1.0), pluss::series::full_like(x, 0.125));
  const auto p = pluss::series::where(left, pluss::series::full_like(x, 1.0), pluss::series::full_like(x, 0.1));
  return gas::make(common::SeriesQuantity<gas::Density>::from_si(rho),
                   common::SeriesQuantity<gas::MomDensity>::from_si(pluss::series::zeros({1, n}, pluss::ScalarType::Float64)),
                   common::SeriesQuantity<gas::EnergyDensity>::from_si(p.div(0.4)), grid);
}

// the exact density of the Sod problem at time t: find the star pressure p* from  fL(p) + fR(p) = 0, then read off the waves
static double exact_density(double x, double t) {
  const double g = 1.4, rl = 1.0, pl = 1.0, rr = 0.125, pr = 0.1;
  const double cl = std::sqrt(g * pl / rl), cr = std::sqrt(g * pr / rr);
  const auto f = [&](double p, double rk, double pk, double ck) {
    if (p > pk) return (p - pk) * std::sqrt((2.0 / ((g + 1.0) * rk)) / (p + (g - 1.0) / (g + 1.0) * pk));  // shock
    return 2.0 * ck / (g - 1.0) * (std::pow(p / pk, (g - 1.0) / (2.0 * g)) - 1.0);                        // rarefaction
  };
  double ps = 0.3;
  for (int i = 0; i < 60; ++i) {  // Newton with a numerical derivative: the residual is smooth and monotone in p
    const double h = 1e-7, r = f(ps, rl, pl, cl) + f(ps, rr, pr, cr);
    ps -= r / ((f(ps + h, rl, pl, cl) + f(ps + h, rr, pr, cr) - r) / h);
  }
  const double us = 0.5 * (f(ps, rr, pr, cr) - f(ps, rl, pl, cl));
  const double rsl = rl * std::pow(ps / pl, 1.0 / g), csl = std::sqrt(g * ps / rsl);
  const double rsr = rr * ((ps / pr + (g - 1.0) / (g + 1.0)) / ((g - 1.0) / (g + 1.0) * ps / pr + 1.0));
  const double shock = (x - 0.5) / t, shock_speed = us * rsr / (rsr - rr);
  if (shock < -cl) return rl;
  if (shock < us - csl) {  // inside the fan
    const double u = 2.0 / (g + 1.0) * (cl + shock), c = cl - 0.5 * (g - 1.0) * u;
    return rl * std::pow(c / cl, 2.0 / (g - 1.0));
  }
  if (shock < us) return rsl;
  if (shock < shock_speed) return rsr;
  return rr;
}

// the L1 error of the density at time 0.2, and the smallest density the run ever produced
static std::pair<double, double> run(std::int64_t n, gas::FluxScheme flux, gas::Recon recon) {
  const auto grid = spatial::make_box_grid({n}, {metres(1.0 / n)});
  const auto rules = gas::bounds(grid, {gas::slip({0, spatial::Side::Low}), gas::slip({0, spatial::Side::High})});
  gas::RunOptions run;
  run.step.rule = gas::Rule::Rk3;
  run.step.recon = recon;
  run.step.flux = flux;
  const auto path = gas::evolve_to(sod_state(grid, n), grid, rules, seconds(0.0), seconds(0.2), {}, run);
  const auto rho = path.trajectory.state().get<gas::Density>("density").si_value().select(0, -1);
  double error = 0.0;
  for (std::int64_t i = 0; i < n; ++i) error += std::abs(rho.select(0, i).item<double>() - exact_density((static_cast<double>(i) + 0.5) / n, 0.2)) / n;
  return {error, rho.min().item<double>()};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  out.writeln("how the flux and the reconstruction change the answer (200 cells)");
  const struct { const char *name; gas::FluxScheme flux; gas::Recon recon; } schemes[] = {
      {"Rusanov, piecewise constant", gas::FluxScheme::Rusanov, gas::Recon::Cell},
      {"Rusanov, minmod", gas::FluxScheme::Rusanov, gas::Recon::Minmod},
      {"HLLC, minmod", gas::FluxScheme::Hllc, gas::Recon::Minmod},
      {"HLLC, van Leer", gas::FluxScheme::Hllc, gas::Recon::VanLeer},
      {"HLLC, WENO5", gas::FluxScheme::Hllc, gas::Recon::Weno5},
      {"Roe, MC", gas::FluxScheme::Roe, gas::Recon::Mc}};
  for (const auto &s : schemes) {
    const auto r = run(200, s.flux, s.recon);
    out.writeln(s.name, ": L1 density error ", r.first, ", smallest density ", r.second);
  }
}
```

Output

```text
how the flux and the reconstruction change the answer (200 cells)
Rusanov, piecewise constant: L1 density error 0.01825, smallest density 0.12500
Rusanov, minmod: L1 density error 0.00566, smallest density 0.12500
HLLC, minmod: L1 density error 0.00455, smallest density 0.12500
HLLC, van Leer: L1 density error 0.00346, smallest density 0.12500
HLLC, WENO5: L1 density error 0.00262, smallest density 0.12500
Roe, MC: L1 density error 0.00304, smallest density 0.12500
```

The errors rank the schemes. Piecewise-constant values with the Rusanov flux smear every wave over many cells and give `0.01825`; adding a limited slope (`minmod`) cuts that by a factor of three to `0.00566`. Replacing the Rusanov flux by HLLC, which resolves the contact wave that Rusanov averages away, gives `0.00455`; a sharper limiter (`van Leer`) gives `0.00346`, the fifth-order `Weno5` gives `0.00262`, and the Roe flux with the MC limiter reaches `0.00304`. The smallest density stays at the initial `0.12500` in every case: none of the schemes produced an undershoot, because `step` falls back to a robust flux in any cell where the high-order update would leave the admissible states.

## Refining the grid

The error of a method that is second order for smooth flow is not second order for a shock. The program runs the HLLC scheme with minmod reconstruction on 100, 200, 400 and 800 cells and prints the error and the factor by which each refinement reduces it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace gas = natsci::physics::fluids::gas;

static common::ScalarQuantity<common::Length> metres(double v) { return common::ScalarQuantity<common::Length>::from_si(v); }
static common::ScalarQuantity<common::Time> seconds(double v) { return common::ScalarQuantity<common::Time>::from_si(v); }

static gas::State sod_state(const spatial::BoxGrid &grid, std::int64_t n) {
  const auto x = pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).div(static_cast<double>(n));
  const auto left = x.lt(0.5);
  const auto rho = pluss::series::where(left, pluss::series::full_like(x, 1.0), pluss::series::full_like(x, 0.125));
  const auto p = pluss::series::where(left, pluss::series::full_like(x, 1.0), pluss::series::full_like(x, 0.1));
  return gas::make(common::SeriesQuantity<gas::Density>::from_si(rho),
                   common::SeriesQuantity<gas::MomDensity>::from_si(pluss::series::zeros({1, n}, pluss::ScalarType::Float64)),
                   common::SeriesQuantity<gas::EnergyDensity>::from_si(p.div(0.4)), grid);
}

// the exact density of the Sod problem at time t: find the star pressure p* from  fL(p) + fR(p) = 0, then read off the waves
static double exact_density(double x, double t) {
  const double g = 1.4, rl = 1.0, pl = 1.0, rr = 0.125, pr = 0.1;
  const double cl = std::sqrt(g * pl / rl), cr = std::sqrt(g * pr / rr);
  const auto f = [&](double p, double rk, double pk, double ck) {
    if (p > pk) return (p - pk) * std::sqrt((2.0 / ((g + 1.0) * rk)) / (p + (g - 1.0) / (g + 1.0) * pk));  // shock
    return 2.0 * ck / (g - 1.0) * (std::pow(p / pk, (g - 1.0) / (2.0 * g)) - 1.0);                        // rarefaction
  };
  double ps = 0.3;
  for (int i = 0; i < 60; ++i) {  // Newton with a numerical derivative: the residual is smooth and monotone in p
    const double h = 1e-7, r = f(ps, rl, pl, cl) + f(ps, rr, pr, cr);
    ps -= r / ((f(ps + h, rl, pl, cl) + f(ps + h, rr, pr, cr) - r) / h);
  }
  const double us = 0.5 * (f(ps, rr, pr, cr) - f(ps, rl, pl, cl));
  const double rsl = rl * std::pow(ps / pl, 1.0 / g), csl = std::sqrt(g * ps / rsl);
  const double rsr = rr * ((ps / pr + (g - 1.0) / (g + 1.0)) / ((g - 1.0) / (g + 1.0) * ps / pr + 1.0));
  const double shock = (x - 0.5) / t, shock_speed = us * rsr / (rsr - rr);
  if (shock < -cl) return rl;
  if (shock < us - csl) {  // inside the fan
    const double u = 2.0 / (g + 1.0) * (cl + shock), c = cl - 0.5 * (g - 1.0) * u;
    return rl * std::pow(c / cl, 2.0 / (g - 1.0));
  }
  if (shock < us) return rsl;
  if (shock < shock_speed) return rsr;
  return rr;
}

// the L1 error of the density at time 0.2, and the smallest density the run ever produced
static std::pair<double, double> run(std::int64_t n, gas::FluxScheme flux, gas::Recon recon) {
  const auto grid = spatial::make_box_grid({n}, {metres(1.0 / n)});
  const auto rules = gas::bounds(grid, {gas::slip({0, spatial::Side::Low}), gas::slip({0, spatial::Side::High})});
  gas::RunOptions run;
  run.step.rule = gas::Rule::Rk3;
  run.step.recon = recon;
  run.step.flux = flux;
  const auto path = gas::evolve_to(sod_state(grid, n), grid, rules, seconds(0.0), seconds(0.2), {}, run);
  const auto rho = path.trajectory.state().get<gas::Density>("density").si_value().select(0, -1);
  double error = 0.0;
  for (std::int64_t i = 0; i < n; ++i) error += std::abs(rho.select(0, i).item<double>() - exact_density((static_cast<double>(i) + 0.5) / n, 0.2)) / n;
  return {error, rho.min().item<double>()};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  out.writeln("refining the grid with HLLC and minmod");
  double previous = 0.0;
  for (std::int64_t n : {100, 200, 400, 800}) {
    const double e = run(n, gas::FluxScheme::Hllc, gas::Recon::Minmod).first;
    out.write(n, " cells: L1 error ", e);
    if (previous > 0.0) out.write("  (", previous / e, " times smaller than on the coarser grid)");
    out.writeln("");
    previous = e;
  }
}
```

Output

```text
refining the grid with HLLC and minmod
100 cells: L1 error 0.00847
200 cells: L1 error 0.00455  (1.86219 times smaller than on the coarser grid)
400 cells: L1 error 0.00254  (1.78897 times smaller than on the coarser grid)
800 cells: L1 error 0.00145  (1.75756 times smaller than on the coarser grid)
```

Doubling the number of cells reduces the error by a factor of `1.86`, then `1.79`, then `1.76`: less than the factor of four a smooth second-order method would give, and close to the factor of two of a first-order one. The error is dominated by the discontinuities: a shock or a contact occupies a fixed number of cells whatever the grid, so its width shrinks only in proportion to the cell size and its contribution to the L1 error falls only linearly. The same behaviour is why a shock-capturing scheme is judged by how few cells a discontinuity occupies, not by its formal order.

## Decisions

- Choose the **flux** by what you need to resolve. `Rusanov` is the most robust and the most diffusive. `Hllc` restores the contact wave and is the usual choice for gas dynamics. `Roe` is the least diffusive for isolated waves but needs its entropy policy (`options.roe`) to be set sensibly near sonic points.
- Choose the **reconstruction** by the smoothness of the problem. `Cell` is first order and smears everything. `Minmod` is the most dissipative limiter and the safest, `Mc` and `VanLeer` are sharper, and `Weno5` is the most accurate on smooth regions between shocks and costs the most.
- Use `evolve_to` when you want the state at a given time; the step is chosen for you. Use `evolve` when you want a fixed step and a fixed number of steps, for example to compare against another code step for step.
- Give the Courant number `options.step.courant` (default `0.45`) and the safety factor `run.safety` (default `0.9`) room: the step that is used is their product times the largest acoustic step.

## Pitfalls

The grid is made from the width of one cell, not the length of the domain. Passing the tube length gives a grid of 200 metre-wide cells, a domain 200 metres long with the wave speed in metres per second, and the run needs one huge step. The program shows the step limit under both readings and the number of steps the run takes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace gas = natsci::physics::fluids::gas;

static common::ScalarQuantity<common::Length> metres(double v) { return common::ScalarQuantity<common::Length>::from_si(v); }
static common::ScalarQuantity<common::Time> seconds(double v) { return common::ScalarQuantity<common::Time>::from_si(v); }

static gas::State sod_state(const spatial::BoxGrid &grid, std::int64_t n) {
  const auto x = pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).div(static_cast<double>(n));
  const auto left = x.lt(0.5);
  const auto rho = pluss::series::where(left, pluss::series::full_like(x, 1.0), pluss::series::full_like(x, 0.125));
  const auto p = pluss::series::where(left, pluss::series::full_like(x, 1.0), pluss::series::full_like(x, 0.1));
  return gas::make(common::SeriesQuantity<gas::Density>::from_si(rho),
                   common::SeriesQuantity<gas::MomDensity>::from_si(pluss::series::zeros({1, n}, pluss::ScalarType::Float64)),
                   common::SeriesQuantity<gas::EnergyDensity>::from_si(p.div(0.4)), grid);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const std::int64_t n = 200;
  // the grid takes the width of one cell. Passing the length of the tube instead makes every cell a metre wide
  const auto wrong = spatial::make_box_grid({n}, {metres(1.0)});
  const auto right = spatial::make_box_grid({n}, {metres(1.0 / n)});
  out.writeln("step limit with the tube length given as the spacing: ", gas::step_limit(sod_state(wrong, n), wrong).si_value().item<double>(), " s");
  out.writeln("step limit with the cell width given:                 ", gas::step_limit(sod_state(right, n), right).si_value().item<double>(), " s");
  gas::RunOptions run;
  run.step.flux = gas::FluxScheme::Hllc;
  for (const auto *grid : {&wrong, &right}) {
    const auto rules = gas::bounds(*grid, {gas::slip({0, spatial::Side::Low}), gas::slip({0, spatial::Side::High})});
    const auto path = gas::evolve_to(sod_state(*grid, n), *grid, rules, seconds(0.0), seconds(0.2), {}, run);
    out.writeln("steps needed to reach t = 0.2: ", path.steps);
  }
}
```

Output

```text
step limit with the tube length given as the spacing: 0.3803 s
step limit with the cell width given:                 0.0019 s
steps needed to reach t = 0.2: 1
steps needed to reach t = 0.2: 215
```

With the tube length given as the spacing the stable step is `0.3803` s and the run takes a single step to reach `t = 0.2`, so the result is a smeared, meaningless profile that nothing reports as an error. With the cell width the stable step is `0.0019` s and the run takes 215 steps. When a run finishes in a handful of steps, check the step limit with `gas::step_limit(state, grid)` first.

A fixed step is checked against the stability limit and refused when it is too wide. `gas::step` also reports the limit it used.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace gas = natsci::physics::fluids::gas;

static common::ScalarQuantity<common::Length> metres(double v) { return common::ScalarQuantity<common::Length>::from_si(v); }
static common::ScalarQuantity<common::Time> seconds(double v) { return common::ScalarQuantity<common::Time>::from_si(v); }

static gas::State sod_state(const spatial::BoxGrid &grid, std::int64_t n) {
  const auto x = pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).div(static_cast<double>(n));
  const auto left = x.lt(0.5);
  const auto rho = pluss::series::where(left, pluss::series::full_like(x, 1.0), pluss::series::full_like(x, 0.125));
  const auto p = pluss::series::where(left, pluss::series::full_like(x, 1.0), pluss::series::full_like(x, 0.1));
  return gas::make(common::SeriesQuantity<gas::Density>::from_si(rho),
                   common::SeriesQuantity<gas::MomDensity>::from_si(pluss::series::zeros({1, n}, pluss::ScalarType::Float64)),
                   common::SeriesQuantity<gas::EnergyDensity>::from_si(p.div(0.4)), grid);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const std::int64_t n = 200;
  const auto grid = spatial::make_box_grid({n}, {metres(1.0 / n)});
  const auto rules = gas::bounds(grid, {gas::slip({0, spatial::Side::Low}), gas::slip({0, spatial::Side::High})});
  const auto state = sod_state(grid, n);
  const double limit = gas::step_limit(state, grid).si_value().item<double>();
  out.writeln("the stable step for this gas is ", limit, " s");
  // a fixed step wider than the stability limit is refused rather than run
  try {
    (void)gas::step(state, grid, rules, seconds(0.01));
  } catch (const pluss::Error &e) {
    out.writeln("a step of 0.01 s: ", e.message());
  }
  // a step inside the limit goes through, and reports the limit it was checked against
  const auto result = gas::step(state, grid, rules, seconds(0.5 * limit));
  out.writeln("a step of half the limit succeeds; the limit it reports is ", result.limit.si_value().item<double>(), " s, in ", result.flux_evaluations, " flux evaluations");
}
```

Output

```text
the stable step for this gas is 0.0019 s
a step of 0.01 s: natsci::fluids::gas::step: width exceeds the acoustic CFL limit
a step of half the limit succeeds; the limit it reports is 0.0019 s, in 2 flux evaluations
```

The step of `0.01` s is five times the stable `0.0019` s and is refused with the message that the acoustic limit is exceeded; a step of half the limit goes through, costing two flux evaluations for the second-order time rule, and the result reports the limit `0.0019` s. `evolve_to` never trips this check, because it picks its own steps.

## Where next

The closed ends of this tube are the simplest boundary rule. [Walls, inlets and outlets](walls_inlets_outlets.md) covers the others: a shock reflecting from a wall, a pressure outlet and a reservoir feeding a duct. For the effect of viscosity and heat conduction on a gas, read [A viscous gas](viscous_gas.md). The incompressible counterpart of this chapter is [the Taylor-Green vortex](../phys_02_fluids/taylor_green_vortex.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="walls_inlets_outlets.html">
    <span class="label">Next</span>
    <span class="title">Walls, inlets and outlets</span>
  </a>
</div>
<!-- apiref:nav:end -->
