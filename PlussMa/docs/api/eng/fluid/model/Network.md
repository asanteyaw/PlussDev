<!-- generated from Network.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::Network`

The compiled, immutable pipe network.

## Signature

```cpp
fluid::Network
```

## What It Does

The result of `network`: a validated, immutable network whose topology maps and physical arrays are ready for `steady`, `run`, `transport` and `heat`. It cannot be built directly, and it preserves the precision and platform of its source arrays.

## Methods

`nodes()`
: The number of nodes.

`junctions()`
: The number of junctions.

`reservoirs()`
: The number of reservoirs.

`tanks()`
: The number of tanks.

`pipes()`
: The number of pipes.

`pumps()`
: The number of pumps.

`valves()`
: The number of valves.

`links()`
: Pipes plus pumps plus valves; the order of the link axis of every result.

`batch_shape()`
: The leading scenario axes shared by the physical arrays.

`dtype()`
: The precision, `Float32` or `Float64`.

`platform()`
: The platform of the arrays.

`liquid()`
: The `Liquid` it was compiled with.

## Returns

Returned by `network`.

## Errors

Accessors do not throw.

## Changelog

- **0.1** — Initial public contract.

## Interpretation and Use

Link results are ordered pipes first, then pumps, then valves. A `Network` is a value that can be solved many times: `steady` once, `run` over a schedule, and `transport` or `heat` on top of a run.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The counts and liquid of a compiled network.

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
### Example 2

The batch shape of a scenario network.

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

// a reservoir (node 0) and four junctions joined by six pipes that form two loops
static fluid::Net looped() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction,
                    fluid::NodeKind::Junction, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 10.0, 12.0, 8.0, 15.0});
  net.nodes.demand = vec({0.0, 0.010, 0.015, 0.012, 0.008});
  net.nodes.head = vec({60.0, 55.0, 55.0, 55.0, 55.0});
  net.pipes.from = {0, 1, 1, 2, 3, 2};
  net.pipes.to = {1, 2, 3, 4, 4, 3};
  net.pipes.length = vec({800.0, 500.0, 600.0, 450.0, 550.0, 300.0});
  net.pipes.diameter = vec({0.30, 0.20, 0.20, 0.15, 0.15, 0.10});
  net.pipes.roughness = vec({1e-4, 1e-4, 1e-4, 1e-4, 1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // three demand scenarios share one topology: only the demand array carries a leading axis
  auto net = looped();
  const double scale[3] = {0.5, 1.0, 2.0};
  std::vector<double> demand;
  for (double s : scale)
    for (double d : {0.0, 0.010, 0.015, 0.012, 0.008}) demand.push_back(d * s);
  net.nodes.demand = pluss::series::make(demand, {3, 5});
  const auto network = fluid::network(net);
  out.writeln("batch shape ", network.batch_shape(), ", dtype Float64: ", network.dtype() == pluss::ScalarType::Float64);
  const auto r = fluid::steady(network);
  out.writeln("head ", r.head.sizes(), ", flow ", r.flow.sizes(), ", converged ", r.converged.sizes());
  const auto head = flat(r.head);
  for (int s = 0; s < 3; ++s) out.writeln("demand x", scale[s], ": head at junction 4 ", head[s * 5 + 4]);
}
```

Output

```text
batch shape [3], dtype Float64: true
head [3, 5], flow [3, 6], converged [3]
demand x0.500000: head at junction 4 59.425413
demand x1.000000: head at junction 4 57.917692
demand x2.000000: head at junction 4 52.250448
```
### Example 3

Counts by node kind.

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

// reservoir (60 m) -> pipe -> junction 1 (demand 0.04) -> pipe -> tank; the tank bottom is at 20 m, its surface starts at 23 m
static fluid::Net with_tank(double area = 400.0, bool overflow = false, double initial_head = 23.0) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Tank};
  net.nodes.elevation = vec({0.0, 10.0, 20.0});
  net.nodes.demand = vec({0.0, 0.04, 0.0});
  net.nodes.head = vec({60.0, 50.0, initial_head});
  net.tanks.area = vec({area});
  net.tanks.min_level = vec({0.5});
  net.tanks.max_level = vec({6.0});
  net.tanks.overflow = {overflow};
  net.pipes.from = {0, 1};
  net.pipes.to = {1, 2};
  net.pipes.length = vec({800.0, 400.0});
  net.pipes.diameter = vec({0.15, 0.20});
  net.pipes.roughness = vec({1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0});
  return net;
}

static fluid::Net one_pipe_with_tank_entry() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, 0.02});
  net.nodes.head = vec({50.0, 49.0});
  net.tanks.area = vec({400.0});
  net.tanks.min_level = vec({0.5});
  net.tanks.max_level = vec({6.0});
  net.tanks.overflow = {false};
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.30});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto two = fluid::network(two_sources());
  out.writeln("two sources: ", two.reservoirs(), " reservoirs, ", two.junctions(), " junction, ", two.tanks(), " tanks");
  const auto tank = fluid::network(with_tank());
  out.writeln("with a tank: ", tank.reservoirs(), " reservoir, ", tank.junctions(), " junction, ", tank.tanks(), " tank");
  attempt("two junctions and no fixed-grade node", [] {
    auto net = two_sources();
    net.nodes.kind = {fluid::NodeKind::Junction, fluid::NodeKind::Junction, fluid::NodeKind::Junction};
    (void)fluid::network(net);
  });
  attempt("a tank node with no tank described", [] {
    auto net = with_tank();
    net.tanks = fluid::Tanks{};
    (void)fluid::network(net);
  });
  attempt("a tank described for a network that has none", [] {
    auto net = one_pipe_with_tank_entry();
    (void)fluid::network(net);
  });
}
```

Output

```text
two sources: 2 reservoirs, 1 junction, 0 tanks
with a tank: 1 reservoir, 1 junction, 1 tank
two junctions and no fixed-grade node: eng::fluid::network: every connected component needs a reservoir or tank
a tank node with no tank described: eng::fluid::network: tank overflow metadata must match tank nodes
a tank described for a network that has none: eng::fluid::network: tank overflow metadata must match tank nodes
```

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Liquid.html">
    <span class="label">Previous</span>
    <span class="title">Liquid</span>
  </a>
  <a class="next-link" href="network_fn.html">
    <span class="label">Next</span>
    <span class="title">network</span>
  </a>
</div>
<!-- apiref:nav:end -->
