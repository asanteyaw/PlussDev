# Heat and work along a path

[Back to Physics 4: Thermodynamics](../phys_04_thermodynamics.md)

When a gas is heated or compressed the energy it receives as heat, the energy it delivers as work and the change in what it stores must balance. The four elementary processes, at constant volume, constant pressure, constant temperature and without heat flow, each fix a path through the states of the gas. This page evaluates each path and returns the heat, the work and the change in stored energy, checks the first law, and integrates the pressure along a path to get the work directly.

## How it works

The first law of thermodynamics says that the change in stored energy equals the heat put in minus the work taken out:

```{math}
\Delta U = Q - W
```

At constant volume the gas does no work, so {math}`Q = \Delta U = m c_v \Delta T`. At constant pressure the gas also pushes on its surroundings, so {math}`Q = m c_p \Delta T` and the work is the difference {math}`W = m (c_p - c_v) \Delta T = m R_s \Delta T`. `thermo::iso_v(mass, cv, T1, T2)` and `thermo::iso_p(mass, cp, cv, T1, T2)` return a path with the fields `final_temp`, `heat`, `work` and `delta_u`. The helpers `thermo::delta_e(heat, work)`, `thermo::heat_in(delta_u, work)` and `thermo::work_out(heat, delta_u)` each solve the first law for one of its three terms.

## Constant volume and constant pressure

The program heats {math}`1` kg of air from {math}`300` K to {math}`400` K both ways and checks the balance with the helpers.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace thermo = natsci::physics::thermo;

template <class Path> static void show(OutputEngine &out, const char *name, const Path &p) {
  out.writeln(name, ": final T = ", p.final_temp.si_value(), " K, heat in = ", p.heat.si_value(), " J, work out = ", p.work.si_value(), " J, change in U = ", p.delta_u.si_value(), " J");
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  // 1 kg of air: cv = 717.6 J/(kg K), cp = 1004.7 J/(kg K), heated from 300 K to 400 K
  const auto mass = common::ScalarQuantity<common::Mass>::from_si(1.0);
  const auto cv = common::ScalarQuantity<thermo::SpecificHeat>::from_si(717.6187);
  const auto cp = common::ScalarQuantity<thermo::SpecificHeat>::from_si(1004.6662);
  const auto cold = common::TemperaturePoint<>::from_si(300.0), hot = common::TemperaturePoint<>::from_si(400.0);

  show(out, "constant volume  ", thermo::iso_v(mass, cv, cold, hot));    // no work, all the heat is stored
  show(out, "constant pressure", thermo::iso_p(mass, cp, cv, cold, hot)); // the gas also pushes on its surroundings

  // the first law ties the three together: change in stored energy = heat in - work out
  const auto path = thermo::iso_p(mass, cp, cv, cold, hot);
  out.writeln("delta_e(heat, work) = ", thermo::delta_e(path.heat, path.work).si_value(), " J");
  out.writeln("heat_in(stored, work) = ", thermo::heat_in(path.delta_u, path.work).si_value(), " J");
  out.writeln("work_out(heat, stored) = ", thermo::work_out(path.heat, path.delta_u).si_value(), " J");
}
```

Output

```text
constant volume  : final T = 400.00 K, heat in = 71761.87 J, work out = 0.00 J, change in U = 71761.87 J
constant pressure: final T = 400.00 K, heat in = 100466.62 J, work out = 28704.75 J, change in U = 71761.87 J
delta_e(heat, work) = 71761.87 J
heat_in(stored, work) = 100466.62 J
work_out(heat, stored) = 28704.75 J
```

At constant volume {math}`1` kg of air heated from {math}`300` K to {math}`400` K takes in `71761.87` J, all of it stored. At constant pressure it takes in `100466.62` J, of which `28704.75` J leaves as work, and the stored change is still `71761.87` J: the helpers return all three numbers consistently.

## Isothermal and adiabatic expansion

If one mole of ideal gas expands from {math}`V_1` to {math}`V_2` at constant temperature, its stored energy does not change, so all the work it does is paid for by heat flowing in:

```{math}
W = Q = n R T \ln\frac{V_2}{V_1}
```

If no heat flows, the gas cools as it does work, and the temperature and volume obey

```{math}
T_2 = T_1\left(\frac{V_1}{V_2}\right)^{\gamma - 1}, \qquad W = -\Delta U = m c_v (T_1 - T_2)
```

`thermo::iso_t(amount, T, V1, V2)` and `thermo::iso_s(mass, cv, gamma, T1, V1, V2)` return these paths. For a path that follows an arbitrary pressure law, `thermo::pv_work(pressure_fn, V1, V2)` integrates {math}`W = \int p\,\mathrm{d}V` numerically. The function receives the volume and returns a pressure; the program uses it for the isothermal pressure {math}`p = nRT/V` and compares the result with the closed form.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace thermo = natsci::physics::thermo;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  // one mole of air at 300 K expands from 10 litres to 40 litres, in two ways
  const auto amount = common::ScalarQuantity<common::Amount>::from_si(1.0);
  const auto start = common::TemperaturePoint<>::from_si(300.0);
  const auto v1 = common::ScalarQuantity<common::Volume>::from_si(0.010), v2 = common::ScalarQuantity<common::Volume>::from_si(0.040);

  // 1. isothermal: the gas stays at 300 K, so the work it does is paid for by heat flowing in
  const auto isothermal = thermo::iso_t(amount, start, v1, v2);
  out.writeln("isothermal: final T = ", isothermal.final_temp.si_value(), " K, work out = ", isothermal.work.si_value(), " J, heat in = ", isothermal.heat.si_value(), " J, change in U = ", isothermal.delta_u.si_value(), " J");

  // 2. adiabatic: no heat flows, so the gas cools as it does the work (0.029 kg of air, gamma = 1.4)
  const auto mass = common::ScalarQuantity<common::Mass>::from_si(0.02896546);
  const auto cv = common::ScalarQuantity<thermo::SpecificHeat>::from_si(717.6187);
  const auto gamma = common::ScalarQuantity<common::Dimensionless>::from_si(1.4);
  const auto adiabatic = thermo::iso_s(mass, cv, gamma, start, v1, v2);
  out.writeln("adiabatic:  final T = ", adiabatic.final_temp.si_value(), " K, work out = ", adiabatic.work.si_value(), " J, heat in = ", adiabatic.heat.si_value(), " J, change in U = ", adiabatic.delta_u.si_value(), " J");

  // 3. the work is the area under the pressure curve: integrate p(V) = n R T / V from V1 to V2
  const auto moles = common::SeriesQuantity<common::Amount>::from_si(pluss::series::make(1.0));
  const auto area = thermo::pv_work(
      [moles, start](const common::SeriesQuantity<common::Volume> &volume) { return thermo::pressure(moles, start, volume); }, v1, v2);
  out.writeln("area under p(V) by quadrature = ", area.value.si_value().item<double>(), " J, converged = ", area.converged ? "yes" : "no");
  out.writeln("closed form n R T ln(V2/V1)   = ", 8.31446261815324 * 300.0 * std::log(4.0), " J");
}
```

Output

```text
isothermal: final T = 300.00 K, work out = 3457.89 J, heat in = 3457.89 J, change in U = 0.00 J
adiabatic:  final T = 172.30 K, work out = 2654.29 J, heat in = 0.00 J, change in U = -2654.29 J
area under p(V) by quadrature = 3457.89 J, converged = yes
closed form n R T ln(V2/V1)   = 3457.89 J
```

In the isothermal expansion from {math}`10` to {math}`40` litres one mole does `3457.89` J of work and absorbs the same amount of heat with no change in {math}`U`; the quadrature of {math}`p\,\mathrm{d}V` gives `3457.89` J and reports that it converged, matching {math}`nRT\ln 4`. The adiabatic path does `2654.29` J of work at the expense of its own energy, cooling from `300` K to `172.30` K.

## Compression and heating

The adiabatic relation explains why a compressed gas is hot: the work done on it has nowhere to go but into stored energy. The program compresses {math}`1` kg of air from {math}`300` K by four compression ratios and prints the final temperature and the work done on the gas; with a negative sign convention, `iso_s` reports the work *out*, so the work done *on* the gas is its negative.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace thermo = pluss::ma::natsci::physics::thermo;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  // compress 1 kg of air (0.8 m^3) adiabatically from 300 K, by compression ratios 2, 4, 8 and 16
  const auto mass = common::ScalarQuantity<common::Mass>::from_si(1.0);
  const auto cv = common::ScalarQuantity<thermo::SpecificHeat>::from_si(717.6187);
  const auto gamma = common::ScalarQuantity<common::Dimensionless>::from_si(1.4);
  const auto start = common::TemperaturePoint<>::from_si(300.0);
  const auto big = common::ScalarQuantity<common::Volume>::from_si(0.8);
  for (double ratio : {2.0, 4.0, 8.0, 16.0}) {
    const auto small = common::ScalarQuantity<common::Volume>::from_si(0.8 / ratio);
    const auto path = thermo::iso_s(mass, cv, gamma, start, big, small);
    out.writeln("compression ratio ", ratio, ": final T = ", path.final_temp.si_value(), " K, work done on the gas = ", -path.work.si_value() / 1000.0, " kJ");
  }
}
```

Output

```text
compression ratio 2.00: final T = 395.85 K, work done on the gas = 68.79 kJ
compression ratio 4.00: final T = 522.33 K, work done on the gas = 159.55 kJ
compression ratio 8.00: final T = 689.22 K, work done on the gas = 279.31 kJ
compression ratio 16.00: final T = 909.43 K, work done on the gas = 437.34 kJ
```

Compressing air adiabatically by ratios of `2`, `4`, `8` and `16` heats it to `395.85`, `522.33`, `689.22` and `909.43` K, and costs `68.79`, `159.55`, `279.31` and `437.34` kJ: the relation {math}`T_2 = T_1 r^{0.4}` is why a diesel engine can ignite fuel by compression alone.

## Decisions

- Pick the path function that matches how the process is held: `iso_v` for a rigid container, `iso_p` for a free piston, `iso_t` when heat flows freely, `iso_s` when it cannot.
- Use `pv_work` for a path none of these describes: give it the pressure as a function of volume.
- Read the three first-law helpers as a check on any set of numbers you have by other means.

## Pitfalls

`iso_p` takes the heat capacity at constant pressure first, then the one at constant volume. The program calls it in both orders, and also compresses the gas adiabatically to show the sign of the work.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace thermo = pluss::ma::natsci::physics::thermo;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  // 1 kg of air heated from 300 K to 400 K at constant pressure: iso_p takes cp first, then cv. Swapping them changes the work
  const auto mass = common::ScalarQuantity<common::Mass>::from_si(1.0);
  const auto cv = common::ScalarQuantity<thermo::SpecificHeat>::from_si(717.6187);
  const auto cp = common::ScalarQuantity<thermo::SpecificHeat>::from_si(1004.6662);
  const auto cold = common::TemperaturePoint<>::from_si(300.0), hot = common::TemperaturePoint<>::from_si(400.0);
  const auto right = thermo::iso_p(mass, cp, cv, cold, hot);
  out.writeln("iso_p(mass, cp, cv, ...): heat ", right.heat.si_value(), " J, work out ", right.work.si_value(), " J");
  try {
    const auto swapped = thermo::iso_p(mass, cv, cp, cold, hot);
    out.writeln("iso_p(mass, cv, cp, ...): heat ", swapped.heat.si_value(), " J, work out ", swapped.work.si_value(), " J");
  } catch (const pluss::Error &e) {
    out.writeln("swapped: ", e.message());
  }
  // iso_s reports the work the gas does, so compressing gives a negative number; the work done on the gas is its negative
  const auto gamma = common::ScalarQuantity<common::Dimensionless>::from_si(1.4);
  const auto big = common::ScalarQuantity<common::Volume>::from_si(0.8), small = common::ScalarQuantity<common::Volume>::from_si(0.2);
  const auto squeeze = thermo::iso_s(mass, cv, gamma, cold, big, small);
  out.writeln("compression: work out = ", squeeze.work.si_value(), " J (negative: the surroundings did work on the gas)");
}
```

Output

```text
iso_p(mass, cp, cv, ...): heat 100466.62 J, work out 28704.75 J
swapped: natsci::thermo::heat_ratio: cp - cv must be finite and positive
compression: work out = -159548.41 J (negative: the surroundings did work on the gas)
```

In the right order the gas takes in `100466.62` J and does `28704.75` J of work. Swapped, the call is refused because `cp - cv` is no longer positive. The compression returns `-159548.41` J of work: the sign convention is work done *by* the gas, so a negative value is work done on it.

## Where next

[The ideal gas](ideal_gas.md) is the page for the state variables used here. [Kepler orbits](../phys_01_orbits/kepler_orbits.md) is the other place where the energy of a system is tracked.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ideal_gas.html">
    <span class="label">Previous</span>
    <span class="title">The ideal gas</span>
  </a>
</div>
<!-- apiref:nav:end -->
