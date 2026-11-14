<!-- generated from rh.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::rh`

The relative humidity of moist air.

## Signature

```cpp
Series thermal::rh(const Series& temperature,
                    const Series& w,
                    const Series& pressure);
```

## What It Does

Inverts `ratio`: returns the vapour pressure `p w / (0.621945 + w)` divided by the saturation pressure at the temperature.

## Parameters

`temperature`
: The dry-bulb temperature in degrees Celsius.

`w`
: The humidity ratio in kg/kg, not negative.

`pressure`
: The total pressure in Pa, positive.

## Returns

The relative humidity as a fraction with the broadcast shape of the inputs. A value above 1 means the air could not hold the water as vapour.

## Errors

Throws `pluss::Error` for a temperature outside -60 to 150, a negative humidity ratio, or a non-positive pressure.

## Interpretation and Use

Warming air at fixed `w` lowers its relative humidity; cooling raises it, and above 1 water condenses. The function reports such a state rather than rejecting it, so that you can see how far past saturation a mixture is.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Relative humidity of air at 30 degrees.

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
  // Relative humidity of air at 30 degrees with w = 0.010.
  const pluss::Series phi = th::rh(vec({30.0}), vec({0.010}), vec({101325.0}));
  out.writeln("relative humidity", flat(phi));
  out.writeln("warming the same air to 35 degrees lowers it:", flat(th::rh(vec({35.0}), vec({0.010}), vec({101325.0}))));
  out.writeln("cooling it to 10 degrees pushes it above 1:", flat(th::rh(vec({10.0}), vec({0.010}), vec({101325.0}))), "(supersaturated: it would condense)");
}
```

Output

```text
relative humidity [0.37762]
warming the same air to 35 degrees lowers it: [0.28490]
cooling it to 10 degrees pushes it above 1: [1.30569] (supersaturated: it would condense)
```
### Example 2

A round trip through `ratio`.

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
  // rh inverts ratio.
  const pluss::Series t = vec({5.0, 15.0, 25.0, 40.0});
  const pluss::Series w = th::ratio(t, vec({0.6}), vec({101325.0}));
  out.writeln("w", flat(w));
  out.writeln("rh back", flat(th::rh(t, w, vec({101325.0}))));
}
```

Output

```text
w [0.00323, 0.00635, 0.01190, 0.02844]
rh back [0.60000, 0.60000, 0.60000, 0.60000]
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
  report([&] { (void)th::rh(vec({30.0}), vec({-0.01}), vec({101325.0})); });   // negative humidity ratio
  report([&] { (void)th::rh(vec({300.0}), vec({0.01}), vec({101325.0})); });   // temperature outside the range
}
```

Output

```text
eng::thermal::rh: w must not be negative
eng::thermal::rh: temperature must lie between -60 and 150 deg C
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ratio.html">
    <span class="label">Previous</span>
    <span class="title">ratio</span>
  </a>
  <a class="next-link" href="enth.html">
    <span class="label">Next</span>
    <span class="title">enth</span>
  </a>
</div>
<!-- apiref:nav:end -->
