<!-- generated from source.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::source`

Where the properties of a fluid come from.

## Signature

```cpp
thermal::Origin thermal::source(thermal::Fluid fluid);
```

## What It Does

Returns the provenance of a fluid: its name, the correlations and published tables it is built from, the state it describes and its range of validity.

## Parameters

`fluid`
: The fluid.

## Returns

An `Origin`.

## Errors

None: both fluids are always valid.

## Interpretation and Use

Quote the origin in a report: a number from `props` is only as good as its source and its range.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Water.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const th::Origin s = th::source(th::Fluid::Water);
  out.writeln("name:", s.name);
  out.writeln("range:", s.t_min, "to", s.t_max, "K");
  out.writeln("state:", s.state);
  out.writeln("reference:", s.reference);
}
```

Output

```text
name: water
range: 273.15000 to 373.15000 K
state: saturated liquid; pressure does not enter; no enthalpy reference is carried
reference: density: Kell (1975) polynomial; specific heat, conductivity and viscosity: Incropera, DeWitt, Bergman and Lavine, Fundamentals of Heat and Mass Transfer, Table A.6, linearly interpolated
```
### Example 2

Air.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const th::Origin s = th::source(th::Fluid::Air);
  out.writeln("name:", s.name);
  out.writeln("range:", s.t_min, "to", s.t_max, "K");
  out.writeln("state:", s.state);
  out.writeln("reference:", s.reference);
}
```

Output

```text
name: air
range: 250.00000 to 600.00000 K
state: dry air as an ideal gas at the given pressure (default 101325 Pa); transport properties do not depend on pressure
reference: density: ideal gas with R = 287.05 J/(kg K); specific heat, conductivity and viscosity: Incropera, DeWitt, Bergman and Lavine, Fundamentals of Heat and Mass Transfer, Table A.4 (one atmosphere), linearly interpolated
```
### Example 3

The valid ranges.

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
  <a class="prev-link" href="Origin.html">
    <span class="label">Previous</span>
    <span class="title">Origin</span>
  </a>
</div>
<!-- apiref:nav:end -->
