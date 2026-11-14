<!-- generated from Net.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::Net`

The description of a pipe network.

## Signature

```cpp
fluid::Net
```

## What It Does

Collects the arrays that describe a network: the nodes, the tanks, the pipes, the pumps and the valves. `network` validates it and compiles it into an immutable `Network`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `nodes` | A `Nodes`: kinds, elevations, demands, heads and emitters. | required |
| `tanks` | A `Tanks`: area, level limits and overflow. | empty (no tanks) |
| `pipes` | A `Pipes`: ends, dimensions, roughness and minor losses. | empty (no pipes) |
| `pumps` | A `Pumps`: ends, curves and speeds. | empty (no pumps) |
| `valves` | A `Valves`: kinds, bores, losses and settings. | empty (no valves) |

## Returns

A value; compiled by `network`.

## Errors

Every inconsistency of the groups is reported by `network`: see the individual groups.

## Interpretation and Use

Every array ends in its node, pipe, pump, valve or tank axis, and may carry leading scenario axes. Arrays that carry them must agree on those axes, and an array without them is shared by every scenario. All arrays must share one precision, `Float32` or `Float64`.

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

Three demand scenarios in one call.

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Valves.html">
    <span class="label">Previous</span>
    <span class="title">Valves</span>
  </a>
  <a class="next-link" href="Liquid.html">
    <span class="label">Next</span>
    <span class="title">Liquid</span>
  </a>
</div>
<!-- apiref:nav:end -->
