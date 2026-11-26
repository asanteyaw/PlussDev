# Reactors in time

[Back to Chemistry 9: Gas-Phase Kinetics and Equilibrium](../chem_09_gas_phase.md)

You want to follow a reacting gas in time: how fast a species is used up, whether the temperature changes as it does, and how long an event takes. This page integrates a closed, rigid vessel, the standard model for a shock tube and for the ignition of a gas, at constant temperature or adiabatically. It starts with the small system of the previous pages, where every number can be checked by hand, then follows oxygen in argon at `3400` K, which dissociates and, because the dissociation absorbs heat, cools while it does so. It checks the conservation laws of the integration, compares an adiabatic with a thermostatted vessel, runs several vessels at once, and shows the controls of the stiff integrator.

## How it works

The state of the vessel is a vector {math}`y = (T, C_1, \dots, C_S)` with the concentrations in mol/m³. The concentrations change by the net production rates, {math}`dC_k/dt = \omega_k`. In an adiabatic rigid vessel the internal energy is fixed, which gives the temperature equation

```{math}
\begin{aligned}
&\frac{dT}{dt} = -\frac{\sum_k u_k\,\omega_k}{\sum_k C_k\, c_{v,k}}, \\
&u_k = h_k - RT, \quad c_{v,k} = c_{p,k} - R,
\end{aligned}
```

and the pressure follows from {math}`p = RT\sum_k C_k`. In an isothermal vessel the temperature is held fixed instead. Two quantities are conserved by the adiabatic equations and serve as checks: the internal energy density {math}`\sum_k C_k u_k` and the total amount of each element.

The functions are in `pluss::ma::natsci::chemistry::reactor`. `make_state(t, c)` joins a temperature Series of shape `[...]` and a concentration Series of shape `[..., S]` into the state; `temperature(y)`, `concentrations(y)`, `pressure(y)`, `internal_energy_density(mechanism, y)` and `element_totals(mechanism, y)` read it back; `rate(mechanism, y, adiabatic)` is the right-hand side. `integrate(mechanism, y0, t_end, adiabatic, options)` returns the result at `t_end`, and `trajectory(mechanism, y0, times, adiabatic, options)` returns the states on a grid of times with shape `[len(times), ..., 1 + S]`. All lanes of a batch share one adaptive step.

The integrator is stiff and adaptive. Its options and the fields of its result are:

| Option | Default | Meaning |
|---|---|---|
| `rtol`, `atol` | `1e-6`, `1e-10` | relative and absolute error tolerances |
| `max_steps` | `200000` | step budget |
| `h_initial`, `h_max` | `0` (automatic), unbounded | first step and largest step |
| `fd_relative_step` | `1.5e-8` | relative step of the finite-difference Jacobian |

| Result field | Contents |
|---|---|
| `y`, `t` | the final state and the time reached |
| `steps`, `rejected` | accepted and rejected steps |
| `rhs_evaluations`, `jacobian_evaluations` | the work done |
| `converged` | whether `t_end` was reached |

The starting state is checked: it needs one temperature and one concentration for every species, the concentrations must be finite and not negative, and the temperature must lie in the range of the thermodynamic data.

To read a characteristic time from a trajectory, `max_slope_time(times, T_path)` returns the time of steepest temperature change, the usual definition of an ignition time; it needs a sampled path of at least three points.

## An isothermal vessel relaxing to equilibrium

In the small system, a vessel at {math}`2000` K and {math}`1` atm is filled with pure molecule. The program follows the concentrations of both species at four times from {math}`1` microsecond to {math}`1` ms, and checks the element total.

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
  const double r_gas = pluss::ma::natsci::common::constants::codata2022::molar_gas_constant.measurement.value().si_value();  // J/(mol K), read inside a function
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

## Following a shock-heated gas

Oxygen diluted to `20%` in argon at `3400` K and `1` atm, in a closed adiabatic vessel. The program converts the pressure to concentrations, builds the state, follows it over six times from `10 µs` to `0.1` s and prints the temperature and the concentrations of `O2` and `O`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
namespace reactor = chemistry::reactor;
using chemistry::thermo::SpeciesData;
namespace common = pluss::ma::natsci::common;
static double gas_constant() { return common::constants::codata2022::molar_gas_constant.measurement.value().si_value(); }  // J/(mol K)
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// one species: name, atoms, molar mass (kg/mol), NASA-7 validity range, and the low- and high-temperature coefficient sets
static SpeciesData species(const char *name, std::vector<std::pair<std::string, double>> atoms, double mass, double t_min, double t_mid, double t_max,
                           std::array<double, 7> low, std::array<double, 7> high) {
  SpeciesData s;
  s.name = name; s.composition = std::move(atoms); s.molar_mass = mass; s.t_min = t_min; s.t_mid = t_mid; s.t_max = t_max; s.low = low; s.high = high;
  return s;
}


// oxygen molecules and atoms in an argon bath
static std::vector<SpeciesData> gases() {
  return {
      species("O2", {{"O", 2}}, 31.998e-3, 200, 1000, 3500, {3.78245636, -2.99673416e-03, 9.84730201e-06, -9.68129509e-09, 3.24372837e-12, -1063.94356, 3.65767573},
              {3.28253784, 1.48308754e-03, -7.57966669e-07, 2.09470555e-10, -2.16717794e-14, -1088.45772, 5.45323129}),
      species("O", {{"O", 1}}, 15.999e-3, 200, 1000, 3500, {3.1682671, -3.27931884e-03, 6.64306396e-06, -6.12806624e-09, 2.11265971e-12, 2.91222592e+04, 2.05193346},
              {2.56942078, -8.59741137e-05, 4.19484589e-08, -1.00177799e-11, 1.22833691e-15, 2.92175791e+04, 4.78433864}),
      species("AR", {{"Ar", 1}}, 39.95e-3, 300, 1000, 5000, {2.5, 0, 0, 0, 0, -745.375, 4.366}, {2.5, 0, 0, 0, 0, -745.375, 4.366})};
}

static kinetics::Mechanism oxygen() {
  kinetics::ReactionSpec r;
  r.reactants = {{"O", 2}};
  r.products = {{"O2", 1}};
  r.type = kinetics::Type::ThreeBody;
  r.a = 1.2e17;
  r.b = -1.0;
  r.efficiencies = {{"AR", 0.83}};
  return kinetics::Mechanism(gases(), {r}, vec({300.0}), kinetics::Units::cgs_cal());
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));
  const auto mech = oxygen();
  // shock-heated oxygen in argon: 3400 K and 1 atm, 20% O2. Concentrations from p / (R T)
  const double total = 101325.0 / (gas_constant() * 3400.0);
  const auto y0 = reactor::make_state(vec({3400.0}), vec({0.2 * total, 0.0, 0.8 * total}).reshape({1, 3}));
  out.writeln("initial total concentration (mol/m^3) ", total);
  out.writeln("initial pressure (Pa)                 ", flat(reactor::pressure(y0)));
  // follow the closed, adiabatic vessel over a range of times
  const std::vector<double> times = {0.0, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1};
  const auto path = reactor::trajectory(mech, y0, times);  // [times, 1, 1 + 3]
  out.writeln("time (s)        ", times);
  out.writeln("temperature (K) ", flat(reactor::temperature(path)));
  out.writeln("[O2] (mol/m^3)  ", flat(reactor::concentrations(path).select(-1, 0)));
  out.writeln("[O]  (mol/m^3)  ", flat(reactor::concentrations(path).select(-1, 1)));
}
```

Output

```text
initial total concentration (mol/m^3) 3.5843e+00
initial pressure (Pa)                 [1.0132e+05]
time (s)        [0.0000e+00, 1.0000e-05, 1.0000e-04, 1.0000e-03, 1.0000e-02, 1.0000e-01]
temperature (K) [3.4000e+03, 3.3967e+03, 3.3691e+03, 3.2052e+03, 2.9210e+03, 2.8830e+03]
[O2] (mol/m^3)  [7.1686e-01, 7.1646e-01, 7.1310e-01, 6.9331e-01, 6.5935e-01, 6.5486e-01]
[O]  (mol/m^3)  [0.0000e+00, 8.0442e-04, 7.5079e-03, 4.7092e-02, 1.1501e-01, 1.2400e-01]
```

The total concentration is `3.584` mol/m³ and the starting pressure `1.0132e5` Pa. Atoms appear at once (`8.0e-4` mol/m³ after `10 µs`), the temperature falls from `3400` to `3205` K after a millisecond and levels off at `2883` K, and the concentrations stop changing: `0.124` mol/m³ of atoms has formed and `0.655` mol/m³ of `O2` is left.

## Conservation and equilibrium

The same run, now checking the conserved quantities at five times and the balance at the end: the internal energy density, the atoms of each element, the pressure, and whether `[O2] = K_c [O]²` holds for the final temperature.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
namespace reactor = chemistry::reactor;
using chemistry::thermo::SpeciesData;
namespace common = pluss::ma::natsci::common;
static double gas_constant() { return common::constants::codata2022::molar_gas_constant.measurement.value().si_value(); }  // J/(mol K)
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// one species: name, atoms, molar mass (kg/mol), NASA-7 validity range, and the low- and high-temperature coefficient sets
static SpeciesData species(const char *name, std::vector<std::pair<std::string, double>> atoms, double mass, double t_min, double t_mid, double t_max,
                           std::array<double, 7> low, std::array<double, 7> high) {
  SpeciesData s;
  s.name = name; s.composition = std::move(atoms); s.molar_mass = mass; s.t_min = t_min; s.t_mid = t_mid; s.t_max = t_max; s.low = low; s.high = high;
  return s;
}


// oxygen molecules and atoms in an argon bath
static std::vector<SpeciesData> gases() {
  return {
      species("O2", {{"O", 2}}, 31.998e-3, 200, 1000, 3500, {3.78245636, -2.99673416e-03, 9.84730201e-06, -9.68129509e-09, 3.24372837e-12, -1063.94356, 3.65767573},
              {3.28253784, 1.48308754e-03, -7.57966669e-07, 2.09470555e-10, -2.16717794e-14, -1088.45772, 5.45323129}),
      species("O", {{"O", 1}}, 15.999e-3, 200, 1000, 3500, {3.1682671, -3.27931884e-03, 6.64306396e-06, -6.12806624e-09, 2.11265971e-12, 2.91222592e+04, 2.05193346},
              {2.56942078, -8.59741137e-05, 4.19484589e-08, -1.00177799e-11, 1.22833691e-15, 2.92175791e+04, 4.78433864}),
      species("AR", {{"Ar", 1}}, 39.95e-3, 300, 1000, 5000, {2.5, 0, 0, 0, 0, -745.375, 4.366}, {2.5, 0, 0, 0, 0, -745.375, 4.366})};
}

static kinetics::Mechanism oxygen() {
  kinetics::ReactionSpec r;
  r.reactants = {{"O", 2}};
  r.products = {{"O2", 1}};
  r.type = kinetics::Type::ThreeBody;
  r.a = 1.2e17;
  r.b = -1.0;
  r.efficiencies = {{"AR", 0.83}};
  return kinetics::Mechanism(gases(), {r}, vec({300.0}), kinetics::Units::cgs_cal());
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(5).with_sep(""));
  const auto mech = oxygen();
  const double total = 101325.0 / (gas_constant() * 3400.0);
  const auto y0 = reactor::make_state(vec({3400.0}), vec({0.2 * total, 0.0, 0.8 * total}).reshape({1, 3}));
  const std::vector<double> times = {0.0, 1e-4, 1e-3, 1e-2, 1.0};
  const auto path = reactor::trajectory(mech, y0, times);
  // an adiabatic, rigid vessel conserves the internal energy density and the number of atoms of each element, while the pressure follows the temperature and the mole count
  for (std::size_t i = 0; i < times.size(); ++i) {
    const auto state = path.select(0, static_cast<std::int64_t>(i));
    out.writeln("t = ", times[i], "  energy density ", flat(reactor::internal_energy_density(mech, state))[0], "  atoms O, Ar ", flat(reactor::element_totals(mech, state)), "  p ", flat(reactor::pressure(state))[0]);
  }
  // at the end of the run the reaction has relaxed to equilibrium: [O2] = K_c [O]^2 at the final temperature
  const auto last = path.select(0, static_cast<std::int64_t>(times.size() - 1));
  const double t_end = flat(reactor::temperature(last))[0];
  const auto c = reactor::concentrations(last);
  const double kc = flat(mech.equilibrium_constants(vec({t_end})))[0];
  out.writeln("final temperature   ", t_end);
  out.writeln("[O2]                ", flat(c.select(-1, 0))[0]);
  out.writeln("K_c [O]^2           ", kc * std::pow(flat(c.select(-1, 1))[0], 2.0));
}
```

Output

```text
t = 0.00000e+00  energy density 1.65465e+05  atoms O, Ar [1.43372e+00, 2.86743e+00]  p 1.01325e+05
t = 1.00000e-04  energy density 1.65465e+05  atoms O, Ar [1.43372e+00, 2.86743e+00]  p 1.00508e+05
t = 1.00000e-03  energy density 1.65465e+05  atoms O, Ar [1.43372e+00, 2.86743e+00]  p 9.61485e+04
t = 1.00000e-02  energy density 1.65465e+05  atoms O, Ar [1.43372e+00, 2.86743e+00]  p 8.84454e+04
t = 1.00000e+00  energy density 1.65465e+05  atoms O, Ar [1.43372e+00, 2.86743e+00]  p 8.74048e+04
final temperature   2.88303e+03
[O2]                6.54858e-01
K_c [O]^2           6.54858e-01
```

The internal energy density is `1.65465e5` J/m³ at every time and the atoms are `1.43372` (oxygen) and `2.86743` (argon) mol/m³ at every time, to the six digits shown. The pressure falls from `1.01325e5` to `8.74e4` Pa, since the gas cools by more than it gains in moles. At the end `[O2] = 0.654858` and `K_c [O]² = 0.654858`: the vessel has relaxed to exactly the equilibrium the thermodynamics predicts.

## Adiabatic against thermostatted

The same gas allowed to relax for one second in an adiabatic vessel, and in an isothermal one in which the temperature is held at `3400` K.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
namespace reactor = chemistry::reactor;
using chemistry::thermo::SpeciesData;
namespace common = pluss::ma::natsci::common;
static double gas_constant() { return common::constants::codata2022::molar_gas_constant.measurement.value().si_value(); }  // J/(mol K)
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// one species: name, atoms, molar mass (kg/mol), NASA-7 validity range, and the low- and high-temperature coefficient sets
static SpeciesData species(const char *name, std::vector<std::pair<std::string, double>> atoms, double mass, double t_min, double t_mid, double t_max,
                           std::array<double, 7> low, std::array<double, 7> high) {
  SpeciesData s;
  s.name = name; s.composition = std::move(atoms); s.molar_mass = mass; s.t_min = t_min; s.t_mid = t_mid; s.t_max = t_max; s.low = low; s.high = high;
  return s;
}


// oxygen molecules and atoms in an argon bath
static std::vector<SpeciesData> gases() {
  return {
      species("O2", {{"O", 2}}, 31.998e-3, 200, 1000, 3500, {3.78245636, -2.99673416e-03, 9.84730201e-06, -9.68129509e-09, 3.24372837e-12, -1063.94356, 3.65767573},
              {3.28253784, 1.48308754e-03, -7.57966669e-07, 2.09470555e-10, -2.16717794e-14, -1088.45772, 5.45323129}),
      species("O", {{"O", 1}}, 15.999e-3, 200, 1000, 3500, {3.1682671, -3.27931884e-03, 6.64306396e-06, -6.12806624e-09, 2.11265971e-12, 2.91222592e+04, 2.05193346},
              {2.56942078, -8.59741137e-05, 4.19484589e-08, -1.00177799e-11, 1.22833691e-15, 2.92175791e+04, 4.78433864}),
      species("AR", {{"Ar", 1}}, 39.95e-3, 300, 1000, 5000, {2.5, 0, 0, 0, 0, -745.375, 4.366}, {2.5, 0, 0, 0, 0, -745.375, 4.366})};
}

static kinetics::Mechanism oxygen() {
  kinetics::ReactionSpec r;
  r.reactants = {{"O", 2}};
  r.products = {{"O2", 1}};
  r.type = kinetics::Type::ThreeBody;
  r.a = 1.2e17;
  r.b = -1.0;
  r.efficiencies = {{"AR", 0.83}};
  return kinetics::Mechanism(gases(), {r}, vec({300.0}), kinetics::Units::cgs_cal());
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto mech = oxygen();
  const double total = 101325.0 / (gas_constant() * 3400.0);
  const auto y0 = reactor::make_state(vec({3400.0}), vec({0.2 * total, 0.0, 0.8 * total}).reshape({1, 3}));
  // adiabatic: the dissociation absorbs heat and cools the gas. Isothermal: a thermostat holds 3400 K and more oxygen dissociates
  const auto adiabatic = reactor::integrate(mech, y0, 1.0, true);
  const auto isothermal = reactor::integrate(mech, y0, 1.0, false);
  const auto show = [&](const char *label, const auto &result) {
    const auto y = result.y;
    out.writeln(label, " T = ", flat(reactor::temperature(y))[0], " K   [O] = ", flat(reactor::concentrations(y).select(-1, 1))[0], " mol/m^3   p = ", flat(reactor::pressure(y))[0], " Pa");
  };
  show("adiabatic ", adiabatic);
  show("isothermal", isothermal);
}
```

Output

```text
adiabatic  T = 2883.0329 K   [O] = 0.1240 mol/m^3   p = 87404.8218 Pa
isothermal T = 3400.0000 K   [O] = 0.4933 mol/m^3   p = 108297.5483 Pa
```

The adiabatic vessel ends at `2883.0` K with `0.1240` mol/m³ of atoms and a pressure of `87405` Pa. The isothermal vessel stays at `3400` K, dissociates `4` times as much (`0.4933` mol/m³) and ends at a higher pressure, `108298` Pa, because it has more molecules and keeps its temperature.

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
  const double r_gas = pluss::ma::natsci::common::constants::codata2022::molar_gas_constant.measurement.value().si_value();  // J/(mol K), read inside a function
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

## Controlling the integrator

The same adiabatic run to `10 ms` with default tolerances, with very tight and with loose ones, and with a budget of only `5` steps. The program prints the counters of the result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
namespace reactor = chemistry::reactor;
using chemistry::thermo::SpeciesData;
namespace common = pluss::ma::natsci::common;
static double gas_constant() { return common::constants::codata2022::molar_gas_constant.measurement.value().si_value(); }  // J/(mol K)
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// one species: name, atoms, molar mass (kg/mol), NASA-7 validity range, and the low- and high-temperature coefficient sets
static SpeciesData species(const char *name, std::vector<std::pair<std::string, double>> atoms, double mass, double t_min, double t_mid, double t_max,
                           std::array<double, 7> low, std::array<double, 7> high) {
  SpeciesData s;
  s.name = name; s.composition = std::move(atoms); s.molar_mass = mass; s.t_min = t_min; s.t_mid = t_mid; s.t_max = t_max; s.low = low; s.high = high;
  return s;
}


// oxygen molecules and atoms in an argon bath
static std::vector<SpeciesData> gases() {
  return {
      species("O2", {{"O", 2}}, 31.998e-3, 200, 1000, 3500, {3.78245636, -2.99673416e-03, 9.84730201e-06, -9.68129509e-09, 3.24372837e-12, -1063.94356, 3.65767573},
              {3.28253784, 1.48308754e-03, -7.57966669e-07, 2.09470555e-10, -2.16717794e-14, -1088.45772, 5.45323129}),
      species("O", {{"O", 1}}, 15.999e-3, 200, 1000, 3500, {3.1682671, -3.27931884e-03, 6.64306396e-06, -6.12806624e-09, 2.11265971e-12, 2.91222592e+04, 2.05193346},
              {2.56942078, -8.59741137e-05, 4.19484589e-08, -1.00177799e-11, 1.22833691e-15, 2.92175791e+04, 4.78433864}),
      species("AR", {{"Ar", 1}}, 39.95e-3, 300, 1000, 5000, {2.5, 0, 0, 0, 0, -745.375, 4.366}, {2.5, 0, 0, 0, 0, -745.375, 4.366})};
}

static kinetics::Mechanism oxygen() {
  kinetics::ReactionSpec r;
  r.reactants = {{"O", 2}};
  r.products = {{"O2", 1}};
  r.type = kinetics::Type::ThreeBody;
  r.a = 1.2e17;
  r.b = -1.0;
  r.efficiencies = {{"AR", 0.83}};
  return kinetics::Mechanism(gases(), {r}, vec({300.0}), kinetics::Units::cgs_cal());
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(5).with_sep(""));
  const auto mech = oxygen();
  const double total = 101325.0 / (gas_constant() * 3400.0);
  const auto y0 = reactor::make_state(vec({3400.0}), vec({0.2 * total, 0.0, 0.8 * total}).reshape({1, 3}));
  const auto show = [&](const char *label, const reactor::stiff::Result &r) {
    out.writeln(label, " converged = ", r.converged ? "yes" : "no", "  steps = ", r.steps, "  rejected = ", r.rejected, "  rhs evaluations = ", r.rhs_evaluations, "  Jacobians = ", r.jacobian_evaluations,
                "  [O] = ", flat(reactor::concentrations(r.y).select(-1, 1))[0]);
  };
  reactor::stiff::Options tight;
  tight.rtol = 1e-9;
  tight.atol = 1e-14;
  reactor::stiff::Options loose;
  loose.rtol = 1e-3;
  reactor::stiff::Options starved;
  starved.max_steps = 5;
  show("defaults      ", reactor::integrate(mech, y0, 1e-2));
  show("tight (1e-9)  ", reactor::integrate(mech, y0, 1e-2, true, tight));
  show("loose (1e-3)  ", reactor::integrate(mech, y0, 1e-2, true, loose));
  show("5 steps only  ", reactor::integrate(mech, y0, 1e-2, true, starved));
}
```

Output

```text
defaults       converged = yes  steps = 99  rejected = 0  rhs evaluations = 1089  Jacobians = 99  [O] = 1.15014e-01
tight (1e-9)   converged = yes  steps = 933  rejected = 0  rhs evaluations = 10263  Jacobians = 933  [O] = 1.15013e-01
loose (1e-3)   converged = yes  steps = 17  rejected = 0  rhs evaluations = 187  Jacobians = 17  [O] = 1.15084e-01
5 steps only   converged = no  steps = 5  rejected = 0  rhs evaluations = 64  Jacobians = 6  [O] = 1.68365e-03
```

The default run takes `99` steps and no step is rejected; tight tolerances take `933` steps and loose ones `17`, and all three agree on the number of atoms to `4` digits or better (`0.115014`, `0.115013` and `0.115084`). With `5` steps allowed the integrator stops early, reports `converged = no`, and its state (`1.68e-3` mol/m³ of atoms) is far from the answer: always check `converged`.

## Decisions

- Use `adiabatic = true` for a gas that has no heat exchange, as in a shock tube or an ignition test, and `adiabatic = false` to impose the temperature, for example to get the composition at a fixed temperature. The choice decides whether the temperature is an unknown.
- Use `integrate` when only the final state matters and `trajectory` when you want a history. Give `trajectory` enough times to resolve what you are looking for, and choose them on a logarithmic grid when the time scales span several decades, which is the usual case for a stiff system.
- Run several starting states as the lanes of one batch. They share the step size, so the stiffest lane sets the cost of all of them: group states with similar time scales.
- Tighten `rtol` first if two runs differ in a quantity you care about, and judge the integration by `converged`, the step counters and the conservation of the energy density and the element totals, not by the wall clock.
- Work in SI units: concentrations in mol/m³, times in seconds, temperatures in kelvin.

## Pitfalls

The starting state is checked: its width has to match the number of species, the concentrations cannot be negative, and the temperature has to be inside the thermodynamic data. A concentration in the wrong unit is legal and is not caught.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <functional>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
namespace reactor = chemistry::reactor;
using chemistry::thermo::SpeciesData;
namespace common = pluss::ma::natsci::common;
static double gas_constant() { return common::constants::codata2022::molar_gas_constant.measurement.value().si_value(); }  // J/(mol K)
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// one species: name, atoms, molar mass (kg/mol), NASA-7 validity range, and the low- and high-temperature coefficient sets
static SpeciesData species(const char *name, std::vector<std::pair<std::string, double>> atoms, double mass, double t_min, double t_mid, double t_max,
                           std::array<double, 7> low, std::array<double, 7> high) {
  SpeciesData s;
  s.name = name; s.composition = std::move(atoms); s.molar_mass = mass; s.t_min = t_min; s.t_mid = t_mid; s.t_max = t_max; s.low = low; s.high = high;
  return s;
}


// oxygen molecules and atoms in an argon bath
static std::vector<SpeciesData> gases() {
  return {
      species("O2", {{"O", 2}}, 31.998e-3, 200, 1000, 3500, {3.78245636, -2.99673416e-03, 9.84730201e-06, -9.68129509e-09, 3.24372837e-12, -1063.94356, 3.65767573},
              {3.28253784, 1.48308754e-03, -7.57966669e-07, 2.09470555e-10, -2.16717794e-14, -1088.45772, 5.45323129}),
      species("O", {{"O", 1}}, 15.999e-3, 200, 1000, 3500, {3.1682671, -3.27931884e-03, 6.64306396e-06, -6.12806624e-09, 2.11265971e-12, 2.91222592e+04, 2.05193346},
              {2.56942078, -8.59741137e-05, 4.19484589e-08, -1.00177799e-11, 1.22833691e-15, 2.92175791e+04, 4.78433864}),
      species("AR", {{"Ar", 1}}, 39.95e-3, 300, 1000, 5000, {2.5, 0, 0, 0, 0, -745.375, 4.366}, {2.5, 0, 0, 0, 0, -745.375, 4.366})};
}

static kinetics::Mechanism oxygen() {
  kinetics::ReactionSpec r;
  r.reactants = {{"O", 2}};
  r.products = {{"O2", 1}};
  r.type = kinetics::Type::ThreeBody;
  r.a = 1.2e17;
  r.b = -1.0;
  r.efficiencies = {{"AR", 0.83}};
  return kinetics::Mechanism(gases(), {r}, vec({300.0}), kinetics::Units::cgs_cal());
}

static void attempt(OutputEngine &out, const char *label, const std::function<void()> &call) {
  try { call(); out.writeln(label, "  no error"); } catch (const pluss::Error &e) { out.writeln(label, "  ", e.message()); }
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));
  const auto mech = oxygen();
  const double total = 101325.0 / (gas_constant() * 3400.0);
  // two concentrations for a three-species mechanism: the state has the wrong width
  attempt(out, "two concentrations:     ", [&] {
    (void)reactor::integrate(mech, reactor::make_state(vec({3400.0}), vec({0.2 * total, 0.8 * total}).reshape({1, 2})), 1e-3);
  });
  // a temperature outside the thermodynamic data
  attempt(out, "starting at 4000 K:     ", [&] {
    (void)reactor::integrate(mech, reactor::make_state(vec({4000.0}), vec({0.2 * total, 0.0, 0.8 * total}).reshape({1, 3})), 1e-3);
  });
  // the same gas written per cubic centimetre instead of per cubic metre looks almost inert: the rates scale with the concentrations
  const auto per_cm3 = reactor::make_state(vec({3400.0}), vec({0.2 * total * 1e-6, 0.0, 0.8 * total * 1e-6}).reshape({1, 3}));
  const auto stalled = reactor::integrate(mech, per_cm3, 1e-3);
  out.writeln("per-cm^3 concentrations: pressure ", flat(reactor::pressure(per_cm3))[0], " Pa, [O] after 1 ms = ", flat(reactor::concentrations(stalled.y).select(-1, 1))[0]);
  // an ignition-delay search needs a sampled temperature path of at least three points
  attempt(out, "two sampled points:     ", [&] { (void)reactor::max_slope_time({0.0, 1.0}, vec({1000.0, 1100.0})); });
}
```

Output

```text
two concentrations:       natsci: reactor: the state needs one temperature and 3 concentrations, got 2
starting at 4000 K:       natsci::thermo: temperature must be inside the common NASA validity range [300.000000, 3500.000000] K
per-cm^3 concentrations: pressure 1.0132e-01 Pa, [O] after 1 ms = 8.1098e-14
two sampled points:       natsci: reactor: need a sampled temperature path
```

Two concentrations for three species and a temperature of `4000` K are refused with a message that names the width or the range; a sampled path of two points cannot give an ignition delay. The same gas written per cubic centimetre instead of per cubic metre has a pressure of `0.10` Pa and, after a millisecond, only `8e-14` mol/m³ of atoms: it looks inert because the rates scale with the concentrations.

The integrator can also run out of steps, and the time found from a coarse trajectory is only as good as the grid. The program restricts the step budget to five, and then locates the time of steepest cooling on a coarse and on a fine grid.

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

[Hydrogen ignition](hydrogen_ignition.md) uses this reactor with a chain-branching mechanism to find an ignition delay. [Mixture properties](mixture_properties.md) shows how the same mechanism gives the density, the heat capacity and the sound speed of the gas in the vessel, [Mechanisms and rate constants](mechanisms.md) defines the rates that these integrations solve, and [Equilibrium composition](equilibrium_composition.md) gives the state they tend to. [Chemistry 4: Reaction Rates](../chem_04_rates.md) explains where the rate constants come from.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="equilibrium_composition.html">
    <span class="label">Previous</span>
    <span class="title">Equilibrium composition</span>
  </a>
  <a class="next-link" href="hydrogen_ignition.html">
    <span class="label">Next</span>
    <span class="title">Hydrogen ignition</span>
  </a>
</div>
<!-- apiref:nav:end -->
