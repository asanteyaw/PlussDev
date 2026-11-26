# Walls, inlets and outlets

[Back to Physics 6: Gas dynamics](../phys_06_gas_dynamics.md)

A real flow starts and stops somewhere: gas hits a wall, leaves through an open end, or enters from a reservoir. This page applies the boundary rules of the compressible solver to three problems that have exact answers: a shock reflecting from a wall, the same shock tube with a closed and an open end, and a duct fed from a reservoir.

## How it works

Each face of the box takes exactly one rule from `gas::bounds(grid, {...})`, and a face is named by an axis and a side: `{0, spatial::Side::Low}`. The rules are

| Rule | What it imposes |
|---|---|
| `gas::slip(face)` | a closed end where the gas slides: the normal speed is reflected, the tangential speed is kept |
| `gas::wall(face)` | a no-slip wall, optionally with a wall velocity; adiabatic by default, or `wall_temp` / `wall_heat` to fix the temperature or the heat flux |
| `gas::fixed(face, state)` | the outside state is a given `gas::primitive(density, velocity, pressure)` |
| `gas::far(face, state)` | a characteristic far field: waves leave, the given state enters |
| `gas::outlet(face)` / `gas::outlet(face, pressure)` | gas leaves: extrapolated, or at a given back pressure |
| `gas::inlet(face, total_temperature, total_pressure, direction)` | a subsonic reservoir given by its total conditions; `gas::inlet(face, density, speed, direction)` gives the flow instead |

A rule can be a function of time (`gas::BoundFn`) and two rules can be mixed over a face with `gas::blend(mask, a, b)`. A `gas::Model` carries the ratio of specific heats `gamma` (default `1.4`) and the specific gas constant `gas_constant` (default that of air); `gas::pressure`, `gas::temperature`, `gas::sound` and `gas::velocity` read the physical fields back from a state.

## A shock reflecting from a wall

A stream of gas of density 1, speed -1 (towards the wall) and pressure 1 hits a closed wall at `x = 0`. The wall stops the gas, a shock runs back into the stream, and the gas behind it is at rest. For a closing speed {math}`U` and a sound speed {math}`c_1` the shock moves relative to the stream at

```{math}
W = \tfrac{\gamma + 1}{4} U + \sqrt{\left(\tfrac{\gamma + 1}{4} U\right)^2 + c_1^2}
```

so it moves away from the wall at {math}`s = W - U`; mass and momentum balance then give the density {math}`\rho_2 = \rho_1 (s + U)/s` and the pressure {math}`p_2 = p_1 + \rho_1 U (s + U)` behind it. The program runs 400 cells to `t = 0.3` with a dimensionless gas (gas constant 1, so that {math}`T = p/\rho`) and compares with these formulas.

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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  gas::Model model;  // dimensionless gas: gamma 1.4 and a gas constant of 1, so that T = p / rho
  model.gas_constant = common::ScalarQuantity<gas::SpecificGasConstant>::from_si(1.0);

  // a stream of density 1, speed -1 (towards the wall at x = 0) and pressure 1
  const std::int64_t n = 400;
  const auto grid = spatial::make_box_grid({n}, {metres(1.0 / n)});
  const auto ones = pluss::series::ones({n}, pluss::ScalarType::Float64);
  const auto stream = gas::make(common::SeriesQuantity<gas::Density>::from_si(ones),
                                common::SeriesQuantity<gas::MomDensity>::from_si(ones.neg().unsqueeze(0)),
                                common::SeriesQuantity<gas::EnergyDensity>::from_si(ones.div(0.4).add(0.5)),  // p/(gamma-1) + rho u^2 / 2
                                grid, model);
  // the wall at x = 0 is closed; the stream keeps arriving from x = 1
  const auto inflow = gas::primitive(common::SeriesQuantity<gas::Density>::from_si(pluss::series::full({}, 1.0, pluss::ScalarType::Float64)),
                                     common::SeriesQuantity<common::Speed>::from_si(pluss::series::make({-1.0}, pluss::ScalarType::Float64)),
                                     common::SeriesQuantity<common::Pressure>::from_si(pluss::series::full({}, 1.0, pluss::ScalarType::Float64)));
  const auto rules = gas::bounds(grid, {gas::wall({0, spatial::Side::Low}), gas::fixed({0, spatial::Side::High}, inflow)});

  gas::RunOptions run;
  run.step.rule = gas::Rule::Rk3;
  run.step.flux = gas::FluxScheme::Hllc;
  run.step.recon = gas::Recon::Mc;
  const double stop = 0.3;
  const auto path = gas::evolve_to(stream, grid, rules, seconds(0.0), seconds(stop), model, run);
  const auto &record = path.trajectory.state();
  const auto end = gas::make(common::SeriesQuantity<gas::Density>::from_si(record.get<gas::Density>("density").si_value().select(0, -1)),
                             common::SeriesQuantity<gas::MomDensity>::from_si(record.get<gas::MomDensity>("momentum").si_value().select(0, -1)),
                             common::SeriesQuantity<gas::EnergyDensity>::from_si(record.get<gas::EnergyDensity>("energy").si_value().select(0, -1)), grid, model);
  const auto rho = end.density().si_value();
  const auto u = gas::velocity(end, grid).si_value().select(0, 0);
  const auto p = gas::pressure(end, grid, model).si_value();
  const auto T = gas::temperature(end, grid, model).si_value();
  const auto c = gas::sound(end, grid, model).si_value();

  out.writeln("after ", stop, " the gas next to the wall (cells 0 and 50) and far from it (cell 380):");
  for (std::int64_t i : {0, 50, 380})
    out.writeln("  cell ", i, ": density ", rho.select(0, i).item<double>(), ", speed ", u.select(0, i).item<double>(), ", pressure ", p.select(0, i).item<double>(),
                ", temperature ", T.select(0, i).item<double>(), ", sound speed ", c.select(0, i).item<double>());

  // the reflected shock is where the density jumps from the plateau to the stream's value; its speed follows from the jump conditions
  // relative to the stream the shock moves at (gamma+1)/4 + sqrt(((gamma+1)/4)^2 + c^2) for a closing speed of 1; the wall sees that minus the closing speed
  const double speed = 0.6 + std::sqrt(0.36 + 1.4) - 1.0;
  std::int64_t front = 0;
  for (std::int64_t i = 0; i < n; ++i)
    if (rho.select(0, i).item<double>() < 1.5) { front = i; break; }
  out.writeln("exact shock speed ", speed, ", exact position ", speed * stop, ", computed position (density crosses 1.5) ", (static_cast<double>(front) + 0.5) / n);
  const double rho_behind = (speed + 1.0) / speed;
  out.writeln("exact density behind the shock ", rho_behind, ", exact pressure behind it ", 1.0 + (speed + 1.0));
}
```

Output

```text
after 0.3000 the gas next to the wall (cells 0 and 50) and far from it (cell 380):
  cell 0: density 1.9548, speed -0.0000, pressure 2.9268, temperature 1.4972, sound speed 1.4478
  cell 50: density 2.0789, speed 0.0000, pressure 2.9265, temperature 1.4077, sound speed 1.4039
  cell 380: density 1.0000, speed -1.0000, pressure 1.0000, temperature 1.0000, sound speed 1.1832
exact shock speed 0.9266, exact position 0.2780, computed position (density crosses 1.5) 0.2787
exact density behind the shock 2.0792, exact pressure behind it 2.9266
```

The shock sits at `0.2787` against the exact `0.2780` (the position is read as the first cell where the density falls below 1.5, so it is good to a cell, `0.0025`). The plateau at cell 50 has density `2.0789` and pressure `2.9265` against the exact `2.0792` and `2.9266`, the gas is at rest, and the temperature `1.4077` agrees with {math}`p/\rho`. The far gas is untouched: density, speed -1 and pressure 1. Cell 0 shows the one blemish of a numerical wall: its density `1.9548` is lower than the plateau's `2.0789` while the pressure agrees, the familiar wall-heating error, a small entropy defect confined to the first cell.

## A closed end against an open end

Run the shock tube of the previous page to `t = 0.4`, by which time the shock has reached the right end, once with a closed right end and once with a plain `gas::outlet`, which lets the gas leave as if the tube continued. The program prints the density near the right end for both.

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

// the density profile at time 0.4, by which time the right-running shock has reached the right end of the tube
static pluss::Series density_at_end(const gas::Bounds &rules, const spatial::BoxGrid &grid, std::int64_t n) {
  gas::RunOptions run;
  run.step.rule = gas::Rule::Rk3;
  run.step.flux = gas::FluxScheme::Hllc;
  const auto path = gas::evolve_to(sod_state(grid, n), grid, rules, seconds(0.0), seconds(0.4), {}, run);
  return path.trajectory.state().get<gas::Density>("density").si_value().select(0, -1);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const std::int64_t n = 200;
  const auto grid = spatial::make_box_grid({n}, {metres(1.0 / n)});
  const auto low = gas::slip({0, spatial::Side::Low});

  // a closed right end reflects the shock back into the tube; an outlet with no pressure given extrapolates the gas outward and lets it leave
  const auto closed = density_at_end(gas::bounds(grid, {low, gas::slip({0, spatial::Side::High})}), grid, n);
  const auto open = density_at_end(gas::bounds(grid, {low, gas::outlet({0, spatial::Side::High})}), grid, n);

  out.writeln("   x     density with a closed end   density with an outlet");
  for (std::int64_t i : {120, 150, 170, 180, 190, 199})
    out.writeln(" ", (static_cast<double>(i) + 0.5) / n, "        ", closed.select(0, i).item<double>(), "                    ", open.select(0, i).item<double>());
}
```

Output

```text
   x     density with a closed end   density with an outlet
 0.6025        0.4262                    0.4262
 0.7525        0.4261                    0.4261
 0.8525        0.3968                    0.3968
 0.9025        0.5112                    0.2693
 0.9525        0.5095                    0.2644
 0.9975        0.4958                    0.2612
```

Up to `x = 0.8525` the two profiles are identical, because nothing has reached the end yet. After that the closed end has reflected the shock: the density behind the reflected shock is `0.5112`, `0.5095` and `0.4958`, higher than anything in the single-pass solution. With the outlet the shock leaves without a trace, and the cells next to the end hold the density of the gas behind the contact, `0.2693`, `0.2644` and `0.2612`, which is the plateau value `0.2655` of the exact solution. An outlet with no pressure given extrapolates the interior and is the right choice when the end should be invisible.

## A duct fed from a reservoir

A subsonic inlet specified by total temperature 1.0 and total pressure 1.2 feeds a duct whose outlet is held at pressure 1.0. Starting from gas at rest, the flow settles to a uniform state that isentropic relations give exactly: {math}`p/p_0 = (1 + \tfrac{\gamma - 1}{2} M^2)^{-\gamma/(\gamma - 1)}` fixes the Mach number from the pressure ratio, and {math}`T = T_0/(1 + \tfrac{\gamma-1}{2} M^2)` fixes the rest. The program runs to `t = 12`, about twenty flow-through times, and compares the mid-duct state with the theory.

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
static pluss::Series scalar(double v) { return pluss::series::full({}, v, pluss::ScalarType::Float64); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  gas::Model model;  // dimensionless gas, T = p / rho
  model.gas_constant = common::ScalarQuantity<gas::SpecificGasConstant>::from_si(1.0);
  const double gamma = model.gamma, total_temperature = 1.0, total_pressure = 1.2, exit_pressure = 1.0;

  // a duct that holds gas at rest at the exit pressure, an inlet fed from a reservoir (total conditions) at its left end and a pressure outlet at its right end
  const std::int64_t n = 100;
  const auto grid = spatial::make_box_grid({n}, {metres(1.0 / n)});
  const auto ones = pluss::series::ones({n}, pluss::ScalarType::Float64);
  const auto rest = gas::make(common::SeriesQuantity<gas::Density>::from_si(ones), common::SeriesQuantity<gas::MomDensity>::from_si(pluss::series::zeros({1, n}, pluss::ScalarType::Float64)),
                              common::SeriesQuantity<gas::EnergyDensity>::from_si(ones.div(gamma - 1.0)), grid, model);
  const auto rules = gas::bounds(grid, {gas::inlet({0, spatial::Side::Low}, common::SeriesQuantity<common::Temperature>::from_si(scalar(total_temperature)),
                                                  common::SeriesQuantity<common::Pressure>::from_si(scalar(total_pressure)), pluss::series::make({1.0}, pluss::ScalarType::Float64)),
                                       gas::outlet({0, spatial::Side::High}, common::SeriesQuantity<common::Pressure>::from_si(scalar(exit_pressure)))});

  gas::RunOptions run;
  run.step.rule = gas::Rule::Rk3;
  run.step.flux = gas::FluxScheme::Hllc;
  run.step.recon = gas::Recon::Mc;
  const auto path = gas::evolve_to(rest, grid, rules, seconds(0.0), seconds(12.0), model, run);
  const auto &record = path.trajectory.state();
  const auto end = gas::make(common::SeriesQuantity<gas::Density>::from_si(record.get<gas::Density>("density").si_value().select(0, -1)),
                             common::SeriesQuantity<gas::MomDensity>::from_si(record.get<gas::MomDensity>("momentum").si_value().select(0, -1)),
                             common::SeriesQuantity<gas::EnergyDensity>::from_si(record.get<gas::EnergyDensity>("energy").si_value().select(0, -1)), grid, model);
  const double rho = end.density().si_value().select(0, 50).item<double>(), u = gas::velocity(end, grid).si_value().select(0, 0).select(0, 50).item<double>();
  const double p = gas::pressure(end, grid, model).si_value().select(0, 50).item<double>(), T = gas::temperature(end, grid, model).si_value().select(0, 50).item<double>();

  // the isentropic relations give the steady state: p / p0 = (1 + (gamma-1) M^2 / 2)^(-gamma/(gamma-1)) and T / T0 = 1 / (1 + (gamma-1) M^2 / 2)
  const double mach = std::sqrt(2.0 / (gamma - 1.0) * (std::pow(total_pressure / exit_pressure, (gamma - 1.0) / gamma) - 1.0));
  const double t_exact = total_temperature / (1.0 + 0.5 * (gamma - 1.0) * mach * mach);
  out.writeln("mid-duct state at t = 12 after ", path.steps, " steps:   density ", rho, ", speed ", u, ", pressure ", p, ", temperature ", T);
  out.writeln("isentropic theory:                          density ", exit_pressure / t_exact, ", speed ", mach * std::sqrt(gamma * t_exact), ", pressure ", exit_pressure, ", temperature ", t_exact, "  (Mach ", mach, ")");
}
```

Output

```text
mid-duct state at t = 12 after 4919 steps:   density 1.0545, speed 0.5934, pressure 1.0014, temperature 0.9496
isentropic theory:                          density 1.0535, speed 0.5961, pressure 1.0000, temperature 0.9492  (Mach 0.5171)
```

The Mach number the pressure ratio demands is `0.5171`. The mid-duct values after 4919 steps are density `1.0545`, speed `0.5934`, pressure `1.0014` and temperature `0.9496`, against the isentropic `1.0535`, `0.5961`, `1.0000` and `0.9492`: agreement to a fraction of a percent, with the small remaining difference a slow acoustic transient. Nothing in the setup fixes the speed or the density: the inlet imposes the total conditions, the outlet imposes the pressure, and the flow finds the speed that satisfies both.

## Decisions

- Use `slip` for a closed end in an inviscid run and `wall` when viscosity matters; with `wall` you choose between an adiabatic wall (the default), a wall at a given temperature (`wall_temp`) and a wall with a given heat flux (`wall_heat`).
- Use `outlet(face)` when the boundary should be transparent, `outlet(face, pressure)` when a back pressure is known, and `far` when waves may arrive from outside as well as leave. A pressure outlet is only valid for gas leaving; a far field accepts both directions.
- Use `inlet` with total conditions when the gas comes from a reservoir and you know its stagnation state, and with density and speed when you know the flow. Use `fixed` only when the inflow is supersonic: a fixed state on a subsonic inflow over-specifies the problem and sends reflections back into the domain.
- Pass the same `gas::Model` to `gas::make`, `evolve_to` and the field readers: the pressure of a state depends on `gamma`.

## Pitfalls

A pressure outlet will not accept gas flowing back in, and `bounds` needs a rule for every face. The program shows an outlet with leaving and with entering gas, and a box with only one face given a rule.

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
  const std::int64_t n = 8;
  const auto grid = spatial::make_box_grid({n}, {metres(0.1)});
  const auto ones = pluss::series::ones({n}, pluss::ScalarType::Float64);
  const auto outlet_pressure = common::SeriesQuantity<common::Pressure>::from_si(pluss::series::full({}, 1.0, pluss::ScalarType::Float64));
  const auto rules = gas::bounds(grid, {gas::slip({0, spatial::Side::Low}), gas::outlet({0, spatial::Side::High}, outlet_pressure)});
  // a stream that leaves through the outlet is fine; one that comes in through it is refused
  for (double u : {0.1, -0.1}) {
    const auto state = gas::make(common::SeriesQuantity<gas::Density>::from_si(ones), common::SeriesQuantity<gas::MomDensity>::from_si(ones.mul(u).unsqueeze(0)),
                                 common::SeriesQuantity<gas::EnergyDensity>::from_si(ones.div(0.4).add(0.5 * u * u)), grid);
    try {
      (void)gas::rate(state, grid, rules);
      out.writeln("speed ", u, ": accepted");
    } catch (const pluss::Error &e) {
      out.writeln("speed ", u, ": ", e.message());
    }
  }
  // every face of the box needs a rule: leaving the right end out is an error, not an open end
  try {
    (void)gas::bounds(grid, {gas::slip({0, spatial::Side::Low})});
  } catch (const pluss::Error &e) {
    out.writeln("one face only: ", e.message());
  }
}
```

Output

```text
speed 0.1000: accepted
speed -0.1000: natsci::fluids::gas::boundary: pressure outlet encountered backflow; use a characteristic far field or prescribed state
one face only: natsci::fluids::gas::bounds: expected exactly one rule per face
```

The stream leaving at speed `0.1` is accepted; the same stream entering at speed `-0.1` is refused with a message that names the remedy: use a characteristic far field or a prescribed state. Leaving out a face is an error rather than an open end, because `bounds` expects exactly one rule per face. A tiny reversal of a few units in the last place, which rounding can produce in gas at rest, is not counted as backflow.

## Where next

For the effect of viscosity at a no-slip wall, read [A viscous gas](viscous_gas.md). The same wall and outlet ideas for an incompressible fluid are in [the Taylor-Green vortex](../phys_02_fluids/taylor_green_vortex.md) and its boundary pages; the turbulence closures that make a wall-bounded flow usable are the subject of [Physics 7](../phys_07_les.md) and [Physics 8](../phys_08_rans.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shock_tube.html">
    <span class="label">Previous</span>
    <span class="title">A shock tube</span>
  </a>
  <a class="next-link" href="viscous_gas.html">
    <span class="label">Next</span>
    <span class="title">A viscous gas</span>
  </a>
</div>
<!-- apiref:nav:end -->
