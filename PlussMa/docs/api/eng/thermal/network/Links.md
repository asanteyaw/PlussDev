<!-- generated from Links.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::Links`

Thermal conductances between pairs of nodes.

## Signature

```cpp
thermal::Links
```

## What It Does

Lists the conduction paths of a network. Link `k` joins nodes `a[k]` and `b[k]` with the conductance `conductance[k]`, the heat flow in watts per kelvin of temperature difference. Heat flows from `a` to `b` when `a` is warmer, and the sign of every reported link flow follows that direction. A conductance is the reciprocal of a resistance, so the closed-form resistances of `wall`, `tube`, `shell` and `film` enter a network through `reciprocal()`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `a` | The first node of each link, an integer in `0 .. nodes - 1`. | empty |
| `b` | The second node of each link; a link may not join a node to itself. | empty |
| `conductance` | The conductance of each link in W/K, positive. The last axis has one value per link; leading axes are parameter-sweep batches. | undefined (no links) |

## Returns

A value; read by `steady` and `tran`.

## Errors

`steady` and `tran` throw `pluss::Error` for `a` and `b` of different lengths, a node index outside the network, a link from a node to itself, a conductance array whose last axis does not have one value per link, a conductance that is not positive or not finite, or conductances without links.

## Interpretation and Use

Several links may join the same pair of nodes; they add. A leading axis on `conductance` sweeps a design variable (insulation thickness, a film coefficient) through one call; every other group of the network either shares the batch shape or has none.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A series chain of two links.

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
  // Two conductances of 2 W/K in series between 100 and 20 degrees: nodes 0 and 2 are fixed, node 1 floats.
  th::Net net;
  net.nodes = 3;
  net.links = {{0, 1}, {1, 2}, vec({2.0, 2.0})};
  net.fixed = {{0, 2}, vec({100.0, 20.0})};
  const auto run = th::steady(net);
  out.writeln("temperature", flat(run.temperature));
  out.writeln("link flow", flat(run.link_flow), " (a to b)");
  out.writeln("boundary delivers", flat(run.fixed_flow), " (negative: absorbs)");
  out.writeln("closed form: midpoint", 60.0, " flow", 80.0 / (0.5 + 0.5));
  out.writeln("largest residual below 1e-9:", run.residual.abs().amax().item<double>() < 1e-9);
}
```

Output

```text
temperature [100.00000, 60.00000, 20.00000]
link flow [80.00000, 80.00000]  (a to b)
boundary delivers [80.00000, -80.00000]  (negative: absorbs)
closed form: midpoint 60.00000  flow 80.00000
largest residual below 1e-9: true
```
### Example 2

A sweep of the conductance of one link.

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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A leading axis sweeps the conductance of the second link; one call solves every case.
  th::Net net;
  net.nodes = 3;
  net.links = {{0, 1}, {1, 2}, mat({2.0, 0.5, 2.0, 2.0, 2.0, 8.0}, {3, 2})};
  net.fixed = {{0, 2}, vec({100.0, 20.0})};
  const auto run = th::steady(net);
  out.writeln("temperature shape", run.temperature.sizes(), " (case, node)");
  out.writeln("middle node per case", flat(run.temperature.select(1, 1)));
  out.writeln("heat flow per case", flat(run.link_flow.select(1, 0)));
  out.writeln("closed form", 80.0 / (1.0 / 2.0 + 1.0 / 0.5), 80.0 / (1.0 / 2.0 + 1.0 / 2.0), 80.0 / (1.0 / 2.0 + 1.0 / 8.0));
}
```

Output

```text
temperature shape [3, 3]  (case, node)
middle node per case [84.00000, 60.00000, 36.00000]
heat flow per case [32.00000, 80.00000, 128.00000]
closed form 32.00000 80.00000 128.00000
```
### Example 3

Rejected link descriptions.

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
  th::Net net;
  net.nodes = 2;
  net.fixed = {{1}, vec({20.0})};
  net.links = {{0}, {1}, vec({1.0, 2.0})};
  report([&] { (void)th::steady(net); });                      // two conductances for one link
  net.links = {{0, 1}, {1}, vec({1.0, 2.0})};
  report([&] { (void)th::steady(net); });                      // links.a and links.b of different lengths
}
```

Output

```text
eng::thermal::steady: links.conductance must have one value per link (last axis 1)
eng::thermal::steady: links.a and links.b must have the same length
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Stores.html">
    <span class="label">Next</span>
    <span class="title">Stores</span>
  </a>
</div>
<!-- apiref:nav:end -->
