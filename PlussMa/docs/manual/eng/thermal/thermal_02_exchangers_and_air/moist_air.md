<!-- generated from moist_air.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Moist air

[Back to Thermal 2: Exchangers and Air](../thermal_02_exchangers_and_air.md)

Air in a building carries water vapour, and the water matters for comfort, for condensation and for the energy it takes to cool: a good part of a cooling coil's load is latent. This page computes the state of moist air from dry-bulb temperature, humidity and pressure, finds its dew point and wet bulb, splits a load into sensible and latent parts, and mixes streams.

## The mental model

Moist air is a mixture of dry air and water vapour, and its state is fixed by three numbers: the dry-bulb temperature `t` (degrees Celsius), the humidity ratio `w` (kilograms of water per kilogram of dry air) and the total pressure `p` (pascals). Everything else follows: relative humidity compares the vapour pressure with the saturation pressure at `t`; enthalpy is per kilogram of **dry** air; the dew point is the temperature at which the saturation pressure equals the vapour pressure. The reference state of the enthalpy is dry air at 0 degrees and liquid water at 0 degrees, both zero. These functions use degrees Celsius and pascals; the property provider on the next page uses kelvin.

## Saturation pressure

The pressure at which water vapour is in equilibrium with liquid water, and below 0 degrees with ice:

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
The last value is the boiling point at one atmosphere. Across the freezing point the liquid and ice correlations meet:

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
## Humidity ratio and relative humidity

The humidity ratio of air at 25 degrees and 50 % relative humidity:

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
At altitude the same temperature and relative humidity carry more water per kilogram of dry air, because the total pressure is lower:

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
`rh` runs the other way. Warming air at constant `w` lowers its relative humidity; cooling raises it, and above 1 the air could not hold the water as vapour:

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
## Enthalpy, sensible and latent

The enthalpy per kilogram of dry air is `1006 t + w (2501000 + 1860 t)`: a sensible part for the air and the vapour, and the latent heat carried by the vapour.

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
A cooling coil that takes 1 kg/s of dry air from 30 degrees and `w = 0.014` to 15 degrees and `w = 0.0106` removes the difference of the two enthalpies; the formula splits that load exactly into a sensible part for the temperature drop and a latent part for the water removed:

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
More than a third of this load is latent, which a sensible-only calculation would miss entirely.

## Volume and density

The specific volume per kilogram of dry air gives the volume flow of an air stream, and the density of the mixture is `(1 + w) / vol`:

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
Humid air is lighter than dry air at the same temperature and pressure, because water vapour is lighter than air:

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
## Dew point and wet bulb

The dew point is the temperature to which air must be cooled, at constant `w`, before water starts to condense; at that temperature `rh` is exactly one.

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
The wet bulb is the temperature a wetted thermometer settles at in a fast stream. For unsaturated air the order is always dew point below wet bulb below dry bulb:

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
They coincide at saturation and separate as the air dries:

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
Below freezing the wet bulb becomes an ice bulb, and the function switches to the ice energy balance automatically:

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
## Mixing streams

Mixing conserves dry air, water and enthalpy. Equal flows of warm, humid air and cool, dry air:

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
  // Equal mass flows of 30 degree air (w = 0.012) and 10 degree air (w = 0.006): w averages, enthalpy averages, temperature follows.
  const auto m = th::mix(vec({30.0}), vec({0.012}), vec({1.0}), vec({10.0}), vec({0.006}), vec({1.0}));
  out.writeln("mixed w", flat(m.w), " temperature", flat(m.temperature));
  out.writeln("enthalpy of the mixture", flat(th::enth(m.temperature, m.w)), " mean of the two", 0.5 * (one(th::enth(vec({30.0}), vec({0.012}))) + one(th::enth(vec({10.0}), vec({0.006})))));
}
```

Output

```text
mixed w [0.00900]  temperature [20.05456]
enthalpy of the mixture [43019.60000]  mean of the two 43019.60000
```
The humidity ratio averages exactly and the temperature, recovered from the averaged enthalpy, is 20.05 rather than the arithmetic 20 because the specific heat of the mixture depends on `w`. An economiser mixes outdoor air into the return air in a controllable fraction:

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
## Decisions

- **Which state variables:** `t`, `w` and `p`; convert from relative humidity with `ratio` and back with `rh`.
- **Load:** enthalpy difference times dry-air mass flow, split into sensible (temperature) and latent (humidity ratio) parts.
- **Is a surface wet:** compare its temperature with the dew point of the air next to it.
- **Mixing:** use `mix`, not an average of temperatures.

## Pitfalls

Mixing two saturated streams can make air that is supersaturated: the straight-line mix lies above the saturation curve and fog forms.

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
  // Mixing two saturated streams makes supersaturated air: the straight-line mix lies above the saturation curve and fog forms.
  const double p = 101325.0;
  const pluss::Series w1 = th::ratio(vec({5.0}), vec({1.0}), vec({p})), w2 = th::ratio(vec({30.0}), vec({1.0}), vec({p}));
  const auto m = th::mix(vec({5.0}), w1, vec({1.0}), vec({30.0}), w2, vec({1.0}));
  out.writeln("mixed temperature", flat(m.temperature), " w", flat(m.w));
  out.writeln("saturation w at that temperature", flat(th::ratio(m.temperature, vec({1.0}), vec({p}))), "-> supersaturated, relative humidity", flat(th::rh(m.temperature, m.w, vec({p}))));
}
```

Output

```text
mixed temperature [17.74455]  w [0.01630]
saturation w at that temperature [0.01272] -> supersaturated, relative humidity [1.27406]
```
`mix` returns the state before condensation, and `rh` reports 1.27, so the result is visibly unphysical rather than silently wrong.

Units are the other trap: these functions take degrees Celsius, so a temperature in kelvin is refused (and the property provider, which takes kelvin, refuses Celsius):

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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  // The moist-air functions take degrees Celsius; the property provider takes kelvin. Passing kelvin to a psychrometric call is refused.
  const double kelvin = 293.15;
  report([&] { (void)th::psat(vec({kelvin})); });
  out.writeln("psat at 20 degrees Celsius:", flat(th::psat(vec({kelvin - 273.15}))), "Pa");
  report([&] { (void)th::props(th::Fluid::Water, vec({20.0})); });
  out.writeln("water density at 293.15 K:", flat(th::props(th::Fluid::Water, vec({kelvin})).density), "kg/m^3");
}
```

Output

```text
eng::thermal::psat: temperature must lie between -60 and 150 deg C
psat at 20 degrees Celsius: [2338.80370] Pa
eng::thermal::props: temperature must lie between 273.15 and 373.15 K for water (nothing is extrapolated)
water density at 293.15 K: [998.20413] kg/m^3
```
Humidity above 1, a vapour pressure that reaches the total pressure, a dew point that does not exist (dry air) and supersaturated air in `wet` are all refused with the reason named:

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
## Where next

Reference pages: [`psat`](../../../../api/eng/thermal/air/psat.md), [`ratio`](../../../../api/eng/thermal/air/ratio.md), [`rh`](../../../../api/eng/thermal/air/rh.md), [`enth`](../../../../api/eng/thermal/air/enth.md), [`vol`](../../../../api/eng/thermal/air/vol.md), [`dew`](../../../../api/eng/thermal/air/dew.md), [`wet`](../../../../api/eng/thermal/air/wet.md), [`mix`](../../../../api/eng/thermal/air/mix.md), [`MixState`](../../../../api/eng/thermal/air/MixState.md). Next: [Properties and a recovery ventilator](properties_and_systems.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="exchangers.html">
    <span class="label">Previous</span>
    <span class="title">Heat exchangers</span>
  </a>
  <a class="next-link" href="properties_and_systems.html">
    <span class="label">Next</span>
    <span class="title">Properties and a recovery ventilator</span>
  </a>
</div>
<!-- apiref:nav:end -->
