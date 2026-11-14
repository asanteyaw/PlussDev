<!-- generated from properties_and_systems.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Properties and a recovery ventilator

[Back to Thermal 2: Exchangers and Air](../thermal_02_exchangers_and_air.md)

Every thermal calculation needs density, specific heat and conductivity, and an easy way to get a project wrong is to type them in from different places. This page uses the property provider, reads where its numbers come from, and then combines everything in the chapter in a heat-recovery ventilator.

## The mental model

The provider is one function, `props`, that takes a fluid and a temperature in kelvin and returns density, specific heat, conductivity, viscosity and the Prandtl number. Each fluid has an explicit **state** (saturated liquid water; dry air as an ideal gas at the pressure you give), a **range** in which it is valid, and a **provenance** string. A temperature outside the range is an error: nothing is extrapolated, so a model that has left its validity says so.

## Liquid water

From freezing to boiling:

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
  // Liquid water from 0 to 100 degrees (kelvin in, SI out).
  const pluss::Series t = vec({273.15, 293.15, 313.15, 333.15, 353.15, 373.15});
  const auto p = th::props(th::Fluid::Water, t);
  out.writeln("density       ", flat(p.density));
  out.writeln("specific heat ", flat(p.specific_heat));
  out.writeln("conductivity  ", flat(p.conductivity));
  out.writeln("viscosity (uPa s)", flat(p.viscosity.mul(1e6)));
  out.writeln("Prandtl       ", flat(p.prandtl));
}
```

Output

```text
density        [999.83952, 998.20413, 992.21577, 983.19888, 971.79778, 958.36366]
specific heat  [4217.00000, 4182.11000, 4178.63000, 4185.26000, 4197.52000, 4217.00000]
conductivity   [0.56900, 0.60304, 0.63178, 0.65378, 0.66989, 0.68000]
viscosity (uPa s) [1750.00000, 1003.77000, 654.68000, 466.32000, 351.14000, 279.00000]
Prandtl        [12.96968, 6.96119, 4.33009, 2.98521, 2.20024, 1.73021]
```
The density falls from 999.8 kg/m^3 at 0 degrees to 958.4 at 100 degrees; the viscosity falls by a factor of six; the Prandtl number from 13 to 1.7 is what changes convective heat transfer so much with temperature. Viscosity is printed in micropascal-seconds, because in pascal-seconds it is of order `1e-3` and loses its digits at five places.

## Dry air

Air is an ideal gas, so its density follows the pressure, while its transport properties are tabulated at one atmosphere:

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
  // Dry air at one atmosphere from 250 K to 600 K.
  const pluss::Series t = vec({250.0, 300.0, 400.0, 500.0, 600.0});
  const auto p = th::props(th::Fluid::Air, t);
  out.writeln("density       ", flat(p.density));
  out.writeln("specific heat ", flat(p.specific_heat));
  out.writeln("conductivity  ", flat(p.conductivity));
  out.writeln("viscosity (uPa s)", flat(p.viscosity.mul(1e6)));
  out.writeln("Prandtl       ", flat(p.prandtl));
}
```

Output

```text
density        [1.41195, 1.17662, 0.88247, 0.70597, 0.58831]
specific heat  [1006.00000, 1007.00000, 1014.00000, 1030.00000, 1051.00000]
conductivity   [0.02230, 0.02630, 0.03380, 0.04070, 0.04690]
viscosity (uPa s) [15.96000, 18.46000, 23.01000, 27.01000, 30.58000]
Prandtl        [0.71999, 0.70681, 0.69030, 0.68355, 0.68528]
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
  // Air density follows the pressure; the transport properties do not. Water ignores the pressure argument.
  const pluss::Series t = vec({300.0});
  const auto atm = th::props(th::Fluid::Air, t);
  const auto high = th::props(th::Fluid::Air, t, vec({5.0 * 101325.0}));
  out.writeln("air density  1 atm", flat(atm.density), "  5 atm", flat(high.density));
  out.writeln("air viscosity (uPa s) 1 atm", flat(atm.viscosity.mul(1e6)), "  5 atm", flat(high.viscosity.mul(1e6)));
  out.writeln("water density", flat(th::props(th::Fluid::Water, t).density), "  with a pressure", flat(th::props(th::Fluid::Water, t, vec({5.0e6})).density));
}
```

Output

```text
air density  1 atm [1.17662]   5 atm [5.88312]
air viscosity (uPa s) 1 atm [18.46000]   5 atm [18.46000]
water density [996.55437]   with a pressure [996.55437]
```
Water ignores the pressure argument, since the liquid is saturated and incompressible here; air's density is exactly proportional to it, and its viscosity does not change.

## Where the numbers come from

`source` returns the name, the correlations and tables, the state and the valid range, and the range is exactly the one `props` enforces:

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
Quote the origin in a report: a number is only as good as its source and its range.

## A heat-recovery ventilator

Exhaust air at 22 degrees and 40 % relative humidity warms outdoor air at -5 degrees and 80 %, 0.1 kg/s of dry air each way, through a counter-flow exchanger of `UA = 120 W/K`. The program finds the humidity ratios with `ratio`, the capacity rates from the moist specific heat, the outlet states with `rate` and the dew point of the exhaust with `dew`.

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
  // A heat-recovery ventilator: exhaust air at 22 degrees and 40 % warms outdoor air at -5 degrees and 80 %; 0.1 kg/s of dry air each way.
  const double p = 101325.0, flow = 0.1;
  const pluss::Series w_out = th::ratio(vec({-5.0}), vec({0.8}), vec({p})), w_in = th::ratio(vec({22.0}), vec({0.4}), vec({p}));
  const double c_out = flow * (1006.0 + 1860.0 * one(w_out)), c_in = flow * (1006.0 + 1860.0 * one(w_in));
  const auto r = th::rate(vec({c_in}), vec({c_out}), vec({22.0}), vec({-5.0}), vec({120.0}), th::Flow::Counter);
  out.writeln("capacity rates: exhaust", c_in, "W/K  outdoor air", c_out, "W/K");
  out.writeln("effectiveness", flat(r.effectiveness), " supply air", flat(r.cold_out), " exhaust leaves at", flat(r.hot_out));
  const pluss::Series dew = th::dew(w_in, vec({p}));
  out.writeln("dew point of the exhaust", flat(dew), "-> the exchanger wall is colder than that, so the exhaust condenses:", one(r.hot_out) < one(dew));
  out.writeln("heat recovered (sensible)", flat(r.duty), "W");
}
```

Output

```text
capacity rates: exhaust 101.82054 W/K  outdoor air 100.96812 W/K
effectiveness [0.54430]  supply air [9.69610]  exhaust leaves at [7.42693]
dew point of the exhaust [7.79419] -> the exchanger wall is colder than that, so the exhaust condenses: true
heat recovered (sensible) [1483.83764] W
```
The effectiveness is 54 %: the supply air leaves at 9.7 degrees instead of -5 and 1.5 kW is recovered. The check at the end is the part that matters for the design: the exhaust leaves the exchanger at 7.4 degrees, below its own dew point of 7.8, so water condenses on the exhaust side. The sensible calculation is then a slight under-estimate of the recovered heat (the condensing water releases latent heat), and at lower outdoor temperatures the condensate would freeze.

## Decisions

- **Always use the provider:** it keeps every program on the same reference values and gives you a range check.
- **Kelvin for properties, Celsius for moist air:** convert at the boundary and nowhere else.
- **A fluid or a range the provider lacks:** that is a missing feature, not a reason to type constants; extend the provider.
- **Quote the origin:** the string is for the report.

## Pitfalls

A temperature outside the validity range is refused with the range stated, as is a non-positive pressure for air or a temperature that is not finite:

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
  report([&] { (void)th::props(th::Fluid::Water, vec({400.0})); });            // above the validity range of the liquid
  report([&] { (void)th::props(th::Fluid::Air, vec({100.0})); });              // below the validity range of the air tables
  report([&] { (void)th::props(th::Fluid::Air, vec({300.0}), vec({-1.0})); }); // non-positive pressure
  report([&] { (void)th::props(th::Fluid::Water, vec({std::nan("")})); });
}
```

Output

```text
eng::thermal::props: temperature must lie between 273.15 and 373.15 K for water (nothing is extrapolated)
eng::thermal::props: temperature must lie between 250 and 600 K for air (nothing is extrapolated)
eng::thermal::props: pressure must be positive
eng::thermal::props: temperature must be finite
```
Passing the wrong temperature scale to the wrong function is the commonest mistake, and both families refuse it:

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
## Where next

Reference pages: [`props`](../../../../api/eng/thermal/properties/props.md), [`Properties`](../../../../api/eng/thermal/properties/Properties.md), [`Fluid`](../../../../api/eng/thermal/properties/Fluid.md), [`source`](../../../../api/eng/thermal/properties/source.md), [`Origin`](../../../../api/eng/thermal/properties/Origin.md), and for the exchanger [`rate`](../../../../api/eng/thermal/exchangers/rate.md). Back to [Engineering guides](../../../eng.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="moist_air.html">
    <span class="label">Previous</span>
    <span class="title">Moist air</span>
  </a>
</div>
<!-- apiref:nav:end -->
