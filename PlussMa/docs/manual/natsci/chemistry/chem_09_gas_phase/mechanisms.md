# Mechanisms and rate constants

[Back to Chemistry 9: Gas-Phase Kinetics and Equilibrium](../chem_09_gas_phase.md)

You have a set of gas-phase reactions with Arrhenius parameters and thermodynamic data for the species, and you want how fast each reaction runs, how fast each species is made or lost, and where the reactions balance, for any temperature and composition. This page builds a mechanism from species and reactions, first for a small system you can verify by hand and then for oxygen atoms recombining in an argon bath with real thermodynamic data, evaluates equilibrium constants, rates of progress and net production over a grid of states, and covers third-body, pressure-dependent and irreversible reactions.

## How it works

Each reaction has a forward rate constant {math}`k_f = A\,T^{b}\,e^{-E_a/RT}`. In a third-body reaction such as {math}`\mathrm{O}+\mathrm{O}+M \rightleftharpoons \mathrm{O_2}+M` the partner `M` can be any molecule, and the rate is multiplied by the third-body concentration {math}`[M] = \sum_k \epsilon_k [X_k]`, where the efficiency {math}`\epsilon_k` says how well species `k` acts as a partner (`1` by default). A falloff reaction interpolates between a low-pressure limit {math}`k_0[M]` and a high-pressure limit {math}`k_\infty`:

```{math}
\begin{aligned}
&k = k_\infty \frac{P_r}{1+P_r}\,F, \\
&P_r = \frac{k_0 [M]}{k_\infty},
\end{aligned}
```

with `F = 1` for the Lindemann form and the Troe broadening for the other,

```{math}
\begin{aligned}
&\log_{10}F = \frac{\log_{10}F_{cent}}{1 + f_1^2}, \\
&F_{cent} = (1-a)e^{-T/T_3} + a\,e^{-T/T_1} + e^{-T_2/T}.
\end{aligned}
```

The reverse rate constant comes from the thermodynamics: {math}`k_r = k_f/K_c` with {math}`K_c = \exp(-\Delta G^\circ/RT)\,(p^\circ/RT)^{\Delta\nu}`, computed from the NASA-7 polynomials of the species. The rates of progress are {math}`q_f = k_f\prod_k c_k^{\nu_k'}` and {math}`q_r = k_r\prod_k c_k^{\nu_k''}`, and the net production rate of species `k` is {math}`\omega_k = \sum_r \nu_{kr}(q_{f,r} - q_{r,r})`.

You describe the mechanism with `pluss::ma::natsci::chemistry::thermo::SpeciesData` records (name, atoms, molar mass, the temperature range and the two sets of seven coefficients) and `pluss::ma::natsci::chemistry::kinetics::ReactionSpec` records. A reaction is given by:

| Field | Meaning |
|---|---|
| `reactants`, `products` | maps from species name to stoichiometric coefficient |
| `a`, `b`, `ea` | pre-exponential factor, temperature exponent, activation energy (high-pressure limit for a falloff reaction) |
| `reversible` | `true` by default; `false` removes the reverse reaction |
| `type` | `kinetics::Type::Elementary`, `ThreeBody` or `Falloff` |
| `efficiencies`, `default_efficiency` | third-body efficiencies by species name, and the value for those not listed |
| `a_low`, `b_low`, `ea_low` | low-pressure limit of a falloff reaction |
| `troe`, `troe_params` | `true` to use the Troe form, and its four numbers `a`, `T3`, `T1`, `T2` (a `T2` that is not positive leaves the third term out) |

`kinetics::Mechanism(species, reactions, like, units)` assembles them; `like` is any Series whose dtype and device the mechanism should use, and `units` says in which units `a` and `ea` are written: `kinetics::Units::si()` or `kinetics::Units::cgs_cal()` for centimetres, moles, seconds and calories per mole, the units mechanisms are usually published in. Inside, everything is SI. The methods take a temperature Series of shape `[...]` and a concentration Series of shape `[..., S]` in mol/m³, so a batch of states runs in one call: `forward_rate_constants(t, c)`, `equilibrium_constants(t)`, `third_body(c)`, `rates_of_progress(t, c)` (fields `forward` and `reverse`), and `net_production_rates(t, c)`. The accessors `species()`, `reactions()`, `names()`, `elements()`, `index(name)`, `stoichiometry()`, `composition()`, `molar_masses()` and `has_molar_masses()` read the mechanism back.

## A small system you can check by hand

A small system that you can check by hand is used here and on the next two pages: a diatomic {math}`\mathrm{X_2}` that dissociates into two atoms {math}`\mathrm{X}`, {math}`\mathrm{X_2} \rightleftharpoons 2\mathrm{X}`. Its thermodynamic data have constant heat capacities, {math}`c_p/R = 3.5` for {math}`\mathrm{X_2}` and {math}`2.5` for {math}`\mathrm{X}`, and the atom is {math}`\Delta H/R = 12000` K (about {math}`100` kJ/mol) above the molecule. The program builds the mechanism, prints its size and stoichiometry, evaluates {math}`K_c` at four temperatures, and then evaluates the forward and reverse rates of progress and the net production rates at {math}`2000` K and {math}`1` atm for a mixture that is {math}`90\%` molecule by mole. The forward rate constant is {math}`k = 10^{8}e^{-E_a/RT}` s{math}`^{-1}` with {math}`E_a = 100` kJ/mol.

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
  out.writeln("species ", m.names().size(), ", reactions ", m.reactions(), ", elements ", m.elements().size());
  out.writeln("stoichiometry (reaction x species)  ", flat(m.stoichiometry()));
  // equilibrium constants (in mol/m^3) over temperature, and the forward and reverse rates at a given state
  const auto t = vec({1000.0, 1500.0, 2000.0, 3000.0});
  out.writeln("temperature (K)          ", flat(t));
  out.writeln("log10 Kc (mol/m^3)       ", flat(m.equilibrium_constants(t).log10()));
  // concentrations at 2000 K and 1 atm: 90 % X2 by mole
  const double c_total = 101325.0 / (pluss::ma::natsci::common::constants::codata2022::molar_gas_constant.measurement.value().si_value() * 2000.0);
  const auto c = pluss::make_series(std::vector<double>{0.9 * c_total, 0.1 * c_total}, {1, 2}).to(pluss::ScalarType::Float64);
  const auto q = m.rates_of_progress(vec({2000.0}), c);
  out.writeln("forward / reverse rate of progress (mol/m^3/s) ", flat(q.forward), " / ", flat(q.reverse));
  out.writeln("net production of X2, X (mol/m^3/s)            ", flat(m.net_production_rates(vec({2000.0}), c)));
}
```

Output

```text
species 2, reactions 1, elements 1
stoichiometry (reaction x species)  [-1.0000, 2.0000]
temperature (K)          [1000.0000, 1500.0000, 2000.0000, 3000.0000]
log10 Kc (mol/m^3)       [1.4601, 3.2853, 4.2164, 5.1730]
forward / reverse rate of progress (mol/m^3/s) [1340954.1698] / [5.5164]
net production of X2, X (mol/m^3/s)            [-1340948.6534, 2681897.3068]
```

The model has `2` species, `1` reaction and `1` element, and the stoichiometry row is {math}`(-1, +2)`: one molecule lost, two atoms made. The equilibrium constant grows from {math}`10^{1.46}` to {math}`10^{5.17}` mol/m{math}`^3` between {math}`1000` and {math}`3000` K, as it must for a dissociation that absorbs heat. At the chosen state the forward rate of progress is `1340954.1698` mol/(m{math}`^3` s) and the reverse rate only `5.5164`, because there is little atom to recombine. The net production is {math}`-q` for the molecule, `-1340948.6534`, and {math}`+2q` for the atom, `2681897.3068`: the atom is made twice as fast as the molecule is lost.

## A grid of states in one call

The temperature has shape {math}`[4, 3]` and the concentrations {math}`[4, 3, 2]`: four temperatures against three compositions. The net production of atoms is evaluated on the whole grid, with the equilibrium constant at {math}`1500` K for reference.

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
  // net production of X2 and X on a grid of 4 temperatures and 3 compositions at once: shape [4, 3, 2]
  const auto t = pluss::make_series(std::vector<double>{1000, 1000, 1000, 1500, 1500, 1500, 2000, 2000, 2000, 2500, 2500, 2500}, {4, 3}).to(pluss::ScalarType::Float64);
  std::vector<double> cs;
  for (int i = 0; i < 4; ++i)
    for (double x : {0.9, 0.5, 0.1}) { cs.push_back(x); cs.push_back(1.0 - x); }
  const auto c = pluss::make_series(cs, {4, 3, 2}).to(pluss::ScalarType::Float64);
  const auto w = m.net_production_rates(t, c);
  out.writeln("shape of the result ", w.sizes()[0], " x ", w.sizes()[1], " x ", w.sizes()[2]);
  out.writeln("net production of X (mol/m^3/s), rows 1000..2500 K, columns x(X2) = 0.9, 0.5, 0.1");
  const auto wx = w.select(-1, 1);
  for (int i = 0; i < 4; ++i) out.writeln(flat(wx.select(0, i)));
  // the rate vanishes where the composition is at equilibrium: find the ratio c_X^2 / c_X2 that equals Kc at 1500 K
  out.writeln("Kc at 1500 K (mol/m^3) ", flat(m.equilibrium_constants(vec({1500.0}))));
}
```

Output

```text
shape of the result 4 x 3 x 2
net production of X (mol/m^3/s), rows 1000..2500 K, columns x(X2) = 0.9, 0.5, 0.1
[1075.8288, 587.5491, 86.0036]
[59296.4457, 32934.1203, 6560.8647]
[440140.3717, 244515.1649, 48880.4492]
[1465303.6158, 814051.0497, 162789.9553]
Kc at 1500 K (mol/m^3) [1928.8942]
```

The result has shape {math}`4\times3\times2`, one production rate for every species at every state. The rate of making atoms rises by a factor of about `1400` from `1075.8288` at {math}`1000` K to `1465303.6158` at {math}`2500` K for the same composition, and falls as the molecule is used up, from `59296.4457` at {math}`x_{\mathrm{X_2}} = 0.9` to `6560.8647` at {math}`0.1` at {math}`1500` K. All values are positive: these states are far below the equilibrium ratio {math}`c_{\mathrm X}^2/c_{\mathrm{X_2}} = K_c = 1928.8942` mol/m{math}`^3`, so the net reaction is always dissociation.

## Rates of progress and net production

Real thermodynamic data come next. The program defines three species, `O2`, `O` and `Ar`, with their seven-coefficient polynomials, and one reaction, the recombination {math}`\mathrm{O}+\mathrm{O}+M\to\mathrm{O_2}+M` with argon as a weaker third body than the others. At `3000` K with `[O2] = 4`, `[O] = 8` and `[Ar] = 20` mol/m³, the program prints the two rates of progress, the net production of each species, the change of each element, the same production computed from thermodynamic data evaluated once, and the state at which the reaction balances.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
using chemistry::thermo::SpeciesData;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// one species: name, atoms, molar mass (kg/mol), NASA-7 validity range, and the low- and high-temperature coefficient sets
static SpeciesData species(const char *name, std::vector<std::pair<std::string, double>> atoms, double mass, double t_min, double t_mid, double t_max,
                           std::array<double, 7> low, std::array<double, 7> high) {
  SpeciesData s;
  s.name = name; s.composition = std::move(atoms); s.molar_mass = mass; s.t_min = t_min; s.t_mid = t_mid; s.t_max = t_max; s.low = low; s.high = high;
  return s;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));
  // oxygen molecules and atoms in an argon bath
  const std::vector<SpeciesData> gases = {
      species("O2", {{"O", 2}}, 31.998e-3, 200, 1000, 3500, {3.78245636, -2.99673416e-03, 9.84730201e-06, -9.68129509e-09, 3.24372837e-12, -1063.94356, 3.65767573},
              {3.28253784, 1.48308754e-03, -7.57966669e-07, 2.09470555e-10, -2.16717794e-14, -1088.45772, 5.45323129}),
      species("O", {{"O", 1}}, 15.999e-3, 200, 1000, 3500, {3.1682671, -3.27931884e-03, 6.64306396e-06, -6.12806624e-09, 2.11265971e-12, 2.91222592e+04, 2.05193346},
              {2.56942078, -8.59741137e-05, 4.19484589e-08, -1.00177799e-11, 1.22833691e-15, 2.92175791e+04, 4.78433864}),
      species("AR", {{"Ar", 1}}, 39.95e-3, 300, 1000, 5000, {2.5, 0, 0, 0, 0, -745.375, 4.366}, {2.5, 0, 0, 0, 0, -745.375, 4.366})};
  // O + O + M -> O2 + M with A = 1.2e17 cm^6 mol^-2 s^-1, T^-1 and no barrier; argon is a weaker third body than the others
  kinetics::ReactionSpec recombination;
  recombination.reactants = {{"O", 2}};
  recombination.products = {{"O2", 1}};
  recombination.type = kinetics::Type::ThreeBody;
  recombination.a = 1.2e17;
  recombination.b = -1.0;
  recombination.ea = 0.0;
  recombination.efficiencies = {{"AR", 0.83}};
  // the rate constants are written in the usual centimetre-mole-second-calorie units; the engine works in SI internally
  const kinetics::Mechanism mech(gases, {recombination}, vec({300.0}), kinetics::Units::cgs_cal());
  // the state: 3000 K and the concentrations of O2, O and Ar in mol/m^3
  const auto t = vec({3000.0});
  const auto c = vec({4.0, 8.0, 20.0}).reshape({1, 3});
  const auto q = mech.rates_of_progress(t, c);
  out.writeln("forward progress  (mol/m^3/s) ", flat(q.forward));
  out.writeln("reverse progress  (mol/m^3/s) ", flat(q.reverse));
  const auto omega = mech.net_production_rates(t, c);
  out.writeln("net production O2, O, Ar      ", flat(omega));
  // atoms are conserved: the production rate times the composition matrix gives the rate of change of each element
  out.writeln("element rates (O, Ar)         ", flat(omega.matmul(mech.composition())));
  // evaluating the thermodynamic polynomials once and passing them in gives the same rates: this is what a reactor does at every step
  const auto props = mech.thermo().eval(t);
  out.writeln("with shared thermo            ", flat(mech.net_production_rates(t, c, props)));
  // choose [O2] = K_c [O]^2 and the two directions cancel exactly
  const double kc = flat(mech.equilibrium_constants(t))[0];
  const auto balanced = vec({kc * 64.0, 8.0, 20.0}).reshape({1, 3});
  out.writeln("at equilibrium [O2] = Kc [O]^2 = ", kc * 64.0);
  out.writeln("net production at that state   ", flat(mech.net_production_rates(t, balanced)));
}
```

Output

```text
forward progress  (mol/m^3/s) [7.3216e+04]
reverse progress  (mol/m^3/s) [2.3795e+02]
net production O2, O, Ar      [7.2978e+04, -1.4596e+05, 0.0000e+00]
element rates (O, Ar)         [0.0000e+00, 0.0000e+00]
with shared thermo            [7.2978e+04, -1.4596e+05, 0.0000e+00]
at equilibrium [O2] = Kc [O]^2 = 1.2308e+03
net production at that state   [-1.3970e-09, 2.7940e-09, 0.0000e+00]
```

The forward reaction runs at `7.32e4` mol/m³/s and the reverse at `2.38e2`, so `O2` is produced at `7.30e4` and `O` consumed at `1.46e5`, twice as fast, as the stoichiometry says; argon is untouched. The element rates are zero, since atoms are conserved. Passing the thermodynamic data in gives the same numbers, which is what a reactor does at every step to avoid evaluating them twice. At {math}`[\mathrm{O_2}] = K_c[\mathrm{O}]^2 = 1230.8` mol/m³ the net production vanishes up to rounding (`1e-9` against `1e5`).

## Third bodies and units

The same reaction with `[Ar] = 30` mol/m³, written in centimetre-mole-second units and again in SI units, and with argon counting as an inert bath.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
using chemistry::thermo::SpeciesData;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// one species: name, atoms, molar mass (kg/mol), NASA-7 validity range, and the low- and high-temperature coefficient sets
static SpeciesData species(const char *name, std::vector<std::pair<std::string, double>> atoms, double mass, double t_min, double t_mid, double t_max,
                           std::array<double, 7> low, std::array<double, 7> high) {
  SpeciesData s;
  s.name = name; s.composition = std::move(atoms); s.molar_mass = mass; s.t_min = t_min; s.t_mid = t_mid; s.t_max = t_max; s.low = low; s.high = high;
  return s;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));
  // oxygen molecules and atoms in an argon bath
  const std::vector<SpeciesData> gases = {
      species("O2", {{"O", 2}}, 31.998e-3, 200, 1000, 3500, {3.78245636, -2.99673416e-03, 9.84730201e-06, -9.68129509e-09, 3.24372837e-12, -1063.94356, 3.65767573},
              {3.28253784, 1.48308754e-03, -7.57966669e-07, 2.09470555e-10, -2.16717794e-14, -1088.45772, 5.45323129}),
      species("O", {{"O", 1}}, 15.999e-3, 200, 1000, 3500, {3.1682671, -3.27931884e-03, 6.64306396e-06, -6.12806624e-09, 2.11265971e-12, 2.91222592e+04, 2.05193346},
              {2.56942078, -8.59741137e-05, 4.19484589e-08, -1.00177799e-11, 1.22833691e-15, 2.92175791e+04, 4.78433864}),
      species("AR", {{"Ar", 1}}, 39.95e-3, 300, 1000, 5000, {2.5, 0, 0, 0, 0, -745.375, 4.366}, {2.5, 0, 0, 0, 0, -745.375, 4.366})};
  const auto build = [&](double a, double efficiency_ar, const kinetics::Units &units) {
    kinetics::ReactionSpec r;
    r.reactants = {{"O", 2}};
    r.products = {{"O2", 1}};
    r.type = kinetics::Type::ThreeBody;
    r.a = a;
    r.b = -1.0;
    r.efficiencies = {{"AR", efficiency_ar}};
    return kinetics::Mechanism(gases, {r}, vec({300.0}), units);
  };
  const auto t = vec({2000.0});
  const auto c = vec({1.0, 2.0, 30.0}).reshape({1, 3});
  // the third-body concentration [M] = sum_k eff_k [k]: argon at 0.83 counts as 24.9 of its 30 mol/m^3
  const auto mech = build(1.2e17, 0.83, kinetics::Units::cgs_cal());
  out.writeln("[M]               ", flat(mech.third_body(c)));
  // the same rate constant written in SI units (1.2e17 cm^6 mol^-2 s^-1 = 1.2e5 m^6 mol^-2 s^-1) gives the same result
  const auto si = build(1.2e5, 0.83, kinetics::Units::si());
  out.writeln("k_f  cgs units   ", flat(mech.forward_rate_constants(t, c)));
  out.writeln("k_f  SI units    ", flat(si.forward_rate_constants(t, c)));
  // a weaker third body slows the reaction in proportion
  const auto inert = build(1.2e17, 0.0, kinetics::Units::cgs_cal());
  out.writeln("k_f  argon inert ", flat(inert.forward_rate_constants(t, c)));
}
```

Output

```text
[M]               [2.7900e+01]
k_f  cgs units   [1.6740e+03]
k_f  SI units    [1.6740e+03]
k_f  argon inert [1.8000e+02]
```

The third-body concentration is `1 + 2 + 0.83 x 30 = 27.9` mol/m³. The rate constant `1.2e17` cm⁶/mol²/s and the same number in SI, `1.2e5` m⁶/mol²/s, give the same `1674` per second. With the efficiency of argon set to zero the rate falls to `180`, the ratio `3/27.9` of the third-body concentrations.

## Pressure-dependent reactions

A falloff version of the recombination with a high-pressure limit, a low-pressure limit and Troe parameters, at `1000` K and five argon concentrations from `0.01` to `10⁶` mol/m³. The program prints the effective rate constant with the Lindemann form and with the Troe form, and the high-pressure limit.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
using chemistry::thermo::SpeciesData;
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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));
  // O + O (+M) <=> O2 (+M) as a pressure-dependent reaction: a high-pressure limit (cm^3 mol^-1 s^-1), a low-pressure limit (cm^6 mol^-2 s^-1)
  // and, for the Troe form, four broadening parameters. The numbers are illustrative.
  const auto falloff = [&](bool troe) {
    kinetics::ReactionSpec r;
    r.reactants = {{"O", 2}};
    r.products = {{"O2", 1}};
    r.type = kinetics::Type::Falloff;
    r.a = 1e10;  r.b = 0.0;  r.ea = 0.0;
    r.a_low = 1.2e17;  r.b_low = -1.0;  r.ea_low = 0.0;
    r.troe = troe;
    r.troe_params = {0.7, 100.0, 1000.0, 3000.0};
    return kinetics::Mechanism(gases(), {r}, vec({300.0}), kinetics::Units::cgs_cal());
  };
  const auto lindemann = falloff(false), troe = falloff(true);
  // the effective bimolecular rate constant at 1000 K as the bath gets denser: [AR] from 0.01 to 10000 mol/m^3 with a little O and O2
  const std::vector<double> argon = {0.01, 1.0, 100.0, 1e4, 1e6};
  std::vector<double> rows;
  for (const double ar : argon) for (const double x : {1e-3, 1e-3, ar}) rows.push_back(x);
  const auto c = vec(rows).reshape({5, 3});
  const auto t = vec(std::vector<double>(5, 1000.0));
  out.writeln("[AR] (mol/m^3)         ", argon);
  out.writeln("Lindemann k (m^3/mol/s) ", flat(lindemann.forward_rate_constants(t, c)));
  out.writeln("Troe k      (m^3/mol/s) ", flat(troe.forward_rate_constants(t, c)));
  out.writeln("high-pressure limit     ", 1e10 * 1e-6);
}
```

Output

```text
[AR] (mol/m^3)         [1.0000e-02, 1.0000e+00, 1.0000e+02, 1.0000e+04, 1.0000e+06]
Lindemann k (m^3/mol/s) [1.4398e+00, 1.1881e+02, 5.4546e+03, 9.9174e+03, 9.9992e+03]
Troe k      (m^3/mol/s) [1.1377e+00, 7.2497e+01, 1.6768e+03, 7.5254e+03, 9.5207e+03]
high-pressure limit     1.0000e+04
```

At low density the rate constant grows in proportion to the bath concentration (`1.44` to `118.8` for a hundredfold increase, from the Lindemann form), then saturates at the high-pressure limit `1.0000e4` m³/mol/s (`5.45e3` at `100`, `9.9992e3` at `10⁶`). The Troe broadening lowers the curve everywhere, by about a third in the middle (`1.68e3` against `5.45e3` at `100`) and by `5%` at the highest density, where it has not quite reached the limit.

## Irreversible reactions and reading the mechanism back

The recombination marked irreversible shows a zero reverse rate. The program then lists the species, elements, the stoichiometry matrix (reactions by species), the composition matrix (species by elements) and the molar masses.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
using chemistry::thermo::SpeciesData;
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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));
  // the same recombination, once reversible (the default) and once marked irreversible
  const auto build = [&](bool reversible) {
    kinetics::ReactionSpec r;
    r.reactants = {{"O", 2}};
    r.products = {{"O2", 1}};
    r.type = kinetics::Type::ThreeBody;
    r.a = 1.2e17;
    r.b = -1.0;
    r.reversible = reversible;
    return kinetics::Mechanism(gases(), {r}, vec({300.0}), kinetics::Units::cgs_cal());
  };
  const auto both = build(true), forward_only = build(false);
  const auto t = vec({3000.0});
  const auto c = vec({4.0, 8.0, 20.0}).reshape({1, 3});
  out.writeln("reversible:   forward ", flat(both.rates_of_progress(t, c).forward), "  reverse ", flat(both.rates_of_progress(t, c).reverse));
  out.writeln("irreversible: forward ", flat(forward_only.rates_of_progress(t, c).forward), "  reverse ", flat(forward_only.rates_of_progress(t, c).reverse));
  // reading the mechanism back: the names in order, the elements, and the matrices that connect species to reactions and atoms
  out.print_defaults(PrintOptions{}.fixed(0).with_sep(""));
  for (const auto &name : both.names()) out.write(name, " ");
  out.writeln("  (index of O = ", both.index("O"), ")");
  for (const auto &element : both.elements()) out.write(element, " ");
  out.writeln("  elements");
  out.writeln("stoichiometry [reaction x species]  ", flat(both.stoichiometry()));
  out.writeln("composition   [species x element]   ", flat(both.composition()));
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));
  out.writeln("has molar masses: ", both.has_molar_masses() ? "yes" : "no", "   molar masses (kg/mol) ", std::vector<double>{flat(both.molar_masses())[0], flat(both.molar_masses())[1], flat(both.molar_masses())[2]});
}
```

Output

```text
reversible:   forward [8.1920e+04]  reverse [2.6624e+02]
irreversible: forward [8.1920e+04]  reverse [0.0000e+00]
O2 O AR   (index of O = 1)
O Ar   elements
stoichiometry [reaction x species]  [1, -2, 0]
composition   [species x element]   [2, 0, 1, 0, 0, 1]
has molar masses: yes   molar masses (kg/mol) [3.1998e-02, 1.5999e-02, 3.9950e-02]
```

The forward progress rate is the same `8.19e4` in both versions, and the reverse is `2.66e2` for the reversible reaction and exactly `0` for the irreversible one. The names are `O2 O Ar` in the order given (`O` has index `1`); the stoichiometry row `[1, -2, 0]` says that one `O2` is made and two `O` are consumed; the composition matrix has `O2 = 2 O`, `O = 1 O` and `Ar = 1 Ar`.

## Decisions

- Give a reversible reaction only its forward rate: the reverse one comes from the thermodynamics, which keeps the mechanism consistent with the equilibrium composition of [Equilibrium composition](equilibrium_composition.md). Mark a reaction `reversible = false` only when its reverse is truly negligible. The price is that the thermodynamic data of every species must be right.
- Write `a` and `ea` in the units you found them in and say so with `kinetics::Units::cgs_cal()`; the mechanism converts them. A published mechanism in centimetre-mole-second-calorie units is entered as it is printed, and `Units::si()` is the default.
- A reaction in which any molecule can carry off the excess energy is a third-body reaction (`Type::ThreeBody`), with a rate proportional to `[M]`. If the rate saturates at high pressure, use a falloff reaction (`Type::Falloff`) with the Lindemann form, or the Troe form when you have the four broadening numbers. Use `Elementary` otherwise.
- List efficiencies only for species that differ from the default; `default_efficiency` changes the value for all the others at once.
- Evaluate a batch of states in one call rather than in a loop: the leading axes of temperature and concentration are free.

## Pitfalls

The mechanism is checked when it is built and the thermodynamics when it is evaluated: unknown species names, non-positive pre-exponential factors and temperatures outside the data are refused. A rate constant in the wrong units is not refused, because the number is legal.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <functional>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
using chemistry::thermo::SpeciesData;
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

static void attempt(OutputEngine &out, const char *label, const std::function<void()> &call) {
  try { call(); out.writeln(label, "  no error"); } catch (const pluss::Error &e) { out.writeln(label, "  ", e.message()); }
}

static kinetics::ReactionSpec recombination(double a, const std::string &product = "O2") {
  kinetics::ReactionSpec r;
  r.reactants = {{"O", 2}};
  r.products = {{product, 1}};
  r.type = kinetics::Type::ThreeBody;
  r.a = a;
  r.b = -1.0;
  return r;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));
  const auto t = vec({2000.0});
  const auto c = vec({1.0, 2.0, 30.0}).reshape({1, 3});
  // 1.2e17 is a centimetre-mole-second number: given with the default SI units it is read as 1.2e17 m^6 mol^-2 s^-1
  const kinetics::Mechanism right(gases(), {recombination(1.2e17)}, vec({300.0}), kinetics::Units::cgs_cal());
  const kinetics::Mechanism wrong(gases(), {recombination(1.2e17)}, vec({300.0}));
  out.writeln("k_f with the right units  ", flat(right.forward_rate_constants(t, c)));
  out.writeln("k_f with SI units assumed ", flat(wrong.forward_rate_constants(t, c)));
  // a reaction may only name species the mechanism contains, with a positive pre-exponential factor
  attempt(out, "unknown product:      ", [&] { kinetics::Mechanism(gases(), {recombination(1.2e17, "O3")}, vec({300.0}), kinetics::Units::cgs_cal()); });
  attempt(out, "negative A:           ", [&] { kinetics::Mechanism(gases(), {recombination(-1.0)}, vec({300.0}), kinetics::Units::cgs_cal()); });
  // temperatures outside the thermodynamic data are refused rather than extrapolated
  attempt(out, "5000 K:               ", [&] { (void)right.equilibrium_constants(vec({5000.0})); });
  attempt(out, "100 K:                ", [&] { (void)right.equilibrium_constants(vec({100.0})); });
}
```

Output

```text
k_f with the right units  [1.9800e+03]
k_f with SI units assumed [1.9800e+15]
unknown product:        natsci: kinetics: unknown species 'O3'
negative A:             natsci: kinetics: pre-exponential must be positive (reaction 0)
5000 K:                 natsci::thermo: temperature must be inside the common NASA validity range [300.000000, 3500.000000] K
100 K:                  natsci::thermo: temperature must be inside the common NASA validity range [300.000000, 3500.000000] K
```

The same `1.2e17` gives `1.98e3` per second with the right units and `1.98e15` when the default SI units are assumed, a factor of `10^12` that nothing can detect for you. An unknown product, a negative pre-exponential factor and temperatures of `5000` and `100` K, outside the `300`-`3500` K range of the data, are refused with a message.

The validity range is closed: its two end points are accepted and anything outside is refused. The reactor integrators call the rates through a path that does not check, so that a trial step of the integrator beyond the range does not abort the integration; check the final temperature of a trajectory against `m.thermo().t_min()` and `t_max()`.

## Where next

[Equilibrium composition](equilibrium_composition.md) solves for the state that these rates drive the mixture towards, and [Reactors in time](reactors.md) integrates them. [Mixture properties](mixture_properties.md) turns a mechanism into an ideal-gas mixture model with density, heat capacity and sound speed. For the rate constant of a single step from its barrier, see [Transition-state theory](../chem_04_rates/transition_state_theory.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="equilibrium_composition.html">
    <span class="label">Next</span>
    <span class="title">Equilibrium composition</span>
  </a>
</div>
<!-- apiref:nav:end -->
