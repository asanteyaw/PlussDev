# Mixture properties

[Back to Chemistry 22: Gas-phase kinetics](../chem_22_gas_kinetics.md)

The same species data that gives the rates also describes the gas as a mixture: how heavy it is, how much energy it stores, how fast sound travels in it. This page builds the ideal-gas model of a mechanism's mixture and uses it to compute the density and the heat capacities, to carry the energy of a burning mixture through a reaction and recover its temperature, and to sweep the properties across a family of compositions.

## How it works

For a mixture with mass fractions {math}`Y_k` (they sum to one) and species molar masses {math}`M_k`, the ideal-gas relations are

```{math}
\frac{1}{M} = \sum_k \frac{Y_k}{M_k}, \qquad p = \frac{\rho R T}{M}, \qquad e = \sum_k Y_k\,\frac{RT}{M_k}\left(\frac{h_k}{RT} - 1\right),
```

with the heat capacities {math}`c_v = \sum_k Y_k (R/M_k)(c_{p,k}/R - 1)` and {math}`c_p = \sum_k Y_k (R/M_k)\,c_{p,k}/R`, the ratio {math}`\gamma = c_p/c_v` and the speed of sound {math}`a = \sqrt{\gamma p/\rho}`. The enthalpy and entropy of each species come from the NASA-7 polynomials, so the energy contains the heat of formation and a reaction that changes the composition changes the temperature at fixed energy.

`pluss::ma::natsci::chemistry::thermo::IdealGas(mechanism)` builds the model from a mechanism that carries molar masses. Its methods take a temperature (or density, or pressure) Series of shape `[...]` and a mass-fraction Series of shape `[..., S]`: `mean_molar_mass(y)`, `concentrations(density, y)`, `density(pressure, t, y)`, `pressure(density, t, y)`, `properties(t, y)` (fields `mean_molar_mass`, `internal_energy`, `enthalpy`, `cv`, `cp` and the method `gamma()`), `internal_energy(t, y)`, `enthalpy(t, y)`, `sound_speed(t, y)` and `temperature(e, y, guess)`. The last inverts the energy for the temperature, with a safeguarded Newton iteration over the whole range of the data, and takes an optional starting guess, for example the previous temperature. The accessors `species()`, `names()`, `molar_masses()`, `t_min()` and `t_max()` describe the model. The energies are per kilogram, the pressure is in pascals, and the Series must have the dtype of the mechanism.

## Density, heat capacity and sound speed

A mixture of `2` mol of hydrogen, `1` mol of oxygen and `7` mol of argon, converted to mass fractions, at `400`, `800` and `1600` K and `1` atm. The program prints the mean molar mass, the density, the pressure recovered from it, the heat capacities, `gamma` and the speed of sound.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
namespace reactor = chemistry::reactor;
namespace thermo = chemistry::thermo;
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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto mech = hydrogen();
  const thermo::IdealGas gas(mech);
  // species order H2, H, O, O2, OH, H2O, AR. Mass fractions of a mixture made of 2 mol H2, 1 mol O2 and 7 mol argon
  const double m_h2 = 2.0 * 2.016e-3, m_o2 = 31.998e-3, m_ar = 7.0 * 39.95e-3, m = m_h2 + m_o2 + m_ar;
  const auto y = vec({m_h2 / m, 0.0, 0.0, m_o2 / m, 0.0, 0.0, m_ar / m});
  out.writeln("mass fractions            ", flat(y));
  out.writeln("mean molar mass (kg/mol)  ", flat(gas.mean_molar_mass(y)));
  // density at 1 atm for three temperatures: rho = p M / (R T); lanes are [temperature]
  const auto t = vec({400.0, 800.0, 1600.0});
  const auto yy = y.unsqueeze(0).expand({3, 7}, true).contiguous();
  const auto rho = gas.density(vec({101325.0, 101325.0, 101325.0}), t, yy);
  out.writeln("density (kg/m^3)          ", flat(rho));
  out.writeln("pressure back (Pa)        ", flat(gas.pressure(rho, t, yy)));
  // heat capacities per kilogram and the ratio of specific heats
  const auto p = gas.properties(t, yy);
  out.writeln("cv (J/kg/K)               ", flat(p.cv));
  out.writeln("cp (J/kg/K)               ", flat(p.cp));
  out.writeln("gamma                     ", flat(p.gamma()));
  out.writeln("sound speed (m/s)         ", flat(gas.sound_speed(t, yy)));
}
```

Output

```text
mass fractions            [0.0128, 0.0000, 0.0000, 0.1014, 0.0000, 0.0000, 0.8859]
mean molar mass (kg/mol)  [0.0316]
density (kg/m^3)          [0.9618, 0.4809, 0.2404]
pressure back (Pa)        [101325.0000, 101325.0000, 101325.0000]
cv (J/kg/K)               [478.4425, 492.0798, 521.3977]
cp (J/kg/K)               [741.8252, 755.4625, 784.7803]
gamma                     [1.5505, 1.5352, 1.5051]
sound speed (m/s)         [404.1657, 568.7576, 796.4216]
```

The mean molar mass is `0.0316` kg/mol. The density is `0.9618` kg/m³ at `400` K and half of that for each doubling of the temperature, and the pressure recovered from the density is exactly `101325` Pa. The heat capacities rise with temperature (`cv` from `478` to `521`) and `gamma` falls from `1.55` to `1.51`, the usual trend as more internal motions come alive; the sound speed climbs from `404` to `796` m/s.

## Energy conservation through a reaction

The stoichiometric mixture at `1200` K and `1` atm in a rigid vessel is burnt in the reactor. The program computes the internal energy per kilogram at the start, converts the burnt concentrations to mass fractions, and recovers the temperature that goes with that same energy and the new composition.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
namespace reactor = chemistry::reactor;
namespace thermo = chemistry::thermo;
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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto mech = hydrogen();
  const thermo::IdealGas gas(mech);
  // the same stoichiometric mixture as the reactor example, in a rigid vessel at 1200 K and 1 atm
  const double total = 101325.0 / (thermo::gas_constant() * 1200.0);
  const auto c0 = vec({0.2 * total, 1e-8 * total, 0.0, 0.1 * total, 0.0, 0.0, 0.7 * total}).reshape({1, 7});
  const auto masses = gas.molar_masses();
  const auto rho = c0.mul(masses).sum(-1, false);                  // kg/m^3
  const auto y0 = c0.mul(masses).div(rho.unsqueeze(-1));           // mass fractions
  const auto e0 = gas.internal_energy(vec({1200.0}), y0);          // J/kg
  out.writeln("density (kg/m^3)               ", flat(rho));
  out.writeln("internal energy (J/kg)          ", flat(e0));
  // burn the mixture in the reactor and convert the burnt concentrations back to mass fractions
  const auto burnt = reactor::integrate(mech, reactor::make_state(vec({1200.0}), c0), 1e-2);
  const auto c1 = reactor::concentrations(burnt.y);
  const auto y1 = c1.mul(masses).div(c1.mul(masses).sum(-1, false).unsqueeze(-1));
  out.writeln("burnt mass fractions H2O, OH    ", std::vector<double>{flat(y1.select(-1, 5))[0], flat(y1.select(-1, 4))[0]});
  out.writeln("reactor temperature (K)         ", flat(reactor::temperature(burnt.y)));
  // a rigid adiabatic vessel keeps e fixed: the temperature recovered from e0 and the burnt composition is the reactor's
  out.writeln("temperature from e0 and burnt Y ", flat(gas.temperature(e0, y1)));
  // the mass density is unchanged, so is the mass: only the pressure moves
  out.writeln("pressure before / after (Pa)    ", std::vector<double>{flat(gas.pressure(rho, vec({1200.0}), y0))[0], flat(gas.pressure(rho, gas.temperature(e0, y1), y1))[0]});
}
```

Output

```text
density (kg/m^3)               [0.3206]
internal energy (J/kg)          [363708.9699]
burnt mass fractions H2O, OH    [0.0843, 0.0121]
reactor temperature (K)         [2951.2463]
temperature from e0 and burnt Y [2951.2463]
pressure before / after (Pa)    [101325.0010, 233563.9556]
```

The mixture has a density of `0.3206` kg/m³ and an energy of `363709` J/kg. After the burn it contains `8.4%` water and `1.2%` hydroxyl by mass. The temperature recovered from the unchanged energy and the burnt composition, `2951.2463` K, is the reactor's own result to every digit shown. The mass in a rigid vessel is fixed, so the pressure follows the temperature and the mole count: from `101325` to `233564` Pa.

## Sweeping a family of compositions

Five mixtures of hydrogen and argon at `500` K with the hydrogen mole fraction from `0` to `1`, converted to mass fractions inside the program and run as the lanes of one batch.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
namespace reactor = chemistry::reactor;
namespace thermo = chemistry::thermo;
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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto mech = hydrogen();
  const thermo::IdealGas gas(mech);
  // five mixtures of hydrogen and argon, with the hydrogen mole fraction x running from 0 to 1: the lanes are the mixtures
  const auto x_h2 = vec({0.0, 0.25, 0.5, 0.75, 1.0});
  const auto zeros = x_h2.mul(0.0);
  // mole fractions in the species order H2, H, O, O2, OH, H2O, AR, converted to mass fractions Y_k = x_k M_k / sum x M
  const auto x = x_h2.unsqueeze(-1).unite(std::vector<pluss::Series>{zeros.unsqueeze(-1), zeros.unsqueeze(-1), zeros.unsqueeze(-1), zeros.unsqueeze(-1), zeros.unsqueeze(-1), x_h2.neg().add(1.0).unsqueeze(-1)}, -1);
  const auto weighted = x.mul(gas.molar_masses());
  const auto y = weighted.div(weighted.sum(-1, false).unsqueeze(-1));
  const auto t = x_h2.mul(0.0).add(500.0);
  const auto p = gas.properties(t, y);
  out.writeln("x(H2)               ", flat(x_h2));
  out.writeln("mean molar mass     ", flat(gas.mean_molar_mass(y)));
  out.writeln("gamma               ", flat(p.gamma()));
  out.writeln("sound speed (m/s)   ", flat(gas.sound_speed(t, y)));
  out.writeln("density at 1 atm    ", flat(gas.density(x_h2.mul(0.0).add(101325.0), t, y)));
}
```

Output

```text
x(H2)               [0.0000, 0.2500, 0.5000, 0.7500, 1.0000]
mean molar mass     [0.0399, 0.0305, 0.0210, 0.0115, 0.0020]
gamma               [1.6667, 1.5695, 1.4971, 1.4410, 1.3962]
sound speed (m/s)   [416.4550, 462.7768, 544.6120, 721.7533, 1696.8267]
density at 1 atm    [0.9737, 0.7426, 0.5114, 0.2803, 0.0491]
```

The mean molar mass falls from `0.0399` (argon) to `0.0020` kg/mol (hydrogen). Argon has `gamma = 1.6667`, the monatomic value `5/3`, and hydrogen at this temperature `1.3962`. The speed of sound grows from `416` to `1697` m/s as the light gas takes over, while the density at one atmosphere falls from `0.974` to `0.049` kg/m³.

## Decisions

- Pass mass fractions, not mole fractions. Convert with {math}`Y_k = x_k M_k / \sum_j x_j M_j` using `molar_masses()`, as the last example does.
- Give `temperature(e, y, guess)` the temperature of the previous step as a guess when you update a state a little at a time; without a guess it starts from an interpolation between the ends of the data range, which is also robust.
- Build the model from the mechanism you integrate with, so that the energy it assigns to a state is the one the reactor conserves.
- Keep temperatures inside `t_min()` to `t_max()` of the model; the thermodynamic data are not extrapolated.

## Pitfalls

The inputs are validated: mass fractions must be non-negative and sum to one in every lane, the lanes must match in shape, the temperature must lie in the data range and the dtype must be that of the mechanism. A set of mole fractions that happens to sum to one is accepted, and describes a different gas.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <functional>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace kinetics = chemistry::kinetics;
namespace reactor = chemistry::reactor;
namespace thermo = chemistry::thermo;
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

static void attempt(OutputEngine &out, const char *label, const std::function<void()> &call) {
  try { call(); out.writeln(label, "  no error"); } catch (const pluss::Error &e) { out.writeln(label, "  ", e.message()); }
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto mech = hydrogen();
  const thermo::IdealGas gas(mech);
  const auto t = vec({800.0});
  // mass fractions of 2 H2 + O2 + 7 Ar (species order H2, H, O, O2, OH, H2O, AR) next to the percentages and mole fractions a user may type by mistake
  const auto good = vec({0.0128, 0.0, 0.0, 0.1014, 0.0, 0.0, 0.8858}).reshape({1, 7});
  const auto percent = vec({20.0, 0.0, 0.0, 10.0, 0.0, 0.0, 70.0}).reshape({1, 7});
  out.writeln("mean molar mass from mass fractions    ", flat(gas.mean_molar_mass(good)));
  attempt(out, "percentages instead of fractions: ", [&] { (void)gas.mean_molar_mass(percent); });
  // mole fractions are not mass fractions: this sums to one, so it is accepted, and describes a different gas
  const auto mole = vec({0.2, 0.0, 0.0, 0.1, 0.0, 0.0, 0.7}).reshape({1, 7});
  out.writeln("mole fractions read as mass fractions  ", flat(gas.mean_molar_mass(mole)), "  (the mixture itself has 0.0316)");
  attempt(out, "temperature below the data:      ", [&] { (void)gas.properties(vec({250.0}), good); });
  attempt(out, "one temperature, two mixtures:   ", [&] { (void)gas.properties(vec({800.0}), good.unite(std::vector<pluss::Series>{good}, 0)); });
  attempt(out, "float32 against a float64 model: ", [&] { (void)gas.properties(t.to(pluss::ScalarType::Float32), good.to(pluss::ScalarType::Float32)); });
}
```

Output

```text
mean molar mass from mass fractions    [0.0316]
percentages instead of fractions:   natsci::chemistry::ideal_gas: mass fractions must sum to one in every lane
mole fractions read as mass fractions  [0.0083]  (the mixture itself has 0.0316)
temperature below the data:        natsci::thermo: temperature must be inside the common NASA validity range [300.000000, 3500.000000] K
one temperature, two mixtures:     natsci::chemistry::ideal_gas: temperature shape must match every non-species mass-fraction axis
float32 against a float64 model:   natsci::chemistry::ideal_gas: mass fractions must match the model floating dtype and platform
```

Percentages are refused because they sum to `100`, a temperature of `250` K is outside the data, a single temperature against two mixtures has the wrong shape and a single-precision input against a double-precision model has the wrong dtype. The mole fractions `0.2, 0.1, 0.7` are read as mass fractions without complaint and give a mean molar mass of `0.0083` kg/mol instead of `0.0316`.

## Where next

[The constant-volume reactor](closed_reactor.md) supplies the burnt states used above. [Reaction rates and equilibrium](reaction_rates.md) builds the mechanism that both models share. For the transport properties of the same gases, see [Pure gases](../chem_08_transport/pure_gases.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hydrogen_ignition.html">
    <span class="label">Previous</span>
    <span class="title">Hydrogen ignition</span>
  </a>
</div>
<!-- apiref:nav:end -->
