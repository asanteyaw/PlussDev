# Hydrogen ignition

[Back to Chemistry 22: Gas-phase kinetics](../chem_22_gas_kinetics.md)

Hydrogen and oxygen can sit side by side for a while at a high temperature and then, within a fraction of a millisecond, burn. The delay before that sudden release is the ignition delay, a basic property of a fuel. This page builds the chain-branching core of hydrogen combustion, finds the delay for one mixture, sweeps it over five starting temperatures in a single batch, and shows the exponential growth of the radicals that causes the ignition.

## How it works

The mechanism has seven species (`H2`, `H`, `O`, `O2`, `OH`, `H2O` and an argon bath) and eight reactions:

| Reaction | Role |
|---|---|
| {math}`\mathrm{H}+\mathrm{O_2}\rightleftharpoons\mathrm{O}+\mathrm{OH}` | branching: one radical becomes two |
| {math}`\mathrm{O}+\mathrm{H_2}\rightleftharpoons\mathrm{H}+\mathrm{OH}` | branching |
| {math}`\mathrm{OH}+\mathrm{H_2}\rightleftharpoons\mathrm{H}+\mathrm{H_2O}` | propagation, and the source of the product water |
| {math}`2\,\mathrm{OH}\rightleftharpoons\mathrm{O}+\mathrm{H_2O}` | propagation |
| `H + OH + M`, `H + H + M`, `O + O + M`, `O + H + M` | recombination with a third body: termination, which also releases heat |

Adding the first three steps gives {math}`\mathrm{H}+3\,\mathrm{H_2}+\mathrm{O_2}\to 2\,\mathrm{H}+2\,\mathrm{H_2O}`: one `H` atom goes in and two come out, so the pool of radicals grows exponentially as {math}`e^{\lambda t}` at a rate {math}`\lambda` that depends on the temperature. The temperature hardly moves until the pool is large, and then the recombination steps release the chemical energy within a very short time. The ignition delay is defined here as the time of the steepest temperature rise, which `reactor::max_slope_time(times, temperature_path)` finds on a sampled path: `temperature_path` has the shape `[len(times), ...]` and the result has the shape `[...]`, one delay per lane.

Because the delay is read from a sampled path it is only as fine as the time grid, and a log-spaced grid covers the many decades between the first reactions and the ignition economically.

## One mixture, one delay

Two moles of hydrogen, one of oxygen and seven of argon at `1200` K and `1` atm, seeded with a trace of `H` atoms. The program samples `121` times from `0.1 µs` to `10 ms` on a logarithmic grid, prints the temperature at six of them and the ignition delay.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
namespace reactor = chemistry::reactor;
using chemistry::thermo::SpeciesData;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static SpeciesData species(const char *name, std::vector<std::pair<std::string, double>> atoms, double mass, double t_min, double t_mid, double t_max,
                           std::array<double, 7> low, std::array<double, 7> high) {
  SpeciesData s;
  s.name = name; s.composition = std::move(atoms); s.molar_mass = mass; s.t_min = t_min; s.t_mid = t_mid; s.t_max = t_max; s.low = low; s.high = high;
  return s;
}

// hydrogen and oxygen with the radicals H, O and OH, products H2O, and an argon bath
static std::vector<SpeciesData> gases() {
  return {
      species("H2", {{"H", 2}}, 2.016e-3, 200, 1000, 3500, {2.34433112, 7.98052075e-03, -1.9478151e-05, 2.01572094e-08, -7.37611761e-12, -917.935173, 0.683010238},
              {3.3372792, -4.94024731e-05, 4.99456778e-07, -1.79566394e-10, 2.00255376e-14, -950.158922, -3.20502331}),
      species("H", {{"H", 1}}, 1.008e-3, 200, 1000, 3500, {2.5, 7.05332819e-13, -1.99591964e-15, 2.30081632e-18, -9.27732332e-22, 2.54736599e+04, -0.446682853},
              {2.50000001, -2.30842973e-11, 1.61561948e-14, -4.73515235e-18, 4.98197357e-22, 2.54736599e+04, -0.446682914}),
      species("O", {{"O", 1}}, 15.999e-3, 200, 1000, 3500, {3.1682671, -3.27931884e-03, 6.64306396e-06, -6.12806624e-09, 2.11265971e-12, 2.91222592e+04, 2.05193346},
              {2.56942078, -8.59741137e-05, 4.19484589e-08, -1.00177799e-11, 1.22833691e-15, 2.92175791e+04, 4.78433864}),
      species("O2", {{"O", 2}}, 31.998e-3, 200, 1000, 3500, {3.78245636, -2.99673416e-03, 9.84730201e-06, -9.68129509e-09, 3.24372837e-12, -1063.94356, 3.65767573},
              {3.28253784, 1.48308754e-03, -7.57966669e-07, 2.09470555e-10, -2.16717794e-14, -1088.45772, 5.45323129}),
      species("OH", {{"O", 1}, {"H", 1}}, 17.007e-3, 200, 1000, 3500, {3.99201543, -2.40131752e-03, 4.61793841e-06, -3.88113333e-09, 1.3641147e-12, 3615.08056, -0.103925458},
              {3.09288767, 5.48429716e-04, 1.26505228e-07, -8.79461556e-11, 1.17412376e-14, 3858.657, 4.4766961}),
      species("H2O", {{"H", 2}, {"O", 1}}, 18.015e-3, 200, 1000, 3500, {4.19864056, -2.0364341e-03, 6.52040211e-06, -5.48797062e-09, 1.77197817e-12, -3.02937267e+04, -0.849032208},
              {3.03399249, 2.17691804e-03, -1.64072518e-07, -9.7041987e-11, 1.68200992e-14, -3.00042971e+04, 4.9667701}),
      species("AR", {{"Ar", 1}}, 39.95e-3, 300, 1000, 5000, {2.5, 0, 0, 0, 0, -745.375, 4.366}, {2.5, 0, 0, 0, 0, -745.375, 4.366})};
}

// a reaction A + B <=> C + D (or with a third body M) with its Arrhenius coefficients in cm, mol, s, cal
static kinetics::ReactionSpec reaction(std::map<std::string, double> in, std::map<std::string, double> products, double a, double b, double ea, bool third = false,
                                       std::map<std::string, double> efficiencies = {}) {
  kinetics::ReactionSpec r;
  r.reactants = std::move(in); r.products = std::move(products); r.a = a; r.b = b; r.ea = ea;
  if (third) { r.type = kinetics::Type::ThreeBody; r.efficiencies = std::move(efficiencies); }
  return r;
}

// the chain-branching core of hydrogen combustion plus the recombination steps that end the chains
static kinetics::Mechanism hydrogen() {
  return kinetics::Mechanism(gases(),
      {reaction({{"H", 1}, {"O2", 1}}, {{"O", 1}, {"OH", 1}}, 2.65e16, -0.6707, 17041.0),
       reaction({{"O", 1}, {"H2", 1}}, {{"H", 1}, {"OH", 1}}, 3.87e4, 2.7, 6260.0),
       reaction({{"OH", 1}, {"H2", 1}}, {{"H", 1}, {"H2O", 1}}, 2.16e8, 1.51, 3430.0),
       reaction({{"OH", 2}}, {{"O", 1}, {"H2O", 1}}, 3.57e4, 2.4, -2110.0),
       reaction({{"H", 1}, {"OH", 1}}, {{"H2O", 1}}, 2.2e22, -2.0, 0.0, true, {{"H2", 0.73}, {"H2O", 3.65}, {"AR", 0.38}}),
       reaction({{"H", 2}}, {{"H2", 1}}, 1.0e18, -1.0, 0.0, true, {{"H2", 0.0}, {"H2O", 0.0}, {"AR", 0.63}}),
       reaction({{"O", 2}}, {{"O2", 1}}, 1.2e17, -1.0, 0.0, true, {{"H2", 2.4}, {"H2O", 15.4}, {"AR", 0.83}}),
       reaction({{"O", 1}, {"H", 1}}, {{"OH", 1}}, 5.0e17, -1.0, 0.0, true, {{"H2", 2.0}, {"H2O", 6.0}, {"AR", 0.7}})},
      vec({300.0}), kinetics::Units::cgs_cal());
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));
  const auto mech = hydrogen();
  out.writeln("species ", mech.species(), "   reactions ", mech.reactions(), "   elements ", mech.elements().size());
  // 2 H2 + O2 diluted in argon at 1 atm and 1200 K, seeded with a trace of H atoms; concentrations in mol/m^3 in the order H2, H, O, O2, OH, H2O, AR
  const double total = 101325.0 / (chemistry::thermo::gas_constant() * 1200.0);
  const auto y0 = reactor::make_state(vec({1200.0}), vec({0.2 * total, 1e-8 * total, 0.0, 0.1 * total, 0.0, 0.0, 0.7 * total}).reshape({1, 7}));
  std::vector<double> times;
  for (int i = 0; i <= 120; ++i) times.push_back(1e-7 * std::pow(10.0, 5.0 * i / 120.0));
  const auto path = reactor::trajectory(mech, y0, times);
  const auto temperature = reactor::temperature(path);  // [times, 1]
  out.writeln("time (s)        ", std::vector<double>{times[0], times[40], times[60], times[80], times[100], times[120]});
  out.writeln("temperature (K) ", std::vector<double>{flat(temperature)[0], flat(temperature)[40], flat(temperature)[60], flat(temperature)[80], flat(temperature)[100], flat(temperature)[120]});
  out.writeln("ignition delay  ", flat(reactor::max_slope_time(times, temperature)));
}
```

Output

```text
species 7   reactions 8   elements 3
time (s)        [1.0000e-07, 4.6416e-06, 3.1623e-05, 2.1544e-04, 1.4678e-03, 1.0000e-02]
temperature (K) [1.2000e+03, 1.2000e+03, 1.2001e+03, 2.8056e+03, 2.9512e+03, 2.9512e+03]
ignition delay  [6.5013e-05]
```

The temperature stays at `1200.0` K for the first `30 µs`, has jumped to `2806` K by `215 µs` and ends at `2951` K. The steepest rise lies at `6.5e-5` s, the ignition delay on this grid.

## Delay against starting temperature

The same mixture started at `1000`, `1100`, `1200`, `1300` and `1400` K as five lanes of one batch, with `241` times from `0.1 µs` to `1` s. The program prints the delay, the final temperature and the temperature rise of every lane.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
namespace reactor = chemistry::reactor;
using chemistry::thermo::SpeciesData;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static SpeciesData species(const char *name, std::vector<std::pair<std::string, double>> atoms, double mass, double t_min, double t_mid, double t_max,
                           std::array<double, 7> low, std::array<double, 7> high) {
  SpeciesData s;
  s.name = name; s.composition = std::move(atoms); s.molar_mass = mass; s.t_min = t_min; s.t_mid = t_mid; s.t_max = t_max; s.low = low; s.high = high;
  return s;
}

// hydrogen and oxygen with the radicals H, O and OH, products H2O, and an argon bath
static std::vector<SpeciesData> gases() {
  return {
      species("H2", {{"H", 2}}, 2.016e-3, 200, 1000, 3500, {2.34433112, 7.98052075e-03, -1.9478151e-05, 2.01572094e-08, -7.37611761e-12, -917.935173, 0.683010238},
              {3.3372792, -4.94024731e-05, 4.99456778e-07, -1.79566394e-10, 2.00255376e-14, -950.158922, -3.20502331}),
      species("H", {{"H", 1}}, 1.008e-3, 200, 1000, 3500, {2.5, 7.05332819e-13, -1.99591964e-15, 2.30081632e-18, -9.27732332e-22, 2.54736599e+04, -0.446682853},
              {2.50000001, -2.30842973e-11, 1.61561948e-14, -4.73515235e-18, 4.98197357e-22, 2.54736599e+04, -0.446682914}),
      species("O", {{"O", 1}}, 15.999e-3, 200, 1000, 3500, {3.1682671, -3.27931884e-03, 6.64306396e-06, -6.12806624e-09, 2.11265971e-12, 2.91222592e+04, 2.05193346},
              {2.56942078, -8.59741137e-05, 4.19484589e-08, -1.00177799e-11, 1.22833691e-15, 2.92175791e+04, 4.78433864}),
      species("O2", {{"O", 2}}, 31.998e-3, 200, 1000, 3500, {3.78245636, -2.99673416e-03, 9.84730201e-06, -9.68129509e-09, 3.24372837e-12, -1063.94356, 3.65767573},
              {3.28253784, 1.48308754e-03, -7.57966669e-07, 2.09470555e-10, -2.16717794e-14, -1088.45772, 5.45323129}),
      species("OH", {{"O", 1}, {"H", 1}}, 17.007e-3, 200, 1000, 3500, {3.99201543, -2.40131752e-03, 4.61793841e-06, -3.88113333e-09, 1.3641147e-12, 3615.08056, -0.103925458},
              {3.09288767, 5.48429716e-04, 1.26505228e-07, -8.79461556e-11, 1.17412376e-14, 3858.657, 4.4766961}),
      species("H2O", {{"H", 2}, {"O", 1}}, 18.015e-3, 200, 1000, 3500, {4.19864056, -2.0364341e-03, 6.52040211e-06, -5.48797062e-09, 1.77197817e-12, -3.02937267e+04, -0.849032208},
              {3.03399249, 2.17691804e-03, -1.64072518e-07, -9.7041987e-11, 1.68200992e-14, -3.00042971e+04, 4.9667701}),
      species("AR", {{"Ar", 1}}, 39.95e-3, 300, 1000, 5000, {2.5, 0, 0, 0, 0, -745.375, 4.366}, {2.5, 0, 0, 0, 0, -745.375, 4.366})};
}

// a reaction A + B <=> C + D (or with a third body M) with its Arrhenius coefficients in cm, mol, s, cal
static kinetics::ReactionSpec reaction(std::map<std::string, double> in, std::map<std::string, double> products, double a, double b, double ea, bool third = false,
                                       std::map<std::string, double> efficiencies = {}) {
  kinetics::ReactionSpec r;
  r.reactants = std::move(in); r.products = std::move(products); r.a = a; r.b = b; r.ea = ea;
  if (third) { r.type = kinetics::Type::ThreeBody; r.efficiencies = std::move(efficiencies); }
  return r;
}

// the chain-branching core of hydrogen combustion plus the recombination steps that end the chains
static kinetics::Mechanism hydrogen() {
  return kinetics::Mechanism(gases(),
      {reaction({{"H", 1}, {"O2", 1}}, {{"O", 1}, {"OH", 1}}, 2.65e16, -0.6707, 17041.0),
       reaction({{"O", 1}, {"H2", 1}}, {{"H", 1}, {"OH", 1}}, 3.87e4, 2.7, 6260.0),
       reaction({{"OH", 1}, {"H2", 1}}, {{"H", 1}, {"H2O", 1}}, 2.16e8, 1.51, 3430.0),
       reaction({{"OH", 2}}, {{"O", 1}, {"H2O", 1}}, 3.57e4, 2.4, -2110.0),
       reaction({{"H", 1}, {"OH", 1}}, {{"H2O", 1}}, 2.2e22, -2.0, 0.0, true, {{"H2", 0.73}, {"H2O", 3.65}, {"AR", 0.38}}),
       reaction({{"H", 2}}, {{"H2", 1}}, 1.0e18, -1.0, 0.0, true, {{"H2", 0.0}, {"H2O", 0.0}, {"AR", 0.63}}),
       reaction({{"O", 2}}, {{"O2", 1}}, 1.2e17, -1.0, 0.0, true, {{"H2", 2.4}, {"H2O", 15.4}, {"AR", 0.83}}),
       reaction({{"O", 1}, {"H", 1}}, {{"OH", 1}}, 5.0e17, -1.0, 0.0, true, {{"H2", 2.0}, {"H2O", 6.0}, {"AR", 0.7}})},
      vec({300.0}), kinetics::Units::cgs_cal());
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(3).with_sep(""));
  const auto mech = hydrogen();
  // five starting temperatures run side by side as lanes: each is stoichiometric H2/O2/Ar at 1 atm, seeded with a trace of H atoms
  const std::vector<double> starts = {1000.0, 1100.0, 1200.0, 1300.0, 1400.0};
  std::vector<double> c;
  for (const double t0 : starts) {
    const double total = 101325.0 / (chemistry::thermo::gas_constant() * t0);
    for (const double x : {0.2, 1e-8, 0.0, 0.1, 0.0, 0.0, 0.7}) c.push_back(x * total);
  }
  const auto y0 = reactor::make_state(vec(starts), vec(c).reshape({5, 7}));
  std::vector<double> times;
  for (int i = 0; i <= 240; ++i) times.push_back(1e-7 * std::pow(10.0, 7.0 * i / 240.0));
  const auto path = reactor::trajectory(mech, y0, times);  // [times, lanes, 8]
  const auto temperature = reactor::temperature(path);       // [times, lanes]
  out.writeln("start (K)           ", starts);
  out.writeln("ignition delay (s)  ", flat(reactor::max_slope_time(times, temperature)));
  out.writeln("final T (K)         ", flat(temperature.select(0, 240)));
  // the heat released: the final temperature rise above the start
  out.writeln("temperature rise (K)", flat(temperature.select(0, 240).sub(vec(starts))));
}
```

Output

```text
start (K)           [1.000e+03, 1.100e+03, 1.200e+03, 1.300e+03, 1.400e+03]
ignition delay (s)  [1.788e-04, 9.769e-05, 6.105e-05, 4.080e-05, 2.916e-05]
final T (K)         [2.921e+03, 2.936e+03, 2.951e+03, 2.966e+03, 2.980e+03]
temperature rise (K)[1.921e+03, 1.836e+03, 1.751e+03, 1.666e+03, 1.580e+03]
```

The delay falls from `1.79e-4` s at `1000` K to `2.92e-5` s at `1400` K, a factor of `6`. A fit of {math}`\ln\tau` against {math}`1/T` between the two ends gives an apparent activation temperature of `6.3e3` K, about `53` kJ/mol, below the `71` kJ/mol barrier of the branching step {math}`\mathrm{H}+\mathrm{O_2}`, because several steps contribute. The final temperature rises with the starting temperature (`2921` to `2980` K) but the rise falls from `1921` to `1580` K: the hotter gas ends in a state with more of its energy in dissociated products.

## The radicals multiply before the temperature moves

The `1200` K case again, now looking at the radical concentrations on a uniform grid from `0` to `30 µs`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
namespace reactor = chemistry::reactor;
using chemistry::thermo::SpeciesData;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static SpeciesData species(const char *name, std::vector<std::pair<std::string, double>> atoms, double mass, double t_min, double t_mid, double t_max,
                           std::array<double, 7> low, std::array<double, 7> high) {
  SpeciesData s;
  s.name = name; s.composition = std::move(atoms); s.molar_mass = mass; s.t_min = t_min; s.t_mid = t_mid; s.t_max = t_max; s.low = low; s.high = high;
  return s;
}

// hydrogen and oxygen with the radicals H, O and OH, products H2O, and an argon bath
static std::vector<SpeciesData> gases() {
  return {
      species("H2", {{"H", 2}}, 2.016e-3, 200, 1000, 3500, {2.34433112, 7.98052075e-03, -1.9478151e-05, 2.01572094e-08, -7.37611761e-12, -917.935173, 0.683010238},
              {3.3372792, -4.94024731e-05, 4.99456778e-07, -1.79566394e-10, 2.00255376e-14, -950.158922, -3.20502331}),
      species("H", {{"H", 1}}, 1.008e-3, 200, 1000, 3500, {2.5, 7.05332819e-13, -1.99591964e-15, 2.30081632e-18, -9.27732332e-22, 2.54736599e+04, -0.446682853},
              {2.50000001, -2.30842973e-11, 1.61561948e-14, -4.73515235e-18, 4.98197357e-22, 2.54736599e+04, -0.446682914}),
      species("O", {{"O", 1}}, 15.999e-3, 200, 1000, 3500, {3.1682671, -3.27931884e-03, 6.64306396e-06, -6.12806624e-09, 2.11265971e-12, 2.91222592e+04, 2.05193346},
              {2.56942078, -8.59741137e-05, 4.19484589e-08, -1.00177799e-11, 1.22833691e-15, 2.92175791e+04, 4.78433864}),
      species("O2", {{"O", 2}}, 31.998e-3, 200, 1000, 3500, {3.78245636, -2.99673416e-03, 9.84730201e-06, -9.68129509e-09, 3.24372837e-12, -1063.94356, 3.65767573},
              {3.28253784, 1.48308754e-03, -7.57966669e-07, 2.09470555e-10, -2.16717794e-14, -1088.45772, 5.45323129}),
      species("OH", {{"O", 1}, {"H", 1}}, 17.007e-3, 200, 1000, 3500, {3.99201543, -2.40131752e-03, 4.61793841e-06, -3.88113333e-09, 1.3641147e-12, 3615.08056, -0.103925458},
              {3.09288767, 5.48429716e-04, 1.26505228e-07, -8.79461556e-11, 1.17412376e-14, 3858.657, 4.4766961}),
      species("H2O", {{"H", 2}, {"O", 1}}, 18.015e-3, 200, 1000, 3500, {4.19864056, -2.0364341e-03, 6.52040211e-06, -5.48797062e-09, 1.77197817e-12, -3.02937267e+04, -0.849032208},
              {3.03399249, 2.17691804e-03, -1.64072518e-07, -9.7041987e-11, 1.68200992e-14, -3.00042971e+04, 4.9667701}),
      species("AR", {{"Ar", 1}}, 39.95e-3, 300, 1000, 5000, {2.5, 0, 0, 0, 0, -745.375, 4.366}, {2.5, 0, 0, 0, 0, -745.375, 4.366})};
}

// a reaction A + B <=> C + D (or with a third body M) with its Arrhenius coefficients in cm, mol, s, cal
static kinetics::ReactionSpec reaction(std::map<std::string, double> in, std::map<std::string, double> products, double a, double b, double ea, bool third = false,
                                       std::map<std::string, double> efficiencies = {}) {
  kinetics::ReactionSpec r;
  r.reactants = std::move(in); r.products = std::move(products); r.a = a; r.b = b; r.ea = ea;
  if (third) { r.type = kinetics::Type::ThreeBody; r.efficiencies = std::move(efficiencies); }
  return r;
}

// the chain-branching core of hydrogen combustion plus the recombination steps that end the chains
static kinetics::Mechanism hydrogen() {
  return kinetics::Mechanism(gases(),
      {reaction({{"H", 1}, {"O2", 1}}, {{"O", 1}, {"OH", 1}}, 2.65e16, -0.6707, 17041.0),
       reaction({{"O", 1}, {"H2", 1}}, {{"H", 1}, {"OH", 1}}, 3.87e4, 2.7, 6260.0),
       reaction({{"OH", 1}, {"H2", 1}}, {{"H", 1}, {"H2O", 1}}, 2.16e8, 1.51, 3430.0),
       reaction({{"OH", 2}}, {{"O", 1}, {"H2O", 1}}, 3.57e4, 2.4, -2110.0),
       reaction({{"H", 1}, {"OH", 1}}, {{"H2O", 1}}, 2.2e22, -2.0, 0.0, true, {{"H2", 0.73}, {"H2O", 3.65}, {"AR", 0.38}}),
       reaction({{"H", 2}}, {{"H2", 1}}, 1.0e18, -1.0, 0.0, true, {{"H2", 0.0}, {"H2O", 0.0}, {"AR", 0.63}}),
       reaction({{"O", 2}}, {{"O2", 1}}, 1.2e17, -1.0, 0.0, true, {{"H2", 2.4}, {"H2O", 15.4}, {"AR", 0.83}}),
       reaction({{"O", 1}, {"H", 1}}, {{"OH", 1}}, 5.0e17, -1.0, 0.0, true, {{"H2", 2.0}, {"H2O", 6.0}, {"AR", 0.7}})},
      vec({300.0}), kinetics::Units::cgs_cal());
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));
  const auto mech = hydrogen();
  const double total = 101325.0 / (chemistry::thermo::gas_constant() * 1200.0);
  const auto y0 = reactor::make_state(vec({1200.0}), vec({0.2 * total, 1e-8 * total, 0.0, 0.1 * total, 0.0, 0.0, 0.7 * total}).reshape({1, 7}));
  // watch the radicals H, O and OH multiply before the temperature moves: a chain reaction grows exponentially
  const std::vector<double> times = {0.0, 5e-6, 1e-5, 1.5e-5, 2e-5, 2.5e-5, 3e-5};
  const auto path = reactor::trajectory(mech, y0, times);
  const auto c = reactor::concentrations(path);  // [times, 1, 7]
  out.writeln("time (s)        ", times);
  out.writeln("[H]  (mol/m^3)  ", flat(c.select(-1, 1)));
  out.writeln("[O]  (mol/m^3)  ", flat(c.select(-1, 2)));
  out.writeln("[OH] (mol/m^3)  ", flat(c.select(-1, 4)));
  out.writeln("T (K)           ", flat(reactor::temperature(path)));
  // the growth rate of the radical pool, from the last two samples of [OH]
  const auto oh = flat(c.select(-1, 4));
  out.writeln("growth rate of [OH] (1/s) ", std::log(oh[6] / oh[5]) / 5e-6);
}
```

Output

```text
time (s)        [0.0000e+00, 5.0000e-06, 1.0000e-05, 1.5000e-05, 2.0000e-05, 2.5000e-05, 3.0000e-05]
[H]  (mol/m^3)  [1.0155e-07, 3.2186e-07, 1.2850e-06, 5.1278e-06, 2.0459e-05, 8.1615e-05, 3.2555e-04]
[O]  (mol/m^3)  [0.0000e+00, 4.0519e-08, 1.6179e-07, 6.4562e-07, 2.5759e-06, 1.0276e-05, 4.0990e-05]
[OH] (mol/m^3)  [0.0000e+00, 2.1582e-08, 8.6172e-08, 3.4387e-07, 1.3720e-06, 5.4732e-06, 2.1832e-05]
T (K)           [1.2000e+03, 1.2000e+03, 1.2000e+03, 1.2000e+03, 1.2000e+03, 1.2000e+03, 1.2000e+03]
growth rate of [OH] (1/s) 2.7670e+05
```

Every `5 µs` each radical concentration grows by the same factor of `4` (`[H]` from `3.2e-7` to `1.3e-6` to `5.1e-6` mol/m³), a straight line on a logarithmic scale, which is the signature of the chain reaction. The growth rate of `[OH]` is `2.77e5` per second, {math}`\ln 4/5\,\mu s`. Over the same `30 µs` the temperature stays at `1200` K: the pool must become large before the heat release is felt, which is why the delay looks sudden.

## Decisions

- Use a log-spaced grid from a time far below the expected delay to a time well beyond it, and refine the grid until the delay stops changing in the digits you need; the pitfalls below show how a coarse grid and a short window mislead.
- Seed the mixture with a trace of `H` atoms if your mechanism, like this one, is a reduced core. The seed sets the first radicals and changes the delay.
- Run a sweep over starting temperature or pressure as the lanes of one batch. The step is shared, so keep the lanes within a factor of a few in time scale, or the fastest lane is integrated with the steps the slowest one needs.
- A mechanism without the hydroperoxyl steps, such as this one, is a teaching core. Its high-temperature delays are meaningful as a demonstration of the structure, not as design data for a fuel.

## Pitfalls

The delay is only as good as the time grid and the window. A coarse grid gives a coarse number, a window that ends before the ignition still returns a number, and the starting radical pool matters.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
namespace reactor = chemistry::reactor;
using chemistry::thermo::SpeciesData;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static SpeciesData species(const char *name, std::vector<std::pair<std::string, double>> atoms, double mass, double t_min, double t_mid, double t_max,
                           std::array<double, 7> low, std::array<double, 7> high) {
  SpeciesData s;
  s.name = name; s.composition = std::move(atoms); s.molar_mass = mass; s.t_min = t_min; s.t_mid = t_mid; s.t_max = t_max; s.low = low; s.high = high;
  return s;
}

// hydrogen and oxygen with the radicals H, O and OH, products H2O, and an argon bath
static std::vector<SpeciesData> gases() {
  return {
      species("H2", {{"H", 2}}, 2.016e-3, 200, 1000, 3500, {2.34433112, 7.98052075e-03, -1.9478151e-05, 2.01572094e-08, -7.37611761e-12, -917.935173, 0.683010238},
              {3.3372792, -4.94024731e-05, 4.99456778e-07, -1.79566394e-10, 2.00255376e-14, -950.158922, -3.20502331}),
      species("H", {{"H", 1}}, 1.008e-3, 200, 1000, 3500, {2.5, 7.05332819e-13, -1.99591964e-15, 2.30081632e-18, -9.27732332e-22, 2.54736599e+04, -0.446682853},
              {2.50000001, -2.30842973e-11, 1.61561948e-14, -4.73515235e-18, 4.98197357e-22, 2.54736599e+04, -0.446682914}),
      species("O", {{"O", 1}}, 15.999e-3, 200, 1000, 3500, {3.1682671, -3.27931884e-03, 6.64306396e-06, -6.12806624e-09, 2.11265971e-12, 2.91222592e+04, 2.05193346},
              {2.56942078, -8.59741137e-05, 4.19484589e-08, -1.00177799e-11, 1.22833691e-15, 2.92175791e+04, 4.78433864}),
      species("O2", {{"O", 2}}, 31.998e-3, 200, 1000, 3500, {3.78245636, -2.99673416e-03, 9.84730201e-06, -9.68129509e-09, 3.24372837e-12, -1063.94356, 3.65767573},
              {3.28253784, 1.48308754e-03, -7.57966669e-07, 2.09470555e-10, -2.16717794e-14, -1088.45772, 5.45323129}),
      species("OH", {{"O", 1}, {"H", 1}}, 17.007e-3, 200, 1000, 3500, {3.99201543, -2.40131752e-03, 4.61793841e-06, -3.88113333e-09, 1.3641147e-12, 3615.08056, -0.103925458},
              {3.09288767, 5.48429716e-04, 1.26505228e-07, -8.79461556e-11, 1.17412376e-14, 3858.657, 4.4766961}),
      species("H2O", {{"H", 2}, {"O", 1}}, 18.015e-3, 200, 1000, 3500, {4.19864056, -2.0364341e-03, 6.52040211e-06, -5.48797062e-09, 1.77197817e-12, -3.02937267e+04, -0.849032208},
              {3.03399249, 2.17691804e-03, -1.64072518e-07, -9.7041987e-11, 1.68200992e-14, -3.00042971e+04, 4.9667701}),
      species("AR", {{"Ar", 1}}, 39.95e-3, 300, 1000, 5000, {2.5, 0, 0, 0, 0, -745.375, 4.366}, {2.5, 0, 0, 0, 0, -745.375, 4.366})};
}

// a reaction A + B <=> C + D (or with a third body M) with its Arrhenius coefficients in cm, mol, s, cal
static kinetics::ReactionSpec reaction(std::map<std::string, double> in, std::map<std::string, double> products, double a, double b, double ea, bool third = false,
                                       std::map<std::string, double> efficiencies = {}) {
  kinetics::ReactionSpec r;
  r.reactants = std::move(in); r.products = std::move(products); r.a = a; r.b = b; r.ea = ea;
  if (third) { r.type = kinetics::Type::ThreeBody; r.efficiencies = std::move(efficiencies); }
  return r;
}

// the chain-branching core of hydrogen combustion plus the recombination steps that end the chains
static kinetics::Mechanism hydrogen() {
  return kinetics::Mechanism(gases(),
      {reaction({{"H", 1}, {"O2", 1}}, {{"O", 1}, {"OH", 1}}, 2.65e16, -0.6707, 17041.0),
       reaction({{"O", 1}, {"H2", 1}}, {{"H", 1}, {"OH", 1}}, 3.87e4, 2.7, 6260.0),
       reaction({{"OH", 1}, {"H2", 1}}, {{"H", 1}, {"H2O", 1}}, 2.16e8, 1.51, 3430.0),
       reaction({{"OH", 2}}, {{"O", 1}, {"H2O", 1}}, 3.57e4, 2.4, -2110.0),
       reaction({{"H", 1}, {"OH", 1}}, {{"H2O", 1}}, 2.2e22, -2.0, 0.0, true, {{"H2", 0.73}, {"H2O", 3.65}, {"AR", 0.38}}),
       reaction({{"H", 2}}, {{"H2", 1}}, 1.0e18, -1.0, 0.0, true, {{"H2", 0.0}, {"H2O", 0.0}, {"AR", 0.63}}),
       reaction({{"O", 2}}, {{"O2", 1}}, 1.2e17, -1.0, 0.0, true, {{"H2", 2.4}, {"H2O", 15.4}, {"AR", 0.83}}),
       reaction({{"O", 1}, {"H", 1}}, {{"OH", 1}}, 5.0e17, -1.0, 0.0, true, {{"H2", 2.0}, {"H2O", 6.0}, {"AR", 0.7}})},
      vec({300.0}), kinetics::Units::cgs_cal());
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(3).with_sep(""));
  const auto mech = hydrogen();
  const double total = 101325.0 / (chemistry::thermo::gas_constant() * 1200.0);
  const auto state = [&](double h_fraction) {
    return reactor::make_state(vec({1200.0}), vec({0.2 * total, h_fraction * total, 0.0, 0.1 * total, 0.0, 0.0, 0.7 * total}).reshape({1, 7}));
  };
  const auto grid = [](int points, double end) {
    std::vector<double> times;
    for (int i = 0; i <= points; ++i) times.push_back(1e-7 * std::pow(end / 1e-7, static_cast<double>(i) / points));
    return times;
  };
  const auto delay = [&](double seed, const std::vector<double> &times) {
    const auto t = reactor::temperature(reactor::trajectory(mech, state(seed), times));
    return std::make_pair(flat(reactor::max_slope_time(times, t))[0], flat(t.select(0, static_cast<std::int64_t>(times.size()) - 1))[0] - 1200.0);
  };
  // 1. the delay is read from a sampled path, so it is only as fine as the grid
  for (const int points : {20, 80, 320}) out.writeln(points, " points:   ignition delay = ", delay(1e-8, grid(points, 1e-3)).first, " s");
  // 2. a window that ends before the ignition still returns a number: the steepest rise inside the window, with almost no rise at all
  const auto early = delay(1e-8, grid(40, 1.8e-5));
  out.writeln("window ends at 18 us:  'delay' = ", early.first, " s, temperature rise only ", early.second, " K");
  // 3. the starting radical pool matters: the same mixture without the H seed ignites later
  out.writeln("seed 1e-8:   delay = ", delay(1e-8, grid(320, 1e-3)).first, " s");
  out.writeln("no seed:     delay = ", delay(0.0, grid(320, 1e-3)).first, " s");
}
```

Output

```text
20 points:   ignition delay = 8.155e-05 s
80 points:   ignition delay = 6.695e-05 s
320 points:   ignition delay = 6.220e-05 s
window ends at 18 us:  'delay' = 1.690e-05 s, temperature rise only 1.567e-03 K
seed 1e-8:   delay = 6.220e-05 s
no seed:     delay = 1.172e-04 s
```

The same mixture gives `8.2e-5`, `6.7e-5` and `6.2e-5` s with `20`, `80` and `320` grid points, so the first two digits need a fine grid. A window that ends at `18 µs` returns `1.7e-5` s although the gas warmed by only `0.0016` K, so check that the temperature rose before you trust the value. Without the `H` seed the delay is `1.17e-4` s instead of `6.2e-5` s.

## Where next

[The constant-volume reactor](closed_reactor.md) describes the integrator and its options in detail. [Mixture properties](mixture_properties.md) connects the burnt composition and temperature to the thermodynamic state of the gas.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="closed_reactor.html">
    <span class="label">Previous</span>
    <span class="title">The constant-volume reactor</span>
  </a>
  <a class="next-link" href="mixture_properties.html">
    <span class="label">Next</span>
    <span class="title">Mixture properties</span>
  </a>
</div>
<!-- apiref:nav:end -->
