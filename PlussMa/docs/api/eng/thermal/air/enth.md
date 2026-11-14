<!-- generated from enth.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::enth`

The specific enthalpy of moist air.

## Signature

```cpp
Series thermal::enth(const Series& temperature,
                      const Series& w);
```

## What It Does

Returns `1006 t + w (2501000 + 1860 t)` in joules per kilogram of dry air. The reference state is dry air at 0 degrees Celsius and saturated liquid water at 0 degrees Celsius, both with zero enthalpy; `2501000` J/kg is the latent heat of vaporisation at 0 degrees and `1006` and `1860` J/(kg K) the specific heats of dry air and of water vapour.

## Parameters

`temperature`
: The dry-bulb temperature in degrees Celsius.

`w`
: The humidity ratio in kg/kg, not negative.

## Returns

The enthalpy in J per kg of dry air with the broadcast shape of the inputs.

## Errors

Throws `pluss::Error` for a temperature outside -60 to 150 or a negative humidity ratio.

## Interpretation and Use

A cooling load is the difference of enthalpies times the dry-air mass flow. The first term of the enthalpy is the sensible part and the term in `w` the latent part, so a load splits cleanly into both.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Enthalpy at 25 degrees.

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
  // Enthalpy of air at 25 degrees with w = 0.010, per kg of dry air; zero is dry air at 0 degrees and liquid water at 0 degrees.
  const pluss::Series h = th::enth(vec({25.0}), vec({0.010}));
  out.writeln("enthalpy", flat(h), "J/kg  closed form", 1006.0 * 25.0 + 0.010 * (2501000.0 + 1860.0 * 25.0));
  out.writeln("dry air at 0 degrees:", flat(th::enth(vec({0.0}), vec({0.0}))), " saturated vapour only at 0 degrees, w = 0.01:", flat(th::enth(vec({0.0}), vec({0.010}))));
}
```

Output

```text
enthalpy [50625.00000] J/kg  closed form 50625.00000
dry air at 0 degrees: [0.00000]  saturated vapour only at 0 degrees, w = 0.01: [25010.00000]
```
### Example 2

Sensible and latent parts of a cooling load.

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
  // Sensible and latent parts of a cooling load: cooling 30 degrees, w = 0.014 to 15 degrees, w = 0.0106 for 1 kg/s of dry air.
  const double h1 = one(th::enth(vec({30.0}), vec({0.014}))), h2 = one(th::enth(vec({15.0}), vec({0.0106})));
  const double sensible = 1006.0 * 15.0 + 0.0106 * 1860.0 * 15.0;
  out.writeln("total load", h1 - h2, "W   sensible", sensible, "W   latent", (0.014 - 0.0106) * (2501000.0 + 1860.0 * 30.0), "W");
}
```

Output

```text
total load 24078.86000 W   sensible 15385.74000 W   latent 8693.12000 W
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
  report([&] { (void)th::enth(vec({25.0}), vec({-0.01})); });    // negative humidity ratio
  report([&] { (void)th::enth(vec({-100.0}), vec({0.0})); });    // temperature below the supported range
}
```

Output

```text
eng::thermal::enth: w must not be negative
eng::thermal::enth: temperature must lie between -60 and 150 deg C
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rh.html">
    <span class="label">Previous</span>
    <span class="title">rh</span>
  </a>
  <a class="next-link" href="vol.html">
    <span class="label">Next</span>
    <span class="title">vol</span>
  </a>
</div>
<!-- apiref:nav:end -->
