# Transition-state theory

[Back to Chemistry 4: Reaction Rates](../chem_04_rates.md)

A reaction proceeds at a rate set by the height of the free-energy barrier between reactants and products and by the temperature. Transition-state theory turns that barrier into a rate constant, and the temperature dependence of measured rates gives the barrier back. This page computes rate constants from a barrier, recovers the activation enthalpy and entropy, and adds the tunnelling correction that matters for the transfer of a light atom.

## How it works

`rate_theory::eyring_rate(delta_g, T)` is the Eyring rate constant `(kT/h) exp(-delta_g / RT)`, with the barrier `delta_g` in J/mol and the temperature in K. `eyring_fit(T, k)` runs the analysis in reverse: a straight-line fit of `ln(k/T)` against `1/T` gives the enthalpy of activation from the slope and the entropy of activation from the intercept. For a light atom the reacting system can pass through the barrier instead of over it, which raises the rate by a factor `kappa >= 1`: `wigner(nu, T)` is the correction for a barrier whose top has an imaginary frequency `nu` in cm^-1, accurate for weak tunnelling, and `eckart(v1, v2, nu, T)` is the correction for an asymmetric Eckart barrier with forward and backward heights `v1` and `v2`, valid for deep tunnelling.

## Rates, activation parameters and tunnelling

The barrier is `80` kJ/mol at four temperatures from `280` to `340` K. The program prints the rate constants, recovers the activation parameters from them, and evaluates the Wigner and Eckart corrections for an imaginary frequency of `1500` cm^-1 with barriers of `40` and `20` kJ/mol.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace rate = chemistry::rate_theory;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a first-order reaction with a free-energy barrier of 80 kJ/mol, at four temperatures
  const auto temperature = vec({280.0, 300.0, 320.0, 340.0});
  const auto barrier = temperature.mul(0.0).add(80000.0);
  const auto k = rate::eyring_rate(barrier, temperature);
  out.writeln("temperature (K)         ", flat(temperature));
  out.writeln("rate constant (1/s)     ", flat(k));

  // recover the enthalpy and entropy of activation from those rates
  const auto fit = rate::eyring_fit(temperature, k);
  out.writeln("activation enthalpy (kJ/mol) = ", flat(fit.enthalpy.div(1000.0)));
  out.writeln("activation entropy (J/mol/K) = ", flat(fit.entropy));

  // tunnelling raises the rate of a hydrogen transfer: Wigner and Eckart corrections for an imaginary frequency of 1500 cm^-1
  const auto imaginary = temperature.mul(0.0).add(1500.0);
  out.writeln("Wigner correction at those temperatures = ", flat(rate::wigner(imaginary, temperature)));
  const auto forward = temperature.mul(0.0).add(40000.0), backward = temperature.mul(0.0).add(20000.0);
  out.writeln("Eckart correction (barriers 40 and 20 kJ/mol) = ", flat(rate::eckart(forward, backward, imaginary, temperature)));
}
```

Output

```text
temperature (K)         [280.0000, 300.0000, 320.0000, 340.0000]
rate constant (1/s)     [0.0070, 0.0736, 0.5829, 3.6311]
activation enthalpy (kJ/mol) = [80.0000]
activation entropy (J/mol/K) = [0.0000]
Wigner correction at those temperatures = [3.4754, 3.1563, 2.8952, 2.6788]
Eckart correction (barriers 40 and 20 kJ/mol) = [11.1325, 8.2344, 6.4305, 5.2380]
```

A barrier of `80` kJ/mol gives a rate constant of `0.0070` per second at `280` K that grows to `3.6311` per second at `340` K, a factor of about five hundred for a twenty percent rise in temperature. The fit returns the activation enthalpy `80.0000` kJ/mol and an entropy of `0.0000`, which is what the rates were built from. Tunnelling raises the rate most at low temperature: the Wigner factor falls from `3.4754` at `280` K to `2.6788` at `340` K, and the Eckart factor, which is larger for these barriers, from `11.1325` to `5.2380`.

<!-- addex:start -->
## Collision theory

Collision theory builds the rate from the frequency of collisions with enough energy. `rate_theory::collision_rate(sigma, mu, Ea, T, steric)` takes the collision cross-section, the reduced mass, the activation energy and the temperature, and returns the rate constant in {math}`\mathrm{m^3\,mol^{-1}\,s^{-1}}`. The steric factor, `1` by default, accounts for the orientation of the colliding molecules. The program uses an activation energy of `50` kJ/mol at four temperatures, with and without a steric factor of `0.1`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace rate = chemistry::rate_theory;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // collision theory: the rate constant (m^3 / (mol s)) of a bimolecular reaction, cross-section 0.4 nm^2, reduced mass 3e-26 kg, activation energy 50 kJ/mol, at four temperatures
  const auto temperature = vec({300.0, 400.0, 500.0, 700.0});
  out.writeln("temperature (K)                  ", flat(temperature));
  out.writeln("rate constant (m^3/mol/s)        ", flat(rate::collision_rate(0.4e-18, 3.0e-26, 50000.0, temperature)));
  out.writeln("with steric factor 0.1           ", flat(rate::collision_rate(0.4e-18, 3.0e-26, 50000.0, temperature, 0.1)));
}
```

Output

```text
temperature (K)                  [300.0000, 400.0000, 500.0000, 700.0000]
rate constant (m^3/mol/s)        [0.2813, 48.7629, 1102.5161, 40535.7413]
with steric factor 0.1           [0.0281, 4.8763, 110.2516, 4053.5741]
```

The rate constant rises from `0.2813` at `300` K to `40535.7413` at `700` K, an increase of five orders of magnitude, dominated by the Arrhenius factor {math}`e^{-E_a/RT}`. The steric factor scales every value by exactly `0.1` (`0.0281` to `4053.5741`).

## Reading activation parameters from data

The Eyring plot of {math}`\ln(k/T)` against {math}`1/T` is a straight line whose slope gives the enthalpy and whose intercept gives the entropy of activation. `rate_theory::eyring_fit(T, k)` fits it and returns the standard errors too. The program builds rate constants from {math}`\Delta H^\ddagger = 70` kJ/mol and {math}`\Delta S^\ddagger = -40` J/(mol K) at five temperatures and fits them, then applies a transmission coefficient of `0.5`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace rate = chemistry::rate_theory;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // measured rate constants at five temperatures (a barrier of 70 kJ/mol with an entropy of activation of -40 J/mol/K), analysed with the Eyring plot
  const auto temperature = vec({290.0, 300.0, 310.0, 320.0, 330.0});
  const auto dg = temperature.mul(-1.0).mul(-40.0).add(70000.0);   // dG = dH - T dS
  const auto k = rate::eyring_rate(dg, temperature);
  out.writeln("rate constants (1/s)             ", flat(k));
  const auto fit = rate::eyring_fit(temperature, k);
  out.writeln("activation enthalpy (kJ/mol)     = ", flat(fit.enthalpy.div(1000.0)));
  out.writeln("activation entropy (J/mol/K)     = ", flat(fit.entropy));
  out.writeln("standard errors                  = ", flat(fit.enthalpy_error), " and ", flat(fit.entropy_error));
  // a transmission coefficient kappa of 0.5 halves every rate constant
  out.writeln("with kappa = 0.5                 ", flat(rate::eyring_rate(dg, temperature, 0.5)));
}
```

Output

```text
rate constants (1/s)             [0.0121, 0.0330, 0.0844, 0.2035, 0.4657]
activation enthalpy (kJ/mol)     = [70.0000]
activation entropy (J/mol/K)     = [-40.0000]
standard errors                  = [0.0000] and [0.0000]
with kappa = 0.5                 [0.0061, 0.0165, 0.0422, 0.1017, 0.2329]
```

The fit returns `70.0000` kJ/mol and `-40.0000` J/(mol K) with zero standard errors, since the data are exact; with measured data the errors would show how well the line is determined. A transmission coefficient `kappa = 0.5` halves every rate constant (`0.0121` becomes `0.0061`).

<!-- addex:end -->

## Decisions

- Use `eyring_rate` when you know the free-energy barrier and want a rate; use `collision_rate` for a gas-phase bimolecular step where the cross-section is known.
- Pass a transmission coefficient `kappa` below 1 when not every crossing of the barrier completes; add `wigner` or `eckart` for light atoms that tunnel.
- Collect measurements over as wide a temperature range as you can before using `eyring_fit`, and read its standard errors before trusting the entropy.

## Pitfalls

The barrier goes in joules per mole. The program evaluates the same reaction at 300 K with the barrier as `80000` and as `80`, the number a kilojoule table would give.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;

namespace rate = chemistry::rate_theory;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the barrier is in J/mol; typing it in kJ/mol is a thousandfold error in the exponent
  const auto temperature = vec({300.0});
  out.writeln("80000 J/mol     ", flat(rate::eyring_rate(vec({80000.0}), temperature)));
  out.writeln("80 (kJ typed)   ", flat(rate::eyring_rate(vec({80.0}), temperature)));
}
```

Output

```text
80000 J/mol     [0.0736]
80 (kJ typed)   [6053681147976.1719]
```

`80000` J/mol gives a rate constant of `0.0736` per second; `80` is read as 80 J/mol and gives `6.05e12` per second, essentially the rate of a barrierless step. Convert the barrier before you call.

An Eyring plot needs a spread of temperatures. The program analyses the same reaction, with one percent of scatter in the measured constants, over a 40 K and a 4 K window.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;

namespace rate = chemistry::rate_theory;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // an Eyring plot over a narrow temperature window cannot separate the enthalpy from the entropy of activation
  const auto run = [&](std::vector<double> temps) {
    const auto temperature = vec(temps);
    const auto dg = temperature.mul(-1.0).mul(-40.0).add(70000.0);
    const auto k = rate::eyring_rate(dg, temperature).mul(1.0 + 0.0);
    // 1 percent alternating scatter in the measured constants
    const auto noise = vec({1.01, 0.99, 1.01, 0.99, 1.01}).slice(0, 0, (int64_t)temps.size());
    const auto fit = rate::eyring_fit(temperature, k.mul(noise));
    out.writeln("window ", temps.front(), " to ", temps.back(), " K: enthalpy ", flat(fit.enthalpy.div(1000.0)), " +/- ", flat(fit.enthalpy_error.div(1000.0)), " kJ/mol, entropy ", flat(fit.entropy), " +/- ", flat(fit.entropy_error), " J/mol/K");
  };
  run({290.0, 300.0, 310.0, 320.0, 330.0});
  run({298.0, 299.0, 300.0, 301.0, 302.0});
}
```

Output

```text
window 290.0000 to 330.0000 K: enthalpy [69.9897] +/- [0.3182] kJ/mol, entropy [-40.0170] +/- [1.0297] J/mol/K
window 298.0000 to 302.0000 K: enthalpy [69.9900] +/- [2.9932] kJ/mol, entropy [-40.0170] +/- [9.9775] J/mol/K
```

Both windows return the same activation enthalpy (`69.9897` and `69.9900` kJ/mol) and entropy, but the standard errors grow from `0.3182` kJ/mol and `1.0297` J/mol/K to `2.9932` and `9.9775`, about ten times larger. The narrow window cannot separate the enthalpy from the entropy of activation.

## Where next

[Adsorption isotherms](../chem_01_surfaces/adsorption_isotherms.md) shows how temperature changes the amount adsorbed, the same Arrhenius-type dependence. [Nucleation and growth](../chem_03_crystallization/nucleation_and_growth.md) applies a barrier to the formation of a solid.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
