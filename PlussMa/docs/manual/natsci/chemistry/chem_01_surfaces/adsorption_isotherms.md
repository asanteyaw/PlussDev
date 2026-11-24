# Adsorption isotherms

[Back to Chemistry 1: Surfaces](../chem_01_surfaces.md)

Gas molecules stick to a solid surface until its sites are filled. How much sticks depends on the pressure, and the shape of that dependence tells how the molecules bind. This page evaluates the standard isotherms, shows two gases competing for the same sites, and fits a measured isotherm to find the surface area of a powder.

## How it works

`surface::langmuir(pressure, k)` is the fraction of sites covered, `k p / (1 + k p)`: it rises linearly at low pressure and saturates at `1` when every site is filled. If the molecule splits in two on adsorbing, `langmuir_dissociative` uses the square root of `k p`, which saturates more slowly. `freundlich(pressure, k, n)` is the empirical power law `k p^(1/n)`, which has no saturation. For several gases sharing the sites, `langmuir_competitive(pressures, k)` returns the coverage of each. `bet_fit(x, q)` fits the Brunauer-Emmett-Teller model to an isotherm measured at relative pressures `x` and uptakes `q`, and returns the amount that makes one full layer, `monolayer`, the constant `c` and the quality of the fit `r_squared`; `specific_surface_area(monolayer, cross_section)` turns the monolayer amount into an area per kilogram.

## Four isotherms and a fit

The program evaluates the Langmuir, dissociative Langmuir and Freundlich isotherms at four pressures, two competing gases, and fits a BET isotherm that was generated with a monolayer of `2` mol/kg and `c = 100`; nitrogen covers `0.162` nm² per molecule.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace surface = chemistry::surface;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // fraction of surface sites covered by gas at pressures from 0.1 to 100 (units of 1/K)
  const auto pressure = vec({0.1, 1.0, 10.0, 100.0});
  out.writeln("pressure                    ", flat(pressure));
  out.writeln("Langmuir, K = 1             ", flat(surface::langmuir(pressure, 1.0)));
  out.writeln("Langmuir, molecule splits   ", flat(surface::langmuir_dissociative(pressure, 1.0)));
  out.writeln("Freundlich, K = 1, n = 2    ", flat(surface::freundlich(pressure, 1.0, 2.0)));

  // two gases competing for the same sites
  const auto both = vec({1.0, 1.0});
  out.writeln("two gases at pressure 1, K = 1 and 3: coverage of each = ", flat(surface::langmuir_competitive(both, vec({1.0, 3.0}))));

  // a measured nitrogen isotherm: relative pressure x and uptake q (mol/kg)
  const auto x = vec({0.05, 0.10, 0.15, 0.20, 0.25, 0.30});
  const auto q = surface::bet(x, 100.0).mul(2.0);   // synthetic data with a monolayer of 2 mol/kg and C = 100
  const auto fit = surface::bet_fit(x, q);
  out.writeln("BET fit: monolayer (mol/kg) = ", flat(fit.monolayer), ", C = ", flat(fit.c), ", R^2 = ", flat(fit.r_squared));
  out.writeln("specific surface area (m^2/kg) for N2 at 0.162 nm^2 = ", flat(surface::specific_surface_area(fit.monolayer, 0.162e-18)));
}
```

Output

```text
pressure                    [0.1000, 1.0000, 10.0000, 100.0000]
Langmuir, K = 1             [0.0909, 0.5000, 0.9091, 0.9901]
Langmuir, molecule splits   [0.2403, 0.5000, 0.7597, 0.9091]
Freundlich, K = 1, n = 2    [0.3162, 1.0000, 3.1623, 10.0000]
two gases at pressure 1, K = 1 and 3: coverage of each = [0.2000, 0.6000]
BET fit: monolayer (mol/kg) = [2.0000], C = [100.0000], R^2 = [1.0000]
specific surface area (m^2/kg) for N2 at 0.162 nm^2 = [195117.3606]
```

The Langmuir coverage is `0.0909` at a pressure of `0.1`, half at `1` and `0.9901` at `100`. The dissociating molecule covers the surface faster at low pressure (`0.2403` at `0.1`) and slower at high pressure (`0.9091` at `100`), the signature that distinguishes the two. The Freundlich law does not saturate and passes `1` at a pressure of `1`. Two gases at the same pressure share the sites in proportion to their binding constants: the gas with `K = 3` covers `0.6000` and the one with `K = 1` covers `0.2000`, leaving `0.2000` empty. The BET fit recovers the monolayer `2.0000` mol/kg and `c = 100.0000` with `R^2 = 1.0000` and gives a surface of `195117.3606` m² per kilogram, about 195 m² per gram.

<!-- addex:start -->
## Surface reactions

Molecules that stick to a surface can react there. When both reactants are adsorbed side by side, `surface::langmuir_hinshelwood_rate(pA, pB, kA, kB, k)` is the rate {math}`k\,\theta_A\theta_B`, which is zero if either coverage is zero; when one reactant comes from the gas phase and meets an adsorbed one, `surface::eley_rideal_rate(pA, pB, kA, k)` is {math}`k\,\theta_A p_B`. The program raises the pressure of gas B over four decades with gas A held at pressure `1`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace surface = chemistry::surface;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // surface reactions: two gases adsorbed side by side (Langmuir-Hinshelwood), or one adsorbed and one from the gas phase (Eley-Rideal). Rate against the pressure of B
  const auto pb = vec({0.01, 0.1, 1.0, 10.0, 100.0});
  const auto pa = vec({1.0, 1.0, 1.0, 1.0, 1.0});
  out.writeln("pressure of B                         ", flat(pb));
  out.writeln("Langmuir-Hinshelwood rate             ", flat(surface::langmuir_hinshelwood_rate(pa, pb, 1.0, 1.0, 1.0)));
  out.writeln("Eley-Rideal rate                      ", flat(surface::eley_rideal_rate(pa, pb, 1.0, 1.0)));
}
```

Output

```text
pressure of B                         [0.0100, 0.1000, 1.0000, 10.0000, 100.0000]
Langmuir-Hinshelwood rate             [0.0025, 0.0227, 0.1111, 0.0694, 0.0096]
Eley-Rideal rate                      [0.0050, 0.0500, 0.5000, 5.0000, 50.0000]
```

The Eley-Rideal rate keeps rising with the pressure of B, `5.0000` at `10` and `50.0000` at `100`, because B needs no surface site. The Langmuir-Hinshelwood rate rises to a maximum of `0.1111` at a pressure of `1`, where both gases cover half the sites in the best proportion, and then falls (`0.0096` at `100`) since B crowds out A: more of one reactant can slow the reaction.

## Temperature and the Sips isotherm

At a fixed coverage the pressure needed grows with the temperature, because the molecules leave the surface more easily when it is warm. `surface::langmuir_pressure(theta, T, k0, heat)` gives the pressure at which the coverage is `theta` for a heat of adsorption `heat`. `surface::sips(p, qm, K, n)` is a hybrid isotherm for a surface with sites of different strengths. The program prints the pressure for half coverage with a heat of adsorption of `30` kJ/mol at three temperatures, and the Sips curve at three pressures.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace surface = chemistry::surface;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // temperature changes how much sticks: for a given coverage, the pressure needed rises with temperature (heat of adsorption 30 kJ/mol)
  const auto theta = vec({0.5, 0.5, 0.5});
  const auto temperature = vec({250.0, 300.0, 350.0});
  out.writeln("temperature (K)                      ", flat(temperature));
  out.writeln("pressure for half coverage           ", flat(surface::langmuir_pressure(theta, temperature, 1.0e-6, 30000.0)));
  // the Sips isotherm (a Langmuir-Freundlich hybrid with maximum uptake qm and heterogeneity n)
  const auto p = vec({0.1, 1.0, 10.0});
  out.writeln("Sips (qm = 2, K = 1, n = 2)           ", flat(surface::sips(p, 2.0, 1.0, 2.0)));
}
```

Output

```text
temperature (K)                      [250.0000, 300.0000, 350.0000]
pressure for half coverage           [0.5395, 5.9791, 33.3298]
Sips (qm = 2, K = 1, n = 2)           [0.4805, 1.0000, 1.5195]
```

To half-cover the surface needs a pressure of `0.5395` at `250` K, `5.9791` at `300` K and `33.3298` at `350` K, in the pressure units of the binding constant `k0`: a rise by a factor of about sixty over a hundred degrees. The Sips isotherm with {math}`q_m = 2`, {math}`K = 1` and {math}`n = 2` gives `1.0000`, half the capacity, at a pressure of `1`, and `0.4805` and `1.5195` a decade below and above it.

<!-- addex:end -->

## Decisions

- Choose the isotherm from the shape of your data. A curve that levels off at a plateau is `surface::langmuir`; if the molecule splits in two on the surface, use `langmuir_dissociative`; a curve that keeps climbing over your whole pressure range is `freundlich`; a plateau approached more gently than Langmuir allows, from sites of unequal strength, is `sips`.
- Use `langmuir_competitive` whenever two gases share the same sites; fitting each gas on its own overstates the coverage of both.
- Pick the reaction mechanism by what is on the surface: `langmuir_hinshelwood_rate` when both reactants adsorb, `eley_rideal_rate` when one of them arrives from the gas.
- Fit `bet_fit` only to relative pressures between about 0.05 and 0.35, where a layer is completing; read the surface area from the monolayer only if the fit is good.

## Pitfalls

The BET model describes a gas that builds layers. If you apply `bet_fit` to a plateau-shaped (Langmuir) isotherm, it returns numbers, but they are not a surface area. The program fits the same Langmuir data over a narrow and a wide range of relative pressure.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;

namespace surface = chemistry::surface;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // type-I data (a Langmuir isotherm, K = 50, capacity 2) analysed with the BET fit: the answer depends on the range of relative pressure you hand it
  const auto x = vec({0.05, 0.10, 0.15, 0.20, 0.25, 0.30, 0.50, 0.70, 0.90});
  const auto q = surface::langmuir(x, 50.0).mul(2.0);
  const auto low_x = vec({0.05, 0.10, 0.15, 0.20, 0.25, 0.30});
  const auto low = surface::bet_fit(low_x, surface::langmuir(low_x, 50.0).mul(2.0));
  const auto wide = surface::bet_fit(x, q);
  out.writeln("range 0.05 to 0.30: monolayer ", flat(low.monolayer), ", C ", flat(low.c), ", R^2 ", flat(low.r_squared));
  out.writeln("range 0.05 to 0.90: monolayer ", flat(wide.monolayer), ", C ", flat(wide.c), ", R^2 ", flat(wide.r_squared));
}
```

Output

```text
range 0.05 to 0.30: monolayer [1.3248], C [-86.9154], R^2 [0.9921]
range 0.05 to 0.90: monolayer [0.2772], C [-4.9153], R^2 [0.7295]
```

The fit constant `c` is an energy ratio and must be positive. Here it is `-86.9154` and `-4.9153`, and the quality of the fit falls from `0.9921` to `0.7295` as the range widens. A negative `c` or a poor `r_squared` is the signal that BET does not describe your data; use `langmuir` for it instead of reading a surface area from the monolayer.

The binding constant `k` and the pressure must be in reciprocal units. The program evaluates one gas at 1 bar with `K = 2` per bar, once in consistent units and twice with the pressure in pascal.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;

namespace surface = chemistry::surface;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the same gas at 1 bar, with K = 2 per bar, once with the pressure in bar and once (wrongly) in pascal
  out.writeln("pressure in bar, K per bar     ", flat(surface::langmuir(vec({1.0}), 2.0)));
  out.writeln("pressure in Pa,  K per bar     ", flat(surface::langmuir(vec({1.0e5}), 2.0)));
  out.writeln("pressure in Pa,  K per pascal  ", flat(surface::langmuir(vec({1.0e5}), 2.0e-5)));
  try {
    out.writeln("negative pressure              ", flat(surface::langmuir(vec({-1.0}), 2.0)));
  } catch (const pluss::Error &e) {
    out.writeln("negative pressure: ", e.message());
  }
}
```

Output

```text
pressure in bar, K per bar     [0.6667]
pressure in Pa,  K per bar     [1.0000]
pressure in Pa,  K per pascal  [0.6667]
negative pressure: natsci::surface::langmuir: the pressure must not be negative
```

In bar the coverage is `0.6667`. With the pressure in pascal but `k` still per bar, the product `k p` is `200000` and the coverage comes out `1.0000`, a full surface: the call cannot know your units, so no error is possible. Converting `k` to per pascal (`2.0e-5`) restores `0.6667`. A pressure below zero, which cannot be a unit slip, is refused with the message shown.

## Where next

[Chain statistics](../chem_02_polymers/chain_statistics.md) moves from surfaces to chains of molecules. For how temperature changes a rate, which surface reactions share, go to [Transition-state theory](../chem_04_rates/transition_state_theory.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
