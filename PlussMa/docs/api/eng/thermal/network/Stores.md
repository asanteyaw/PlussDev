<!-- generated from Stores.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::Stores`

Thermal capacities of nodes.

## Signature

```cpp
thermal::Stores
```

## What It Does

Gives nodes the ability to hold heat. Store `k` is a thermal capacity `capacity[k]` in joules per kelvin attached to `node[k]`: heat flowing into the node raises its temperature at the rate `Q / C`. Only `tran` uses capacities; `steady` ignores them because a steady network stores nothing. A node without a store responds instantly, which is how a massless surface node is modelled.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `node` | The node of each store, an integer in `0 .. nodes - 1`. | empty |
| `capacity` | The capacity of each store in J/K, positive. The last axis has one value per store; leading axes are batches. | undefined (no stores) |

## Returns

A value; read by `tran`.

## Errors

`steady` and `tran` throw `pluss::Error` for a node index outside the network, a capacity array whose last axis does not have one value per store, a capacity that is not positive or not finite, or capacities without nodes.

## Interpretation and Use

Two stores on one node add. The capacity of a body is its mass times its specific heat; the time constant of a body of capacity `C` that loses heat through a conductance `G` is `C / G`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A body cooling through one conductance.

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
### Example 2

The default start of a store.

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
### Example 3

Rejected capacities.

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
  net.fixed = {{1}, vec({20.0})};
  const pluss::Series time = vec(grid(10.0, 3));
  net.stores = {{0, 0}, vec({10.0, 20.0})};
  out.writeln("two capacities on one node run:", flat(th::tran(net, time).temperature).size(), "values");
  net.stores = {{4}, vec({10.0})};
  report([&] { (void)th::tran(net, time); });                  // a node outside the network
  net.stores = {{0}, vec({0.0})};
  report([&] { (void)th::tran(net, time); });                  // zero capacity
  net.stores = {{0}, vec({10.0, 20.0})};
  report([&] { (void)th::tran(net, time); });                  // one value too many
}
```

Output

```text
two capacities on one node run: 6 values
eng::thermal::tran: a store node index 4 is outside 0..1
eng::thermal::tran: stores.capacity must be positive
eng::thermal::tran: stores.capacity must have one value per store (last axis 1)
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Links.html">
    <span class="label">Previous</span>
    <span class="title">Links</span>
  </a>
  <a class="next-link" href="Fixed.html">
    <span class="label">Next</span>
    <span class="title">Fixed</span>
  </a>
</div>
<!-- apiref:nav:end -->
