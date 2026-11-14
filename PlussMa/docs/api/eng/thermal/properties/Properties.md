<!-- generated from Properties.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::Properties`

The properties of a fluid at a temperature.

## Signature

```cpp
thermal::Properties
```

## What It Does

Holds the outputs of `props`, each with the shape of the temperature (and of the pressure, when given).

## Fields

| Field | Meaning | Default |
|---|---|---|
| `density` | The density in kg/m^3. |  |
| `specific_heat` | The specific heat at constant pressure in J/(kg K). |  |
| `conductivity` | The thermal conductivity in W/(m K). |  |
| `viscosity` | The dynamic viscosity in Pa s. |  |
| `prandtl` | The Prandtl number `mu cp / k`. |  |

## Returns

A value returned by `props`.

## Errors

None: a value is only constructed by a successful call.

## Interpretation and Use

Viscosities are of order `1e-3` Pa s for water and `2e-5` for air; print them in micropascal-seconds to see the digits. The Prandtl number is derived, so it is consistent with the three properties it is built from.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Liquid water from 0 to 100 degrees.

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

Dry air from 250 to 600 K.

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

Pressure dependence.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Fluid.html">
    <span class="label">Previous</span>
    <span class="title">Fluid</span>
  </a>
  <a class="next-link" href="props.html">
    <span class="label">Next</span>
    <span class="title">props</span>
  </a>
</div>
<!-- apiref:nav:end -->
