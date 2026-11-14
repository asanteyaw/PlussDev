<!-- generated from ratio.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::ratio`

The humidity ratio of moist air from relative humidity.

## Signature

```cpp
Series thermal::ratio(const Series& temperature,
                       const Series& relative,
                       const Series& pressure);
```

## What It Does

Returns `w = 0.621945 p_v / (p - p_v)` with the vapour pressure `p_v = relative * psat(temperature)`: kilograms of water per kilogram of dry air.

## Parameters

`temperature`
: The dry-bulb temperature in degrees Celsius.

`relative`
: The relative humidity as a fraction in `[0, 1]`.

`pressure`
: The total pressure in Pa, positive.

## Returns

The humidity ratio in kg/kg with the broadcast shape of the inputs.

## Errors

Throws `pluss::Error` for a temperature outside -60 to 150, a relative humidity outside `[0, 1]`, a non-positive pressure, and a vapour pressure that reaches the total pressure.

## Interpretation and Use

At the same temperature and relative humidity air at altitude carries more water per kilogram of dry air, because the total pressure is lower.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

25 degrees and 50 %.

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
  // Humidity ratio of air at 25 degrees and 50 % relative humidity at sea-level pressure.
  const double p = 101325.0;
  const pluss::Series w = th::ratio(vec({25.0}), vec({0.5}), vec({p}));
  const double pw = 0.5 * one(th::psat(vec({25.0})));
  out.writeln("vapour pressure", pw, "Pa  w =", flat(w), "kg/kg  closed form", 0.621945 * pw / (p - pw));
  out.writeln("saturated at 25 degrees:", flat(th::ratio(vec({25.0}), vec({1.0}), vec({p}))));
  out.writeln("dry air:", flat(th::ratio(vec({25.0}), vec({0.0}), vec({p}))));
}
```

Output

```text
vapour pressure 1584.60824 Pa  w = [0.00988] kg/kg  closed form 0.00988
saturated at 25 degrees: [0.02008]
dry air: [0.00000]
```
### Example 2

The effect of altitude.

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
  // The same temperature and relative humidity at altitude hold more water per kilogram of dry air: the total pressure is lower.
  const pluss::Series p = vec({101325.0, 89875.0, 79495.0, 54020.0});
  out.writeln("pressure (sea level, 1000 m, 2000 m, 5000 m)", flat(p));
  out.writeln("w at 20 degrees, 50 %", flat(th::ratio(vec({20.0}), vec({0.5}), p)));
}
```

Output

```text
pressure (sea level, 1000 m, 2000 m, 5000 m) [101325.00000, 89875.00000, 79495.00000, 54020.00000]
w at 20 degrees, 50 % [0.00726, 0.00820, 0.00929, 0.01376]
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
  report([&] { (void)th::ratio(vec({25.0}), vec({1.5}), vec({101325.0})); });    // relative humidity above 1
  report([&] { (void)th::ratio(vec({25.0}), vec({-0.1}), vec({101325.0})); });   // negative
  report([&] { (void)th::ratio(vec({25.0}), vec({0.5}), vec({0.0})); });         // zero pressure
  report([&] { (void)th::ratio(vec({120.0}), vec({0.9}), vec({50000.0})); });    // vapour pressure reaches the total
}
```

Output

```text
eng::thermal::ratio: relative humidity must lie in [0, 1]
eng::thermal::ratio: relative humidity must lie in [0, 1]
eng::thermal::ratio: pressure must be positive
eng::thermal::ratio: the vapour pressure reaches the total pressure (the air boils)
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="psat.html">
    <span class="label">Previous</span>
    <span class="title">psat</span>
  </a>
  <a class="next-link" href="rh.html">
    <span class="label">Next</span>
    <span class="title">rh</span>
  </a>
</div>
<!-- apiref:nav:end -->
