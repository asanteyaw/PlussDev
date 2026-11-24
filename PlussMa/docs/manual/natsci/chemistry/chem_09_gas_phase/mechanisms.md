# Mechanisms and rate constants

[Back to Chemistry 9: Gas-Phase Kinetics and Equilibrium](../chem_09_gas_phase.md)

You have a set of gas-phase reactions with Arrhenius parameters and thermodynamic data for the species, and you want the rate of every reaction and the net rate at which every species is made or lost, for any temperature and composition. This page builds a mechanism from species and reactions, evaluates the equilibrium constants that fix the reverse rates, the forward rate constants of elementary and three-body reactions, and the production rates over a whole grid of states. All three pages use one small system so that every number can be checked by hand: a diatomic {math}`\mathrm{X_2}` that dissociates into two atoms {math}`\mathrm{X}`, {math}`\mathrm{X_2} \rightleftharpoons 2\mathrm{X}`. Its thermodynamic data have constant heat capacities, {math}`c_p/R = 3.5` for {math}`\mathrm{X_2}` and {math}`2.5` for {math}`\mathrm{X}`, and the atom is {math}`\Delta H/R = 12000` K (about {math}`100` kJ/mol) above the molecule.

## How it works

A `kinetics::Mechanism` is built from a list of `thermo::SpeciesData` (name, element composition, molar mass, and the seven-coefficient thermodynamic polynomials for a low and a high temperature range), a list of `kinetics::ReactionSpec` (reactants and products with their coefficients, and the rate parameters {math}`A`, {math}`b`, {math}`E_a`), and an exemplar Series that fixes the numeric type. The forward rate constant is the extended Arrhenius law,

```{math}
k_f = A\,T^{b}\,e^{-E_a/RT}
```

and a reaction can be flagged `ThreeBody`, in which case {math}`k_f` is multiplied by the effective third-body concentration {math}`[M] = \sum_j \varepsilon_j c_j` with a collision efficiency {math}`\varepsilon_j` for each species, or `Falloff`, which blends a low-pressure and a high-pressure limit. For a reversible reaction the reverse rate constant is not an independent input; it follows from the thermodynamics through the concentration equilibrium constant {math}`K_c = \exp(-\Delta G^\circ/RT)\,(p^\circ/RT)^{\Delta\nu}`, so that {math}`k_r = k_f/K_c` and the mechanism is consistent with the equilibrium state at every temperature. The rate of progress of each reaction is the forward rate minus the reverse one,

```{math}
q_r = k_{f,r}\prod_j c_j^{\nu'_{jr}} - k_{r,r}\prod_j c_j^{\nu''_{jr}},\qquad \dot\omega_j = \sum_r \nu_{jr}\,q_r
```

`equilibrium_constants(T)`, `forward_rate_constants(T, c)`, `rates_of_progress(T, c)` and `net_production_rates(T, c)` evaluate these for temperatures of shape {math}`[\ldots]` and concentrations of shape {math}`[\ldots, \text{species}]` in mol/m{math}`^3`. The units of {math}`A` are those of the `Units` argument (SI by default: m, mol, s, J).

## Equilibrium constants and rates for one state

The program builds the mechanism, prints its size and stoichiometry, evaluates {math}`K_c` at four temperatures, and then evaluates the forward and reverse rates of progress and the net production rates at {math}`2000` K and {math}`1` atm for a mixture that is {math}`90\%` molecule by mole. The forward rate constant is {math}`k = 10^{8}e^{-E_a/RT}` s{math}`^{-1}` with {math}`E_a = 100` kJ/mol.

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
  const double c_total = 101325.0 / (8.31446261815324 * 2000.0);
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

## Elementary, temperature-power and three-body rate constants

Three versions of the dissociation are put in one mechanism: elementary, with a prefactor {math}`10^5` and a {math}`T^{1.5}` dependence, and a three-body form with third-body efficiencies of {math}`5` for the atom and {math}`1` for the molecule. The program evaluates the forward rate constant of each at three temperatures for a mixture of {math}`5` mol/m{math}`^3` of molecule and {math}`1` mol/m{math}`^3` of atom.

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
  // three versions of the dissociation: elementary, with a temperature exponent, and with a third body M (efficiency 5 for X)
  auto elementary = reactions()[0];
  auto powered = elementary;
  powered.b = 1.5; powered.a = 1e5;
  kin::ReactionSpec third;
  third.reactants = {{"X2", 1}};
  third.products = {{"X", 2}};
  third.type = kin::Type::ThreeBody;
  third.a = 1e4; third.ea = 100000.0;
  third.efficiencies = {{"X", 5.0}};
  const kin::Mechanism m(species(), {elementary, powered, third}, vec({300.0}));
  const auto t = vec({1000.0, 1500.0, 2000.0});
  // concentrations of 5 mol/m^3 X2 and 1 mol/m^3 X in every lane
  const auto c = pluss::make_series(std::vector<double>{5, 1, 5, 1, 5, 1}, {3, 2}).to(pluss::ScalarType::Float64);
  const auto kf = m.forward_rate_constants(t, c);   // [3 temperatures, 3 reactions]
  out.writeln("temperature (K)                 ", flat(t));
  out.writeln("k elementary (1/s)              ", flat(kf.select(-1, 0)));
  out.writeln("k with T^1.5 (1/s)              ", flat(kf.select(-1, 1)));
  out.writeln("k three-body (m^3/mol/s)        ", flat(kf.select(-1, 2)));
  out.writeln("effective third-body concentration M ", flat(m.third_body(c).select(-1, 2)));
}
```

Output

```text
temperature (K)                 [1000.0000, 1500.0000, 2000.0000]
k elementary (1/s)              [597.9130, 32942.6596, 244522.5938]
k with T^1.5 (1/s)              [18907.6689, 1913795.5800, 21870765.6710]
k three-body (m^3/mol/s)        [0.5979, 32.9427, 244.5226]
effective third-body concentration M [10.0000, 10.0000, 10.0000]
```

The elementary constant rises from `597.9130` to `244522.5938` s{math}`^{-1}` between {math}`1000` and {math}`2000` K, a factor of about four hundred. The {math}`T^{1.5}` version rises by a factor of about twelve hundred because the power law adds to the exponential. The third-body constant `0.5979` is the Arrhenius part times the effective third-body concentration `10.0000` mol/m{math}`^3`, which is {math}`5\cdot1 + 1\cdot5`: the atoms count five times. It is a rate constant of the product {math}`[M]\,k`, and so it carries the factor already.

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

## Decisions

- Give reversible reactions only a forward rate: the reverse one comes from the thermodynamics. That keeps the mechanism consistent with the equilibrium composition of [Equilibrium composition](equilibrium_composition.md), at the price that the thermodynamic data for every species must be right.
- Use `ThreeBody` for {math}`A + B + M` reactions with a table of collision efficiencies, `Falloff` (with Troe parameters) when a reaction passes from second order at low pressure to first order at high pressure, and `Elementary` otherwise.
- Fix the units once with the `Units` argument: `Units::si()` for m, mol, s, J, or `Units::cgs_cal()` for cm, mol, s, cal, the convention of most published mechanisms.
- Evaluate a batch of states in one call rather than in a loop: the leading axes of temperature and concentration are free.

## Pitfalls

The constructor checks the mechanism: unknown species, a non-positive {math}`A` and an empty reaction list are errors, and so is evaluating the thermodynamics outside the range of the data ({math}`200` to {math}`5000` K here). The program triggers each of them.

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
  // a reaction that names a species the mechanism does not contain
  try {
    kin::ReactionSpec bad;
    bad.reactants = {{"X2", 1}};
    bad.products = {{"Y", 2}};
    bad.a = 1e8;
    kin::Mechanism m(species(), {bad}, vec({300.0}));
  } catch (const pluss::Error &e) {
    out.writeln("unknown species : ", e.message());
  }
  // a pre-exponential factor of zero, and no reactions at all
  try {
    auto rs = reactions();
    rs[0].a = 0.0;
    kin::Mechanism m(species(), rs, vec({300.0}));
  } catch (const pluss::Error &e) {
    out.writeln("zero A          : ", e.message());
  }
  try {
    kin::Mechanism m(species(), {}, vec({300.0}));
  } catch (const pluss::Error &e) {
    out.writeln("no reactions    : ", e.message());
  }
  // thermodynamic data are valid from 200 to 5000 K, and an evaluation outside that range is an error
  const kin::Mechanism m(species(), reactions(), vec({300.0}));
  out.writeln("valid range (K) ", std::vector<double>{m.thermo().t_min(), m.thermo().t_max()});
  for (double t : {100.0, 6000.0}) {
    try {
      m.equilibrium_constants(vec({t}));
    } catch (const pluss::Error &e) {
      out.writeln("T = ", std::to_string(static_cast<int>(t)), " K: ", e.message());
    }
  }
  const auto c = pluss::make_series(std::vector<double>{1.0, 1.0}, {1, 2}).to(pluss::ScalarType::Float64);
  try {
    m.net_production_rates(vec({6000.0}), c);
  } catch (const pluss::Error &e) {
    out.writeln("net production at 6000 K: ", e.message());
  }
  out.writeln("log10 Kc at the two ends of the range ", flat(m.equilibrium_constants(vec({200.0, 5000.0})).log10()));
}
```

Output

```text
unknown species : natsci: kinetics: unknown species 'Y'
zero A          : natsci: kinetics: pre-exponential must be positive (reaction 0)
no reactions    : natsci: kinetics: need at least one reaction
valid range (K) [200.0000, 5000.0000]
T = 100 K: natsci::thermo: temperature must be inside the common NASA validity range [200.000000, 5000.000000] K
T = 6000 K: natsci::thermo: temperature must be inside the common NASA validity range [200.000000, 5000.000000] K
net production at 6000 K: natsci::thermo: temperature must be inside the common NASA validity range [200.000000, 5000.000000] K
log10 Kc at the two ends of the range [-19.7355, 5.9788]
```

The three construction errors name what is wrong. A temperature of {math}`100` K or {math}`6000` K is refused by `equilibrium_constants` and by `net_production_rates` with the validity range in the message, because the polynomials are fits that are meaningless outside it; the two ends of the range themselves are accepted, `-19.7355` and `5.9788` in {math}`\log_{10}K_c`. The reactor integrators call the rates through a path that does not check, so that a trial step of the integrator beyond the range does not abort the integration; check the final temperature of a trajectory against `m.thermo().t_min()` and `t_max()`.

## Where next

[Equilibrium composition](equilibrium_composition.md) solves for the state that these rates drive the mixture towards, and [Reactors in time](reactors.md) integrates them. [Chemistry 4: Reaction Rates](../chem_04_rates.md) shows how to estimate {math}`A` and {math}`E_a` from theory.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="equilibrium_composition.html">
    <span class="label">Next</span>
    <span class="title">Equilibrium composition</span>
  </a>
</div>
<!-- apiref:nav:end -->
