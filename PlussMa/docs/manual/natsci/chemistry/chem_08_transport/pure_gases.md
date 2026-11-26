# Viscosity and conductivity of pure gases

[Back to Chemistry 8: Gas Transport](../chem_08_transport.md)

You need the viscosity and the thermal conductivity of a gas at some temperature, and all you have are the molecule's size and the depth of its intermolecular potential. This page computes both from Lennard-Jones parameters, shows the collision integrals behind them, and explains why a molecule conducts heat better than an atom of the same viscosity.

## How it works

A species is described by `transport::SpeciesData`: the geometry (`0` for an atom, `1` for a linear molecule, `2` for a non-linear one), the Lennard-Jones well depth {math}`\varepsilon/k_B` in K and diameter {math}`\sigma` in angstrom, the dipole moment (which must be zero), the rotational relaxation number and the molar mass in kg/mol. `GasTransport(species, like)` builds the model for a list of them. The kinetic theory of dilute gases gives the viscosity

```{math}
\begin{aligned}
&\eta = \frac{5}{16}\frac{\sqrt{\pi m k_B T}}{\pi\sigma^2\,\Omega^{(2,2)*}(T^*)}, \\
&T^* = \frac{k_B T}{\varepsilon}
\end{aligned}
```

where {math}`\Omega^{(2,2)*}` is the reduced collision integral, which `omega22(T*)` evaluates, and `omega11(T*)` is its companion for diffusion. The thermal conductivity adds the heat carried by internal energy to the translational part through a modified Eucken correction, so it needs the heat capacity: `species_properties(T, cp_over_r)` takes {math}`T` of shape {math}`[\ldots]` and {math}`c_p/R` of shape {math}`[\ldots, \text{species}]` and returns `viscosity` and `conductivity` of shape {math}`[\ldots, \text{species}]`, in Pa s and W/(m K), together with the binary diffusion matrix of the next page. Every number is a function of {math}`T^*` alone, so a table over temperature is a leading axis.

## Viscosity and conductivity of five gases

The program builds a model for nitrogen, oxygen, argon, helium and hydrogen from their Lennard-Jones data, with {math}`c_p/R = 3.5` for the linear molecules and {math}`2.5` for the atoms, and evaluates the properties at {math}`300` K. It then repeats the evaluation at {math}`300`, {math}`600` and {math}`1200` K on a leading axis.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace tr = pluss::ma::natsci::chemistry::transport;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// Lennard-Jones data: geometry (0 atom, 1 linear), well depth / k_B in K, diameter in angstrom, dipole, rotational relaxation, molar mass in kg/mol
static std::vector<tr::SpeciesData> gases() {
  return {{"N2", 1, 97.53, 3.621, 0.0, 4.0, 28.014e-3},  {"O2", 1, 107.4, 3.458, 0.0, 3.8, 31.998e-3},
          {"Ar", 0, 136.5, 3.330, 0.0, 0.0, 39.948e-3},  {"He", 0, 10.2, 2.576, 0.0, 0.0, 4.0026e-3},
          {"H2", 1, 38.0, 2.920, 0.0, 280.0, 2.016e-3}};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const tr::GasTransport model(gases(), vec({300.0}));
  // reduced heat capacity cp/R of each species at 300 K: 3.5 for a linear molecule, 2.5 for an atom
  const auto cp = vec({3.5, 3.5, 2.5, 2.5, 3.5});
  const auto p = model.species_properties(vec({300.0}), cp);
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  out.writeln("species                      N2, O2, Ar, He, H2");
  out.writeln("viscosity (micro Pa s)       ", flat(p.viscosity.mul(1e6)));
  out.writeln("conductivity (mW / m / K)    ", flat(p.conductivity.mul(1e3)));
  // the same properties at three temperatures: a leading axis of temperatures gives [3, S]
  const auto q = model.species_properties(vec({300.0, 600.0, 1200.0}), mat({3.5, 3.5, 2.5, 2.5, 3.5, 3.5, 3.5, 2.5, 2.5, 3.5, 3.5, 3.5, 2.5, 2.5, 3.5}, 3, 5));
  out.writeln("N2 viscosity at 300, 600, 1200 K (micro Pa s) ", flat(q.viscosity.select(-1, 0).mul(1e6)));
  out.writeln("Ar viscosity at 300, 600, 1200 K (micro Pa s) ", flat(q.viscosity.select(-1, 2).mul(1e6)));
}
```

Output

```text
species                      N2, O2, Ar, He, H2
viscosity (micro Pa s)       [18.081, 20.650, 23.142, 19.858, 9.006]
conductivity (mW / m / K)    [26.354, 26.319, 18.062, 154.689, 188.504]
N2 viscosity at 300, 600, 1200 K (micro Pa s) [18.081, 29.619, 46.739]
Ar viscosity at 300, 600, 1200 K (micro Pa s) [23.142, 39.222, 62.707]
```

The viscosities, `18.081`, `20.650`, `23.142`, `19.858` and `9.006` micro Pa s, are the measured values for the five gases to within about one percent. Helium and hydrogen conduct heat several times better than the heavier gases, `154.689` and `188.504` mW/(m K) against `26.354` for nitrogen, because their light molecules move fast. At higher temperature the viscosity of a gas rises: nitrogen from `18.081` to `29.619` to `46.739`, argon from `23.142` to `62.707`, the opposite of a liquid.

## The collision integrals and the temperature dependence

The reduced collision integrals are evaluated at six reduced temperatures. The program then computes the viscosity of argon from {math}`200` to {math}`1600` K and the local exponent {math}`n` in {math}`\eta \propto T^n` between neighbouring temperatures.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace tr = pluss::ma::natsci::chemistry::transport;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// Lennard-Jones data: geometry (0 atom, 1 linear), well depth / k_B in K, diameter in angstrom, dipole, rotational relaxation, molar mass in kg/mol
static std::vector<tr::SpeciesData> gases() {
  return {{"N2", 1, 97.53, 3.621, 0.0, 4.0, 28.014e-3},  {"O2", 1, 107.4, 3.458, 0.0, 3.8, 31.998e-3},
          {"Ar", 0, 136.5, 3.330, 0.0, 0.0, 39.948e-3},  {"He", 0, 10.2, 2.576, 0.0, 0.0, 4.0026e-3},
          {"H2", 1, 38.0, 2.920, 0.0, 280.0, 2.016e-3}};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the collision integrals depend only on the reduced temperature T* = T / (epsilon / k_B)
  const auto tstar = vec({0.5, 1.0, 2.0, 5.0, 10.0, 50.0});
  out.writeln("reduced temperature  ", flat(tstar));
  out.writeln("omega(2,2)*          ", flat(tr::omega22(tstar)));
  out.writeln("omega(1,1)*          ", flat(tr::omega11(tstar)));
  // viscosity of argon over temperature, and the exponent in eta ~ T^n between neighbours
  const tr::GasTransport model(gases(), vec({300.0}));
  const auto t = vec({200.0, 400.0, 800.0, 1600.0});
  const auto cp = mat({2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5}, 4, 5);
  const auto eta = model.species_properties(t, cp).viscosity.select(-1, 2);
  out.writeln("argon viscosity at 200, 400, 800, 1600 K (micro Pa s) ", flat(eta.mul(1e6)));
  const auto v = flat(eta);
  out.writeln("local exponent between neighbours                      ", std::vector<double>{std::log2(v[1] / v[0]), std::log2(v[2] / v[1]), std::log2(v[3] / v[2])});
}
```

Output

```text
reduced temperature  [0.5000, 1.0000, 2.0000, 5.0000, 10.0000, 50.0000]
omega(2,2)*          [2.2836, 1.5931, 1.1761, 0.9259, 0.8241, 0.6492]
omega(1,1)*          [2.0675, 1.4405, 1.0754, 0.8431, 0.7419, 0.5758]
argon viscosity at 200, 400, 800, 1600 K (micro Pa s) [16.2138, 29.1068, 47.8780, 75.6524]
local exponent between neighbours                      [0.8441, 0.7180, 0.6600]
```

Both integrals fall steadily as the reduced temperature rises, {math}`\Omega^{(2,2)*}` from `2.2836` at {math}`T^* = 0.5` to `0.6492` at {math}`50`, because fast molecules feel the attractive well less. The viscosity grows faster than {math}`\sqrt T`, which hard spheres would give, because the integral is falling: the exponent is `0.8441` between {math}`200` and {math}`400` K, `0.7180` and then `0.6600` between {math}`800` and {math}`1600` K, tending towards {math}`0.5` as the temperature climbs.

## Why a molecule conducts better than an atom

For an atom, kinetic theory gives the conductivity from the viscosity alone, {math}`\lambda = \tfrac{15}{4}(R/M)\,\eta`. The program compares that estimate with the model's conductivity for all five gases, then gives nitrogen two more units of heat capacity.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace tr = pluss::ma::natsci::chemistry::transport;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// Lennard-Jones data: geometry (0 atom, 1 linear), well depth / k_B in K, diameter in angstrom, dipole, rotational relaxation, molar mass in kg/mol
static std::vector<tr::SpeciesData> gases() {
  return {{"N2", 1, 97.53, 3.621, 0.0, 4.0, 28.014e-3},  {"O2", 1, 107.4, 3.458, 0.0, 3.8, 31.998e-3},
          {"Ar", 0, 136.5, 3.330, 0.0, 0.0, 39.948e-3},  {"He", 0, 10.2, 2.576, 0.0, 0.0, 4.0026e-3},
          {"H2", 1, 38.0, 2.920, 0.0, 280.0, 2.016e-3}};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const tr::GasTransport model(gases(), vec({300.0}));
  const auto cp = vec({3.5, 3.5, 2.5, 2.5, 3.5});
  const auto sp = model.species_properties(vec({300.0}), cp);
  // conductivity of a monatomic gas against its viscosity: lambda = (15/4) (R / M) eta, the Eucken relation for atoms
  const double r = 8.31446261815324;
  const auto eucken = sp.viscosity.mul(3.75 * r).div(model.molar_masses());
  out.writeln("species                     N2, O2, Ar, He, H2");
  out.writeln("conductivity (mW / m / K)   ", flat(sp.conductivity.mul(1e3)));
  out.writeln("monatomic estimate          ", flat(eucken.mul(1e3)));
  out.writeln("ratio                       ", flat(sp.conductivity.div(eucken)));
  // the effect of the heat capacity: giving N2 extra internal degrees of freedom (cp/R = 4.5 at high temperature)
  const auto hot = model.species_properties(vec({300.0}), vec({4.5, 3.5, 2.5, 2.5, 3.5}));
  out.writeln("N2 conductivity with cp/R = 3.5 and 4.5 (mW / m / K) ", std::vector<double>{flat(sp.conductivity)[0] * 1e3, flat(hot.conductivity)[0] * 1e3});
}
```

Output

```text
species                     N2, O2, Ar, He, H2
conductivity (mW / m / K)   [26.354, 26.319, 18.062, 154.689, 188.504]
monatomic estimate          [20.124, 20.122, 18.062, 154.689, 139.291]
ratio                       [1.310, 1.308, 1.000, 1.000, 1.353]
N2 conductivity with cp/R = 3.5 and 4.5 (mW / m / K) [26.354, 33.398]
```

The ratio is exactly `1.000` for argon and helium, so the model reproduces the atomic result. For the molecules it is higher, `1.310` and `1.308` for nitrogen and oxygen and `1.353` for hydrogen: their internal degrees of freedom carry heat in addition to the translational energy. Raising {math}`c_p/R` of nitrogen from `3.5` to `4.5` takes its conductivity from `26.354` to `33.398` mW/(m K): the internal heat capacity enters directly, which is why a vibrating gas at high temperature conducts better.

## Decisions

- Pass the heat capacity at the temperature of the calculation: it is the one input that changes with {math}`T` besides {math}`T` itself. The thermodynamics page of the species gives {math}`c_p/R`, and [Diatomic partition functions](../chem_07_spectroscopy/diatomic_thermodynamics.md) gives it from spectroscopic constants.
- Use `omega22` and `omega11` directly only for your own correlations; `species_properties` has them built in.
- Choose the geometry integer to match the molecule: it fixes the rotational heat capacity that the Eucken correction uses (`0`, `1` and `1.5` {math}`R`).
- Compute a table over temperature as one call with {math}`T` on the leading axis, and take the species you need from the last axis.

## Pitfalls

The model is for non-polar gases. The program tries to add water, which has a dipole moment, and then a species with a zero diameter.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace tr = pluss::ma::natsci::chemistry::transport;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// Lennard-Jones data: geometry (0 atom, 1 linear), well depth / k_B in K, diameter in angstrom, dipole, rotational relaxation, molar mass in kg/mol
static std::vector<tr::SpeciesData> gases() {
  return {{"N2", 1, 97.53, 3.621, 0.0, 4.0, 28.014e-3},  {"O2", 1, 107.4, 3.458, 0.0, 3.8, 31.998e-3},
          {"Ar", 0, 136.5, 3.330, 0.0, 0.0, 39.948e-3},  {"He", 0, 10.2, 2.576, 0.0, 0.0, 4.0026e-3},
          {"H2", 1, 38.0, 2.920, 0.0, 280.0, 2.016e-3}};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // a polar species is refused: water has a dipole of 6.2e-30 C m
  try {
    auto bad = gases();
    bad.push_back({"H2O", 2, 572.4, 2.605, 6.2e-30, 4.0, 18.015e-3});
    tr::GasTransport model(bad, vec({300.0}));
  } catch (const pluss::Error &e) {
    out.writeln("polar species : ", e.message());
  }
  try {
    auto bad = gases();
    bad[0].diameter_angstrom = 0.0;
    tr::GasTransport model(bad, vec({300.0}));
  } catch (const pluss::Error &e) {
    out.writeln("bad data      : ", e.message());
  }
}
```

Output

```text
polar species : natsci: transport: polar species (H2O) are not supported
bad data      : natsci: transport: invalid Lennard-Jones data for N2
```

Both are rejected, and each message names the species. A polar molecule cannot be described by the Lennard-Jones collision integrals used here, which is a limit of the method and not an oversight: leave polar species out of the model, or accept that mixtures containing them have to be treated by another method. A diameter or a well depth of zero is not a physical potential and is refused in the same way.

## Where next

[Mixtures and diffusion](mixtures_and_diffusion.md) combines these pure-gas properties into mixture values and gives the diffusion coefficients, and [Chemistry 7: Spectroscopy](../chem_07_spectroscopy.md) supplies the heat capacity.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="mixtures_and_diffusion.html">
    <span class="label">Next</span>
    <span class="title">Mixtures and diffusion</span>
  </a>
</div>
<!-- apiref:nav:end -->
