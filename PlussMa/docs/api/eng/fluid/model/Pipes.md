<!-- generated from Pipes.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::Pipes`

The ends, dimensions, roughness and minor losses of the pipes.

## Signature

```cpp
fluid::Pipes
```

## What It Does

Describes the pipes of a `Net`. A pipe joins its `from` node to its `to` node, and a positive flow runs in that direction.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `from` | The upstream node index of each pipe, a plain `std::vector`. | required |
| `to` | The downstream node index of each pipe. | required |
| `length` | Length, m, `[..., pipe]`. | required |
| `diameter` | Internal diameter, m, `[..., pipe]`. | required |
| `roughness` | Absolute roughness in m for `LossModel::DarcyWeisbach`; the coefficient `C` for `LossModel::HazenWilliams`, `[..., pipe]`. | required |
| `minor` | Dimensionless minor-loss coefficient `K`, `[..., pipe]`. | undefined (zero) |

## Returns

A value; validated by `network`.

## Errors

An endpoint outside the node range, a pipe from a node to itself, a non-positive length or diameter, a negative roughness or minor loss, and mixed precision are rejected. Hazen-Williams coefficients that are not positive are rejected by `steady`.

## Interpretation and Use

Roughness means a different thing under each loss law, so check its magnitude: roughness lengths are a millimetre or less, Hazen-Williams coefficients are tens or hundreds. Declare a pipe in the direction of its expected flow so that results read as positive numbers; a negative flow is valid and means the water runs the other way.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

One pipe checked against the Darcy-Weisbach formula.

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

Both loss laws, roughness and minor losses.

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
  const double q = 0.05, D = 0.30, L = 1000.0, g = 9.80665, kPi = 3.14159265358979323846;
  const double v = 4.0 * q / (kPi * D * D);
  fluid::Options darcy;
  fluid::Options hazen;
  hazen.loss = fluid::LossModel::HazenWilliams;
  auto pipe = [&](double rough, double minor) {
    auto net = one_pipe(q);
    net.nodes.head = vec({100.0, 90.0});
    net.nodes.elevation = vec({0.0, 0.0});
    net.pipes.roughness = vec({rough});
    net.pipes.minor = vec({minor});
    return fluid::network(net);
  };
  for (double eps : {1e-5, 1e-4, 1e-3}) {
    const auto r = fluid::steady(pipe(eps, 0.0), darcy);
    out.writeln("Darcy-Weisbach, roughness ", eps, " m: friction ", flat(r.friction_factor)[0], ", head loss ", flat(r.head_loss)[0], " m");
  }
  for (double C : {80.0, 130.0, 150.0}) {
    const auto r = fluid::steady(pipe(C, 0.0), hazen);
    const double formula = 10.67 * L * std::pow(q, 1.852) / (std::pow(C, 1.852) * std::pow(D, 4.8704));
    out.writeln("Hazen-Williams, C = ", C, ": head loss ", flat(r.head_loss)[0], ", formula ", formula, ", friction field ", flat(r.friction_factor)[0]);
  }
  const double base = flat(fluid::steady(pipe(1e-4, 0.0), darcy).head_loss)[0];
  out.writeln("minor loss K = 20: head loss ", flat(fluid::steady(pipe(1e-4, 20.0), darcy).head_loss)[0], ", base + K v^2/2g = ", base + 20.0 * v * v / (2.0 * g));
}
```

Output

```text
Darcy-Weisbach, roughness 0.000010 m: friction 0.015677, head loss 1.333124 m
Darcy-Weisbach, roughness 0.000100 m: friction 0.017888, head loss 1.521119 m
Darcy-Weisbach, roughness 0.001000 m: friction 0.027725, head loss 2.357617 m
Hazen-Williams, C = 80.000000: head loss 4.372833, formula 4.372833, friction field 0.000000
Hazen-Williams, C = 130.000000: head loss 1.779354, formula 1.779354, friction field 0.000000
Hazen-Williams, C = 150.000000: head loss 1.365100, formula 1.365100, friction field 0.000000
minor loss K = 20: head loss 2.031336, base + K v^2/2g = 2.031336
```
### Example 3

Pipes that are refused.

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
  <a class="prev-link" href="Tanks.html">
    <span class="label">Previous</span>
    <span class="title">Tanks</span>
  </a>
  <a class="next-link" href="Pumps.html">
    <span class="label">Next</span>
    <span class="title">Pumps</span>
  </a>
</div>
<!-- apiref:nav:end -->
