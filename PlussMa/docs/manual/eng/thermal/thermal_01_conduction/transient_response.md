<!-- generated from transient_response.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Transient response

[Back to Thermal 1: Conduction](../thermal_01_conduction.md)

A cup of coffee cools, a room warms after the heating is switched on, a component heats up under load. This page adds thermal capacity to the network and follows the temperatures through time: a body cooling, a start from a given state, boundaries and heaters that change, and the choice of step.

## The mental model

A body that holds heat has a thermal capacity `C` in joules per kelvin. Heat flowing in raises its temperature at `Q / C`, so for one body losing heat through a conductance `G` to surroundings at `T_inf`,

```{math}
C \frac{dT}{dt} = -G\,(T - T_\infty) + q ,
```

and the temperature relaxes towards `T_inf + q / G` with the **time constant** `C / G`. A network is many such bodies coupled; its response is a sum of exponentials, and the fastest of them sets how short a step must be. `tran` takes the same `Net` as `steady`, plus `Stores`, a vector of times, a `Drive` for boundaries and heats that change, and a `Start` for where the stores begin. Every array of the result gains a time axis in front of its element axis.

## A body cooling

A body of 100 J/K at 100 degrees cools through 0.5 W/K to surroundings at 20 degrees. The time constant is `100 / 0.5 = 200 s`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A body of 100 J/K, started at 100 degrees, cools through 0.5 W/K to surroundings at 20 degrees: T = 20 + 80 exp(-t G / C).
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({0.5})};
  net.stores = {{0}, vec({100.0})};
  net.fixed = {{1}, vec({20.0})};
  const auto run = th::tran(net, vec(grid(2.0, 101)), {}, th::Start{vec({100.0})});
  const std::vector<double> body = flat(run.temperature.select(1, 0));
  for (int s : {0, 25, 50, 100}) {
    out.writeln("t =", 2.0 * s, "s  T =", body[s], " closed form", 20.0 + 80.0 * std::exp(-2.0 * s * 0.5 / 100.0));
  }
  out.writeln("shape", run.temperature.sizes(), " (time, node)");
  out.writeln("time constant C / G =", 100.0 / 0.5, "s");
}
```

Output

```text
t = 0.00000 s  T = 100.00000  closed form 100.00000
t = 50.00000 s  T = 82.30393  closed form 82.30406
t = 100.00000 s  T = 68.52225  closed form 68.52245
t = 200.00000 s  T = 49.43011  closed form 49.43036
shape [101, 2]  (time, node)
time constant C / G = 200.00000 s
```
The numerical values agree with `20 + 80 exp(-t / 200)` to better than a thousandth of a kelvin on a 2 s step: the default scheme is second order, so halving the step divides the error by four. The shape `[101, 2]` is time then node; every result follows that rule.

## Where the stores start

Without a `Start` every store begins at zero, which on a Celsius scale is rarely where you want it. A store started at the ambient stays there; a store left at the default warms towards the boundary.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // Without a Start the stores begin at zero on the temperature scale you use.
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({0.5})};
  net.stores = {{0}, vec({100.0})};
  net.fixed = {{1}, vec({20.0})};
  const pluss::Series time = vec(grid(50.0, 5));
  const auto cold = th::tran(net, time);
  th::Start warm;
  warm.temperature = vec({20.0});
  const auto settled = th::tran(net, time, {}, warm);
  out.writeln("default start", flat(cold.temperature.select(1, 0)));
  out.writeln("start at the ambient", flat(settled.temperature.select(1, 0)));
}
```

Output

```text
default start [0.00000, 4.44444, 7.90123, 10.58985, 12.68099]
start at the ambient [20.00000, 20.00000, 20.00000, 20.00000, 20.00000]
```
The default start is a disturbance of 20 degrees below the surroundings, and the body creeps up with the time constant of 200 s. Start where the physics starts: at the ambient, or at the steady state of an earlier operating point.

## Things that change in time

A boundary that moves (the weather, a thermostat) is a `Drive::temperature`; a heater that switches is a `Drive::power`. Each has one row per time sample. Here the surroundings step from 20 to 40 degrees at 100 s and a 50 W heater runs between 200 s and 300 s.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // The surroundings step from 20 to 40 degrees at 100 s, and a 50 W heater runs between 200 s and 300 s.
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({0.5})};
  net.stores = {{0}, vec({100.0})};
  net.fixed = {{1}, vec({20.0})};
  net.heats = {{0}, vec({0.0})};
  std::vector<double> ambient, heater;
  for (int k = 0; k <= 60; ++k) {
    const double t = 10.0 * k;
    ambient.push_back(t < 100.0 ? 20.0 : 40.0);
    heater.push_back(t >= 200.0 && t < 300.0 ? 50.0 : 0.0);
  }
  th::Drive drive;
  drive.temperature = mat(ambient, {61, 1});
  drive.power = mat(heater, {61, 1});
  th::Start start;
  start.temperature = vec({20.0});
  const auto run = th::tran(net, vec(grid(10.0, 61)), drive, start);
  const std::vector<double> body = flat(run.temperature.select(1, 0));
  for (int s : {5, 10, 20, 30, 40, 60}) out.writeln("t =", 10.0 * s, "s  T =", body[s]);
  out.writeln("boundary exchange at 600 s", flat(run.fixed_flow.select(0, 60)), "W");
}
```

Output

```text
t = 50.00000 s  T = 20.00000
t = 100.00000 s  T = 20.48780
t = 200.00000 s  T = 30.60551
t = 300.00000 s  T = 71.21678
t = 400.00000 s  T = 58.93196
t = 600.00000 s  T = 46.96323
boundary exchange at 600 s [-3.48161] W
```
The body stays at 20 degrees until the surroundings step, rises slowly towards 40, takes the heater's 50 W as an extra `50 / 0.5 = 100 K` that it only partly reaches in 100 s, and then relaxes back. The last line is the heat the boundary exchanges at the final sample; it is negative because the body, at about 47 degrees, is warmer than the 40-degree surroundings, so the boundary absorbs heat.

A member of the `Drive` that you leave undefined holds the stored value, and a defined one replaces it. The next program shows both on the same body.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // An undefined Drive member keeps the stored values; a defined one replaces them sample by sample.
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({0.5})};
  net.stores = {{0}, vec({100.0})};
  net.fixed = {{1}, vec({20.0})};
  th::Start warm;
  warm.temperature = vec({20.0});
  const pluss::Series time = vec(grid(100.0, 4));
  const auto held = th::tran(net, time, {}, warm);
  th::Drive hot;
  hot.temperature = mat({20, 60, 60, 60}, {4, 1});
  const auto driven = th::tran(net, time, hot, warm);
  out.writeln("stored ambient   ", flat(held.temperature.select(1, 0)));
  out.writeln("ambient steps up ", flat(driven.temperature.select(1, 0)));
}
```

Output

```text
stored ambient    [20.00000, 20.00000, 20.00000, 20.00000]
ambient steps up  [20.00000, 28.00000, 40.80000, 48.48000]
```
## The step and the scheme

`Trapezoid`, the default, averages the equations at the two ends of the step. It is exact for a straight-line response and conserves energy, but when a step is much longer than the fastest time constant it rings. `Gear2` extends backwards over two earlier steps and damps. A step five time constants long shows the difference:

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A step much longer than the time constant: the trapezoid rule rings around the answer, the second-order backward rule does not.
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({1.0})};
  net.stores = {{0}, vec({1.0})};
  net.fixed = {{1}, vec({10.0})};
  th::Start start;
  start.temperature = vec({0.0});
  const auto trapezoid = th::tran(net, vec(grid(5.0, 6)), {}, start, ckt::Scheme::Trapezoid);
  const auto gear = th::tran(net, vec(grid(5.0, 6)), {}, start, ckt::Scheme::Gear2);
  out.writeln("trapezoid", flat(trapezoid.temperature.select(1, 0)));
  out.writeln("gear2    ", flat(gear.temperature.select(1, 0)));
  std::vector<double> exact;
  for (int k = 0; k <= 5; ++k) exact.push_back(10.0 * (1.0 - std::exp(-5.0 * k)));
  out.writeln("exact    ", exact);
  out.writeln("each step is five time constants long; the exact curve has settled after the first");
}
```

Output

```text
trapezoid [0.00000, 14.28571, 8.16327, 10.78717, 9.66264, 10.14458]
gear2     [0.00000, 8.33333, 10.25641, 10.20710, 10.04400, 9.99761]
exact     [0.00000, 9.93262, 9.99955, 10.00000, 10.00000, 10.00000]
each step is five time constants long; the exact curve has settled after the first
```
The exact answer is within 0.7 % of 10 after the first step. The trapezoid values swing 14.3, 8.2, 10.8, 9.7, 10.1 around it before settling; `Gear2` overshoots by 2.6 % once and is within 0.03 % of 10 by the fifth step. When a long step cannot be avoided, use `Gear2`; when the step is short against every time constant the two agree and the trapezoid is the more accurate.

## A room over six days

All the pieces together: a two-layer wall meshed into cells, films on both faces, ventilation, a heater, and an outdoor temperature that swings by 5 degrees every day. The program lays the network out, drives it for six days in 10-minute steps and compares the settled room temperature with the steady state at the mean outdoor temperature.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A room: air, a two-layer wall (brick then insulation, 20 m^2), the outdoors with a daily swing, ventilation and a heater.
  th::Layers layers;
  layers.cells = {4, 2};
  layers.thickness = vec({0.2, 0.05});
  layers.conductivity = vec({0.8, 0.04});
  layers.density = vec({1800.0, 30.0});
  layers.specific_heat = vec({900.0, 1400.0});
  th::Slab w = th::mesh(layers, vec({20.0}).reshape({}));
  const std::int64_t air = w.net.nodes, outdoors = w.net.nodes + 1;       // nodes after the slab's surfaces
  w.net.nodes += 2;
  const auto link = [&](std::int64_t a, std::int64_t b, double g) {
    w.net.links.a.push_back(a);
    w.net.links.b.push_back(b);
    w.net.links.conductance = w.net.links.conductance.unite({vec({g})}, -1);
  };
  link(air, w.inner, 8.0 * 20.0);          // film inside
  link(w.outer, outdoors, 25.0 * 20.0);    // film outside
  link(air, outdoors, 15.0);               // ventilation
  w.net.stores.node.push_back(air);
  w.net.stores.capacity = w.net.stores.capacity.unite({vec({6.0e4})}, -1);
  w.net.fixed = {{outdoors}, vec({5.0})};
  w.net.heats = {{air}, vec({400.0})};
  // Mean outdoor temperature 5 degrees, swinging 5 degrees either way over a 24 h cycle; six days in 10 minute steps.
  const double day = 86400.0;
  std::vector<double> outside;
  for (int k = 0; k < 865; ++k) outside.push_back(5.0 + 5.0 * std::sin(2.0 * M_PI * 600.0 * k / day));
  th::Drive drive;
  drive.temperature = mat(outside, {865, 1});
  th::Start start;
  start.temperature = vec(std::vector<double>(7, 15.0));
  const auto run = th::tran(w.net, vec(grid(600.0, 865)), drive, start);
  const std::vector<double> room = flat(run.temperature.select(1, air));
  out.writeln("room temperature at the end of day 1, 2, 4, 6", room[144], room[288], room[576], room[864]);
  double low = 1e9, high = -1e9;
  for (int k = 720; k <= 864; ++k) low = std::min(low, room[k]), high = std::max(high, room[k]);
  out.writeln("last day: lowest", low, " highest", high, " swing", high - low);
  th::Net mean = w.net;
  mean.fixed = {{outdoors}, vec({5.0})};
  out.writeln("steady state at the mean outdoor temperature", flat(th::steady(mean).temperature.select(0, air)));
}
```

Output

```text
room temperature at the end of day 1, 2, 4, 6 18.11836 18.55693 19.06690 19.30873
last day: lowest 18.95140  highest 20.15160  swing 1.20020
steady state at the mean outdoor temperature [19.80823]
```
The room approaches its long-run behaviour over several days, because the wall's capacity is large against its conductance, and the daily outdoor swing of 10 K shows up in the room as a swing of only about 1.2 K: the wall's capacity smooths it. The range of the last day, 18.95 to 20.15 degrees, brackets the steady state at the mean outdoor temperature, 19.81: a linear network driven by a periodic input oscillates around its steady state at the mean input.

## Decisions

- **Step size:** a few steps per smallest time constant, and at least a few per period of the fastest drive. Check by halving it.
- **Which scheme:** the trapezoid rule for smooth responses and oscillations, `Gear2` when the network is stiff (very different time constants) or the step is long.
- **Lumped or meshed:** one store per body is right when the Biot number is below about 0.1 (see [Walls, fins and meshes](walls_and_fins.md)); otherwise mesh the body.
- **Starting state:** the ambient for a disturbance, a steady state for continuing a run.

## Pitfalls

A drive or start that does not fit the network is refused with the element named: a power drive with no heats, a drive with the wrong number of rows, a start with no stores, a capacity that is not positive.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

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
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({0.5})};
  net.stores = {{0}, vec({100.0})};
  net.fixed = {{1}, vec({20.0})};
  const pluss::Series time = vec(grid(10.0, 5));
  th::Net bare = net;
  bare.heats = {};
  th::Drive power;
  power.power = mat({1, 1, 1, 1, 1}, {5, 1});
  report([&] { (void)th::tran(bare, time, power); });          // a power drive with no heats
  th::Drive wrong;
  wrong.temperature = mat({20, 20, 20}, {3, 1});
  report([&] { (void)th::tran(net, time, wrong); });           // a drive with the wrong number of time samples
  th::Net open = net;
  open.stores = {};
  th::Start start;
  start.temperature = vec({1.0});
  report([&] { (void)th::tran(open, time, {}, start); });      // a start temperature with no stores
  th::Net heavy = net;
  heavy.stores.capacity = vec({-100.0});
  report([&] { (void)th::tran(heavy, time); });                // a capacity that is not positive
}
```

Output

```text
eng::thermal::tran: drive.power needs heats in the network
eng::thermal::tran: drive.temperature needs one row per time sample (5), got 3
eng::thermal::tran: start.temperature needs stores in the network
eng::thermal::tran: stores.capacity must be positive
```
And, as in steady state, a start in kelvin against boundaries in Celsius is not an error but a wrong answer; keep one scale.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A network is blind to the temperature scale, so mixing scales gives a wrong answer without an error:
  // the boundary is at 20 degrees Celsius, but the body was started at 293.15 (kelvin).
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({0.5})};
  net.stores = {{0}, vec({100.0})};
  net.fixed = {{1}, vec({20.0})};
  th::Start mixed, consistent;
  mixed.temperature = vec({293.15});
  consistent.temperature = vec({20.0});
  const pluss::Series time = vec(grid(200.0, 4));
  out.writeln("mixed scales   ", flat(th::tran(net, time, {}, mixed).temperature.select(1, 0)), " the body appears to cool to 20 from 293");
  out.writeln("one scale      ", flat(th::tran(net, time, {}, consistent).temperature.select(1, 0)), " the body is already at the boundary");
}
```

Output

```text
mixed scales    [293.15000, 111.05000, 50.35000, 30.11667]  the body appears to cool to 20 from 293
one scale       [20.00000, 20.00000, 20.00000, 20.00000]  the body is already at the boundary
```
## Where next

Reference pages: [`tran`](../../../../api/eng/thermal/network/tran.md), [`Stores`](../../../../api/eng/thermal/network/Stores.md), [`Drive`](../../../../api/eng/thermal/network/Drive.md), [`Start`](../../../../api/eng/thermal/network/Start.md), [`Result`](../../../../api/eng/thermal/network/Result.md). Next: [Walls, fins and meshes](walls_and_fins.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="networks_and_steady_state.html">
    <span class="label">Previous</span>
    <span class="title">Networks and steady state</span>
  </a>
  <a class="next-link" href="walls_and_fins.html">
    <span class="label">Next</span>
    <span class="title">Walls, fins and meshes</span>
  </a>
</div>
<!-- apiref:nav:end -->
