<!-- generated from vol.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::vol`

The specific volume of moist air.

## Signature

```cpp
Series thermal::vol(const Series& temperature,
                     const Series& w,
                     const Series& pressure);
```

## What It Does

Returns `287.042 (t + 273.15) (1 + 1.607858 w) / p` in cubic metres per kilogram of dry air.

## Parameters

`temperature`
: The dry-bulb temperature in degrees Celsius.

`w`
: The humidity ratio in kg/kg, not negative.

`pressure`
: The total pressure in Pa, positive.

## Returns

The specific volume in m^3 per kg of dry air with the broadcast shape of the inputs.

## Errors

Throws `pluss::Error` for a temperature outside -60 to 150, a negative humidity ratio, or a non-positive pressure.

## Interpretation and Use

The mixture density is `(1 + w) / vol`; humid air is lighter than dry air at the same temperature and pressure because water vapour is lighter than air. Volume flow of an air stream is dry-air mass flow times `vol`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Specific volume and density at 25 degrees.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static double one(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>()[0]; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // Specific volume (m^3 per kg of dry air) and the density of the moist air it implies.
  const double p = 101325.0;
  const pluss::Series v = th::vol(vec({25.0}), vec({0.010}), vec({p}));
  out.writeln("specific volume", flat(v), " closed form", 287.042 * 298.15 * (1.0 + 1.607858 * 0.010) / p);
  out.writeln("density of the mixture", (1.0 + 0.010) / one(v), "kg/m^3");
  out.writeln("dry air at 25 degrees:", flat(th::vol(vec({25.0}), vec({0.0}), vec({p}))));
}
```

Output

```text
specific volume [0.85820]  closed form 0.85820
density of the mixture 1.17688 kg/m^3
dry air at 25 degrees: [0.84462]
```
### Example 2

Humid air is lighter.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static double one(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>()[0]; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // Humid air is lighter than dry air at the same temperature and pressure.
  const double p = 101325.0;
  const pluss::Series dry = th::vol(vec({25.0}), vec({0.0}), vec({p})), humid = th::vol(vec({25.0}), vec({0.020}), vec({p}));
  out.writeln("dry density ", 1.0 / one(dry), "kg/m^3");
  out.writeln("humid density", 1.020 / one(humid), "kg/m^3 (w = 0.020), lighter by", 1.0 - (1.020 / one(humid)) * one(dry), "of the dry value");
}
```

Output

```text
dry density  1.18396 kg/m^3
humid density 1.17001 kg/m^3 (w = 0.020), lighter by 0.01178 of the dry value
```
### Example 3

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
  report([&] { (void)th::vol(vec({25.0}), vec({0.01}), vec({0.0})); });       // zero pressure
  report([&] { (void)th::vol(vec({25.0}), vec({-0.01}), vec({101325.0})); }); // negative humidity ratio
}
```

Output

```text
eng::thermal::vol: pressure must be positive
eng::thermal::vol: w must not be negative
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="enth.html">
    <span class="label">Previous</span>
    <span class="title">enth</span>
  </a>
  <a class="next-link" href="dew.html">
    <span class="label">Next</span>
    <span class="title">dew</span>
  </a>
</div>
<!-- apiref:nav:end -->
