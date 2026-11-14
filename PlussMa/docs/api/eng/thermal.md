<!-- generated from thermal.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Thermal engineering

Lumped thermal networks, conduction and fins, heat exchangers, moist air and a property provider. A network is solved by the circuit engine, so batches, step schemes and sensitivities carry over. Closed-form pieces take and return arrays that broadcast, so one call sweeps a design.

## Choosing A Surface

| Task | Branch |
|---|---|
| Network of nodes with capacities, fixed temperatures and heats; steady and transient | [Thermal networks](thermal/network.md) |
| Wall, tube, shell and film resistances, Biot number, fins and a layered wall mesh | [Conduction and fins](thermal/conduction.md) |
| Effectiveness, transfer units, rating and log-mean difference | [Heat exchangers](thermal/exchangers.md) |
| Saturation, humidity ratio, enthalpy, dew point, wet bulb and mixing | [Moist air](thermal/air.md) |
| Properties with ranges and provenance | [Fluid properties](thermal/properties.md) |

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

A room as a thermal network.

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
  // A room: air (node 0) and a wall (node 1) against the outside (node 2) at -5 degrees; 1 kW heats the air.
  //   air -- 40 W/K -- wall -- 25 W/K -- outside,   plus 30 W/K of ventilation from air straight to outside
  th::Net net;
  net.nodes = 3;
  net.links = {{0, 1, 0}, {1, 2, 2}, vec({40.0, 25.0, 30.0})};
  net.stores = {{0, 1}, vec({5.0e4, 4.0e6})};
  net.fixed = {{2}, vec({-5.0})};
  net.heats = {{0}, vec({1000.0})};
  const auto run = th::steady(net);
  out.writeln("air, wall", flat(run.temperature));
  out.writeln("flows (air-wall, wall-outside, air-outside)", flat(run.link_flow));
  out.writeln("the outside absorbs", flat(run.fixed_flow), "W of the 1000 W");
  const double rise = 1000.0 / (30.0 + 1.0 / (1.0 / 40.0 + 1.0 / 25.0));
  out.writeln("closed form air rise over the outside", rise, " air", -5.0 + rise);
}
```

Output

```text
air, wall [17.03390, 8.55932, -5.00000]
flows (air-wall, wall-outside, air-outside) [338.98305, 338.98305, 661.01695]
the outside absorbs [-1000.00000] W of the 1000 W
closed form air rise over the outside 22.03390  air 17.03390
```
### Example 2

A fin and its tip conditions.

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
  // The three tip conditions for the same fin at two lengths. A convective tip adds the tip area; once the fin is long the three agree.
  const double h = 25.0, k = 200.0, p = 0.016, a = 1.6e-5;
  for (const double length : {0.05, 0.3}) {
    out.writeln("length", length, "m");
    for (const auto tip : {th::Tip::Adiabatic, th::Tip::Convective, th::Tip::Infinite}) {
      const auto f = th::fin(vec({h}), vec({k}), vec({p}), vec({a}), vec({length}), vec({100.0}), vec({20.0}), tip);
      out.writeln(tip == th::Tip::Adiabatic ? "  adiabatic " : tip == th::Tip::Convective ? "  convective" : "  infinite  ", "heat", flat(f.heat), "W  tip", flat(f.tip));
    }
  }
}
```

Output

```text
length 0.05000 m
  adiabatic  heat [1.45183] W  tip [88.94402]
  convective heat [1.47546] W  tip [88.55523]
  infinite   heat [2.86217] W  tip [20.00000]
length 0.30000 m
  adiabatic  heat [2.85519] W  tip [25.58370]
  convective heat [2.85534] W  tip [25.52211]
  infinite   heat [2.86217] W  tip [20.00000]
```
### Example 3

The state of moist air.

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
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

thermal/network
thermal/conduction
thermal/exchangers
thermal/air
thermal/properties
```
<!-- apiref:toctree:end -->
