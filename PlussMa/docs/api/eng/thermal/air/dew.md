<!-- generated from dew.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::dew`

The dew-point temperature of moist air.

## Signature

```cpp
Series thermal::dew(const Series& w,
                    const Series& pressure);
```

## What It Does

Returns the temperature at which the saturation pressure equals the vapour pressure `p w / (0.621945 + w)`: the temperature to which the air must be cooled, at constant `w`, before water starts to condense. It is found by a bracketed root search over -60 to 150 degrees for the whole batch.

## Parameters

`w`
: The humidity ratio in kg/kg, positive.

`pressure`
: The total pressure in Pa, positive.

## Returns

The dew point in degrees Celsius with the broadcast shape of the inputs.

## Errors

Throws `pluss::Error` for a humidity ratio or pressure that is not positive or not finite, and for a dew point outside -60 to 150 degrees.

## Interpretation and Use

Dry air (`w = 0`) has no dew point and is rejected. A surface colder than the dew point of the air next to it collects water.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The dew point of air at 25 degrees and 50 %.

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
  // The dew point of air at 25 degrees and 50 % relative humidity, and the check that the air is saturated there.
  const double p = 101325.0;
  const pluss::Series w = th::ratio(vec({25.0}), vec({0.5}), vec({p}));
  const pluss::Series td = th::dew(w, vec({p}));
  out.writeln("dew point", flat(td), "degrees");
  out.writeln("relative humidity at the dew point", flat(th::rh(td, w, vec({p}))));
}
```

Output

```text
dew point [13.86397] degrees
relative humidity at the dew point [1.00000]
```
### Example 2

A sweep of humidity ratios.

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
  // A sweep of humidity ratios, the dew point of each, and the saturation pressure there equals the vapour pressure.
  const double p = 101325.0;
  const pluss::Series w = vec({0.002, 0.005, 0.010, 0.020});
  const pluss::Series td = th::dew(w, vec({p}));
  out.writeln("dew points", flat(td));
  out.writeln("psat at the dew point", flat(th::psat(td)));
  out.writeln("vapour pressure p w / (0.621945 + w)", flat(w.mul(p).div(w.add(0.621945))));
}
```

Output

```text
dew points [-7.46530, 3.90540, 14.04537, 24.93423]
psat at the dew point [324.78824, 808.08524, 1603.38321, 3156.81250]
vapour pressure p w / (0.621945 + w) [324.78824, 808.08524, 1603.38321, 3156.81250]
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
  report([&] { (void)th::dew(vec({0.0}), vec({101325.0})); });        // dry air has no dew point
  report([&] { (void)th::dew(vec({0.5}), vec({101325.0})); });        // so humid the dew point is above 150 degrees
  report([&] { (void)th::dew(vec({0.01}), vec({-1.0})); });
}
```

Output

```text
eng::thermal::dew: w must be positive
eng::thermal::dew: pressure must be positive
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="vol.html">
    <span class="label">Previous</span>
    <span class="title">vol</span>
  </a>
  <a class="next-link" href="wet.html">
    <span class="label">Next</span>
    <span class="title">wet</span>
  </a>
</div>
<!-- apiref:nav:end -->
