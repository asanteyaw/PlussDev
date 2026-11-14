<!-- generated from Result.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::Result`

The solution of a thermal network.

## Signature

```cpp
thermal::Result
```

## What It Does

Holds the temperatures and the heat flows of a solved network. In `steady` every array has the batch shape followed by its element axis; in `tran` a time axis is inserted immediately before the element axis.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `temperature` | The temperature of every node, `[..., nodes]`. |  |
| `link_flow` | The heat flow in W through every link from `a` to `b`, `[..., links]`. |  |
| `fixed_flow` | The heat in W that every fixed node delivers into the network, `[..., fixed]`; negative when the boundary absorbs heat. |  |
| `residual` | The defect of the nodal heat balance: `[...]` for `steady`, `[..., time]` for `tran`. |  |

## Returns

A value returned by `steady` and `tran`.

## Errors

None: a result is only constructed by a successful solve.

## Interpretation and Use

Energy checks are one line. In steady state the injected heats plus `fixed_flow` sum to zero. In `tran` the heat stored in the capacities makes up the difference, and at the first sample the stores sit at their start temperatures so the link flows there describe the initial imbalance.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The shapes of both results.

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
  th::Net net;
  net.nodes = 3;
  net.links = {{0, 1}, {1, 2}, vec({2.0, 2.0})};
  net.stores = {{1}, vec({50.0})};
  net.fixed = {{0, 2}, vec({100.0, 20.0})};
  const auto steady = th::steady(net);
  out.writeln("steady: temperature", steady.temperature.sizes(), " link_flow", steady.link_flow.sizes(), " fixed_flow", steady.fixed_flow.sizes(), " residual", steady.residual.sizes());
  const auto run = th::tran(net, vec(grid(10.0, 4)));
  out.writeln("tran:   temperature", run.temperature.sizes(), " link_flow", run.link_flow.sizes(), " fixed_flow", run.fixed_flow.sizes(), " residual", run.residual.sizes());
  out.writeln("link flow at the first sample", flat(run.link_flow.select(0, 0)), "W: the store starts at zero, so heat flows into it");
}
```

Output

```text
steady: temperature [3]  link_flow [2]  fixed_flow [2]  residual []
tran:   temperature [4, 3]  link_flow [4, 2]  fixed_flow [4, 2]  residual [4]
link flow at the first sample [200.00000, -40.00000] W: the store starts at zero, so heat flows into it
```
### Example 2

Heat flows and boundary exchange of a chain.

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
### Example 3

The boundary exchange of a transient.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Net.html">
    <span class="label">Previous</span>
    <span class="title">Net</span>
  </a>
  <a class="next-link" href="Drive.html">
    <span class="label">Next</span>
    <span class="title">Drive</span>
  </a>
</div>
<!-- apiref:nav:end -->
