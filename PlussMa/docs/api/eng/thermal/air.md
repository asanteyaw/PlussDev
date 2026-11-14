<!-- generated from air.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Moist air

The state of moist air in degrees Celsius and pascals, with the enthalpy referenced to dry air and liquid water at 0 degrees: saturation pressure, humidity ratio, relative humidity, enthalpy, volume, dew point, wet bulb and adiabatic mixing.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

Humidity ratio of air at 25 degrees and 50 %.

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

Dew point, wet bulb and dry bulb.

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

Mixing outdoor and return air.

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
  // Unequal flows: an economiser mixing outdoor air (-5 degrees, w = 0.002) into return air (22 degrees, w = 0.008). Fraction of outdoor air from 0 to 1.
  const pluss::Series outdoor = vec({0.0, 0.25, 0.5, 0.75, 1.0});
  const auto m = th::mix(vec({-5.0}), vec({0.002}), outdoor.add(1e-9), vec({22.0}), vec({0.008}), outdoor.neg().add(1.0).add(1e-9));
  out.writeln("outdoor fraction", flat(outdoor));
  out.writeln("mixed temperature", flat(m.temperature));
  out.writeln("mixed w          ", flat(m.w));
}
```

Output

```text
outdoor fraction [0.00000, 0.25000, 0.50000, 0.75000, 1.00000]
mixed temperature [22.00000, 15.30549, 8.57419, 1.80580, -5.00000]
mixed w           [0.00800, 0.00650, 0.00500, 0.00350, 0.00200]
```
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

air/psat
air/ratio
air/rh
air/enth
air/vol
air/dew
air/wet
air/MixState
air/mix
```
<!-- apiref:toctree:end -->
