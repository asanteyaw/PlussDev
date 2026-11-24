# Thermodynamic properties of species

[Back to Physics 12: Species Thermodynamics](../phys_12_species.md)

The ideal gas of Physics 4 has one heat capacity and one heat ratio, and real gases do not: nitrogen at `2000` K stores more energy per degree than at `300` K because its molecules vibrate. This page evaluates the heat capacity, enthalpy, entropy and Gibbs energy of named gases as functions of temperature from the standard seven-coefficient polynomials, turns them into per-mass quantities, and uses them to follow the heat ratio and the speed of sound of air through a range of temperatures.

## How it works

Each species carries two sets of seven coefficients, one for temperatures below a midpoint (usually `1000` K) and one above it. With {math}`R` the molar gas constant and {math}`T` the temperature,

```{math}
\frac{c_p}{R} = a_0 + a_1 T + a_2 T^2 + a_3 T^3 + a_4 T^4, \qquad \frac{h}{RT} = a_0 + \frac{a_1}{2}T + \frac{a_2}{3}T^2 + \frac{a_3}{4}T^3 + \frac{a_4}{5}T^4 + \frac{a_5}{T}
```

```{math}
\frac{s}{R} = a_0\ln T + a_1 T + \frac{a_2}{2}T^2 + \frac{a_3}{3}T^3 + \frac{a_4}{4}T^4 + a_6, \qquad g = h - Ts
```

You describe a species with a `thermo::SpeciesData`: a name, the atoms it is made of, its molar mass in kg/mol, the lowest, middle and highest temperatures of validity, and the two coefficient sets `low` and `high`. `thermo::Nasa7(list_of_species, like)` builds the model for several species at once, with `like` a Series that fixes the precision. `thermo::props(model, temperature)` returns the molar `cp`, `h`, `s` and `g`, where `temperature` is a `common::TemperaturePoint<Series>`; the result has a trailing species axis, so a temperature array of shape `[k]` gives arrays of shape `[k, species]`. `thermo::mass_props(model, temperature, molar_mass)` divides by the molar masses to give properties per kilogram. The model itself also offers `cp_r`, `h_rt`, `s_r` and `g_rt` on plain Series and `check(temperature)`, which refuses temperatures outside the validity range.

## Properties of three gases at three temperatures

The program defines nitrogen, oxygen and water vapour from their published coefficients and prints, for `300`, `1000` and `2000` K, each species' heat capacity, enthalpy, entropy and Gibbs energy. It ends by checking {math}`g = h - Ts`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace thermo = natsci::physics::thermo;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v, std::vector<std::int64_t> shape = {}) { auto s = shape.empty() ? pluss::series::make(v) : pluss::series::make(v, shape); return s.to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// NASA seven-coefficient polynomials: cp/R = a0 + a1 T + a2 T^2 + a3 T^3 + a4 T^4, with the enthalpy and entropy constants a5, a6
static thermo::SpeciesData nitrogen() {
  thermo::SpeciesData s; s.name = "N2"; s.composition = {{"N", 2.0}}; s.molar_mass = 0.0280134; s.t_min = 300.0; s.t_mid = 1000.0; s.t_max = 5000.0;
  s.low = {3.29867700, 1.40824040e-3, -3.96322200e-6, 5.64151500e-9, -2.44485400e-12, -1.02089990e3, 3.95037200};
  s.high = {2.92664000, 1.48797680e-3, -5.68476000e-7, 1.00970380e-10, -6.75335100e-15, -9.22797700e2, 5.98052800};
  return s;
}
static thermo::SpeciesData oxygen() {
  thermo::SpeciesData s; s.name = "O2"; s.composition = {{"O", 2.0}}; s.molar_mass = 0.0319988; s.t_min = 300.0; s.t_mid = 1000.0; s.t_max = 5000.0;
  s.low = {3.78245636, -2.99673416e-3, 9.84730201e-6, -9.68129509e-9, 3.24372837e-12, -1.06394356e3, 3.65767573};
  s.high = {3.28253784, 1.48308754e-3, -7.57966669e-7, 2.09470555e-10, -2.16717794e-14, -1.08845772e3, 5.45323129};
  return s;
}
static thermo::SpeciesData water() {
  thermo::SpeciesData s; s.name = "H2O"; s.composition = {{"H", 2.0}, {"O", 1.0}}; s.molar_mass = 0.01801528; s.t_min = 300.0; s.t_mid = 1000.0; s.t_max = 5000.0;
  s.low = {4.19864056, -2.03643410e-3, 6.52040211e-6, -5.48797062e-9, 1.77197817e-12, -3.02937267e4, -8.49032208e-1};
  s.high = {3.03399249, 2.17691804e-3, -1.64072518e-7, -9.70419870e-11, 1.68200992e-14, -3.00042971e4, 4.96677010};
  return s;
}
static thermo::Nasa7 gases() { return thermo::Nasa7({nitrogen(), oxygen(), water()}, vec({300.0})); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const auto model = gases();
  out.writeln("species = ", model.species(), ", valid from ", model.t_min(), " K to ", model.t_max(), " K");
  const auto temperature = common::TemperaturePoint<pluss::Series>::from_si(vec({300.0, 1000.0, 2000.0}));
  const auto p = thermo::props(model, temperature);
  // one row per temperature, one column per species: N2, O2, H2O
  const auto cp = flat(p.cp.si_value()), h = flat(p.h.si_value()), s = flat(p.s.si_value()), g = flat(p.g.si_value());
  const double T[3] = {300.0, 1000.0, 2000.0};
  for (int i = 0; i < 3; ++i) {
    out.writeln("T = ", T[i], " K");
    out.writeln("  cp (J/mol/K) = ", std::vector<double>{cp[3 * i], cp[3 * i + 1], cp[3 * i + 2]});
    out.writeln("  h (kJ/mol)   = ", std::vector<double>{h[3 * i] / 1e3, h[3 * i + 1] / 1e3, h[3 * i + 2] / 1e3});
    out.writeln("  s (J/mol/K)  = ", std::vector<double>{s[3 * i], s[3 * i + 1], s[3 * i + 2]});
    out.writeln("  g (kJ/mol)   = ", std::vector<double>{g[3 * i] / 1e3, g[3 * i + 1] / 1e3, g[3 * i + 2] / 1e3});
  }
  out.writeln("g - (h - T s) for water at 2000 K = ", g[8] - (h[8] - 2000.0 * s[8]));
}
```

Output

```text
species = 3, valid from 300.000 K to 5000.000 K
T = 300.000 K
  cp (J/mol/K) = [29.075, 29.388, 33.596]
  h (kJ/mol)   = [0.055, 0.054, -241.762]
  s (J/mol/K)  = [191.692, 205.330, 189.036]
  g (kJ/mol)   = [-57.452, -61.545, -298.473]
T = 1000.000 K
  cp (J/mol/K) = [32.762, 34.883, 41.295]
  h (kJ/mol)   = [21.470, 22.707, -215.822]
  s (J/mol/K)  = [228.089, 243.586, 232.735]
  g (kJ/mol)   = [-206.619, -220.880, -448.557]
T = 2000.000 K
  cp (J/mol/K) = [35.988, 37.796, 51.752]
  h (kJ/mol)   = [56.132, 59.205, -168.788]
  s (J/mol/K)  = [251.985, 268.770, 264.916]
  g (kJ/mol)   = [-447.839, -478.335, -698.619]
g - (h - T s) for water at 2000 K = 0.000
```

The model has `3` species valid from `300` K to `5000` K. At `300` K the heat capacities are `29.075`, `29.388` and `33.596` J/mol/K, close to the ideal-gas values for two-atom and three-atom molecules. By `2000` K they have grown to `35.988`, `37.796` and `51.752`: water, with its many vibrations, gains the most. The enthalpies are measured from the elements, so nitrogen and oxygen are about zero at `300` K (`0.055` and `0.054` kJ/mol) and water vapour is `-241.762` kJ/mol, the heat released by forming it. The Gibbs energy identity holds to `0.000` for water at `2000` K.

## Per kilogram, and the heat ratio of air

The program passes the molar masses to get properties per kilogram at four temperatures, and then mixes nitrogen and oxygen in the molar proportions {math}`79\,\%` and {math}`21\,\%` to get the heat ratio {math}`\gamma = c_p/(c_p - R)` of air and the speed of sound {math}`\sqrt{\gamma R_s T}`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace thermo = natsci::physics::thermo;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v, std::vector<std::int64_t> shape = {}) { auto s = shape.empty() ? pluss::series::make(v) : pluss::series::make(v, shape); return s.to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// NASA seven-coefficient polynomials: cp/R = a0 + a1 T + a2 T^2 + a3 T^3 + a4 T^4, with the enthalpy and entropy constants a5, a6
static thermo::SpeciesData nitrogen() {
  thermo::SpeciesData s; s.name = "N2"; s.composition = {{"N", 2.0}}; s.molar_mass = 0.0280134; s.t_min = 300.0; s.t_mid = 1000.0; s.t_max = 5000.0;
  s.low = {3.29867700, 1.40824040e-3, -3.96322200e-6, 5.64151500e-9, -2.44485400e-12, -1.02089990e3, 3.95037200};
  s.high = {2.92664000, 1.48797680e-3, -5.68476000e-7, 1.00970380e-10, -6.75335100e-15, -9.22797700e2, 5.98052800};
  return s;
}
static thermo::SpeciesData oxygen() {
  thermo::SpeciesData s; s.name = "O2"; s.composition = {{"O", 2.0}}; s.molar_mass = 0.0319988; s.t_min = 300.0; s.t_mid = 1000.0; s.t_max = 5000.0;
  s.low = {3.78245636, -2.99673416e-3, 9.84730201e-6, -9.68129509e-9, 3.24372837e-12, -1.06394356e3, 3.65767573};
  s.high = {3.28253784, 1.48308754e-3, -7.57966669e-7, 2.09470555e-10, -2.16717794e-14, -1.08845772e3, 5.45323129};
  return s;
}
static thermo::SpeciesData water() {
  thermo::SpeciesData s; s.name = "H2O"; s.composition = {{"H", 2.0}, {"O", 1.0}}; s.molar_mass = 0.01801528; s.t_min = 300.0; s.t_mid = 1000.0; s.t_max = 5000.0;
  s.low = {4.19864056, -2.03643410e-3, 6.52040211e-6, -5.48797062e-9, 1.77197817e-12, -3.02937267e4, -8.49032208e-1};
  s.high = {3.03399249, 2.17691804e-3, -1.64072518e-7, -9.70419870e-11, 1.68200992e-14, -3.00042971e4, 4.96677010};
  return s;
}
static thermo::Nasa7 gases() { return thermo::Nasa7({nitrogen(), oxygen(), water()}, vec({300.0})); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto model = gases();
  const auto temperature = common::TemperaturePoint<pluss::Series>::from_si(vec({300.0, 800.0, 1500.0, 2500.0}));
  // the same properties per kilogram: one molar mass for each species
  const auto masses = common::SeriesQuantity<thermo::MolarMass>::from_si(vec({0.0280134, 0.0319988, 0.01801528}));
  const auto m = thermo::mass_props(model, temperature, masses);
  const auto cp = flat(m.cp.si_value());
  out.writeln("temperature (K)       = ", std::vector<double>{300.0, 800.0, 1500.0, 2500.0});
  std::vector<double> n2, o2, h2o;
  for (int i = 0; i < 4; ++i) { n2.push_back(cp[3 * i]); o2.push_back(cp[3 * i + 1]); h2o.push_back(cp[3 * i + 2]); }
  out.writeln("cp of N2 (J/kg/K)     = ", n2);
  out.writeln("cp of O2 (J/kg/K)     = ", o2);
  out.writeln("cp of H2O (J/kg/K)    = ", h2o);
  // air is about 79 % N2 and 21 % O2 by mole; its heat ratio falls as it gets hot, where a constant 1.4 would be wrong
  const auto molar = thermo::props(model, temperature);
  const auto mc = flat(molar.cp.si_value());
  const double r = 8.31446261815324;
  std::vector<double> gamma, sound;
  const double T[4] = {300.0, 800.0, 1500.0, 2500.0};
  for (int i = 0; i < 4; ++i) {
    const double cp_air = 0.79 * mc[3 * i] + 0.21 * mc[3 * i + 1];
    const double g = cp_air / (cp_air - r);
    gamma.push_back(g);
    sound.push_back(std::sqrt(g * (r / 0.02884) * T[i]));
  }
  out.writeln("heat ratio of air     = ", gamma);
  out.writeln("speed of sound (m/s)  = ", sound);
}
```

Output

```text
temperature (K)       = [300.0000, 800.0000, 1500.0000, 2500.0000]
cp of N2 (J/kg/K)     = [1037.9134, 1120.6840, 1242.4533, 1308.1495]
cp of O2 (J/kg/K)     = [918.4117, 1054.7162, 1143.0201, 1215.8769]
cp of H2O (J/kg/K)    = [1864.8864, 2150.0095, 2625.0686, 3042.1684]
heat ratio of air     = [1.3992, 1.3527, 1.3095, 1.2886]
speed of sound (m/s)  = [347.8752, 558.5522, 752.5250, 963.7283]
```

Per kilogram the heat capacity of nitrogen rises from `1037.9134` J/kg/K at `300` K to `1308.1495` at `2500` K, and that of water vapour, a lighter molecule, is the largest in every column. For air the heat ratio is `1.3992` at `300` K, the familiar `1.4`, but only `1.2886` at `2500` K; a constant `1.4` would be wrong by `8` per cent. The speed of sound is `347.8752` m/s at `300` K, `558.5522` at `800` K and `963.7283` at `2500` K.

## Continuity, shapes and consistency

The program evaluates water just below and just above the `1000` K midpoint, where the two polynomials meet, passes a `[2, 3]` field of temperatures, and compares the model's Gibbs energy with {math}`h/RT - s/R` for nitrogen.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace thermo = natsci::physics::thermo;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v, std::vector<std::int64_t> shape = {}) { auto s = shape.empty() ? pluss::series::make(v) : pluss::series::make(v, shape); return s.to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// NASA seven-coefficient polynomials: cp/R = a0 + a1 T + a2 T^2 + a3 T^3 + a4 T^4, with the enthalpy and entropy constants a5, a6
static thermo::SpeciesData nitrogen() {
  thermo::SpeciesData s; s.name = "N2"; s.composition = {{"N", 2.0}}; s.molar_mass = 0.0280134; s.t_min = 300.0; s.t_mid = 1000.0; s.t_max = 5000.0;
  s.low = {3.29867700, 1.40824040e-3, -3.96322200e-6, 5.64151500e-9, -2.44485400e-12, -1.02089990e3, 3.95037200};
  s.high = {2.92664000, 1.48797680e-3, -5.68476000e-7, 1.00970380e-10, -6.75335100e-15, -9.22797700e2, 5.98052800};
  return s;
}
static thermo::SpeciesData oxygen() {
  thermo::SpeciesData s; s.name = "O2"; s.composition = {{"O", 2.0}}; s.molar_mass = 0.0319988; s.t_min = 300.0; s.t_mid = 1000.0; s.t_max = 5000.0;
  s.low = {3.78245636, -2.99673416e-3, 9.84730201e-6, -9.68129509e-9, 3.24372837e-12, -1.06394356e3, 3.65767573};
  s.high = {3.28253784, 1.48308754e-3, -7.57966669e-7, 2.09470555e-10, -2.16717794e-14, -1.08845772e3, 5.45323129};
  return s;
}
static thermo::SpeciesData water() {
  thermo::SpeciesData s; s.name = "H2O"; s.composition = {{"H", 2.0}, {"O", 1.0}}; s.molar_mass = 0.01801528; s.t_min = 300.0; s.t_mid = 1000.0; s.t_max = 5000.0;
  s.low = {4.19864056, -2.03643410e-3, 6.52040211e-6, -5.48797062e-9, 1.77197817e-12, -3.02937267e4, -8.49032208e-1};
  s.high = {3.03399249, 2.17691804e-3, -1.64072518e-7, -9.70419870e-11, 1.68200992e-14, -3.00042971e4, 4.96677010};
  return s;
}
static thermo::Nasa7 gases() { return thermo::Nasa7({nitrogen(), oxygen(), water()}, vec({300.0})); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto model = gases();
  // the polynomial changes at 1000 K; the two pieces fit each other to the digits of the coefficients
  const auto below = model.eval(vec({999.999}));
  const auto above = model.eval(vec({1000.001}));
  out.writeln("cp/R just below and above 1000 K, water: ", flat(below.cp_r)[2], " and ", flat(above.cp_r)[2]);
  out.writeln("h/RT just below and above 1000 K, water: ", flat(below.h_rt)[2], " and ", flat(above.h_rt)[2]);
  // a field of temperatures keeps its shape; the species axis is appended
  const auto field = model.eval(vec({400.0, 600.0, 800.0, 1200.0, 1600.0, 2000.0}, {2, 3}));
  out.writeln("shape of cp/R for a [2, 3] temperature field = ", field.cp_r.sizes());
  out.writeln("g/RT from the model for N2 at 400 K: ", flat(model.g_rt(vec({400.0})))[0], ", from h/RT - s/R: ", flat(model.h_rt(vec({400.0})))[0] - flat(model.s_r(vec({400.0})))[0]);
  // heat capacity of water vapour across the range: it grows as vibrations are excited
  out.writeln("cp of water vapour (J/mol/K) at 300, 600, 1200, 2400 K = ", flat(model.cp_r(vec({300.0, 600.0, 1200.0, 2400.0})).select(-1, 2).mul(8.31446261815324)));
}
```

Output

```text
cp/R just below and above 1000 K, water: 4.9666 and 4.9666
h/RT just below and above 1000 K, water: -25.9575 and -25.9574
shape of cp/R for a [2, 3] temperature field = [2, 3, 3]
g/RT from the model for N2 at 400 K: -23.1708, from h/RT - s/R: -23.1708
cp of water vapour (J/mol/K) at 300, 600, 1200, 2400 K = [33.5965, 36.3207, 43.8772, 54.2941]
```

The two polynomials agree at the midpoint: {math}`c_p/R` is `4.9666` on both sides, and {math}`h/RT` differs by `0.0001` in the fourth decimal, which is the rounding of the published coefficients. A temperature field of shape `[2, 3]` gives `[2, 3, 3]`: the species axis is appended at the end. The model's {math}`g/RT` for nitrogen at `400` K, `-23.1708`, equals {math}`h/RT - s/R`. The heat capacity of water vapour climbs from `33.5965` J/mol/K at `300` K to `54.2941` at `2400` K.

## Decisions

- Call `model.check(temperature)` on the data when it comes from outside your program: `props` and `eval` extend the polynomials past their range without a message, because they are smooth and the extension is often a good approximation, but the fit is only guaranteed inside the validity range.
- Use `mass_props` when the next step is a fluid or a reactor model, which works per kilogram, and `props` when the next step is a chemical balance, which works per mole.
- Build the model once for all the species you need: evaluating three species costs about the same as one, and the property arrays keep one species axis.
- Give the model and the temperature the same precision. The `like` argument that you pass when you build the model sets it.

## Pitfalls

The usual slips, all tried in one program with the namespace spelled out: a temperature beyond the fit, a temperature typed in the wrong scale, a precision that differs between model and data, a repeated species name, and a molar mass in the wrong unit.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace thermo = natsci::physics::thermo;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v, std::vector<std::int64_t> shape = {}) { auto s = shape.empty() ? pluss::series::make(v) : pluss::series::make(v, shape); return s.to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// NASA seven-coefficient polynomials: cp/R = a0 + a1 T + a2 T^2 + a3 T^3 + a4 T^4, with the enthalpy and entropy constants a5, a6
static thermo::SpeciesData nitrogen() {
  pluss::ma::natsci::physics::thermo::SpeciesData s; s.name = "N2"; s.composition = {{"N", 2.0}}; s.molar_mass = 0.0280134; s.t_min = 300.0; s.t_mid = 1000.0; s.t_max = 5000.0;
  s.low = {3.29867700, 1.40824040e-3, -3.96322200e-6, 5.64151500e-9, -2.44485400e-12, -1.02089990e3, 3.95037200};
  s.high = {2.92664000, 1.48797680e-3, -5.68476000e-7, 1.00970380e-10, -6.75335100e-15, -9.22797700e2, 5.98052800};
  return s;
}
static thermo::SpeciesData oxygen() {
  pluss::ma::natsci::physics::thermo::SpeciesData s; s.name = "O2"; s.composition = {{"O", 2.0}}; s.molar_mass = 0.0319988; s.t_min = 300.0; s.t_mid = 1000.0; s.t_max = 5000.0;
  s.low = {3.78245636, -2.99673416e-3, 9.84730201e-6, -9.68129509e-9, 3.24372837e-12, -1.06394356e3, 3.65767573};
  s.high = {3.28253784, 1.48308754e-3, -7.57966669e-7, 2.09470555e-10, -2.16717794e-14, -1.08845772e3, 5.45323129};
  return s;
}
static thermo::SpeciesData water() {
  pluss::ma::natsci::physics::thermo::SpeciesData s; s.name = "H2O"; s.composition = {{"H", 2.0}, {"O", 1.0}}; s.molar_mass = 0.01801528; s.t_min = 300.0; s.t_mid = 1000.0; s.t_max = 5000.0;
  s.low = {4.19864056, -2.03643410e-3, 6.52040211e-6, -5.48797062e-9, 1.77197817e-12, -3.02937267e4, -8.49032208e-1};
  s.high = {3.03399249, 2.17691804e-3, -1.64072518e-7, -9.70419870e-11, 1.68200992e-14, -3.00042971e4, 4.96677010};
  return s;
}
static thermo::Nasa7 gases() { return thermo::Nasa7({nitrogen(), oxygen(), water()}, vec({300.0})); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto model = gases();
  // outside the declared range the polynomial is still evaluated, silently: check() is what enforces the range
  out.writeln("cp of N2 at 6000 K, no check: ", flat(model.cp_r(vec({6000.0})))[0] * 8.314, " J/mol/K");
  try { model.check(vec({6000.0})); } catch (const pluss::Error &e) { out.writeln("check at 6000 K: ", e.message()); }
  try { model.check(vec({-5.0})); } catch (const pluss::Error &e) { out.writeln("check at -5 K: ", e.message()); }
  // a value meant as 25 degrees Celsius but typed as kelvin is far outside the fitted range, and props accepts it
  const auto cold = common::TemperaturePoint<pluss::Series>::from_si(vec({25.0}));
  out.writeln("25 K accepted: cp of N2 = ", flat(pluss::ma::natsci::physics::thermo::props(model, cold).cp.si_value())[0], " J/mol/K");
  // a model built in double precision with a single-precision temperature
  try { (void)model.cp_r(vec({300.0}).to(pluss::ScalarType::Float32)); } catch (const pluss::Error &e) { out.writeln("dtype: ", e.message()); }
  // two species of the same name, and a molar mass in g/mol instead of kg/mol
  try { (void)pluss::ma::natsci::physics::thermo::Nasa7({nitrogen(), nitrogen()}, vec({300.0})); } catch (const pluss::Error &e) { out.writeln("duplicate: ", e.message()); }
  const auto grams = common::ScalarQuantity<pluss::ma::natsci::physics::thermo::MolarMass>::from_si(28.0134);
  const auto temperature = common::TemperaturePoint<pluss::Series>::from_si(vec({300.0}));
  const auto one = pluss::ma::natsci::physics::thermo::Nasa7({nitrogen()}, vec({300.0}));
  out.writeln("cp of N2 per mass with the molar mass in g/mol (J/kg/K): ", flat(pluss::ma::natsci::physics::thermo::mass_props(one, temperature, grams).cp.si_value())[0], " instead of ", flat(pluss::ma::natsci::physics::thermo::mass_props(one, temperature, common::ScalarQuantity<pluss::ma::natsci::physics::thermo::MolarMass>::from_si(0.0280134)).cp.si_value())[0]);
}
```

Output

```text
cp of N2 at 6000 K, no check: 36.9692 J/mol/K
check at 6000 K: natsci::thermo: temperature must be inside the common NASA validity range [300.000000, 5000.000000] K
check at -5 K: natsci::thermo: temperature must be finite and strictly positive
25 K accepted: cp of N2 = 27.6996 J/mol/K
dtype: natsci::thermo: temperature dtype must match the model dtype
duplicate: natsci::thermo: species names must be unique: N2
cp of N2 per mass with the molar mass in g/mol (J/kg/K): 1.0379 instead of 1037.9134
```

A temperature of `6000` K, above the fitted range, is evaluated without complaint (`36.9692` J/mol/K for nitrogen), and only `check` refuses it, with the valid range in the message; a non-positive temperature is refused by `check` too. A value of `25` that was meant as degrees Celsius but is typed in kelvin is accepted by `props` as a very cold gas: build temperatures with `TemperaturePoint<>::from(25.0, celsius)`. A temperature of the wrong precision, or a species listed twice, is refused with a clear message. A molar mass in g/mol instead of kg/mol gives `1.0379` J/kg/K instead of `1037.9134`: nothing can refuse it, because both are positive numbers.

## Where next

[Physics 4: Thermodynamics](../phys_04_thermodynamics.md) treats the ideal gas with a constant heat ratio, and the chemistry guide starts from the same polynomials to compute equilibria, where the Gibbs energy of every species enters.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
