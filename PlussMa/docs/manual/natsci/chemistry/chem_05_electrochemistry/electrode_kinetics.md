# Electrode kinetics and cell polarisation

[Back to Chemistry 5: Electrochemistry](../chem_05_electrochemistry.md)

A cell at equilibrium delivers no current. To draw current you must move the electrode away from equilibrium by an overpotential, and the question is how much. This page computes the current at a given overpotential, the overpotential that a given current demands, the effect of temperature on the exchange current, and the voltage of a whole cell as the current rises and the ohmic and mass-transport losses add up.

## How it works

Charge transfer at an electrode follows the Butler-Volmer relation between the current density {math}`i` and the overpotential {math}`\eta`:

```{math}
i = i_0\left[\exp\!\left(\frac{\alpha_a F\eta}{RT}\right) - \exp\!\left(-\frac{\alpha_c F\eta}{RT}\right)\right]
```

where {math}`i_0` is the exchange current density, the rate at which charge crosses in each direction at equilibrium, and {math}`\alpha_a`, {math}`\alpha_c` are the anodic and cathodic transfer coefficients. `butler_volmer(eta, i0, alpha_a, alpha_c, T)` evaluates it. For small {math}`\eta` the current is linear in the overpotential; for large {math}`|\eta|` one exponential wins and the overpotential grows with the logarithm of the current, with the Tafel slope {math}`b = RT\ln 10/(\alpha F)` per decade (`tafel_slope(alpha, T)`). `overpotential(i, i0, alpha_a, alpha_c, T)` inverts the relation numerically for every lane. The exchange current rises with temperature as an Arrhenius law, `exchange_current_arrhenius(i0_ref, Ea, T, T_ref)`. A whole cell loses voltage to activation, to ohmic resistance and to mass transport, and `polarisation_voltage(i, E_rev, i0, alpha, r_area, i_lim, n, T)` returns

```{math}
V = E_\mathrm{rev} - \eta_\mathrm{act}(i) - i\,r + \frac{RT}{nF}\ln\!\left(1 - \frac{i}{i_\mathrm{lim}}\right)
```

with the area-specific resistance {math}`r` and the limiting current density {math}`i_\mathrm{lim}`. The parameters {math}`i_0`, {math}`\alpha`, {math}`r` and {math}`i_\mathrm{lim}` are numbers or Series on the same lanes as the current.

## Current against overpotential

The program evaluates the current density for a symmetric electrode ({math}`\alpha_a = \alpha_c = 0.5`, {math}`i_0 = 10` A/m{math}`^2`) at seven overpotentials, repeats the sweep for an asymmetric one ({math}`0.7` and {math}`0.3`), and then fixes {math}`\eta = 0.05` V and varies {math}`i_0` over six decades with one call.

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
  // current density (A/m^2) at overpotentials from -0.2 V to +0.2 V, exchange current 10 A/m^2, symmetric transfer
  const auto eta = vec({-0.20, -0.10, -0.01, 0.0, 0.01, 0.10, 0.20});
  const auto t = vec({298.15, 298.15, 298.15, 298.15, 298.15, 298.15, 298.15});
  const auto i = ec::butler_volmer(eta, 10.0, 0.5, 0.5, t);
  out.writeln("overpotential (V)     ", flat(eta));
  out.writeln("current density       ", flat(i));
  // asymmetric transfer coefficients: anodic 0.7, cathodic 0.3 bends the curve
  const auto j = ec::butler_volmer(eta, 10.0, 0.7, 0.3, t);
  out.writeln("asymmetric (0.7/0.3)  ", flat(j));
  // an exchange current that varies with the lane: three electrodes at the same overpotential
  const auto k = ec::butler_volmer(vec({0.05, 0.05, 0.05}), vec({0.001, 1.0, 1000.0}), 0.5, 0.5, vec({298.15, 298.15, 298.15}));
  out.writeln("i0 = 0.001, 1, 1000   ", flat(k));
}
```

Output

```text
overpotential (V)     [-0.2000, -0.1000, -0.0100, 0.0000, 0.0100, 0.1000, 0.2000]
current density       [-489.9696, -68.5841, -3.9168, 0.0000, 3.9168, 68.5841, 489.9696]
asymmetric (0.7/0.3)  [-103.2831, -31.4886, -3.6235, 0.0000, 4.2339, 149.3803, 2324.3903]
i0 = 0.001, 1, 1000   [0.0023, 2.2681, 2268.0546]
```

The symmetric curve is odd in the overpotential: {math}`\pm3.9168` A/m{math}`^2` at {math}`\pm10` mV, which is within one percent of the linear estimate {math}`i_0 F\eta/(RT)`, and {math}`\pm489.9696` at {math}`\pm0.2` V, where it has become exponential. The asymmetric curve favours one direction, `2324.3903` A/m{math}`^2` at {math}`+0.2` V but only `-103.2831` at {math}`-0.2` V, because the larger coefficient makes the anodic exponential steeper. The current is proportional to the exchange current: `0.0023`, `2.2681` and `2268.0546` for {math}`i_0 = 10^{-3}`, {math}`1` and {math}`10^{3}`.

## The Tafel slope

On a semi-logarithmic plot the high-overpotential branch is a straight line. The program prints the slope for three transfer coefficients, then recovers it from two currents computed at {math}`0.30` and {math}`0.35` V, and finally varies the temperature.

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
  const auto t = vec({298.15});
  // Tafel slope in V per decade for three transfer coefficients
  out.writeln("Tafel slope (mV/decade), alpha = 0.3, 0.5, 0.7  ",
              std::vector<double>{flat(ec::tafel_slope(0.3, t))[0] * 1000.0, flat(ec::tafel_slope(0.5, t))[0] * 1000.0,
                                  flat(ec::tafel_slope(0.7, t))[0] * 1000.0});
  // measured slope from two points on the high-overpotential branch
  const auto i1 = ec::butler_volmer(vec({0.30}), 1e-3, 0.5, 0.5, t);
  const auto i2 = ec::butler_volmer(vec({0.35}), 1e-3, 0.5, 0.5, t);
  out.writeln("currents at 0.30 and 0.35 V   ", std::vector<double>{flat(i1)[0], flat(i2)[0]});
  out.writeln("measured slope (mV/decade)    ", std::vector<double>{50.0 / std::log10(flat(i2)[0] / flat(i1)[0])});
  // the slope grows with temperature
  out.writeln("slope at 280, 300, 340 K (mV) ", flat(ec::tafel_slope(0.5, vec({280.0, 300.0, 340.0})).mul(1000.0)));
}
```

Output

```text
Tafel slope (mV/decade), alpha = 0.3, 0.5, 0.7  [197.1978, 118.3187, 84.5134]
currents at 0.30 and 0.35 V   [0.3432, 0.9081]
measured slope (mV/decade)    [118.3178]
slope at 280, 300, 340 K (mV) [111.1160, 119.0529, 134.9266]
```

The slope is `197.1978`, `118.3187` and `84.5134` mV per decade for {math}`\alpha = 0.3`, {math}`0.5` and {math}`0.7`: a smaller coefficient makes the electrode harder to push. Two currents `0.3432` and `0.9081` A/m{math}`^2`, fifty millivolts apart, give a measured slope `118.3178` mV/decade, within `0.001` mV of the formula; the difference is the tiny reverse exponential that the Tafel line ignores. The slope rises from `111.1160` to `134.9266` mV between 280 and 340 K, in proportion to {math}`T`.

## Overpotential for a given current, and the effect of temperature

`overpotential` inverts the Butler-Volmer relation. The program asks for the overpotential that drives currents from {math}`10^{-3}` to {math}`10^{4}` A/m{math}`^2` at {math}`333.15` K, checks the answer by evaluating the current again, and then heats an electrode with an exchange current of {math}`1` A/m{math}`^2` at {math}`25` C and an activation energy of {math}`50` kJ/mol.

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
  // the overpotential needed to drive a given current, over seven decades, with i0 = 2 A/m^2
  const auto i = vec({1e-3, 1e-1, 1.0, 10.0, 1e2, 1e3, 1e4});
  const auto t = vec({333.15, 333.15, 333.15, 333.15, 333.15, 333.15, 333.15});
  const auto eta = ec::overpotential(i, 2.0, 0.55, 0.45, t);
  out.writeln("current (A/m^2)     ", flat(i));
  out.writeln("overpotential (V)   ", flat(eta));
  // check by going forward again
  out.writeln("round trip current  ", flat(ec::butler_volmer(eta, 2.0, 0.55, 0.45, t)));
  // heating the electrode raises the exchange current (activation energy 50 kJ/mol) and lowers the overpotential
  const auto temp = vec({298.15, 318.15, 338.15});
  const auto i0 = ec::exchange_current_arrhenius(vec({1.0, 1.0, 1.0}), 50e3, temp, 298.15);
  out.writeln("exchange current at 25/45/65 C  ", flat(i0));
  out.writeln("overpotential for 100 A/m^2 (V) ", flat(ec::overpotential(vec({100.0, 100.0, 100.0}), i0, 0.5, 0.5, temp)));
}
```

Output

```text
current (A/m^2)     [0.0010, 0.1000, 1.0000, 10.0000, 100.0000, 1000.0000, 10000.0000]
overpotential (V)   [0.0000, 0.0014, 0.0139, 0.0866, 0.2042, 0.3244, 0.4446]
round trip current  [0.0010, 0.1000, 1.0000, 10.0000, 100.0000, 1000.0000, 10000.0000]
exchange current at 25/45/65 C  [1.0000, 3.5535, 10.8688]
overpotential for 100 A/m^2 (V) [0.2366, 0.1831, 0.1300]
```

Small currents need almost no overpotential (`0.0000` V at {math}`10^{-3}` A/m{math}`^2`, `0.0139` V at {math}`1`), and from {math}`10^{2}` upward each decade costs `0.1202` V, the Tafel slope for {math}`\alpha_a = 0.55`. The round trip reproduces every current. Heating raises the exchange current to `3.5535` and `10.8688` A/m{math}`^2` at 45 and 65 C, and the overpotential for {math}`100` A/m{math}`^2` falls from `0.2366` to `0.1831` to `0.1300` V: a hotter electrode wastes less voltage on activation.

## The polarisation curve of a cell

A cell with reversible voltage {math}`1.18` V, {math}`i_0 = 1` A/m{math}`^2`, {math}`\alpha = 0.5`, area resistance {math}`5\times10^{-5}\ \Omega\,\mathrm{m}^2` and limiting current {math}`1.2\times10^{4}` A/m{math}`^2` is evaluated at six currents, with the power density {math}`iV`.

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
  // a fuel cell: reversible voltage 1.18 V, exchange current 1 A/m^2, area resistance 5e-5 ohm m^2, limiting current 1.2e4 A/m^2
  const auto i = vec({1.0, 1e2, 1e3, 5e3, 1e4, 1.15e4});
  const auto t = vec({333.15, 333.15, 333.15, 333.15, 333.15, 333.15});
  const auto v = ec::polarisation_voltage(i, vec({1.18, 1.18, 1.18, 1.18, 1.18, 1.18}), 1.0, 0.5, 5e-5, 1.2e4, 2.0, t);
  out.writeln("current (A/m^2)  ", flat(i));
  out.writeln("voltage (V)      ", flat(v));
  out.writeln("power (W/m^2)    ", flat(v.mul(i)));
}
```

Output

```text
current (A/m^2)  [1.0000, 100.0000, 1000.0000, 5000.0000, 10000.0000, 11500.0000]
voltage (V)      [1.1523, 0.9105, 0.7321, 0.4332, 0.1254, 0.0225]
power (W/m^2)    [1.1523, 91.0458, 732.1264, 2166.1445, 1254.4771, 259.0218]
```

The voltage is `1.1523` V at {math}`1` A/m{math}`^2`, already below the reversible voltage because of activation, then falls steeply, almost straight, between {math}`10^2` and {math}`5\times10^3` A/m{math}`^2` where the ohmic loss dominates, and collapses to `0.0225` V at {math}`1.15\times10^{4}`, close to the limiting current, where the logarithmic term diverges. The power density therefore has a maximum, `2166.1445` W/m{math}`^2` at {math}`5000` A/m{math}`^2`, before it falls to `259.0218`: the operating point of a cell is a compromise between current and voltage.

## Decisions

- Use `butler_volmer` to go from potential to current and `overpotential` to go from current to potential; do not invert by hand, because the inverse has no closed form for {math}`\alpha_a \ne \alpha_c`.
- Use the Tafel slope to read {math}`\alpha` off data on the straight part of the semi-logarithmic plot, and keep to overpotentials of a few {math}`RT/F` and more; at low overpotential the line is replaced by the linear relation.
- Use `exchange_current_arrhenius` to carry an {math}`i_0` measured at one temperature to another; it needs the activation energy in J/mol and both temperatures in kelvin.
- Use `polarisation_voltage` for a whole cell, and note that it throws when the current reaches the limiting current rather than returning the meaningless logarithm.

## Pitfalls

The program drives the cell to the limiting current, compares the symmetric curve with an asymmetric one at {math}`\pm0.1` V, and compares the current at two small overpotentials.

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
  const auto t = vec({333.15});
  // asking for a current at or above the limiting current
  try {
    ec::polarisation_voltage(vec({1.3e4}), vec({1.18}), 1.0, 0.5, 5e-5, 1.2e4, 2.0, t);
  } catch (const pluss::Error &e) {
    out.writeln("beyond limit: ", e.message());
  }
  // the symmetric curve has i(-eta) = -i(eta); an asymmetric one does not
  const auto s = ec::butler_volmer(vec({0.1, -0.1}), 1.0, 0.5, 0.5, vec({298.15, 298.15}));
  const auto a = ec::butler_volmer(vec({0.1, -0.1}), 1.0, 0.8, 0.2, vec({298.15, 298.15}));
  out.writeln("symmetric  +0.1 / -0.1 V : ", flat(s));
  out.writeln("asymmetric +0.1 / -0.1 V : ", flat(a));
  // the Tafel slope at one overpotential is not the slope at low overpotential, where the curve is still linear
  const auto low = ec::butler_volmer(vec({0.005, 0.01}), 10.0, 0.5, 0.5, vec({298.15, 298.15}));
  out.writeln("ratio of currents at 0.005 and 0.010 V : ", std::vector<double>{flat(low)[1] / flat(low)[0]});
}
```

Output

```text
beyond limit: natsci: polarisation_voltage: current must stay below the limiting current
symmetric  +0.1 / -0.1 V : [6.8584, -6.8584]
asymmetric +0.1 / -0.1 V : [22.0459, -2.1336]
ratio of currents at 0.005 and 0.010 V : [2.0095]
```

A current at or above the limiting current is an error with a message that names the function. The symmetric curve satisfies {math}`i(-\eta) = -i(\eta)` (`6.8584` and `-6.8584`), but the asymmetric one gives `22.0459` and `-2.1336`, so a single positive-branch measurement tells you nothing about the cathodic side unless {math}`\alpha_a = \alpha_c`. Finally, the currents at {math}`5` and {math}`10` mV are in the ratio `2.0095`, nearly {math}`2`: at small overpotential the relation is linear and there is no Tafel behaviour to measure.

## Where next

[Cell potentials and the Nernst equation](cell_potentials.md) gives the reversible voltage that this page subtracts losses from, and [Ionic strength and activity](ionic_activities.md) corrects the concentrations behind it. The same exponential rate law appears in [Chemistry 4: Reaction Rates](../chem_04_rates.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cell_potentials.html">
    <span class="label">Previous</span>
    <span class="title">Cell potentials and the Nernst equation</span>
  </a>
  <a class="next-link" href="ionic_activities.html">
    <span class="label">Next</span>
    <span class="title">Ionic strength and activity</span>
  </a>
</div>
<!-- apiref:nav:end -->
