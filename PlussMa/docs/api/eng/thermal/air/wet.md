<!-- generated from wet.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::wet`

The thermodynamic wet-bulb temperature of moist air.

## Signature

```cpp
Series thermal::wet(const Series& temperature,
                    const Series& w,
                    const Series& pressure);
```

## What It Does

Returns the temperature at which water evaporating adiabatically into the air saturates it: the solution of the ASHRAE energy balance between the dry-bulb state and the saturated state at the wet-bulb temperature. Above 0 degrees the balance uses liquid water; below it, ice. The root is searched between just below the dew point and the dry-bulb temperature for the whole batch.

## Parameters

`temperature`
: The dry-bulb temperature in degrees Celsius, between -50 and 100.

`w`
: The humidity ratio in kg/kg, positive.

`pressure`
: The total pressure in Pa, positive.

## Returns

The wet-bulb temperature in degrees Celsius with the broadcast shape of the inputs.

## Errors

Throws `pluss::Error` for a temperature outside -50 to 100, a humidity ratio or pressure that is not positive or not finite, and for supersaturated air (a dew point above the dry-bulb temperature).

## Interpretation and Use

Dew point, wet bulb and dry bulb coincide for saturated air and separate as the air gets drier, always in the order dew point, wet bulb, dry bulb. The thermodynamic wet bulb equals what a wetted thermometer reads in a fast stream.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The wet bulb of air at 25 degrees and 50 %.

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
  // The wet-bulb temperature of air at 25 degrees and 50 %: what a wetted thermometer reads in a fast stream.
  const double p = 101325.0;
  const pluss::Series w = th::ratio(vec({25.0}), vec({0.5}), vec({p}));
  const pluss::Series wb = th::wet(vec({25.0}), w, vec({p}));
  out.writeln("dry bulb 25  wet bulb", flat(wb), "  dew point", flat(th::dew(w, vec({p}))));
  out.writeln("the ordering dew point < wet bulb < dry bulb holds for unsaturated air");
}
```

Output

```text
dry bulb 25  wet bulb [17.88934]   dew point [13.86397]
the ordering dew point < wet bulb < dry bulb holds for unsaturated air
```
### Example 2

Saturated and drier air.

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
  // Saturated air: all three temperatures coincide. Drier air widens the gap.
  const double p = 101325.0;
  for (const double phi : {1.0, 0.8, 0.5, 0.2}) {
    const pluss::Series w = th::ratio(vec({30.0}), vec({phi}), vec({p}));
    out.writeln("relative humidity", phi, " wet bulb", flat(th::wet(vec({30.0}), w, vec({p}))), " dew point", flat(th::dew(w, vec({p}))));
  }
}
```

Output

```text
relative humidity 1.00000  wet bulb [30.00000]  dew point [30.00000]
relative humidity 0.80000  wet bulb [27.09097]  dew point [26.16859]
relative humidity 0.50000  wet bulb [22.00498]  dew point [18.44664]
relative humidity 0.20000  wet bulb [15.70417]  dew point [4.61282]
```
### Example 3

Below freezing.

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
  // Below freezing the wet bulb is an ice bulb and the energy balance changes (sublimation heat). The two branches meet at 0.
  const double p = 101325.0;
  const pluss::Series t = vec({-10.0, -2.0, 2.0, 10.0});
  const pluss::Series w = th::ratio(t, vec({0.5}), vec({p}));
  out.writeln("dry bulb", flat(t));
  out.writeln("wet bulb", flat(th::wet(t, w, vec({p}))));
}
```

Output

```text
dry bulb [-10.00000, -2.00000, 2.00000, 10.00000]
wet bulb [-11.63792, -4.67420, -1.35514, 5.53626]
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
  report([&] { (void)th::wet(vec({20.0}), vec({0.03}), vec({101325.0})); });   // supersaturated: the dew point is above 20
  report([&] { (void)th::wet(vec({150.0}), vec({0.01}), vec({101325.0})); });  // dry bulb outside -50..100
  report([&] { (void)th::wet(vec({20.0}), vec({0.0}), vec({101325.0})); });    // dry air
}
```

Output

```text
eng::thermal::wet: the air is supersaturated (dew point above the temperature)
eng::thermal::wet: temperature must lie between -50 and 100 deg C
eng::thermal::wet: w must be positive
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dew.html">
    <span class="label">Previous</span>
    <span class="title">dew</span>
  </a>
  <a class="next-link" href="MixState.html">
    <span class="label">Next</span>
    <span class="title">MixState</span>
  </a>
</div>
<!-- apiref:nav:end -->
