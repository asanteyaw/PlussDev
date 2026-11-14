<!-- generated from steady.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::steady`

Solves the steady state of a thermal network.

## Signature

```cpp
thermal::Result thermal::steady(const thermal::Net& net,
                       const circuit::SolveOptions& options = {});
```

## What It Does

Finds the temperatures at which every node is in heat balance: heat injected plus heat flowing in through links equals zero, with the fixed nodes at their prescribed temperatures. Capacities play no part. The solve is the circuit engine's operating point, so a batch of networks is solved together.

## Parameters

`net`
: The network.

`options`
: The controls of the underlying solve; a thermal network is linear, so they are not needed in practice.

## Returns

A `Result` with the temperatures, link flows, boundary flows and the balance defect.

## Errors

Throws `pluss::Error` for an invalid description, a network with no fixed temperature, or a node that has no conduction path to a fixed node; the message names the node.

## Interpretation and Use

Use `steady` for the long-run state, for the sizing of an insulation layer, or to find a consistent start for a transient. It does not read stores.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A chain between two temperatures.

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

Heat injected into the middle.

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
  // The same chain with 30 W injected into the middle node: both boundaries absorb it.
  th::Net net;
  net.nodes = 3;
  net.links = {{0, 1}, {1, 2}, vec({2.0, 2.0})};
  net.fixed = {{0, 2}, vec({100.0, 20.0})};
  net.heats = {{1}, vec({30.0})};
  const auto run = th::steady(net);
  out.writeln("temperature", flat(run.temperature));
  out.writeln("link flow", flat(run.link_flow));
  out.writeln("boundary delivers", flat(run.fixed_flow));
  out.writeln("injected + delivered =", 30.0 + flat(run.fixed_flow)[0] + flat(run.fixed_flow)[1], " (energy balance)");
}
```

Output

```text
temperature [100.00000, 67.50000, 20.00000]
link flow [65.00000, 95.00000]
boundary delivers [65.00000, -95.00000]
injected + delivered = 0.00000  (energy balance)
```
### Example 3

A sweep over conductances.

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
### Example 4

Rejected networks.

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
  net.nodes = 3;
  net.links = {{0}, {1}, vec({2.0})};
  net.fixed = {{0}, vec({100.0})};
  report([&] { (void)th::steady(net); });             // node 2 has no link
  th::Net open = net;
  open.fixed = {};
  report([&] { (void)th::steady(open); });            // nothing fixes the temperature
  th::Net far = net;
  far.links.b = {7};
  report([&] { (void)th::steady(far); });             // a node index outside the network
  th::Net loop = net;
  loop.links.b = {0};
  report([&] { (void)th::steady(loop); });            // a link from a node to itself
  th::Net dead = net;
  dead.links.conductance = vec({0.0});
  report([&] { (void)th::steady(dead); });            // a conductance that is not positive
}
```

Output

```text
eng::thermal::steady: node 2 has no conduction path to a fixed temperature
eng::thermal::steady: needs at least one fixed temperature
eng::thermal::steady: a link node index 7 is outside 0..2
eng::thermal::steady: a link joins node 0 to itself
eng::thermal::steady: links.conductance must be positive
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Start.html">
    <span class="label">Previous</span>
    <span class="title">Start</span>
  </a>
  <a class="next-link" href="tran.html">
    <span class="label">Next</span>
    <span class="title">tran</span>
  </a>
</div>
<!-- apiref:nav:end -->
