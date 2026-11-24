# Mixtures and diffusion

[Back to Chemistry 8: Gas Transport](../chem_08_transport.md)

A real gas is a mixture, and you want its viscosity, conductivity and the rate at which each species diffuses through the rest, for a given composition and pressure. This page computes the binary diffusion coefficient of every pair of species, combines pure-gas properties into mixture values by the Wilke and the arithmetic-harmonic rules, and shows how a composition sweep is one call.

## How it works

The binary diffusion coefficient of the pair {math}`(j, k)` at pressure {math}`p` is

```{math}
D_{jk} = \frac{3}{16}\frac{\sqrt{2\pi (k_BT)^3/m_{jk}}}{p\,\pi\sigma_{jk}^2\,\Omega^{(1,1)*}(T^*_{jk})}
```

with the reduced mass {math}`m_{jk}`, the mean diameter {math}`\sigma_{jk}` and the geometric-mean well depth. `binary_diffusion(T)` returns the matrix {math}`[\ldots, S, S]` at a pressure of {math}`1` Pa, so the value at pressure {math}`p` is the stored one divided by {math}`p`. The mixture viscosity follows Wilke's rule, in which each species is weighted by its mole fraction and by how its viscosity and mass compare with the others, `mixture_viscosity(props, x)`; the mixture conductivity is the average of the arithmetic and harmonic mole-fraction means, `mixture_conductivity(props, x)`; and `mixture_diffusion(props, x, p)` gives the diffusion coefficient of each species through all the others,

```{math}
D_{k,m} = \frac{1 - x_k}{\sum_{j\ne k} x_j/D_{kj}}
```

`mixture_diffusion_chemkin` is the same with a mass weighting of the other species. Compositions are mole fractions on the last axis, one entry for every species of the model. `mole_fractions(y)` and `mass_fractions(x)` convert between the two.

## Binary diffusion coefficients

The program prints the nitrogen and the helium rows of the diffusion matrix at {math}`300` K and {math}`1` atm in cm{math}`^2`/s, the symmetry of the matrix, the temperature dependence of the nitrogen-oxygen pair and its dependence on pressure.

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
  const tr::GasTransport model(gases(), vec({300.0}));
  // binary diffusion coefficients at 300 K, per pascal: matrix [S, S]; divide by the pressure to get m^2/s
  const auto d1 = model.binary_diffusion(vec({300.0}));          // [1, S, S] at 1 Pa
  const auto d = d1.div(101325.0).mul(1e4);                       // cm^2/s at 1 atm
  out.writeln("species N2, O2, Ar, He, H2; D (cm^2/s) at 1 atm and 300 K, row N2: ", flat(d.select(-2, 0)));
  out.writeln("row He:                                                            ", flat(d.select(-2, 3)));
  // the matrix is symmetric, and the temperature exponent is close to 3/2 over the exponent of the collision integral
  out.writeln("D(N2, O2) = D(O2, N2): ", std::vector<double>{flat(d.select(-2, 0).select(-1, 1))[0], flat(d.select(-2, 1).select(-1, 0))[0]});
  const auto t = vec({300.0, 600.0, 1200.0});
  const auto dt = model.binary_diffusion(t).div(101325.0).mul(1e4);
  out.writeln("D(N2, O2) at 300, 600, 1200 K (cm^2/s): ", flat(dt.select(-2, 0).select(-1, 1)));
  // pressure scaling: the same pair at 1, 10 and 100 atm
  out.writeln("D(N2, O2) at 1, 10, 100 atm, 300 K (cm^2/s): ", flat(d1.select(-2, 0).select(-1, 1).div(vec({1.0, 10.0, 100.0}).mul(101325.0)).mul(1e4)));
}
```

Output

```text
species N2, O2, Ar, He, H2; D (cm^2/s) at 1 atm and 300 K, row N2: [0.2086, 0.2087, 0.1993, 0.7186, 0.7786]
row He:                                                            [0.7186, 0.7459, 0.7541, 1.6449, 1.5934]
D(N2, O2) = D(O2, N2): [0.2087, 0.2087]
D(N2, O2) at 300, 600, 1200 K (cm^2/s): [0.2087, 0.6910, 2.2079]
D(N2, O2) at 1, 10, 100 atm, 300 K (cm^2/s): [0.2087, 0.0209, 0.0021]
```

Nitrogen diffuses through oxygen at `0.2087` cm{math}`^2`/s and through argon at `0.1993`, and the light gases diffuse much faster, `0.7186` through helium. The matrix is symmetric, `0.2087` both ways. The coefficient grows with temperature, `0.2087`, `0.6910` and `2.2079` at {math}`300`, {math}`600` and {math}`1200` K, a growth like {math}`T^{1.7}`, and falls as one over the pressure, `0.2087`, `0.0209` and `0.0021` at {math}`1`, {math}`10` and {math}`100` atm.

## Air and a hydrogen-nitrogen mixture

Air is taken as {math}`78\%` nitrogen, {math}`21\%` oxygen and {math}`1\%` argon by mole. The program computes its viscosity and conductivity, then sweeps a hydrogen-nitrogen mixture over five compositions as five lanes of one call.

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
  // air as 78 % N2, 21 % O2 and 1 % Ar by mole; helium and hydrogen are absent
  const auto air = vec({0.78, 0.21, 0.01, 0.0, 0.0});
  out.writeln("air viscosity (micro Pa s)        ", flat(model.mixture_viscosity(sp, air).mul(1e6)));
  out.writeln("air conductivity (mW / m / K)     ", flat(model.mixture_conductivity(sp, air).mul(1e3)));
  // a hydrogen-nitrogen mixture across composition: x_H2 = 0.1 ... 0.9 on a leading axis
  std::vector<double> rows;
  for (double x : {0.1, 0.3, 0.5, 0.7, 0.9}) { rows.push_back(1.0 - x); rows.push_back(0.0); rows.push_back(0.0); rows.push_back(0.0); rows.push_back(x); }
  const auto mix = mat(rows, 5, 5);
  const auto sp5 = model.species_properties(vec({300.0, 300.0, 300.0, 300.0, 300.0}), mat(std::vector<double>{3.5, 3.5, 2.5, 2.5, 3.5, 3.5, 3.5, 2.5, 2.5, 3.5, 3.5, 3.5, 2.5, 2.5, 3.5, 3.5, 3.5, 2.5, 2.5, 3.5, 3.5, 3.5, 2.5, 2.5, 3.5}, 5, 5));
  out.writeln("x(H2)                        ", std::vector<double>{0.1, 0.3, 0.5, 0.7, 0.9});
  out.writeln("viscosity (micro Pa s)       ", flat(model.mixture_viscosity(sp5, mix).mul(1e6)));
  out.writeln("conductivity (mW / m / K)    ", flat(model.mixture_conductivity(sp5, mix).mul(1e3)));
}
```

Output

```text
air viscosity (micro Pa s)        [18.682]
air conductivity (mW / m / K)     [26.245]
x(H2)                        [0.100, 0.300, 0.500, 0.700, 0.900]
viscosity (micro Pa s)       [18.040, 17.826, 17.276, 15.965, 12.630]
conductivity (mW / m / K)    [35.702, 55.260, 76.836, 103.049, 144.495]
```

Air has a viscosity of `18.682` micro Pa s and a conductivity of `26.245` mW/(m K), the measured values to the third figure. The hydrogen-nitrogen sweep shows two different behaviours: the viscosity falls, from `18.040` to `12.630` micro Pa s, and the conductivity rises from `35.702` to `144.495`. Hydrogen is a light gas that moves fast, so it carries heat well, but it has a low viscosity, and neither property is a mole-fraction average of the pure values.

## Diffusion through a mixture

A mixture of nitrogen and oxygen with a little hydrogen, at {math}`1` atm and {math}`300` K, is evaluated at three hydrogen levels as three lanes. The program prints the diffusion coefficient of hydrogen and of nitrogen through the mixture, for the mole-based rule and for the mass-weighted form.

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
  const tr::GasTransport model(gases(), vec({300.0}));
  const auto t = vec({300.0, 300.0, 300.0});
  const auto cp = mat({3.5, 3.5, 2.5, 2.5, 3.5, 3.5, 3.5, 2.5, 2.5, 3.5, 3.5, 3.5, 2.5, 2.5, 3.5}, 3, 5);
  const auto sp = model.species_properties(t, cp);
  const auto p = vec({101325.0, 101325.0, 101325.0});
  // diffusion of each species through a mixture of N2 (x = 0.7), O2, and a small amount of H2 at three H2 levels
  const auto x = mat({0.70, 0.30, 0.0, 0.0, 1e-6, 0.69, 0.30, 0.0, 0.0, 0.01, 0.65, 0.30, 0.0, 0.0, 0.05}, 3, 5);
  const auto dm = model.mixture_diffusion(sp, x, p);
  out.writeln("H2 in the mixture, x(H2) = 1e-6, 0.01, 0.05 (cm^2/s) ", flat(dm.select(-1, 4).mul(1e4)));
  out.writeln("N2 in the mixture (cm^2/s)                           ", flat(dm.select(-1, 0).mul(1e4)));
  const auto dc = model.mixture_diffusion_chemkin(sp, x, p);
  out.writeln("H2, chemkin-weighted form (cm^2/s)                   ", flat(dc.select(-1, 4).mul(1e4)));
  out.writeln("N2, chemkin-weighted form (cm^2/s)                   ", flat(dc.select(-1, 0).mul(1e4)));
}
```

Output

```text
H2 in the mixture, x(H2) = 1e-6, 0.01, 0.05 (cm^2/s) [0.7870, 0.7871, 0.7874]
N2 in the mixture (cm^2/s)                           [0.2087, 0.2137, 0.2330]
H2, chemkin-weighted form (cm^2/s)                   [0.7870, 0.7945, 0.8259]
N2, chemkin-weighted form (cm^2/s)                   [0.2286, 0.2291, 0.2314]
```

Hydrogen diffuses through the nitrogen-oxygen mixture at `0.7870` cm{math}`^2`/s, almost independently of how much hydrogen there is (`0.7871`, `0.7874`): it is dominated by collisions with the others. Nitrogen diffuses faster as hydrogen is added, `0.2087` to `0.2330`, because the lighter partner replaces the oxygen. The mass-weighted form gives different numbers for nitrogen, `0.2286` to `0.2314`, since it weights the other species by their mass; for hydrogen the two rules agree at the lowest hydrogen level, `0.7870`, and drift apart as it is added, `0.7874` against `0.8259`, so a calculation has to say which rule it uses.

## Decisions

- Use `mixture_viscosity` and `mixture_conductivity` for the gas as a whole, and `mixture_diffusion` for the transport of one species through it; use the mass-weighted form when you have to match a reference that defines it that way.
- Divide by the pressure only once. `binary_diffusion` is per pascal; `mixture_diffusion` takes the pressure and does it for you.
- Pass mole fractions to the mixture rules. Convert mass fractions with `mole_fractions` first.
- Put absent species in the composition as zeros, not by leaving them out: the composition has one entry for every species of the model.

## Pitfalls

Two mistakes do not raise an error: feeding mass fractions where mole fractions are wanted, and using the stored diffusion coefficient as if it were at atmospheric pressure. The program does both.

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
  // mass fractions of a hydrogen-nitrogen mixture (10 % H2 by mass) fed as if they were mole fractions
  const auto mass = vec({0.9, 0.0, 0.0, 0.0, 0.1});
  const auto mole = model.mole_fractions(mass);
  out.writeln("mole fractions from mass fractions : ", flat(mole));
  out.writeln("viscosity with the right fractions : ", flat(model.mixture_viscosity(sp, mole).mul(1e6)));
  out.writeln("viscosity with mass used as mole   : ", flat(model.mixture_viscosity(sp, mass).mul(1e6)));
  // the stored binary diffusion is at one pascal: it is large until divided by the pressure
  out.writeln("D(N2, O2) stored (m^2/s)           : ", flat(model.binary_diffusion(vec({300.0})).select(-2, 0).select(-1, 1)));
}
```

Output

```text
mole fractions from mass fractions : [0.393, 0.000, 0.000, 0.000, 0.607]
viscosity with the right fractions : [16.717]
viscosity with mass used as mole   : [18.040]
D(N2, O2) stored (m^2/s)           : [2.114]
```

A mixture of nitrogen with {math}`10\%` hydrogen by mass is {math}`60.7\%` hydrogen by mole, `0.607`, not `0.1`. Computing the viscosity from the right fractions gives `16.717` micro Pa s, from the mass fractions `18.040`, a {math}`7\%` error with no warning. The stored diffusion coefficient of nitrogen and oxygen is `2.114` m{math}`^2`/s, which is the value at {math}`1` Pa: at one atmosphere it is `2.087e-5`, a factor of {math}`10^5` smaller.

## Where next

[Viscosity and conductivity of pure gases](pure_gases.md) gives the inputs these rules combine, and [Chemistry 4: Reaction Rates](../chem_04_rates.md) uses the diffusion coefficient in the rate of a diffusion-limited reaction.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pure_gases.html">
    <span class="label">Previous</span>
    <span class="title">Viscosity and conductivity of pure gases</span>
  </a>
</div>
<!-- apiref:nav:end -->
