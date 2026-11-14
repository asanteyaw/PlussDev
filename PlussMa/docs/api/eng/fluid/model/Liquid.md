<!-- generated from Liquid.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::Liquid`

Density, viscosity and gravity of the fluid.

## Signature

```cpp
fluid::Liquid
```

## What It Does

Gives the physical constants of the liquid: its density, which turns heads into pressures, its kinematic viscosity, which sets the Reynolds number and so the friction factor, and the gravity of the head-loss laws. The defaults are water at 20 °C.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `density` | kg/m³. | 998.2 |
| `kinematic_viscosity` | m²/s. | 1.0038e-6 |
| `gravity` | m/s². | 9.80665 |

## Returns

A value; passed to `network`.

## Errors

Values that are not finite and positive are rejected by `network`.

## Interpretation and Use

The heads and flows of a network depend on the liquid only through the viscosity; the pressures depend on the density as well. A hot liquid has a lower viscosity and a lower density: its head loss is smaller and a given head is a lower pressure.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Water at two temperatures and an oil.

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

// a reservoir at 50 m feeding one junction through a 1 km pipe
static fluid::Net one_pipe(double demand = 0.02) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.30});
  net.pipes.roughness = vec({1.0e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const double nu[3] = {1.0038e-6, 0.3646e-6, 1.0e-4};
  const double rho[3] = {998.2, 971.8, 870.0};
  const char *name[3] = {"water 20 C", "water 80 C", "oil       "};
  for (int i = 0; i < 3; ++i) {
    fluid::Liquid liquid;
    liquid.density = rho[i];
    liquid.kinematic_viscosity = nu[i];
    const auto network = fluid::network(one_pipe(0.02), liquid);
    const auto r = fluid::steady(network);
    out.writeln(name[i], ": density ", network.liquid().density, ", friction ", flat(r.friction_factor)[0], ", head loss ", flat(r.head_loss)[0], " m, pressure ", flat(r.pressure)[1] / 1000.0, " kPa");
  }
  const fluid::Liquid defaults;
  out.writeln("defaults: density ", defaults.density, ", viscosity ", defaults.kinematic_viscosity * 1e6, "e-6, gravity ", defaults.gravity);
}
```

Output

```text
water 20 C: density 998.200000, friction 0.020163, head loss 0.274338 m, pressure 437.819413 kPa
water 80 C: density 971.800000, friction 0.017710, head loss 0.240963 m, pressure 426.558208 kPa
oil       : density 870.000000, friction 0.075398, head loss 1.025851 m, pressure 375.178005 kPa
defaults: density 998.200000, viscosity 1.003800e-6, gravity 9.806650
```
### Example 2

The liquid of a compiled network.

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

// a reservoir at 50 m feeding one junction through a 1 km pipe
static fluid::Net one_pipe(double demand = 0.02) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.30});
  net.pipes.roughness = vec({1.0e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto network = fluid::network(one_pipe(0.02));
  out.writeln("nodes ", network.nodes(), " (junctions ", network.junctions(), ", reservoirs ", network.reservoirs(), ", tanks ", network.tanks(), ")");
  out.writeln("links ", network.links(), " (pipes ", network.pipes(), ", pumps ", network.pumps(), ", valves ", network.valves(), ")");
  const auto r = fluid::steady(network);
  const double D = 0.30, L = 1000.0, eps = 1.0e-4, q = 0.02, nu = 1.0038e-6, g = 9.80665, pi = 3.14159265358979323846;
  const double v = 4.0 * q / (pi * D * D), Re = v * D / nu;
  const double lg = std::log10(eps / (3.7 * D) + 5.74 / std::pow(Re, 0.9));
  const double loss = 0.25 / (lg * lg) * L / D * v * v / (2.0 * g);
  out.writeln("junction head ", flat(r.head)[1], " m, by hand ", 50.0 - loss, " m");
  out.writeln("pressure ", flat(r.pressure)[1], " Pa gauge, head loss ", flat(r.head_loss)[0], " m, velocity ", flat(r.velocity)[0], " m/s");
}
```

Output

```text
nodes 2 (junctions 1, reservoirs 1, tanks 0)
links 1 (pipes 1, pumps 0, valves 0)
junction head 49.725662 m, by hand 49.725662 m
pressure 437819.413252 Pa gauge, head loss 0.274338 m, velocity 0.282942 m/s
```
### Example 3

The liquid of a thermal run.

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Net.html">
    <span class="label">Previous</span>
    <span class="title">Net</span>
  </a>
  <a class="next-link" href="Network.html">
    <span class="label">Next</span>
    <span class="title">Network</span>
  </a>
</div>
<!-- apiref:nav:end -->
