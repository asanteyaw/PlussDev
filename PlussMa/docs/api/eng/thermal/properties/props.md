<!-- generated from props.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::props`

The properties of a fluid.

## Signature

```cpp
thermal::Properties thermal::props(thermal::Fluid fluid,
                               const Series& temperature,
                               const Series& pressure = {});
```

## What It Does

Evaluates density, specific heat, conductivity, viscosity and the Prandtl number at a temperature. Water is the saturated liquid: its density is the Kell polynomial and the other properties are interpolated tabulated values; the pressure does not enter. Air is an ideal gas: its density is `p / (R T)` with `R = 287.05 J/(kg K)` and the other properties are interpolated tabulated values that do not depend on pressure. Nothing is extrapolated.

## Parameters

`fluid`
: The fluid.

`temperature`
: The temperature in kelvin, within the range of the fluid.

`pressure`
: The pressure in Pa, used by air only; undefined means 101325 Pa.

## Returns

A `Properties` with the shape of the temperature.

## Errors

Throws `pluss::Error` for a temperature that is not a real floating array or not finite, a temperature outside the range of the fluid (the message gives the range), and, for air, a pressure that is not positive.

## Interpretation and Use

Properties are taken from one place so that every thermal calculation shares the same reference values and the same provenance, available from `source`. Use the provider rather than typing a specific heat into a program: the range check will tell you when a model is outside its validity.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Liquid water.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // Liquid water from 0 to 100 degrees (kelvin in, SI out).
  const pluss::Series t = vec({273.15, 293.15, 313.15, 333.15, 353.15, 373.15});
  const auto p = th::props(th::Fluid::Water, t);
  out.writeln("density       ", flat(p.density));
  out.writeln("specific heat ", flat(p.specific_heat));
  out.writeln("conductivity  ", flat(p.conductivity));
  out.writeln("viscosity (uPa s)", flat(p.viscosity.mul(1e6)));
  out.writeln("Prandtl       ", flat(p.prandtl));
}
```

Output

```text
density        [999.83952, 998.20413, 992.21577, 983.19888, 971.79778, 958.36366]
specific heat  [4217.00000, 4182.11000, 4178.63000, 4185.26000, 4197.52000, 4217.00000]
conductivity   [0.56900, 0.60304, 0.63178, 0.65378, 0.66989, 0.68000]
viscosity (uPa s) [1750.00000, 1003.77000, 654.68000, 466.32000, 351.14000, 279.00000]
Prandtl        [12.96968, 6.96119, 4.33009, 2.98521, 2.20024, 1.73021]
```
### Example 2

Dry air.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // Dry air at one atmosphere from 250 K to 600 K.
  const pluss::Series t = vec({250.0, 300.0, 400.0, 500.0, 600.0});
  const auto p = th::props(th::Fluid::Air, t);
  out.writeln("density       ", flat(p.density));
  out.writeln("specific heat ", flat(p.specific_heat));
  out.writeln("conductivity  ", flat(p.conductivity));
  out.writeln("viscosity (uPa s)", flat(p.viscosity.mul(1e6)));
  out.writeln("Prandtl       ", flat(p.prandtl));
}
```

Output

```text
density        [1.41195, 1.17662, 0.88247, 0.70597, 0.58831]
specific heat  [1006.00000, 1007.00000, 1014.00000, 1030.00000, 1051.00000]
conductivity   [0.02230, 0.02630, 0.03380, 0.04070, 0.04690]
viscosity (uPa s) [15.96000, 18.46000, 23.01000, 27.01000, 30.58000]
Prandtl        [0.71999, 0.70681, 0.69030, 0.68355, 0.68528]
```
### Example 3

The effect of pressure.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // Air density follows the pressure; the transport properties do not. Water ignores the pressure argument.
  const pluss::Series t = vec({300.0});
  const auto atm = th::props(th::Fluid::Air, t);
  const auto high = th::props(th::Fluid::Air, t, vec({5.0 * 101325.0}));
  out.writeln("air density  1 atm", flat(atm.density), "  5 atm", flat(high.density));
  out.writeln("air viscosity (uPa s) 1 atm", flat(atm.viscosity.mul(1e6)), "  5 atm", flat(high.viscosity.mul(1e6)));
  out.writeln("water density", flat(th::props(th::Fluid::Water, t).density), "  with a pressure", flat(th::props(th::Fluid::Water, t, vec({5.0e6})).density));
}
```

Output

```text
air density  1 atm [1.17662]   5 atm [5.88312]
air viscosity (uPa s) 1 atm [18.46000]   5 atm [18.46000]
water density [996.55437]   with a pressure [996.55437]
```
### Example 4

Rejected inputs.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  report([&] { (void)th::props(th::Fluid::Water, vec({400.0})); });            // above the validity range of the liquid
  report([&] { (void)th::props(th::Fluid::Air, vec({100.0})); });              // below the validity range of the air tables
  report([&] { (void)th::props(th::Fluid::Air, vec({300.0}), vec({-1.0})); }); // non-positive pressure
  report([&] { (void)th::props(th::Fluid::Water, vec({std::nan("")})); });
}
```

Output

```text
eng::thermal::props: temperature must lie between 273.15 and 373.15 K for water (nothing is extrapolated)
eng::thermal::props: temperature must lie between 250 and 600 K for air (nothing is extrapolated)
eng::thermal::props: pressure must be positive
eng::thermal::props: temperature must be finite
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Properties.html">
    <span class="label">Previous</span>
    <span class="title">Properties</span>
  </a>
  <a class="next-link" href="Origin.html">
    <span class="label">Next</span>
    <span class="title">Origin</span>
  </a>
</div>
<!-- apiref:nav:end -->
