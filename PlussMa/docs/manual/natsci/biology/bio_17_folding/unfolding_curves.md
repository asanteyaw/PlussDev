# Unfolding curves

[Back to Biology 17: Protein Folding](../bio_17_folding.md)

A folded protein is only a few kilocalories per mole more stable than the unfolded chain, and the balance can be shifted by a chemical denaturant or by temperature. This page computes the free energy and the fraction unfolded for a two-state protein as the denaturant concentration rises, the spectroscopic signal that is measured, and the stability as a function of temperature.


## How it works

A two-state protein is either folded or unfolded, and its stability is a free energy {math}`\Delta G` with {math}`f_U = 1/(1 + e^{\Delta G/RT})`. A denaturant lowers {math}`\Delta G` linearly (`unfolding_free_energy`, `two_state_fraction_unfolded`), the measured signal is the population-weighted mix of the two baselines (`two_state_signal`), and temperature changes {math}`\Delta G` through the enthalpy and the heat-capacity change (`thermal_gibbs`, `thermal_enthalpy`). {math}`RT` in kcal/mol is the gas constant 0.0019872 kcal/mol/K times the temperature in kelvin. Every function takes arrays, so a whole titration is one call.

## Chemical denaturation

For a two-state protein the free energy of unfolding varies linearly with the denaturant concentration {math}`x`,

```{math}
\Delta G(x) = m\,(d_{50} - x), \qquad f_U = \frac{1}{1 + e^{\Delta G/RT}}
```

where {math}`d_{50}` is the midpoint and {math}`m` the {math}`m`-value. `protein_folding::unfolding_free_energy(x, m, d50)` and `protein_folding::two_state_fraction_unfolded(x, m, d50, rt)` evaluate them, with `rt` in kcal/mol from `protein_(0.0019872 * T)`. The program uses {math}`m = 2` kcal/mol/M and {math}`d_{50} = 3` M at {math}`25\,^\circ`C.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace folding = biology::biophysics::protein_folding;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // chemical unfolding of a two-state protein: free energy of unfolding dG = m (d50 - [denaturant]), m = 2 kcal/mol/M, midpoint 3 M, at 25 C
  const double rt = (0.0019872 * 298.15);
  const auto d = vec({0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  out.writeln("denaturant (M)           ", flat(d));
  out.writeln("free energy of unfolding (kcal/mol) ", flat(folding::unfolding_free_energy(d, 2.0, 3.0)));
  out.writeln("fraction unfolded        ", flat(folding::two_state_fraction_unfolded(d, 2.0, 3.0, rt)));
}
```

Output

```text
denaturant (M)           [0.0000, 1.0000, 2.0000, 3.0000, 4.0000, 5.0000, 6.0000]
free energy of unfolding (kcal/mol) [6.0000, 4.0000, 2.0000, 0.0000, -2.0000, -4.0000, -6.0000]
fraction unfolded        [0.0000, 0.0012, 0.0331, 0.5000, 0.9669, 0.9988, 1.0000]
```

The free energy of unfolding falls from `6.0000` kcal/mol with no denaturant to zero at the midpoint, `3` M, where half the protein is unfolded (`0.5000`). The transition takes place over about two molar: `0.0331` unfolded at `2` M and `0.9669` at `4` M.

## The measured signal and the m-value

What is measured is a signal, a weighted sum of the native and unfolded baselines, `protein_folding::two_state_signal(x, alpha_n, beta_n, alpha_u, beta_u, m, d50, rt)`, whose baselines are linear in the denaturant. The sharpness of the transition is set by the {math}`m`-value: a larger one means a steeper change. The program prints a signal with a native baseline {math}`10 - 0.5x` and an unfolded baseline {math}`2 + 0.1x`, and the fraction unfolded for three {math}`m`-values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace folding = biology::biophysics::protein_folding;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a spectroscopic signal follows the populations: native baseline 10 - 0.5 x, unfolded baseline 2 + 0.1 x
  const double rt = (0.0019872 * 298.15);
  const auto d = vec({0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  out.writeln("signal  ", flat(folding::two_state_signal(d, 10.0, -0.5, 2.0, 0.1, 2.0, 3.0, rt)));
  // a stiffer transition (larger m-value) is sharper: the fraction unfolded for m = 1, 2, 4
  for (double m : {1.0, 2.0, 4.0}) out.writeln("m = ", m, ": fraction unfolded ", flat(folding::two_state_fraction_unfolded(d, m, 3.0, rt)));
}
```

Output

```text
signal  [9.9997, 9.4914, 8.7752, 5.4000, 2.5852, 2.5058, 2.6002]
m = 1.0000: fraction unfolded [0.0063, 0.0331, 0.1561, 0.5000, 0.8439, 0.9669, 0.9937]
m = 2.0000: fraction unfolded [0.0000, 0.0012, 0.0331, 0.5000, 0.9669, 0.9988, 1.0000]
m = 4.0000: fraction unfolded [0.0000, 0.0000, 0.0012, 0.5000, 0.9988, 1.0000, 1.0000]
```

The signal moves from `9.9997` for the native protein to `2.6002` for the unfolded one, with `5.4000` at the midpoint. A larger {math}`m`-value makes the transition sharper: at `2` M the fraction unfolded is `0.1561` for {math}`m = 1`, `0.0331` for {math}`m = 2` and `0.0012` for {math}`m = 4`.

## Stability against temperature

A protein is also unfolded by heat, and, because the unfolded chain has a larger heat capacity, by cold. With the melting temperature {math}`T_m`, the enthalpy {math}`\Delta H_m` there and the heat-capacity change {math}`\Delta C_p` of unfolding, the free energy of unfolding is

```{math}
\Delta G(T) = \Delta H_m\left(1 - \frac{T}{T_m}\right) - \Delta C_p\left(T_m - T + T\ln\frac{T}{T_m}\right)
```

`protein_folding::thermal_gibbs(T, params)`, with `protein_folding::Thermal` holding `dh_m`, `tm` and `dcp`, evaluates it, and `thermal_enthalpy` gives the enthalpy. The program uses {math}`\Delta H_m = 100` kcal/mol, {math}`T_m = 340` K and {math}`\Delta C_p = 1.5` kcal/mol/K.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace folding = biology::biophysics::protein_folding;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // thermal stability: dG(T) of unfolding from the melting temperature, the enthalpy there and the heat-capacity change
  folding::Thermal p; p.dh_m = 100.0; p.tm = 340.0; p.dcp = 1.5;
  const auto t = vec({260.0, 280.0, 300.0, 320.0, 340.0, 360.0});
  out.writeln("temperature (K)                    ", flat(t));
  out.writeln("enthalpy of unfolding (kcal/mol)   ", flat(folding::thermal_enthalpy(t, p)));
  out.writeln("free energy of unfolding (kcal/mol) ", flat(folding::thermal_gibbs(t, p)));
  // the protein is most stable where dG is largest; scan a fine grid
  std::vector<double> grid; for (double x = 250.0; x <= 350.0; x += 0.5) grid.push_back(x);
  const auto g = flat(folding::thermal_gibbs(vec(grid), p));
  const auto best = std::max_element(g.begin(), g.end()) - g.begin();
  out.writeln("maximum stability ", g[best], " kcal/mol at ", grid[best], " K");
}
```

Output

```text
temperature (K)                    [260.000, 280.000, 300.000, 320.000, 340.000, 360.000]
enthalpy of unfolding (kcal/mol)   [-20.000, 10.000, 40.000, 70.000, 100.000, 130.000]
free energy of unfolding (kcal/mol) [8.152, 9.193, 8.088, 4.982, 0.000, -6.748]
maximum stability 9.193 kcal/mol at 279.500 K
```

The enthalpy of unfolding rises from `-20.000` kcal/mol at `260` K to `100.000` at the melting temperature `340` K, and the free energy of unfolding falls to zero there and to `-6.748` at `360` K: the protein melts. It is also less stable on the cold side of the maximum, `8.152` at `260` K against `9.193` at `279.500` K, where the stability is greatest.

## Decisions

- Fit chemical denaturation with the {math}`m`-value and {math}`d_{50}`; report {math}`\Delta G(0) = m\,d_{50}` as the stability in water.
- Use `two_state_signal` when you compare with an experiment: the baselines are part of the data, not of the protein.
- Use the thermal model when the heat-capacity change matters; without it ({math}`\Delta C_p = 0`) a protein has no cold unfolding and no temperature of maximum stability.

## Pitfalls

Temperatures are in kelvin, the {math}`m`-value must be positive for the transition to run the right way, and the thermal curve needs a positive temperature. The program breaks each.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace systems = biology::systems;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
namespace folding = biology::biophysics::protein_folding;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // temperatures are in kelvin: 25 typed as degrees Celsius gives an absurd thermal energy
  out.writeln("RT in kcal/mol at 298.15 K and at 25 (Celsius by mistake): ", std::vector<double>{(0.0019872 * 298.15), (0.0019872 * 25.0)});
  // the m-value has the units kcal/mol per molar, and a negative one turns the transition upside down
  const double rt = (0.0019872 * 298.15);
  out.writeln("fraction unfolded at 0, 3, 6 M with m = 2:  ", flat(folding::two_state_fraction_unfolded(vec({0.0, 3.0, 6.0}), 2.0, 3.0, rt)));
  out.writeln("fraction unfolded at 0, 3, 6 M with m = -2: ", flat(folding::two_state_fraction_unfolded(vec({0.0, 3.0, 6.0}), -2.0, 3.0, rt)));
  // the thermal curve is only defined for positive temperatures and a positive melting temperature
  folding::Thermal p; p.dh_m = 100.0; p.tm = 340.0; p.dcp = 1.5;
  try { out.writeln("dG at 0 K: ", flat(folding::thermal_gibbs(vec({0.0}), p))); }
  catch (const pluss::Error &e) { out.writeln("dG at 0 K: ", e.message()); }
  // the fraction unfolded is not the signal: a signal needs its baselines
  out.writeln("signal at the midpoint without baselines (native 1, unfolded 0): ", flat(folding::two_state_signal(vec({3.0}), 1.0, 0.0, 0.0, 0.0, 2.0, 3.0, rt)));
}
```

Output

```text
RT in kcal/mol at 298.15 K and at 25 (Celsius by mistake): [0.5925, 0.0497]
fraction unfolded at 0, 3, 6 M with m = 2:  [0.0000, 0.5000, 1.0000]
fraction unfolded at 0, 3, 6 M with m = -2: [1.0000, 0.5000, 0.0000]
dG at 0 K: natsci: thermal: temperatures and the melting temperature must be positive (kelvin)
signal at the midpoint without baselines (native 1, unfolded 0): [0.5000]
```

A temperature of `25` instead of `298.15` gives {math}`RT = 0.0497` instead of `0.5925` kcal/mol, which makes every transition about twelve times too sharp. A negative {math}`m`-value turns the curve upside down: `1.0000` unfolded at `0` M and `0.0000` at `6` M. A temperature of zero is rejected by `thermal_gibbs`. A signal needs its baselines: with a native baseline of `1` and an unfolded one of `0` the signal at the midpoint is just the fraction, `0.5000`.

## Where next

[Biology 10: Proteins](../bio_10_proteins.md) gives the charge that changes with the solution, and [Biology 24: Nucleic Acids](../bio_24_nucleic.md) applies the same two-state ideas to melting DNA.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
