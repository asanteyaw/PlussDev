<!-- generated from Fixed.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::Fixed`

Nodes held at prescribed temperatures.

## Signature

```cpp
thermal::Fixed
```

## What It Does

Holds nodes at a given temperature whatever heat flows through them: the surroundings, a thermostat, the ground. Every node of a network must reach at least one fixed node through links for `steady` to have an answer. The heat a fixed node delivers into the network is reported in `Result::fixed_flow`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `node` | The node of each fixed temperature, an integer in `0 .. nodes - 1`. | empty |
| `temperature` | The temperature of each fixed node. The last axis has one value per fixed node; leading axes are batches. | undefined (no fixed nodes) |

## Returns

A value; read by `steady` and `tran`.

## Errors

`steady` and `tran` throw `pluss::Error` for a node index outside the network, a temperature array whose last axis does not have one value per fixed node, or a temperature that is not finite. `steady` also throws when there is no fixed node, or when a node has no conduction path to a fixed node.

## Interpretation and Use

Any temperature scale that is linear in kelvin works, because conduction depends only on differences; stay with one scale for the whole network. A fixed temperature that changes in time is a `Drive::temperature`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Two fixed boundaries around a chain.

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

Surroundings that step to a higher temperature.

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
### Example 3

Rejected fixed temperatures.

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
  net.links = {{0}, {1}, vec({0.5})};
  net.fixed = {{1}, vec({20.0})};
  net.fixed.temperature = vec({20.0, 30.0});
  report([&] { (void)th::steady(net); });                      // two values for one fixed node
  net.fixed = {{1}, vec({std::nan("")})};
  report([&] { (void)th::steady(net); });                      // a value that is not finite
  net.fixed = {{-1}, vec({20.0})};
  report([&] { (void)th::steady(net); });                      // a node below zero
}
```

Output

```text
eng::thermal::steady: fixed.temperature must have one value per fixed node (last axis 1)
eng::thermal::steady: fixed.temperature must be finite
eng::thermal::steady: a fixed node index -1 is outside 0..1
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Stores.html">
    <span class="label">Previous</span>
    <span class="title">Stores</span>
  </a>
  <a class="next-link" href="Heats.html">
    <span class="label">Next</span>
    <span class="title">Heats</span>
  </a>
</div>
<!-- apiref:nav:end -->
