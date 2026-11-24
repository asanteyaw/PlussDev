# Nucleation and growth

[Back to Chemistry 3: Crystallization](../chem_03_crystallization.md)

A supersaturated solution holds more dissolved solid than it can keep. It relieves the excess by forming tiny crystal nuclei that then grow. Whether a nucleus survives depends on how supersaturated the solution is. This page computes the size a nucleus must reach, the energy barrier it must cross and the resulting rate of nucleation, and then follows how fast the solid forms.

## How it works

`crystallization::critical_radius(S, gamma, molecular_volume, T)` is the smallest nucleus that grows instead of dissolving; `S` is the supersaturation, the concentration divided by the saturation concentration, and `gamma` the interfacial energy. `nucleation_barrier` is the free energy a nucleus of that size must gain to form, and `nucleation_rate(S, prefactor, ...)` is `prefactor` times the Boltzmann factor of that barrier. Once nuclei exist, `jmak_fraction(t, k, n)` is the fraction of the volume crystallised by time `t` in the Johnson-Mehl-Avrami-Kolmogorov model, `1 - exp(-k t^n)`, and `avrami_fit(t, x)` recovers `n` and `k` from a measured curve.

## A salt solution

The salt has interfacial energy `0.03` J/m² and molecular volume `3e-29` m³, at `298` K. The program prints the critical radius, the barrier in units of `kT` and the base-10 logarithm of the nucleation rate at four supersaturations, then the JMAK fraction at six times and the exponent recovered from it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace crystallization = chemistry::crystallization;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a salt crystal growing from solution at 298 K: interfacial energy 0.03 J/m^2, molecular volume 3e-29 m^3
  const double gamma = 0.03, molecular_volume = 3.0e-29, temperature = 298.0;
  const auto supersaturation = vec({1.5, 2.0, 4.0, 8.0});   // concentration divided by the saturation concentration
  out.writeln("supersaturation S               ", flat(supersaturation));
  out.writeln("critical radius (nm)            ", flat(crystallization::critical_radius(supersaturation, gamma, molecular_volume, temperature).mul(1e9)));
  out.writeln("nucleation barrier (units kT)   ", flat(crystallization::nucleation_barrier(supersaturation, gamma, molecular_volume, temperature).div(1.380649e-23 * temperature)));
  out.writeln("log10 nucleation rate (prefactor 1e30) ", flat(crystallization::nucleation_rate(supersaturation, 1.0e30, gamma, molecular_volume, temperature).log10()));

  // how fast the solid forms once nucleated: Avrami (JMAK) kinetics with exponent 3
  const auto time = vec({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  const auto fraction = crystallization::jmak_fraction(time, 0.02, 3.0);
  out.writeln("time                  ", flat(time));
  out.writeln("fraction crystallised ", flat(fraction));
  const auto fit = crystallization::avrami_fit(time, fraction);
  out.writeln("recovered exponent n = ", flat(fit.exponent), ", rate constant k = ", flat(fit.rate));
}
```

Output

```text
supersaturation S               [1.5000, 2.0000, 4.0000, 8.0000]
critical radius (nm)            [1.0790, 0.6312, 0.3156, 0.2104]
nucleation barrier (units kT)   [35.5590, 12.1676, 3.0419, 1.3520]
log10 nucleation rate (prefactor 1e30) [14.5569, 24.7157, 28.6789, 29.4129]
time                  [1.0000, 2.0000, 3.0000, 4.0000, 5.0000, 6.0000]
fraction crystallised [0.0198, 0.1479, 0.4173, 0.7220, 0.9179, 0.9867]
recovered exponent n = [3.0000], rate constant k = [0.0200]
```

Raising the supersaturation from `1.5` to `8` shrinks the critical radius from `1.0790` nm to `0.2104` nm and the barrier from `35.5590` to `1.3520` kT, so the nucleation rate rises from `10^14.56` to `10^29.41` per unit volume and time: nucleation switches on sharply as the solution becomes more supersaturated. Once crystals form, the volume fraction follows the S-shaped JMAK curve (`0.0198` at time `1`, `0.4173` at `3`, `0.9867` at `6`), and fitting that curve returns the exponent `3.0000` and rate `0.0200` that generated it.

<!-- addex:start -->
## Temperature and nucleation

Temperature enters the nucleation barrier through {math}`kT`. The program evaluates the critical radius, the barrier and the rate at a fixed supersaturation of `3` for three temperatures.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace crystallization = chemistry::crystallization;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // temperature and nucleation at a supersaturation of 3: critical radius, barrier in kT, and log10 of the rate
  const auto temperature = vec({280.0, 300.0, 320.0});
  const auto s = vec({3.0, 3.0, 3.0});
  const double gamma = 0.03, vm = 3.0e-29;
  out.writeln("temperature (K)         ", flat(temperature));
  out.writeln("critical radius (nm)    ", flat(crystallization::critical_radius(s, gamma, vm, temperature).mul(1e9)));
  out.writeln("barrier / kT            ", flat(crystallization::nucleation_barrier(s, gamma, vm, temperature).div(temperature.mul(1.380649e-23))));
  out.writeln("log10 rate              ", flat(crystallization::nucleation_rate(s, 1.0e30, gamma, vm, temperature).log10()));
}
```

Output

```text
temperature (K)         [280.0000, 300.0000, 320.0000]
critical radius (nm)    [0.4238, 0.3956, 0.3708]
barrier / kT            [5.8390, 4.7474, 3.9117]
log10 rate              [27.4641, 27.9382, 28.3012]
```

A warmer solution has a smaller critical nucleus, `0.4238` nm at `280` K to `0.3708` nm at `320` K, and a smaller barrier measured in thermal energy, `5.8390` kT to `3.9117` kT, so the rate rises from {math}`10^{27.46}` to {math}`10^{28.30}` at the same supersaturation.

## The shape of the growth curve

The Avrami exponent {math}`n` depends on the shape and on how the nuclei appear. The program evaluates the transformed fraction at five times for {math}`n = 1` to {math}`4` with the same rate constant, and recovers the exponent of an {math}`n = 2.5` curve with `avrami_fit`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace crystallization = chemistry::crystallization;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the Avrami exponent tells how the crystals grow: n = 1 (needles), 2 (discs), 3 (spheres), 4 (spheres with steady nucleation). Same rate constant, same times
  const auto t = vec({1.0, 2.0, 4.0, 6.0, 8.0});
  out.writeln("time                  ", flat(t));
  for (double n : {1.0, 2.0, 3.0, 4.0}) out.writeln("n = ", n, ", k = 0.05      ", flat(crystallization::jmak_fraction(t, 0.05, n)));
  // recover the exponent from a noisy-looking curve of 3 numbers
  const auto fit = crystallization::avrami_fit(t, crystallization::jmak_fraction(t, 0.05, 2.5));
  out.writeln("fit of an n = 2.5 curve: exponent ", flat(fit.exponent), ", rate ", flat(fit.rate));
}
```

Output

```text
time                  [1.0000, 2.0000, 4.0000, 6.0000, 8.0000]
n = 1.0000, k = 0.05      [0.0488, 0.0952, 0.1813, 0.2592, 0.3297]
n = 2.0000, k = 0.05      [0.0488, 0.1813, 0.5507, 0.8347, 0.9592]
n = 3.0000, k = 0.05      [0.0488, 0.3297, 0.9592, 1.0000, 1.0000]
n = 4.0000, k = 0.05      [0.0488, 0.5507, 1.0000, 1.0000, 1.0000]
fit of an n = 2.5 curve: exponent [2.5000], rate [0.0500]
```

With the same rate constant a larger exponent starts later and finishes faster: at time `4` the fractions are `0.1813`, `0.5507`, `0.9592` and `1.0000` for {math}`n = 1, 2, 3, 4`. Fitting the {math}`n = 2.5` curve returns the exponent `2.5000` and the rate `0.0500` that generated it.

<!-- addex:end -->

## Decisions

- Use `critical_radius`, `nucleation_barrier` and `nucleation_rate` to judge whether crystals will form at a given supersaturation and temperature; they apply only when `S > 1`.
- Use `jmak_fraction` to predict the crystallized fraction in time, and `avrami_fit` to recover the exponent and rate constant from measurements. The exponent tells you the growth mode: 1 for needles, 2 for discs, 3 for spheres, 4 for spheres with steady nucleation.
- Give the Avrami fit only the points strictly between 0 and 1, away from the first and the last measurements.

## Pitfalls

The nucleation formulas assume a supersaturated solution, `S > 1`. The program evaluates them below, at and above saturation; the first two are refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;

namespace crystallization = chemistry::crystallization;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // nucleation theory is for a supersaturated solution, S > 1; at S = 1 or below there is nothing to nucleate
  for (double s : {0.8, 1.0, 1.2}) {
    try {
      out.writeln("S = ", s, ": critical radius (nm) ", flat(crystallization::critical_radius(vec({s}), 0.03, 3.0e-29, 298.0).mul(1e9)),
                  ", log10 nucleation rate ", flat(crystallization::nucleation_rate(vec({s}), 1.0e30, 0.03, 3.0e-29, 298.0).log10()));
    } catch (const pluss::Error &e) {
      out.writeln("S = ", s, ": ", e.message());
    }
  }
}
```

Output

```text
S = 0.8000: natsci::crystallization::critical_radius: the supersaturation must be greater than 1 (a solution at or below saturation has no critical nucleus)
S = 1.0000: natsci::crystallization::critical_radius: the supersaturation must be greater than 1 (a solution at or below saturation has no critical nucleus)
S = 1.2000: critical radius (nm) [2.3996], log10 nucleation rate [-46.3773]
```

At `S = 0.8` and `S = 1` there is no critical nucleus, and the call says so instead of returning a negative or infinite radius. At `S = 1.2` the radius is `2.3996` nm and the rate a vanishing `10^-46`: just above saturation the rate changes by many orders of magnitude over a small range of `S`.

`avrami_fit` takes the logarithm of the logarithm of the untransformed fraction, so a fraction of exactly 1 is outside its range. The program fits a clean curve and then a data set with two finished points.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;

namespace crystallization = chemistry::crystallization;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // an Avrami fit needs fractions strictly between 0 and 1: a point at 1 (finished) carries no information about the exponent and is outside its range
  const auto t = vec({1.0, 2.0, 3.0, 4.0, 5.0});
  const auto f = crystallization::jmak_fraction(t, 0.05, 2.0);
  const auto fit = crystallization::avrami_fit(t, f);
  out.writeln("fractions               ", flat(f));
  out.writeln("exponent ", flat(fit.exponent), ", rate ", flat(fit.rate));
  const auto done = vec({0.05, 0.3, 0.8, 1.0, 1.0});
  try {
    const auto fit2 = crystallization::avrami_fit(t, done);
    out.writeln("with two points at 1.0: exponent ", flat(fit2.exponent), ", rate ", flat(fit2.rate));
  } catch (const pluss::Error &e) {
    out.writeln("with two points at 1.0: ", e.message());
  }
}
```

Output

```text
fractions               [0.0488, 0.1813, 0.3624, 0.5507, 0.7135]
exponent [2.0000], rate [0.0500]
with two points at 1.0: natsci::crystallization::avrami_fit: every crystallized fraction must lie strictly between 0 and 1 (drop the points at 0 and at 1)
```

The clean fit recovers the exponent `2.0000` and the rate `0.0500`. With two points at `1.0` the call is refused; drop the saturated (and the zero) points before fitting.

## Where next

[Transition-state theory](../chem_04_rates/transition_state_theory.md) explains how the rate constant of a step depends on temperature. [Gelation](../chem_02_polymers/gelation.md) is another phase change, from liquid to network.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
