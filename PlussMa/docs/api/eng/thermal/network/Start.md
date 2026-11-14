<!-- generated from Start.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::Start`

The temperature of every store at the first time sample.

## Signature

```cpp
thermal::Start
```

## What It Does

Sets where every store begins a transient. `temperature` has one value per store, `[..., stores]`; leading axes are batches. Left undefined, all stores begin at zero on the temperature scale you are using.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `temperature` | The initial temperature of each store. | undefined (zero) |

## Returns

A value; read by `tran`.

## Errors

`tran` throws `pluss::Error` for a start with no stores in the network, a last axis that does not have one value per store, or a value that is not finite.

## Interpretation and Use

Zero is rarely the right start unless your scale is relative to the surroundings. Start at the ambient to see a response to a disturbance, or at a steady-state solution to continue from it.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A store started warm and cold.

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
### Example 2

A body cooling from 100 degrees.

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
### Example 3

Rejected starts.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
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
  net.stores = {{0}, vec({100.0})};
  net.fixed = {{1}, vec({20.0})};
  const pluss::Series time = vec(grid(10.0, 3));
  th::Start two;
  two.temperature = vec({20.0, 30.0});
  report([&] { (void)th::tran(net, time, {}, two); });         // two temperatures for one store
  th::Start unusable;
  unusable.temperature = vec({std::nan("")});
  report([&] { (void)th::tran(net, time, {}, unusable); });    // a temperature that is not finite
}
```

Output

```text
eng::thermal::tran: start.temperature needs one value per store (1) on its last axis
eng::thermal::tran: start.temperature must be finite
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Drive.html">
    <span class="label">Previous</span>
    <span class="title">Drive</span>
  </a>
  <a class="next-link" href="steady.html">
    <span class="label">Next</span>
    <span class="title">steady</span>
  </a>
</div>
<!-- apiref:nav:end -->
