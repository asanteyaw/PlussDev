<!-- generated from Nodes.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::Nodes`

The kinds, elevations, demands, heads and emitters of the nodes.

## Signature

```cpp
fluid::Nodes
```

## What It Does

Describes every node of a `Net`. Each array ends in the node axis and may carry leading scenario axes; an array without them is shared by every scenario.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `kind` | One `NodeKind` per node, a plain `std::vector`. | required |
| `elevation` | Node elevation in m, `[..., node]`. For a tank it is the bottom of the tank. | required |
| `demand` | Flow leaving the network at the node, m³/s, `[..., node]`. Positive is a demand, negative a source, and it must be zero at a reservoir and a tank. | required |
| `head` | The fixed head at a reservoir, the initial water-surface head at a tank, and a starting value elsewhere, m, `[..., node]`. | required |
| `emitter` | Emitter coefficient, `(m³/s) / m^exponent`, `[..., node]`; zero where there is no emitter. | undefined (no emitters) |

## Returns

A value; validated by `network`.

## Errors

Arrays of the wrong length, non-floating or non-finite values, a non-zero demand or emitter at a reservoir or tank, a negative emitter coefficient, and mixed precision or platform are rejected.

## Interpretation and Use

A junction delivers its whole `demand` under `DemandMode::Fixed`, whatever the pressure; under `DemandMode::Pressure` the delivered demand falls with the pressure and an emitter adds a pressure-dependent leak. The head of a junction is only a starting value for the solver.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A reservoir and a junction.

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

Descriptions that are refused.

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
  attempt("a pipe pair with no reservoir", [] {
    auto net = one_pipe(0.01);
    net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction, fluid::NodeKind::Junction};
    net.nodes.elevation = vec({0.0, 5.0, 5.0, 5.0});
    net.nodes.demand = vec({0.0, 0.01, 0.01, 0.01});
    net.nodes.head = vec({50.0, 49.0, 49.0, 49.0});
    net.pipes.from = {0, 2};
    net.pipes.to = {1, 3};
    net.pipes.length = vec({1000.0, 1000.0});
    net.pipes.diameter = vec({0.30, 0.30});
    net.pipes.roughness = vec({1e-4, 1e-4});
    net.pipes.minor = vec({0.0, 0.0});
    (void)fluid::network(net);
  });
  attempt("a demand at the reservoir", [] { auto net = one_pipe(); net.nodes.demand = vec({0.02, 0.0}); (void)fluid::network(net); });
  attempt("a zero diameter", [] { auto net = one_pipe(); net.pipes.diameter = vec({0.0}); (void)fluid::network(net); });
  attempt("a pipe from a node to itself", [] { auto net = one_pipe(); net.pipes.to = {0}; (void)fluid::network(net); });
  attempt("a pipe to node 7 of 2", [] { auto net = one_pipe(); net.pipes.to = {7}; (void)fluid::network(net); });
  attempt("a negative roughness", [] { auto net = one_pipe(); net.pipes.roughness = vec({-1.0}); (void)fluid::network(net); });
  attempt("integer lengths", [] { auto net = one_pipe(); net.pipes.length = pluss::series::make(std::vector<int>{1000}); (void)fluid::network(net); });
  attempt("mixed precision", [] { auto net = one_pipe(); net.pipes.length = net.pipes.length.to(pluss::ScalarType::Float32); (void)fluid::network(net); });
}
```

Output

```text
a pipe pair with no reservoir: eng::fluid::network: every connected component needs a reservoir or tank
a demand at the reservoir: eng::fluid::network: reservoir and tank demand/emitter lanes must be zero
a zero diameter: eng::fluid::network: pipe diameter must be positive
a pipe from a node to itself: eng::fluid::network: a link cannot join a node to itself
a pipe to node 7 of 2: eng::fluid::network: link endpoint is outside the node range
a negative roughness: eng::fluid::network: roughness must be nonnegative
integer lengths: eng::fluid::network: pipe length must be real floating [..., elements]
mixed precision: eng::fluid::network: all physical values must share dtype and platform
```
### Example 3

A leak as an emitter.

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
  fluid::Options pressure_driven;
  pressure_driven.demand_mode = fluid::DemandMode::Pressure;
  pressure_driven.min_pressure = 0.0;
  pressure_driven.full_pressure = 15.0;
  pressure_driven.demand_exponent = 0.5;
  for (double demand : {0.2, 0.6, 1.0}) {
    const auto fixed = fluid::steady(fluid::network(one_pipe(demand)));
    const auto driven = fluid::steady(fluid::network(one_pipe(demand)), pressure_driven);
    out.writeln("requested ", demand, ": Fixed delivers ", flat(fixed.demand)[1], " at pressure head ", flat(fixed.head)[1] - 5.0, "; Pressure delivers ", flat(driven.demand)[1], " at pressure head ", flat(driven.head)[1] - 5.0);
  }
  // an emitter: a leak that grows with the square root of the pressure head
  for (double c : {0.0, 0.002, 0.01}) {
    auto net = one_pipe(0.02);
    net.nodes.emitter = vec({0.0, c});
    const auto r = fluid::steady(fluid::network(net));
    const double p = flat(r.head)[1] - 5.0;
    out.writeln("emitter ", c, ": outflow ", flat(r.emitter)[1], ", C sqrt(p) = ", c * std::sqrt(p), ", pipe flow ", flat(r.flow)[0]);
  }
}
```

Output

```text
requested 0.200000: Fixed delivers 0.200000 at pressure head 23.011199; Pressure delivers 0.200000 at pressure head 23.011199
requested 0.600000: Fixed delivers 0.600000 at pressure head -146.321537; Pressure delivers 0.277544 at pressure head 3.209615
requested 1.000000: Fixed delivers 1.000000 at pressure head -482.190058; Pressure delivers 0.284220 at pressure head 1.211716
emitter 0.000000: outflow 0.000000, C sqrt(p) = 0.000000, pipe flow 0.020000
emitter 0.002000: outflow 0.013310, C sqrt(p) = 0.013310, pipe flow 0.033310
emitter 0.010000: outflow 0.063962, C sqrt(p) = 0.063962, pipe flow 0.083962
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ValveKind.html">
    <span class="label">Previous</span>
    <span class="title">ValveKind</span>
  </a>
  <a class="next-link" href="Tanks.html">
    <span class="label">Next</span>
    <span class="title">Tanks</span>
  </a>
</div>
<!-- apiref:nav:end -->
