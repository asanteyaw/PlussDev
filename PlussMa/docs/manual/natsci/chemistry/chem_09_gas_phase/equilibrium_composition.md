# Equilibrium composition

[Back to Chemistry 9: Gas-Phase Kinetics and Equilibrium](../chem_09_gas_phase.md)

You want to know what a mixture of atoms turns into when it has had time to react: which species are present and how much of each, at a given temperature and pressure, at a given volume, or at a given enthalpy. This page solves the equilibrium composition by minimising the Gibbs energy under the conservation of the elements, for the four standard pairs of constraints, and checks the answer against the mass-action law. This page uses the small system of [Mechanisms and rate constants](mechanisms.md) so that every number can be checked by hand: a diatomic {math}`\mathrm{X_2}` that dissociates into two atoms {math}`\mathrm{X}`, {math}`\mathrm{X_2} \rightleftharpoons 2\mathrm{X}`. Its thermodynamic data have constant heat capacities, {math}`c_p/R = 3.5` for {math}`\mathrm{X_2}` and {math}`2.5` for {math}`\mathrm{X}`, and the atom is {math}`\Delta H/R = 12000` K (about {math}`100` kJ/mol) above the molecule.

## How it works

The equilibrium of an ideal-gas mixture at fixed temperature and pressure minimises the Gibbs energy {math}`G = \sum_j n_j\mu_j` with {math}`\mu_j/RT = g_j^\circ/RT + \ln(n_j/n) + \ln(p/p^\circ)`, subject to the conservation of the atoms of each element, {math}`\sum_j a_{ij}n_j = b_i`, where {math}`a_{ij}` is the number of atoms of element {math}`i` in species {math}`j`. The Lagrange conditions give a Newton iteration on the logarithms of the mole numbers with the element potentials {math}`\pi_i` as multipliers,

```{math}
\frac{\mu_j}{RT} = \sum_i a_{ij}\pi_i \quad\text{for every species present}
```

`equilibrium::tp(m, T, p, b)` solves it for given temperature and pressure; `hp(m, p, h, b)` for given pressure and enthalpy (the adiabatic flame problem, in which the temperature is an unknown too); `tv(m, T, V, b)` for given temperature and volume; and `uv(m, V, u, b)` for given volume and internal energy. The element totals {math}`b` have shape {math}`[\ldots, \text{elements}]` in moles, and the other arguments are Series of shape {math}`[\ldots]`. The result carries `moles` {math}`[\ldots, \text{species}]`, `temperature`, `pressure`, the element potentials, `converged` and `iterations`. The solver works with logarithms and is damped, so it converges from a poor start and keeps every mole number positive.

## Dissociation against temperature and pressure

One mole of {math}`\mathrm{X_2}`, that is two moles of the element X, is brought to equilibrium at {math}`1` atm at six temperatures, and then at {math}`1200` K at four pressures from {math}`0.01` to {math}`10` atm, each set as one call. The element total is printed to show conservation.

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
  // one mole of X2 (two moles of the element X), at 1 atm, over six temperatures
  const auto t = vec({1000.0, 1500.0, 2000.0, 2500.0, 3000.0, 4000.0});
  const auto p = t.mul(0.0).add(101325.0);
  const auto b0 = pluss::make_series(std::vector<double>{2, 2, 2, 2, 2, 2}, {6, 1}).to(pluss::ScalarType::Float64);
  const auto r = eq::tp(m, t, p, b0);
  out.writeln("converged ", r.converged ? "yes" : "no", ", iterations ", std::to_string(r.iterations));
  out.writeln("temperature (K)      ", flat(t));
  out.writeln("moles of X2          ", flat(r.moles.select(-1, 0)));
  out.writeln("moles of X           ", flat(r.moles.select(-1, 1)));
  out.writeln("element X conserved  ", flat(r.moles.select(-1, 0).mul(2.0).add(r.moles.select(-1, 1))));
  // pressure: the same cell at 1200 K and 0.01, 0.1, 1, 10 atm; dissociation is favoured at low pressure
  const auto t2 = vec({1200.0, 1200.0, 1200.0, 1200.0});
  const auto p2 = vec({1013.25, 10132.5, 101325.0, 1013250.0});
  const auto b2 = pluss::make_series(std::vector<double>{2, 2, 2, 2}, {4, 1}).to(pluss::ScalarType::Float64);
  const auto r2 = eq::tp(m, t2, p2, b2);
  out.writeln("pressure (atm)       ", std::vector<double>{0.01, 0.1, 1.0, 10.0});
  out.writeln("moles of X2          ", flat(r2.moles.select(-1, 0)));
  out.writeln("moles of X           ", flat(r2.moles.select(-1, 1)));
}
```

Output

```text
converged yes, iterations 11
temperature (K)      [1000.0000, 1500.0000, 2000.0000, 2500.0000, 3000.0000, 4000.0000]
moles of X2          [0.3903, 0.0083, 0.0007, 0.0002, 0.0001, 0.0000]
moles of X           [1.2194, 1.9834, 1.9985, 1.9997, 1.9999, 2.0000]
element X conserved  [2.0000, 2.0000, 2.0000, 2.0000, 2.0000, 2.0000]
pressure (atm)       [0.0100, 0.1000, 1.0000, 10.0000]
moles of X2          [0.0009, 0.0086, 0.0771, 0.3959]
moles of X           [1.9983, 1.9828, 1.8459, 1.2083]
```

The solver converges in `11` iterations for all lanes. At {math}`1000` K the mixture is `0.3903` mol of molecule and `1.2194` mol of atoms, already mostly dissociated for this toy; by {math}`1500` K it is `0.0083` and `1.9834`, and above {math}`2000` K it is atomic. Every lane conserves the element, `2.0000`. Pressure works against dissociation, since two moles are made from one: at {math}`1200` K the molecule is `0.0009` mol at {math}`0.01` atm and `0.3959` mol at {math}`10` atm, and the atoms fall from `1.9983` to `1.2083`.

## Checking against the mass-action law

The equilibrium constant in pressure units is {math}`K_p = (x_{\mathrm X}^2/x_{\mathrm{X_2}})(p/p^\circ)`. The program computes it from the mole numbers of the solver at {math}`3000` K and {math}`1` atm, and independently from {math}`\exp(-\Delta G^\circ/RT)` with the species free energies, and then solves the constant-volume problem for one mole in a {math}`0.5` m{math}`^3` vessel.

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
  // check the mass-action law against the equilibrium solver: K_p = (x_X^2 / x_X2) (p / p0) at 3000 K, 1 atm
  const auto r = eq::tp(m, vec({3000.0}), vec({101325.0}), pluss::make_series(std::vector<double>{2.0}, {1, 1}).to(pluss::ScalarType::Float64));
  const double n2 = flat(r.moles.select(-1, 0))[0], n1 = flat(r.moles.select(-1, 1))[0];
  const double ntot = n1 + n2;
  const double kp_solver = (n1 / ntot) * (n1 / ntot) / (n2 / ntot);
  const auto g = m.thermo().g_rt(vec({3000.0}));
  const double dg = 2.0 * flat(g.select(-1, 1))[0] - flat(g.select(-1, 0))[0];
  out.writeln("K_p from the solution   ", std::vector<double>{kp_solver});
  out.writeln("K_p from exp(-dG/RT)    ", std::vector<double>{std::exp(-dg)});
  // constant-volume problem: 1 mol of X2 in a 0.5 m^3 vessel at 3000 K
  const auto rv = eq::tv(m, vec({3000.0}), vec({0.5}), pluss::make_series(std::vector<double>{2.0}, {1, 1}).to(pluss::ScalarType::Float64));
  out.writeln("TV problem: moles X2, X ", flat(rv.moles), ", pressure (atm) ", flat(rv.pressure.div(101325.0)));
}
```

Output

```text
K_p from the solution   [36664.0266]
K_p from exp(-dG/RT)    [36664.0266]
TV problem: moles X2, X [0.0001, 1.9999], pressure (atm) [0.9847]
```

Both routes give `36664.0266`, so the solver finds the state that the law demands. The constant-volume problem has the molecule at `0.0001` mol and the atoms at `1.9999` mol, with the pressure `0.9847` atm that follows from two moles of gas at {math}`3000` K in {math}`0.5` m{math}`^3`, {math}`nRT/V`.

## The adiabatic problem: enthalpy and pressure

The enthalpy of the equilibrium mixture at {math}`3000` K and {math}`1` atm is computed from the solution, and `hp` is asked to find the temperature that goes with that enthalpy and pressure. The same enthalpy is then used at ten atmospheres.

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
  // the enthalpy of one mole of X2 at 3000 K, found from the model, defines the starting point of a constant-pressure problem
  const auto b0 = pluss::make_series(std::vector<double>{2.0}, {1, 1}).to(pluss::ScalarType::Float64);
  const auto at3000 = eq::tp(m, vec({3000.0}), vec({101325.0}), b0);
  const auto h_rt = m.thermo().h_rt(vec({3000.0}));            // [1, 2] species enthalpy / RT
  const double rt = pluss::ma::natsci::common::constants::codata2022::molar_gas_constant.measurement.value().si_value() * 3000.0;
  const double h = (flat(at3000.moles.mul(h_rt))[0] + flat(at3000.moles.mul(h_rt))[1]) * rt;
  out.writeln("enthalpy of the equilibrium mixture at 3000 K (J) ", std::vector<double>{h});
  // given that enthalpy and the pressure, the solver finds the temperature back
  const auto hp = eq::hp(m, vec({101325.0}), vec({h}), b0);
  out.writeln("converged ", hp.converged ? "yes" : "no", ", iterations ", std::to_string(hp.iterations));
  out.writeln("temperature (K)  ", flat(hp.temperature));
  out.writeln("moles of X2, X   ", flat(hp.moles));
  // the same enthalpy at ten times the pressure gives a lower temperature
  const auto hp10 = eq::hp(m, vec({1013250.0}), vec({h}), b0);
  out.writeln("temperature at 10 atm (K) ", flat(hp10.temperature));
}
```

Output

```text
enthalpy of the equilibrium mixture at 3000 K (J) [224483.0077]
converged yes, iterations 10
temperature (K)  [3000.0000]
moles of X2, X   [0.0001, 1.9999]
temperature at 10 atm (K) [3001.6136]
```

The enthalpy of the mixture is `224483.0077` J, and `hp` returns `3000.0000` K from it in `10` iterations, with `0.0001` mol of molecule and `1.9999` mol of atoms, so the two problems are the same state seen from two sides. At ten times the pressure the same enthalpy gives `3001.6136` K: more recombination releases a little more heat into the fixed enthalpy and the mixture is slightly hotter.

## Decisions

- Use `tp` for a given state of a gas in a thermostat at fixed pressure, `tv` for a rigid vessel at fixed temperature, `hp` for an adiabatic constant-pressure process (a flame, a burner) and `uv` for an adiabatic constant-volume process (an explosion, a closed vessel).
- Give the element totals in moles, one entry per element in the order of `m.elements()`. A species made of elements whose total is zero is excluded automatically.
- Check `converged` before using the result, and use `Options` to raise `max_iterations`, tighten `tolerance` or change the starting temperature `initial_temperature` of the `hp` and `uv` problems.
- Batch over temperature, pressure or composition on leading axes; the lanes are solved together, and all must converge.

## Pitfalls

The solver reports what it cannot solve. The program gives it zero atoms, element totals of the wrong length, and an iteration limit of two.

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
  const auto b0 = pluss::make_series(std::vector<double>{2.0}, {1, 1}).to(pluss::ScalarType::Float64);
  // an element that the species do not contain makes no species possible; zero atoms of the only element also does
  try {
    eq::tp(m, vec({1000.0}), vec({101325.0}), pluss::make_series(std::vector<double>{0.0}, {1, 1}).to(pluss::ScalarType::Float64));
  } catch (const pluss::Error &e) {
    out.writeln("no atoms        : ", e.message());
  }
  // element totals with the wrong number of columns
  try {
    eq::tp(m, vec({1000.0}), vec({101325.0}), pluss::make_series(std::vector<double>{2.0, 1.0}, {1, 2}).to(pluss::ScalarType::Float64));
  } catch (const pluss::Error &e) {
    out.writeln("two elements    : ", e.message());
  }
  // limiting the iterations: the result is returned, flagged as not converged
  eq::Options few;
  few.max_iterations = 2;
  const auto r = eq::tp(m, vec({1000.0}), vec({101325.0}), b0, few);
  out.writeln("2 iterations : converged ", r.converged ? "yes" : "no", ", moles X2, X ", flat(r.moles));
  const auto ok = eq::tp(m, vec({1000.0}), vec({101325.0}), b0);
  out.writeln("default      : converged ", ok.converged ? "yes" : "no", ", moles X2, X ", flat(ok.moles));
}
```

Output

```text
no atoms        : natsci: equilibrium: no species can form from these elements
two elements    : natsci: equilibrium: element totals need one entry per element
2 iterations : converged no, moles X2, X [0.4011, 1.2379]
default      : converged yes, moles X2, X [0.3903, 1.2194]
```

A total of zero atoms is an error because no species can form, and a total with the wrong number of elements is an error too. An iteration limit that is too small is not an error: the call returns `converged no` with a state that is on its way, `0.4011` and `1.2379` mol where the converged answer is `0.3903` and `1.2194`, and the element total of that state is not exactly conserved. Test `converged` before reading the moles.

## Where next

[Mechanisms and rate constants](mechanisms.md) defines the thermodynamic data that this solver shares with the rate constants, and [Reactors in time](reactors.md) follows the mixture as it approaches the equilibrium found here.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mechanisms.html">
    <span class="label">Previous</span>
    <span class="title">Mechanisms and rate constants</span>
  </a>
  <a class="next-link" href="reactors.html">
    <span class="label">Next</span>
    <span class="title">Reactors in time</span>
  </a>
</div>
<!-- apiref:nav:end -->
