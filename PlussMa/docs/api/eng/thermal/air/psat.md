<!-- generated from psat.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::psat`

The saturation pressure of water vapour.

## Signature

```cpp
Series thermal::psat(const Series& temperature);
```

## What It Does

Returns the pressure in pascals at which water vapour is in equilibrium with liquid water (at 0 degrees and above) or with ice (below 0 degrees), from the Hyland-Wexler correlations of the ASHRAE handbook. Valid from -60 to 150 degrees Celsius.

## Parameters

`temperature`
: The temperature in degrees Celsius, between -60 and 150.

## Returns

The saturation pressure in Pa with the shape of `temperature`.

## Errors

Throws `pluss::Error` for a temperature that is not a real floating array, not finite, or outside -60 to 150.

## Interpretation and Use

The liquid and ice correlations meet at 0 degrees to within a fraction of a pascal. The pressure is the ideal-gas value: the enhancement factor of real moist air (a fraction of a percent at one atmosphere) is not applied.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Saturation pressure from 0 to 100 degrees.

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
  // Saturation pressure of water vapour, in Pa, at familiar temperatures; the last is the boiling point at one atmosphere.
  const pluss::Series t = vec({0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 80.0, 100.0});
  out.writeln("psat", flat(th::psat(t)));
}
```

Output

```text
psat [611.21287, 1227.99528, 2338.80370, 4246.03024, 7383.46001, 12349.85647, 47411.61146, 101418.71683]
```
### Example 2

Across the freezing point.

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
  // Below 0 degrees the vapour is in equilibrium with ice. The two published fits meet at 0 to within a fraction of a pascal.
  const pluss::Series t = vec({-40.0, -20.0, -10.0, -0.001, 0.0, 0.001});
  out.writeln("temperature", flat(t));
  out.writeln("psat       ", flat(th::psat(t)));
  const double below = one(th::psat(vec({-1e-9}))), above = one(th::psat(vec({1e-9})));
  out.writeln("just below 0:", below, " just above 0:", above, " jump", above - below, "Pa");
}
```

Output

```text
temperature [-40.00000, -20.00000, -10.00000, -0.00100, 0.00000, 0.00100]
psat        [12.84525, 103.26038, 259.90286, 611.10325, 611.21287, 611.25727]
just below 0: 611.15357  just above 0: 611.21287  jump 0.05930 Pa
```
### Example 3

Rejected temperatures.

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
  report([&] { (void)th::psat(vec({-80.0})); });
  report([&] { (void)th::psat(vec({200.0})); });
  report([&] { (void)th::psat(vec({std::nan("")})); });
}
```

Output

```text
eng::thermal::psat: temperature must lie between -60 and 150 deg C
eng::thermal::psat: temperature must lie between -60 and 150 deg C
eng::thermal::psat: temperature must be finite
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="ratio.html">
    <span class="label">Next</span>
    <span class="title">ratio</span>
  </a>
</div>
<!-- apiref:nav:end -->
