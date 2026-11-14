<!-- generated from transport.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Transport of quality and heat

This branch follows what the water carries through the flows of a run: `transport` follows a concentration or the age of the water, and `heat` the temperature with its exchange with the surroundings. Both share the cells, step and Courant limit of `RouteOptions`.

## Choosing A Surface

Use `transport` for a disinfectant, a tracer or the age of the water, and `heat` for a thermal main. Choose the cell count and step for the sharpness of the front, and read the conservation error.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

Two sources mixing at a junction.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <functional>
#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static void attempt(const char *label, const std::function<void()> &call) {
  try {
    call();
    std::cout << label << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << label << ": " << e.what() << "\n";
  }
}

// two reservoirs feed junction 2 through pipes of different length
static fluid::Net two_sources() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 0.0, 5.0});
  net.nodes.demand = vec({0.0, 0.0, 0.02});
  net.nodes.head = vec({50.0, 48.0, 40.0});
  net.pipes.from = {0, 1};
  net.pipes.to = {2, 2};
  net.pipes.length = vec({600.0, 300.0});
  net.pipes.diameter = vec({0.15, 0.15});
  net.pipes.roughness = vec({1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto network = fluid::network(two_sources());
  const auto flow = flat(fluid::steady(network).flow);
  fluid::Schedule schedule;
  schedule.duration = 1800.0;
  schedule.step = 120.0;
  schedule.pattern_step = 24.0 * 3600.0;
  fluid::Quality quality;
  quality.initial = vec({1.0, 0.0, 0.0});
  fluid::RouteOptions route;
  route.step = 10.0;
  route.courant = 0.9;
  route.cells = 20;
  const auto r = fluid::transport(network, schedule, quality, route);
  out.writeln("shapes: node ", r.node.sizes(), ", link ", r.link.sizes(), ", tank ", r.tank.sizes(), ", mass ", r.mass.sizes());
  const auto time = flat(r.hydraulic.time), node = flat(r.node), mass = flat(r.mass), err = flat(r.mass_error);
  for (std::size_t i = 0; i < time.size(); i += 3)
    out.writeln("t ", time[i], " s: junction ", node[3 * i + 2], ", stored mass ", mass[i], ", mass error ", err[i]);
  out.writeln("final ", node.back(), ", flow-weighted average ", flow[0] / (flow[0] + flow[1]));
}
```

Output

```text
shapes: node [16, 3], link [16, 2], tank [16, 0], mass [16]
t 0.00000 s: junction 0.00000, stored mass 0.00000, mass error 0.00000
t 360.00000 s: junction 0.00005, stored mass 4.72697, mass error -0.00000
t 720.00000 s: junction 0.20884, stored mass 9.11669, mass error -0.00000
t 1080.00000 s: junction 0.62419, stored mass 10.54425, mass error -0.00000
t 1440.00000 s: junction 0.65622, stored mass 10.60245, mass error -0.00000
t 1800.00000 s: junction 0.65652, stored mass 10.60287, mass error -0.00000
final 0.65652, flow-weighted average 0.65653
```
### Example 2

A decay checked against its exact value.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <functional>
#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static void attempt(const char *label, const std::function<void()> &call) {
  try {
    call();
    std::cout << label << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << label << ": " << e.what() << "\n";
  }
}

// a reservoir at 50 m feeding a junction through a 1 km pipe of 0.15 m; the pipe holds 17.67 m3 and the draw is 0.01 m3/s
static fluid::Net line(double demand = 0.01) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.15});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

static const double kPi = 3.14159265358979323846;
static const double kVolume = kPi * 0.15 * 0.15 / 4.0 * 1000.0; // m3
static const double kTransit = kVolume / 0.01;                   // s

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto network = fluid::network(line());
  const std::int64_t cells = 20;
  fluid::Schedule schedule;
  schedule.duration = 4.0 * 3600.0;
  schedule.step = kTransit / cells;
  schedule.pattern_step = 24.0 * 3600.0;
  fluid::RouteOptions route;
  route.step = kTransit / cells; // Courant number one: the front is carried exactly
  route.courant = 1.0;
  route.cells = cells;
  out.writeln("pipe volume ", kVolume, " m3, transit time ", kTransit, " s");
  for (double k : {0.0, 1e-4, 3e-4}) {
    fluid::Quality quality;
    quality.initial = vec({1.0, 0.0});
    quality.pipe_rate = vec({-k}); // a negative rate is a decay
    const auto r = fluid::transport(network, schedule, quality, route);
    out.writeln("k = ", k, ": junction ", flat(r.node).back(), ", exp(-k T) = ", std::exp(-k * kTransit), ", mass error ", flat(r.mass_error).back());
  }
  // the same decay with Courant 0.9 and 4 cells blurs the front
  fluid::RouteOptions coarse;
  coarse.step = 20.0;
  coarse.courant = 0.9;
  coarse.cells = 4;
  fluid::Quality quality;
  quality.initial = vec({1.0, 0.0});
  quality.pipe_rate = vec({-1e-4});
  fluid::Schedule longer = schedule;
  longer.step = 60.0;
  out.writeln("4 cells, Courant 0.9: junction ", flat(fluid::transport(network, longer, quality, coarse).node).back(), ", exact ", std::exp(-1e-4 * kTransit));
}
```

Output

```text
pipe volume 17.671459 m3, transit time 1767.145868 s
k = 0.000000: junction 1.000000, exp(-k T) = 1.000000, mass error -0.000000
k = 0.000100: junction 0.838020, exp(-k T) = 0.838019, mass error -0.000000
k = 0.000300: junction 0.588525, exp(-k T) = 0.588520, mass error -0.000000
4 cells, Courant 0.9: junction 0.841060, exact 0.838019
```
### Example 3

A pipe cooling.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <functional>
#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static void attempt(const char *label, const std::function<void()> &call) {
  try {
    call();
    std::cout << label << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << label << ": " << e.what() << "\n";
  }
}

// a reservoir at 50 m feeding a junction through a 1 km pipe of 0.15 m; the pipe holds 17.67 m3 and the draw is 0.01 m3/s
static fluid::Net line(double demand = 0.01) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.15});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

static const double kPi = 3.14159265358979323846;
static const double kVolume = kPi * 0.15 * 0.15 / 4.0 * 1000.0; // m3
static const double kTransit = kVolume / 0.01;                   // s

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto network = fluid::network(line());
  const double density = 998.2, cp = 4180.0;
  const std::int64_t cells = 20;
  fluid::Schedule schedule;
  schedule.duration = 4.0 * 3600.0;
  schedule.step = kTransit / cells;
  schedule.pattern_step = 24.0 * 3600.0;
  fluid::RouteOptions route;
  route.step = kTransit / cells;
  route.courant = 1.0;
  route.cells = cells;
  for (double ua : {0.0, 5000.0, 20000.0, 80000.0}) {
    fluid::Heat heat;
    heat.initial = vec({340.0, 288.0});
    heat.ambient = vec({288.0});
    heat.ua = vec({ua});
    const auto r = fluid::heat(network, schedule, heat, route);
    const double exact = 288.0 + 52.0 * std::exp(-ua * kTransit / (density * cp * kVolume));
    out.writeln("UA ", ua, " W/K: junction ", flat(r.node).back(), " K, exact ", exact, " K, energy ", flat(r.energy).back(), " J, energy error ", flat(r.energy_error).back(), " J");
  }
}
```

Output

```text
UA 0.0000 W/K: junction 340.0000 K, exact 340.0000 K, energy 25069470652.2325 J, energy error 0.0000 J
UA 5000.0000 W/K: junction 334.1276 K, exact 334.1276 K, energy 24837837101.9181 J, energy error 0.0000 J
UA 20000.0000 W/K: junction 320.1985 K, exact 320.1982 K, energy 24245008735.7612 J, energy error -0.0004 J
UA 80000.0000 W/K: junction 295.6448 K, exact 295.6440 K, energy 22860817189.1971 J, energy error 0.0001 J
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

transport/QualityMode
transport/Quality
transport/RouteOptions
transport/TransportResult
transport/transport
transport/Heat
transport/HeatResult
transport/heat_fn
```
<!-- apiref:toctree:end -->
