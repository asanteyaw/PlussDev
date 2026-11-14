<!-- generated from Heats.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::Heats`

Heat inflows into nodes.

## Signature

```cpp
thermal::Heats
```

## What It Does

Injects heat into nodes: a heater, an electronic component, solar gain. Heat `k` is `power[k]` watts delivered into `node[k]`; a negative power withdraws heat. In `tran` a heat source that varies in time is a `Drive::power`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `node` | The node of each heat source, an integer in `0 .. nodes - 1`. | empty |
| `power` | The power of each source in W. The last axis has one value per source; leading axes are batches. | undefined (no heat sources) |

## Returns

A value; read by `steady` and `tran`.

## Errors

`steady` and `tran` throw `pluss::Error` for a node index outside the network, a power array whose last axis does not have one value per source, or a power that is not finite.

## Interpretation and Use

In steady state the heat injected into a network leaves through its fixed nodes; `fixed_flow` of the result sums to the negative of the injected power.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

30 W injected into the middle of a chain.

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
### Example 2

A heater switched on and off.

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

Rejected heat sources, and a negative power.

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
  net.heats = {{0}, vec({10.0, 5.0})};
  report([&] { (void)th::steady(net); });                      // two powers for one node
  net.heats = {{9}, vec({10.0})};
  report([&] { (void)th::steady(net); });                      // a node outside the network
  net.heats = {{0}, vec({-10.0})};
  const auto cooling = th::steady(net);
  out.writeln("a negative power withdraws heat:", flat(cooling.temperature));
}
```

Output

```text
eng::thermal::steady: heats.power must have one value per heat node (last axis 1)
eng::thermal::steady: a heat node index 9 is outside 0..1
a negative power withdraws heat: [0.00000, 20.00000]
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Fixed.html">
    <span class="label">Previous</span>
    <span class="title">Fixed</span>
  </a>
  <a class="next-link" href="Net.html">
    <span class="label">Next</span>
    <span class="title">Net</span>
  </a>
</div>
<!-- apiref:nav:end -->
