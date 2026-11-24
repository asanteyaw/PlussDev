# The ideal gas

[Back to Physics 4: Thermodynamics](../phys_04_thermodynamics.md)

An ideal gas ties together its pressure, volume, temperature and amount, and from those and two gas constants follow its density, heat capacities and the speed of sound. This page works with quantities that carry their dimension, so a pressure cannot be added to a volume by accident, and every relation can be run in either direction. It shows the four state relations, the properties of air, and the errors raised for values that no gas can have.

## How it works

The state of {math}`n` moles of gas is fixed by two of the three variables pressure {math}`p`, volume {math}`V` and absolute temperature {math}`T`, through

```{math}
p V = n R T, \qquad R = 8.314\,462\,618\ \mathrm{J\,mol^{-1}\,K^{-1}}
```

`thermo::volume(amount, temperature, pressure)`, `thermo::pressure(amount, temperature, volume)`, `thermo::temp(pressure, volume, amount)` and `thermo::moles(pressure, volume, temperature)` each solve this for the quantity named. Arguments are `common::ScalarQuantity<...>` values (or `SeriesQuantity` values, for many states at once) and `common::TemperaturePoint<>` for a temperature, which can be created from a value in another scale with `TemperaturePoint<>::from(0.0, common::units::celsius)` and is converted to kelvin once. The molar gas constant is the exact CODATA 2022 value.

## Volume, pressure, temperature and amount

The first program fixes one mole at {math}`0\,^\circ\mathrm{C}` and {math}`100` kPa, finds the volume, and then recovers each of the other three quantities from the rest to show that the relations close.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace thermo = pluss::ma::natsci::physics::thermo;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // one mole of an ideal gas at 0 degrees Celsius and 100 kPa
  const auto amount = common::ScalarQuantity<common::Amount>::from_si(1.0);
  const auto temperature = common::TemperaturePoint<>::from(0.0, common::units::celsius);
  const auto pressure = common::ScalarQuantity<common::Pressure>::from_si(100000.0);

  const auto volume = thermo::volume(amount, temperature, pressure);
  out.writeln("volume of one mole             = ", volume.in(common::units::cubic_metre) * 1000.0, " litres");
  // the other three relations recover each quantity from the other two
  out.writeln("pressure from n, T, V (kPa)    = ", thermo::pressure(amount, temperature, volume).si_value() / 1000.0);
  out.writeln("temperature from p, V, n (K)   = ", thermo::temp(pressure, volume, amount).si_value());
  out.writeln("amount from p, V, T (mol)      = ", thermo::moles(pressure, volume, temperature).si_value());
}
```

Output

```text
volume of one mole             = 22.7110 litres
pressure from n, T, V (kPa)    = 100.0000
temperature from p, V, n (K)   = 273.1500
amount from p, V, T (mol)      = 1.0000
```

For one mole at {math}`0\,^\circ\mathrm{C}` and {math}`100` kPa the volume is `22.7110` litres, and each of the other relations returns the input it was built from: `100.0000` kPa, `273.1500` K and `1.0000` mol.

## Air and its properties

On a mass basis the gas constant is {math}`R_s = R / M` for a molar mass {math}`M`. For a calorically perfect gas with heat ratio {math}`\gamma = c_p / c_v` the two specific heats follow from {math}`R_s`:

```{math}
c_v = \frac{R_s}{\gamma - 1}, \qquad c_p = c_v + R_s, \qquad a = \sqrt{\gamma R_s T}
```

`thermo::gas_const(molar_mass)`, `thermo::cv(gas_constant, gamma)`, `thermo::cp(gas_constant, gamma)` and `thermo::heat_ratio(cp, cv)` give these, `thermo::density(pressure, temperature, gas_constant)` is {math}`\rho = p / (R_s T)`, `thermo::sound_speed(gamma, gas_constant, temperature)` is {math}`a`, and `thermo::energy(cv, T)` and `thermo::enthalpy(cp, T)` are {math}`c_v T` and {math}`c_p T` per kilogram. `thermo::kappa(p)` and `thermo::beta(T)` are the ideal-gas compressibility {math}`1/p` and expansion coefficient {math}`1/T`. The program evaluates all of them for air at sea-level pressure and {math}`15\,^\circ\mathrm{C}`, entered in Celsius.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace thermo = natsci::physics::thermo;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // air: molar mass 28.96546 g/mol, heat ratio 1.4, at sea-level pressure and 15 degrees Celsius
  const auto molar_mass = natsci::common::ScalarQuantity<thermo::MolarMass>::from_si(0.02896546);
  const auto gamma = natsci::common::ScalarQuantity<natsci::common::Dimensionless>::from_si(1.4);
  const auto pressure = natsci::common::ScalarQuantity<natsci::common::Pressure>::from_si(101325.0);
  const auto temperature = natsci::common::TemperaturePoint<>::from(15.0, natsci::common::units::celsius);

  const auto r = thermo::gas_const(molar_mass);
  const auto cv = thermo::cv(r, gamma), cp = thermo::cp(r, gamma);
  out.writeln("specific gas constant R  = ", r.si_value(), " J/(kg K)");
  out.writeln("cv                       = ", cv.si_value(), " J/(kg K)");
  out.writeln("cp                       = ", cp.si_value(), " J/(kg K)");
  out.writeln("cp / cv                  = ", thermo::heat_ratio(cp, cv).si_value());
  out.writeln("density                  = ", thermo::density(pressure, temperature, r).si_value(), " kg/m^3");
  out.writeln("speed of sound           = ", thermo::sound_speed(gamma, r, temperature).si_value(), " m/s");
  out.writeln("internal energy (cv T)   = ", thermo::energy(cv, temperature).si_value() / 1000.0, " kJ/kg");
  out.writeln("enthalpy (cp T)          = ", thermo::enthalpy(cp, temperature).si_value() / 1000.0, " kJ/kg");
  out.writeln("isothermal compressibility 1/p = ", thermo::kappa(pressure).si_value() * 1.0e6, " 1/MPa");
  out.writeln("expansion coefficient 1/T      = ", thermo::beta(temperature).si_value(), " 1/K");
}
```

Output

```text
specific gas constant R  = 287.0475 J/(kg K)
cv                       = 717.6187 J/(kg K)
cp                       = 1004.6662 J/(kg K)
cp / cv                  = 1.4000
density                  = 1.2250 kg/m^3
speed of sound           = 340.2908 m/s
internal energy (cv T)   = 206.7818 kJ/kg
enthalpy (cp T)          = 289.4946 kJ/kg
isothermal compressibility 1/p = 9.8692 1/MPa
expansion coefficient 1/T      = 0.0035 1/K
```

For air the specific gas constant is `287.0475` J/(kg K), so with {math}`\gamma = 1.4` the heat capacities are {math}`c_v =` `717.6187` and {math}`c_p =` `1004.6662` J/(kg K), whose ratio returns `1.4000`. At sea level and {math}`15\,^\circ\mathrm{C}` the density is `1.2250` kg/m³ and the speed of sound `340.2908` m/s. The internal energy is `206.7818` kJ/kg and the enthalpy `289.4946` kJ/kg; the difference is {math}`R_s T`, the work of pushing the atmosphere aside. The compressibility is `9.8692` per MPa.

## Values no gas can have

The functions check their inputs before computing and raise an error that names the function and the argument. The program shows the three kinds of check: a volume that is not positive, a temperature at or below absolute zero, and a heat ratio that is not above one, which would make {math}`c_v` negative or infinite.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>


using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace thermo = pluss::ma::natsci::physics::thermo;

int main() {
  OutputEngine out;
  const auto amount = common::ScalarQuantity<common::Amount>::from_si(1.0);
  const auto temperature = common::TemperaturePoint<>::from_si(300.0);
  const auto volume = common::ScalarQuantity<common::Volume>::from_si(0.0249);

  // a volume that is not positive
  try {
    (void)thermo::pressure(amount, temperature, common::ScalarQuantity<common::Volume>::from_si(0.0));
  } catch (const pluss::Error &e) {
    out.writeln("zero volume:        ", e.message());
  }
  // a temperature at or below absolute zero
  try {
    (void)thermo::pressure(amount, common::TemperaturePoint<>::from_si(-5.0), volume);
  } catch (const pluss::Error &e) {
    out.writeln("negative kelvin:    ", e.message());
  }
  // a heat ratio that is not above one
  try {
    const auto r = common::ScalarQuantity<thermo::SpecificGasConstant>::from_si(287.0);
    (void)thermo::cv(r, common::ScalarQuantity<common::Dimensionless>::from_si(1.0));
  } catch (const pluss::Error &e) {
    out.writeln("heat ratio of one:  ", e.message());
  }
}
```

Output

```text
zero volume:         natsci::thermo::pressure: volume must be finite and positive
negative kelvin:     natsci::thermo::pressure: temperature must be finite and positive
heat ratio of one:   natsci::thermo::cv: heat ratio must be finite and greater than one
```

The three error lines read exactly as printed, each naming the function and the argument that failed.

## Decisions

- Build a temperature with `TemperaturePoint<>::from(value, scale)` whenever you start from a scale other than kelvin; use `from_si` only for a value that is already in kelvin.
- Use `ScalarQuantity` for one state and `SeriesQuantity` for a batch of states in one call.
- Use `gas_const`, `cv`, `cp` and `heat_ratio` to move between a gas and its specific heats; pass them the heat ratio you have measured.

## Pitfalls

A temperature point carries its scale. The program computes the volume of one mole at 101.325 kPa for 25 degrees Celsius, and for the bare number 25 entered as if it were kelvin.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace thermo = pluss::ma::natsci::physics::thermo;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // room temperature, 25 degrees Celsius, entered two ways: as a temperature in Celsius, and as the bare number 25 taken to be kelvin
  const auto amount = common::ScalarQuantity<common::Amount>::from_si(1.0);
  const auto pressure = common::ScalarQuantity<common::Pressure>::from_si(101325.0);
  const auto celsius = common::TemperaturePoint<>::from(25.0, common::units::celsius);
  const auto kelvin_by_mistake = common::TemperaturePoint<>::from_si(25.0);
  out.writeln("25 degrees Celsius: temperature ", celsius.si_value(), " K, volume ", thermo::volume(amount, celsius, pressure).in(common::units::cubic_metre) * 1000.0, " litres");
  out.writeln("25 read as kelvin : temperature ", kelvin_by_mistake.si_value(), " K, volume ", thermo::volume(amount, kelvin_by_mistake, pressure).in(common::units::cubic_metre) * 1000.0, " litres");
}
```

Output

```text
25 degrees Celsius: temperature 298.1500 K, volume 24.4654 litres
25 read as kelvin : temperature 25.0000 K, volume 2.0514 litres
```

Room temperature is `298.1500` K and gives `24.4654` litres. Building the point with `from_si(25.0)` means 25 K, and the volume is `2.0514` litres, a gas nearly liquid. Both calls are valid, so use `from` with the scale for anything not in kelvin.

## Where next

[Heat and work along a path](heat_and_work.md) uses these state relations along the four elementary processes. [Hamilton's equations](../phys_05_mechanics/hamiltons_equations.md) turns to mechanics.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="heat_and_work.html">
    <span class="label">Next</span>
    <span class="title">Heat and work along a path</span>
  </a>
</div>
<!-- apiref:nav:end -->
