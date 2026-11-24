# Reactors in time

[Back to Chemistry 9: Gas-Phase Kinetics and Equilibrium](../chem_09_gas_phase.md)

You want to follow a reacting gas in time: how fast the molecule is used up, whether the temperature changes as it does, and how long a particular event takes. This page integrates a closed vessel at constant temperature or adiabatically at constant volume, runs several vessels at once, and reads a characteristic time from the trajectory. All three pages use one small system so that every number can be checked by hand: a diatomic {math}`\mathrm{X_2}` that dissociates into two atoms {math}`\mathrm{X}`, {math}`\mathrm{X_2} \rightleftharpoons 2\mathrm{X}`. Its thermodynamic data have constant heat capacities, {math}`c_p/R = 3.5` for {math}`\mathrm{X_2}` and {math}`2.5` for {math}`\mathrm{X}`, and the atom is {math}`\Delta H/R = 12000` K (about {math}`100` kJ/mol) above the molecule.

## How it works

A closed, well-stirred vessel of constant volume has the state {math}`y = (T, c_1, \ldots, c_S)`. The concentrations change by the production rates, {math}`\mathrm{d}c_j/\mathrm{d}t = \dot\omega_j`. If the vessel is held at constant temperature, {math}`\mathrm{d}T/\mathrm{d}t = 0`. If it is adiabatic, the internal energy per volume {math}`u = \sum_j c_j U_j(T)` is conserved, and the temperature follows,

```{math}
\frac{\mathrm{d}T}{\mathrm{d}t} = -\frac{\sum_j U_j\,\dot\omega_j}{\sum_j c_j C_{v,j}}
```

The equations are stiff, with fast and slow reactions together, so they are solved with an implicit stiff integrator. `reactor::make_state(T, c)` packs the state; `temperature(y)`, `concentrations(y)`, `pressure(y)`, `element_totals(m, y)` and `internal_energy_density(m, y)` take it apart; `reactor::integrate(m, y0, t_end, adiabatic)` returns the final state with the step counts and `converged`; `trajectory(m, y0, times, adiabatic)` returns the state at a list of times, of shape {math}`[\text{times}, \ldots, 1+S]`; and `max_slope_time(times, T_path)` finds the time of steepest temperature change, the usual definition of an ignition time. Leading axes of {math}`y_0` run several vessels together.

## An isothermal vessel relaxing to equilibrium

A vessel at {math}`2000` K and {math}`1` atm is filled with pure molecule. The program follows the concentrations of both species at four times from {math}`1` microsecond to {math}`1` ms, and checks the element total.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chem = pluss::ma::natsci::chemistry;
namespace kin = chem::kinetics;
namespace eq = chem::equilibrium;
namespace rx = chem::reactor;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// A toy dissociation X2 -> 2 X with constant heat capacities. Thermodynamic data in the seven-coefficient polynomial form:
// cp/R = a1, h/RT = a1 + a6/T, s/R = a1 ln T + a7 (the other coefficients are zero).
static std::vector<chem::thermo::SpeciesData> species() {
  chem::thermo::SpeciesData x2, x;
  x2.name = "X2"; x2.composition = {{"X", 2}}; x2.molar_mass = 0.040;
  x2.t_min = 200; x2.t_mid = 1000; x2.t_max = 5000;
  x2.low = x2.high = {3.5, 0, 0, 0, 0, 0.0, 0.0};
  x.name = "X"; x.composition = {{"X", 1}}; x.molar_mass = 0.020;
  x.t_min = 200; x.t_mid = 1000; x.t_max = 5000;
  x.low = x.high = {2.5, 0, 0, 0, 0, 6000.0, 2.0};
  return {x2, x};
}
// X2 -> 2 X, first order, k = 1e8 exp(-Ea / RT) with Ea = 100 kJ/mol (SI units: m, mol, s, J)
static std::vector<kin::ReactionSpec> reactions() {
  kin::ReactionSpec r;
  r.reactants = {{"X2", 1}};
  r.products = {{"X", 2}};
  r.a = 1e8; r.b = 0.0; r.ea = 100000.0;
  return {r};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const kin::Mechanism m(species(), reactions(), vec({300.0}));
  const double r_gas = 8.31446261815324;
  // an isothermal vessel at 2000 K, 1 atm, started with pure X2; concentration state y = [T, c_X2, c_X]
  const double c0 = 101325.0 / (r_gas * 2000.0);
  const auto y0 = rx::make_state(vec({2000.0}), pluss::make_series(std::vector<double>{c0, 0.0}, {1, 2}).to(pluss::ScalarType::Float64));
  const std::vector<double> times{0.0, 1e-6, 1e-5, 1e-4, 1e-3};
  const auto path = rx::trajectory(m, y0, times, false);
  out.writeln("time (microseconds) ", std::vector<double>{0.0, 1.0, 10.0, 100.0, 1000.0});
  out.writeln("c(X2) (mol/m^3)      ", flat(path.select(-1, 1)));
  out.writeln("c(X)  (mol/m^3)      ", flat(path.select(-1, 2)));
  out.writeln("temperature (K)      ", flat(path.select(-1, 0)));
  const auto last = path.select(0, 4);
  out.writeln("element X per m^3 at start and end ", flat(rx::element_totals(m, y0)), flat(rx::element_totals(m, last)));
}
```

Output

```text
time (microseconds) [0.0000, 1.0000, 10.0000, 100.0000, 1000.0000]
c(X2) (mol/m^3)      [6.0933, 4.7716, 0.5334, 0.0090, 0.0090]
c(X)  (mol/m^3)      [0.0000, 2.6435, 11.1197, 12.1686, 12.1686]
temperature (K)      [2000.0000, 2000.0000, 2000.0000, 2000.0000, 2000.0000]
element X per m^3 at start and end [12.1866][12.1866]
```

The molecule falls from `6.0933` mol/m{math}`^3` at the start to `4.7716` after one microsecond, `0.5334` after ten and `0.0090` after a hundred, with the atoms rising to `12.1686`. Nothing changes between a hundred microseconds and a millisecond: this is equilibrium, and indeed {math}`c_{\mathrm X}^2/c_{\mathrm{X_2}} = 12.1686^2/0.0090 = 1.65\times10^{4}` mol/m{math}`^3` is the equilibrium constant, {math}`10^{4.2164}`, of the first page. The temperature stays at `2000.0000` K by construction, and the element total, `12.1866` mol/m{math}`^3`, is the same at the end as at the start.

## An adiabatic vessel cools as it dissociates

The same vessel at {math}`2500` K is closed and adiabatic. The program integrates to {math}`10` ms and prints the final temperature, concentrations and pressure, the conserved internal energy density, and the time of steepest cooling on a sampled grid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chem = pluss::ma::natsci::chemistry;
namespace kin = chem::kinetics;
namespace eq = chem::equilibrium;
namespace rx = chem::reactor;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// A toy dissociation X2 -> 2 X with constant heat capacities. Thermodynamic data in the seven-coefficient polynomial form:
// cp/R = a1, h/RT = a1 + a6/T, s/R = a1 ln T + a7 (the other coefficients are zero).
static std::vector<chem::thermo::SpeciesData> species() {
  chem::thermo::SpeciesData x2, x;
  x2.name = "X2"; x2.composition = {{"X", 2}}; x2.molar_mass = 0.040;
  x2.t_min = 200; x2.t_mid = 1000; x2.t_max = 5000;
  x2.low = x2.high = {3.5, 0, 0, 0, 0, 0.0, 0.0};
  x.name = "X"; x.composition = {{"X", 1}}; x.molar_mass = 0.020;
  x.t_min = 200; x.t_mid = 1000; x.t_max = 5000;
  x.low = x.high = {2.5, 0, 0, 0, 0, 6000.0, 2.0};
  return {x2, x};
}
// X2 -> 2 X, first order, k = 1e8 exp(-Ea / RT) with Ea = 100 kJ/mol (SI units: m, mol, s, J)
static std::vector<kin::ReactionSpec> reactions() {
  kin::ReactionSpec r;
  r.reactants = {{"X2", 1}};
  r.products = {{"X", 2}};
  r.a = 1e8; r.b = 0.0; r.ea = 100000.0;
  return {r};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const kin::Mechanism m(species(), reactions(), vec({300.0}));
  const double r_gas = 8.31446261815324;
  // the same vessel but adiabatic and at constant volume: the dissociation absorbs heat and the temperature falls
  const double c0 = 101325.0 / (r_gas * 2500.0);
  const auto y0 = rx::make_state(vec({2500.0}), pluss::make_series(std::vector<double>{c0, 0.0}, {1, 2}).to(pluss::ScalarType::Float64));
  const auto res = rx::integrate(m, y0, 1e-2, true);
  out.writeln("converged ", res.converged ? "yes" : "no", ", steps ", std::to_string(res.steps));
  out.writeln("final temperature (K) ", flat(rx::temperature(res.y)));
  out.writeln("final c(X2), c(X)     ", flat(rx::concentrations(res.y)));
  out.writeln("pressure before / after (atm) ", flat(rx::pressure(y0).div(101325.0)), " / ", flat(rx::pressure(res.y).div(101325.0)));
  // internal energy density is conserved
  out.writeln("internal energy density before / after (J/m^3) ", flat(rx::internal_energy_density(m, y0)), " / ", flat(rx::internal_energy_density(m, res.y)));
  // ignition-style marker: time of steepest temperature change along a sampled path
  const std::vector<double> times{0.0, 1e-6, 2e-6, 5e-6, 1e-5, 2e-5, 5e-5, 1e-4, 1e-3};
  const auto path = rx::trajectory(m, y0, times, true);
  out.writeln("time of steepest cooling (microseconds) ", flat(rx::max_slope_time(times, path.select(-1, 0).neg()).mul(1e6)));
}
```

Output

```text
converged yes, steps 232
final temperature (K) [850.2897]
final c(X2), c(X)     [3.2566, 3.2361]
pressure before / after (atm) [1.0000] / [0.4530]
internal energy density before / after (J/m^3) [253312.5000] / [253312.7613]
time of steepest cooling (microseconds) [0.5000]
```

The dissociation absorbs heat, so the gas cools from `2500` K to `850.2897` K, where it comes to rest with `3.2566` mol/m{math}`^3` of molecule and `3.2361` of atoms. The pressure falls to `0.4530` atm of its initial `1.0000`, because the gas cooled although the number of moles grew. The internal energy density is conserved to the tolerance of the integrator, `253312.5000` against `253312.7613` J/m{math}`^3`. The steepest cooling is found at `0.5000` microseconds, the middle of the first sampling interval, which is a sign that the grid is too coarse to resolve it.

## Several vessels in one call

Three isothermal vessels at {math}`1400`, {math}`1700` and {math}`2000` K, each filled with pure molecule at {math}`1` atm, are three lanes of one trajectory. The program prints the fraction of the initial reactant that is still there at four times.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chem = pluss::ma::natsci::chemistry;
namespace kin = chem::kinetics;
namespace eq = chem::equilibrium;
namespace rx = chem::reactor;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// A toy dissociation X2 -> 2 X with constant heat capacities. Thermodynamic data in the seven-coefficient polynomial form:
// cp/R = a1, h/RT = a1 + a6/T, s/R = a1 ln T + a7 (the other coefficients are zero).
static std::vector<chem::thermo::SpeciesData> species() {
  chem::thermo::SpeciesData x2, x;
  x2.name = "X2"; x2.composition = {{"X", 2}}; x2.molar_mass = 0.040;
  x2.t_min = 200; x2.t_mid = 1000; x2.t_max = 5000;
  x2.low = x2.high = {3.5, 0, 0, 0, 0, 0.0, 0.0};
  x.name = "X"; x.composition = {{"X", 1}}; x.molar_mass = 0.020;
  x.t_min = 200; x.t_mid = 1000; x.t_max = 5000;
  x.low = x.high = {2.5, 0, 0, 0, 0, 6000.0, 2.0};
  return {x2, x};
}
// X2 -> 2 X, first order, k = 1e8 exp(-Ea / RT) with Ea = 100 kJ/mol (SI units: m, mol, s, J)
static std::vector<kin::ReactionSpec> reactions() {
  kin::ReactionSpec r;
  r.reactants = {{"X2", 1}};
  r.products = {{"X", 2}};
  r.a = 1e8; r.b = 0.0; r.ea = 100000.0;
  return {r};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const kin::Mechanism m(species(), reactions(), vec({300.0}));
  const double r_gas = 8.31446261815324;
  // three isothermal vessels at 1400, 1700 and 2000 K, each 1 atm of pure X2: one call, three lanes
  const auto t = vec({1400.0, 1700.0, 2000.0});
  std::vector<double> c;
  for (double tk : {1400.0, 1700.0, 2000.0}) { c.push_back(101325.0 / (r_gas * tk)); c.push_back(0.0); }
  const auto y0 = rx::make_state(t, pluss::make_series(c, {3, 2}).to(pluss::ScalarType::Float64));
  const std::vector<double> times{0.0, 1e-5, 1e-4, 1e-3, 1e-2};
  const auto path = rx::trajectory(m, y0, times, false);
  out.writeln("shape [times, vessels, 1 + species] ", path.sizes()[0], " x ", path.sizes()[1], " x ", path.sizes()[2]);
  out.writeln("fraction of X2 left at 10, 100, 1000, 10000 microseconds, vessel at 1400 K ", flat(path.slice(0, 1, 5, 1).select(-1, 1).select(-1, 0).div(c[0])));
  out.writeln("1700 K ", flat(path.slice(0, 1, 5, 1).select(-1, 1).select(-1, 1).div(c[2])));
  out.writeln("2000 K ", flat(path.slice(0, 1, 5, 1).select(-1, 1).select(-1, 2).div(c[4])));
}
```

Output

```text
shape [times, vessels, 1 + species] 5 x 3 x 3
fraction of X2 left at 10, 100, 1000, 10000 microseconds, vessel at 1400 K [0.8305, 0.1688, 0.0311, 0.0311]
1700 K [0.4296, 0.0056, 0.0054, 0.0054]
2000 K [0.0875, 0.0015, 0.0015, 0.0015]
```

The trajectory has shape `5 x 3 x 3`: five times, three vessels, and the temperature plus two concentrations. The hotter vessels react faster, as the Arrhenius law demands: after ten microseconds `0.8305` of the molecule is left at {math}`1400` K, `0.4296` at {math}`1700` K and `0.0875` at {math}`2000` K. Each vessel then settles at its own equilibrium, `0.0311`, `0.0054` and `0.0015`, with the lower temperature leaving more molecule.

## Decisions

- Use `adiabatic = false` for a vessel in a thermostat and `true` for an insulated one; the choice changes whether the temperature is an unknown.
- Use `integrate` when only the final state matters, and `trajectory` when you want a history. Give `trajectory` enough times to resolve what you are looking for, and add times where the change is fast.
- Choose the output times on a logarithmic grid when the time scales span several decades, which is the usual case for a stiff system.
- Use `converged` and the step counts to judge the integration, and the conservation of the element totals as a check on the answer.

## Pitfalls

The integrator can run out of steps, and the time found from a coarse trajectory is only as good as the grid. The program restricts the step budget to five, and then locates the time of steepest cooling on a coarse and on a fine grid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chem = pluss::ma::natsci::chemistry;
namespace kin = chem::kinetics;
namespace eq = chem::equilibrium;
namespace rx = chem::reactor;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// A toy dissociation X2 -> 2 X with constant heat capacities. Thermodynamic data in the seven-coefficient polynomial form:
// cp/R = a1, h/RT = a1 + a6/T, s/R = a1 ln T + a7 (the other coefficients are zero).
static std::vector<chem::thermo::SpeciesData> species() {
  chem::thermo::SpeciesData x2, x;
  x2.name = "X2"; x2.composition = {{"X", 2}}; x2.molar_mass = 0.040;
  x2.t_min = 200; x2.t_mid = 1000; x2.t_max = 5000;
  x2.low = x2.high = {3.5, 0, 0, 0, 0, 0.0, 0.0};
  x.name = "X"; x.composition = {{"X", 1}}; x.molar_mass = 0.020;
  x.t_min = 200; x.t_mid = 1000; x.t_max = 5000;
  x.low = x.high = {2.5, 0, 0, 0, 0, 6000.0, 2.0};
  return {x2, x};
}
// X2 -> 2 X, first order, k = 1e8 exp(-Ea / RT) with Ea = 100 kJ/mol (SI units: m, mol, s, J)
static std::vector<kin::ReactionSpec> reactions() {
  kin::ReactionSpec r;
  r.reactants = {{"X2", 1}};
  r.products = {{"X", 2}};
  r.a = 1e8; r.b = 0.0; r.ea = 100000.0;
  return {r};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const kin::Mechanism m(species(), reactions(), vec({300.0}));
  const double r_gas = 8.31446261815324;
  const double c0 = 101325.0 / (r_gas * 2500.0);
  const auto y0 = rx::make_state(vec({2500.0}), pluss::make_series(std::vector<double>{c0, 0.0}, {1, 2}).to(pluss::ScalarType::Float64));
  // a step budget that is too small stops the integration before the end time: converged is false
  chem::reactor::stiff::Options tight;
  tight.max_steps = 5;
  const auto short_run = rx::integrate(m, y0, 1e-2, true, tight);
  out.writeln("5 steps allowed: converged ", short_run.converged ? "yes" : "no", ", reached time (microseconds) ", std::vector<double>{short_run.t * 1e6});
  const auto full = rx::integrate(m, y0, 1e-2, true);
  out.writeln("default budget : converged ", full.converged ? "yes" : "no", ", reached time (microseconds) ", std::vector<double>{full.t * 1e6});
  // a sampling grid that is too coarse puts the steepest-change time at the middle of an interval
  const std::vector<double> coarse{0.0, 1e-3, 1e-2};
  const std::vector<double> fine{0.0, 2e-7, 5e-7, 1e-6, 2e-6, 5e-6, 1e-5, 1e-4};
  out.writeln("steepest cooling, coarse grid (microseconds) ", flat(rx::max_slope_time(coarse, rx::trajectory(m, y0, coarse, true).select(-1, 0).neg()).mul(1e6)));
  out.writeln("steepest cooling, fine grid (microseconds)   ", flat(rx::max_slope_time(fine, rx::trajectory(m, y0, fine, true).select(-1, 0).neg()).mul(1e6)));
}
```

Output

```text
5 steps allowed: converged no, reached time (microseconds) [0.0017]
default budget : converged yes, reached time (microseconds) [10000.0000]
steepest cooling, coarse grid (microseconds) [500.0000]
steepest cooling, fine grid (microseconds)   [0.1000]
```

With five steps allowed the integration stops after `0.0017` microseconds and reports `converged no`; with the default budget it reaches the whole `10000` microseconds. The steepest cooling is located at `500.0000` microseconds on a grid of three points, which is the middle of the first interval, and at `0.1000` microseconds on a grid that resolves the first microsecond: the answer of `max_slope_time` is the midpoint of the interval of steepest change, so it cannot be better than the grid spacing.

## Where next

[Mechanisms and rate constants](mechanisms.md) defines the rates that these integrations solve, and [Equilibrium composition](equilibrium_composition.md) gives the state they tend to. [Chemistry 4: Reaction Rates](../chem_04_rates.md) explains where the rate constants come from.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="equilibrium_composition.html">
    <span class="label">Previous</span>
    <span class="title">Equilibrium composition</span>
  </a>
</div>
<!-- apiref:nav:end -->
