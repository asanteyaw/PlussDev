<!-- generated from Fluid.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::Fluid`

A fluid of the property provider.

## Signature

```cpp
thermal::Fluid
```

## What It Does

Selects the fluid for `props` and `source`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `Water` | Liquid water, saturated, from 273.15 K to 373.15 K. |  |
| `Air` | Dry air as an ideal gas, from 250 K to 600 K. |  |

## Returns

A choice of two fluids.

## Errors

None: both values are always valid.

## Interpretation and Use

Each fluid carries its range and its provenance; `source` returns them.

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

The provenance of each.

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
  // The range in the provenance is the range props accepts: check before calling, or take the error.
  for (const auto fluid : {th::Fluid::Water, th::Fluid::Air}) {
    const th::Origin s = th::source(fluid);
    out.writeln(s.name, "accepts", s.t_min, "to", s.t_max, "K");
    out.writeln("  at the limits", flat(th::props(fluid, vec({s.t_min, s.t_max})).density));
  }
}
```

Output

```text
water accepts 273.15000 to 373.15000 K
  at the limits [999.83952, 958.36366]
air accepts 250.00000 to 600.00000 K
  at the limits [1.41195, 0.58831]
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Properties.html">
    <span class="label">Next</span>
    <span class="title">Properties</span>
  </a>
</div>
<!-- apiref:nav:end -->
