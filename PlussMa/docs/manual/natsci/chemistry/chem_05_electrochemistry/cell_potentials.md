# Cell potentials and the Nernst equation

[Back to Chemistry 5: Electrochemistry](../chem_05_electrochemistry.md)

You have a half-reaction, a cell or a membrane and you want the voltage it supports for the concentrations you actually have. This page computes the potential of a redox couple at any reaction quotient, the equilibrium potential of an ion across a membrane, and the standard potential of a cell from its reaction Gibbs energy, and it goes on to the equilibrium constant that the standard potential implies.

## How it works

The electrical work a cell can deliver per mole of reaction is {math}`nFE`, so a reaction with Gibbs energy {math}`\Delta G` has a cell potential {math}`E = -\Delta G/(nF)` (`standard_cell_potential(delta_g, n)`), with {math}`F = eN_A` the Faraday constant. Away from standard conditions the reaction quotient {math}`Q` enters through the Nernst equation, which `nernst(e0, T, n, Q)` evaluates:

```{math}
E = E^\circ - \frac{RT}{nF}\ln Q
```

At {math}`298.15` K the factor {math}`RT\ln 10/F` is {math}`59.16` mV, so a one-electron couple gains or loses about {math}`59` mV for each factor of ten in {math}`Q`, and a two-electron couple half of that. A membrane is the special case in which the two sides carry the same ion: `ion_potential(outside, inside, z, T)` returns the potential at which diffusion and the field balance,

```{math}
E_\mathrm{ion} = \frac{RT}{zF}\ln\frac{c_\mathrm{out}}{c_\mathrm{in}}
```

All arguments may carry batch axes: a Series of quotients, of temperatures or of energies is one call. The electron number and the valence are plain numbers, not Series, because they belong to the reaction and not to the experiment. Setting {math}`E = 0` in the Nernst equation gives the equilibrium constant, {math}`\ln K = nFE^\circ/(RT)`.

## The Nernst equation for a redox couple

A one-electron couple with {math}`E^\circ = 0.77` V is evaluated at four reaction quotients, each ten times the one before. The program then repeats the sweep for a two-electron couple with {math}`E^\circ = 0.34` V and finally changes the temperature at fixed {math}`Q = 10`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ec = pluss::ma::natsci::common::electrochem;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a one-electron couple with standard potential 0.77 V, at 298.15 K, for a ten-fold range of Q
  const auto temperature = vec({298.15, 298.15, 298.15, 298.15});
  const auto q = vec({0.01, 0.1, 1.0, 10.0});
  const auto e = ec::nernst(vec({0.77, 0.77, 0.77, 0.77}), temperature, 1.0, q);
  out.writeln("reaction quotient  ", flat(q));
  out.writeln("potential (V)      ", flat(e));
  // the same quotients for a two-electron couple: the slope halves
  const auto e2 = ec::nernst(vec({0.34, 0.34, 0.34, 0.34}), temperature, 2.0, q);
  out.writeln("two-electron (V)   ", flat(e2));
  // temperature changes the slope: Q = 10 at three temperatures
  const auto t3 = vec({278.15, 298.15, 318.15});
  const auto e3 = ec::nernst(vec({0.0, 0.0, 0.0}), t3, 1.0, vec({10.0, 10.0, 10.0}));
  out.writeln("potential at Q = 10 (mV), 5/25/45 C  ", flat(e3.mul(1000.0)));
}
```

Output

```text
reaction quotient  [0.0100, 0.1000, 1.0000, 10.0000]
potential (V)      [0.8883, 0.8292, 0.7700, 0.7108]
two-electron (V)   [0.3992, 0.3696, 0.3400, 0.3104]
potential at Q = 10 (mV), 5/25/45 C  [-55.1909, -59.1593, -63.1278]
```

At {math}`Q = 1` the potential equals the standard potential, `0.7700` V. Each factor of ten in {math}`Q` moves it by about `0.059` V for the one-electron couple (`0.8883`, `0.8292`, `0.7700`, `0.7108`) and by half as much, about `0.0296` V, for the two-electron couple. A larger quotient means more product and a lower potential. The temperature changes the slope, not the intercept: the shift for {math}`Q = 10` is `-55.1909` mV at 5 C, `-59.1593` mV at 25 C and `-63.1278` mV at 45 C, proportional to {math}`T`.

## Ion equilibrium potentials across a membrane

The concentrations are typical of a mammalian cell at {math}`310` K: potassium `5` mM outside and `140` mM inside, sodium `145` and `12`, chloride `110` and `10`, and calcium `2` and `0.0001` mM. The valence of chloride is {math}`-1` and that of calcium {math}`+2`. A last line holds a ten-fold ratio fixed and varies the temperature.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ec = pluss::ma::natsci::common::electrochem;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // equilibrium potentials of K+ across a membrane at body temperature: outside 5 mM, inside 140 mM
  const auto t = vec({310.0});
  out.writeln("K+  (mV)  ", flat(ec::ion_potential(vec({5.0}), vec({140.0}), 1.0, t).mul(1000.0)));
  // Na+ outside 145 mM, inside 12 mM; Cl- (valence -1) outside 110 mM, inside 10 mM; Ca2+ outside 2 mM, inside 0.0001 mM
  out.writeln("Na+ (mV)  ", flat(ec::ion_potential(vec({145.0}), vec({12.0}), 1.0, t).mul(1000.0)));
  out.writeln("Cl- (mV)  ", flat(ec::ion_potential(vec({110.0}), vec({10.0}), -1.0, t).mul(1000.0)));
  out.writeln("Ca2+ (mV) ", flat(ec::ion_potential(vec({2.0}), vec({0.0001}), 2.0, t).mul(1000.0)));
  // the ratio of concentrations decides, not their size: ten-fold ratio at four temperatures
  const auto tt = vec({273.15, 293.15, 310.0, 330.0});
  out.writeln("ten-fold K+ ratio (mV)  ", flat(ec::ion_potential(vec({1.0, 1.0, 1.0, 1.0}), vec({0.1, 0.1, 0.1, 0.1}), 1.0, tt).mul(1000.0)));
}
```

Output

```text
K+  (mV)  [-89.0156]
Na+ (mV)  [66.5660]
Cl- (mV)  [-64.0567]
Ca2+ (mV) [132.2796]
ten-fold K+ ratio (mV)  [54.1988, 58.1672, 61.5106, 65.4791]
```

Potassium is held `-89.0156` mV below the outside, sodium `66.5660` mV above, chloride `-64.0567` mV, and calcium, with its enormous ratio of twenty thousand, `132.2796` mV even though {math}`z = 2` halves the effect. The sign says which way the ion would flow if the membrane let it through, and a cell at rest sits close to the potassium value because that is the channel that is open. For a fixed ten-fold ratio the potential grows linearly with temperature, from `54.1988` mV at 273 K to `65.4791` mV at 330 K.

## From Gibbs energy to cell potential and equilibrium constant

Three reactions with two electrons each, with {math}`\Delta G^\circ` of {math}`-212.3`, {math}`-237.1` and {math}`-150.0` kJ/mol, are converted to standard potentials in one call. A cell with {math}`E^\circ = 1.10` V is then run over eight decades of {math}`Q`, and the equilibrium constant that its standard potential implies is printed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ec = pluss::ma::natsci::common::electrochem;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // reaction Gibbs energies in kJ/mol for 2-electron cells: Daniell cell (-212.3), hydrogen-oxygen to liquid water (-237.1)
  const auto dg = vec({-212.3e3, -237.1e3, -150.0e3});
  const auto e0 = ec::standard_cell_potential(dg, 2.0);
  out.writeln("standard cell potential (V)  ", flat(e0));
  // a cell with E0 = 1.10 V, n = 2, at 298.15 K, run from the Nernst equation to equilibrium
  const auto q = vec({1e-4, 1e-2, 1.0, 1e2, 1e4});
  const auto e = ec::nernst(vec({1.10, 1.10, 1.10, 1.10, 1.10}), vec({298.15, 298.15, 298.15, 298.15, 298.15}), 2.0, q);
  out.writeln("Q                       ", flat(q));
  out.writeln("cell potential (V)      ", flat(e));
  // the equilibrium constant follows from E0: ln K = n F E0 / (R T)
  const double lnk = 2.0 * ec::faraday() * 1.10 / (ec::gas_constant() * 298.15);
  out.writeln("log10 K for E0 = 1.10 V = ", std::vector<double>{lnk / std::log(10.0)});
}
```

Output

```text
standard cell potential (V)  [1.1002, 1.2287, 0.7773]
Q                       [0.0001, 0.0100, 1.0000, 100.0000, 10000.0000]
cell potential (V)      [1.2183, 1.1592, 1.1000, 1.0408, 0.9817]
log10 K for E0 = 1.10 V = [37.1877]
```

The potentials are `1.1002`, `1.2287` and `0.7773` V, simply {math}`-\Delta G/(2F)`. The cell potential falls from `1.2183` V at {math}`Q = 10^{-4}` to `0.9817` V at {math}`Q = 10^{4}`, by about `0.059` V per two decades as the two-electron slope requires, and it passes through {math}`E^\circ` at {math}`Q = 1`. The cell would reach {math}`E = 0` only when {math}`Q` equals the equilibrium constant, {math}`\log_{10}K = 37.1877`: a {math}`1.10` V cell is about as far from equilibrium as a reaction can be.

## Decisions

- Use `nernst` when you know the reaction quotient of a full reaction and `ion_potential` when the same ion sits on both sides of a barrier; the second is the first with {math}`E^\circ = 0` and {math}`Q = c_\mathrm{in}/c_\mathrm{out}`, written for the common case.
- Convert a Gibbs energy to a voltage with `standard_cell_potential` and keep it in a Series of any shape, so a whole table of reactions or a temperature sweep of {math}`\Delta G(T)` is one call.
- Pass the electron number {math}`n` and the valence {math}`z` as the number that appears in the balanced reaction. Writing the same reaction with doubled coefficients doubles {math}`n` and squares {math}`Q` and leaves {math}`E` unchanged; the two must stay consistent.
- Use activities, not concentrations, in {math}`Q` when the solution is not dilute: see [Ionic strength and activity](ionic_activities.md).

## Pitfalls

The function checks what can be checked: the quotient must be positive and the electron number non-zero. What it cannot check is whether {math}`n` matches the way {math}`Q` was written. The program shows both errors and then evaluates the same {math}`Q = 10` with the wrong electron number.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ec = pluss::ma::natsci::common::electrochem;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a reaction quotient of zero is not meaningful: the reaction quotient must be positive
  try {
    ec::nernst(vec({0.77}), vec({298.15}), 1.0, vec({0.0}));
  } catch (const pluss::Error &e) {
    out.writeln("zero quotient : ", e.message());
  }
  try {
    ec::nernst(vec({0.77}), vec({298.15}), 0.0, vec({1.0}));
  } catch (const pluss::Error &e) {
    out.writeln("zero electrons: ", e.message());
  }
  // a mistake that does not throw: the electron number in the wrong place changes the answer silently
  const auto good = ec::nernst(vec({0.0}), vec({298.15}), 2.0, vec({10.0}));
  const auto wrong = ec::nernst(vec({0.0}), vec({298.15}), 1.0, vec({10.0}));
  out.writeln("n = 2 (mV): ", flat(good.mul(1000.0)));
  out.writeln("n = 1 (mV): ", flat(wrong.mul(1000.0)));
}
```

Output

```text
zero quotient : natsci: nernst: reaction quotient must be strictly positive
zero electrons: natsci: nernst: electron number must be non-zero
n = 2 (mV): [-29.5797]
n = 1 (mV): [-59.1593]
```

A quotient of zero or an electron number of zero is rejected with a message that names the function. The last two lines are the silent mistake: `-29.5797` mV for {math}`n = 2` and `-59.1593` mV for {math}`n = 1` for the same quotient. Nothing throws, and the voltage is wrong by a factor of two, so derive {math}`n` from the balanced half-reactions rather than guessing it.

## Where next

[Electrode kinetics and cell polarisation](electrode_kinetics.md) asks what happens when current flows and the cell leaves equilibrium, and [Ionic strength and activity](ionic_activities.md) replaces concentrations by activities. The ion potentials reappear as the reversal potentials in [Biology 16: Membranes](../../biology/bio_16_membranes.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="electrode_kinetics.html">
    <span class="label">Next</span>
    <span class="title">Electrode kinetics and cell polarisation</span>
  </a>
</div>
<!-- apiref:nav:end -->
