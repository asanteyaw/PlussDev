<!-- generated from tran.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::tran`

Solves the transient response of a thermal network.

## Signature

```cpp
thermal::Result thermal::tran(const thermal::Net& net,
                     const Series& time,
                     const thermal::Drive& drive = {},
                     const thermal::Start& start = {},
                     circuit::Scheme scheme = circuit::Scheme::Trapezoid);
```

## What It Does

Integrates `C dT/dt = -G T + q` over a uniform time grid. `time` is a rank-1 array of at least two samples in seconds; `drive` supplies boundary temperatures and heats that change in time; `start` gives the initial store temperatures. The step is second order: `Trapezoid` conserves energy and is exact for a straight-line response, `Gear2` damps a response that is faster than the step.

## Parameters

`net`
: The network.

`time`
: The sample times in seconds, uniform, rank 1, at least two.

`drive`
: Time-varying boundary temperatures and heats; undefined members hold the stored values.

`start`
: The initial store temperatures; undefined means zero.

`scheme`
: The time-stepping rule: `Trapezoid` or `Gear2`.

## Returns

A `Result` in which every array gains a time axis before its element axis.

## Errors

Throws `pluss::Error` for an invalid network, a time array that is not rank 1 with two samples, a drive or start that does not match the network, or a singular step equation.

## Interpretation and Use

Pick the step from the time constants of the network. When the step is much longer than the fastest time constant the trapezoid rule rings and `Gear2` does not; if both agree the step is safe. Nodes that have no store settle instantly at each sample.

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

Surroundings that step and a heater pulse.

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

The two time-stepping rules on a long step.

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
  // A step much longer than the time constant: the trapezoid rule rings around the answer, the second-order backward rule does not.
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({1.0})};
  net.stores = {{0}, vec({1.0})};
  net.fixed = {{1}, vec({10.0})};
  th::Start start;
  start.temperature = vec({0.0});
  const auto trapezoid = th::tran(net, vec(grid(5.0, 6)), {}, start, ckt::Scheme::Trapezoid);
  const auto gear = th::tran(net, vec(grid(5.0, 6)), {}, start, ckt::Scheme::Gear2);
  out.writeln("trapezoid", flat(trapezoid.temperature.select(1, 0)));
  out.writeln("gear2    ", flat(gear.temperature.select(1, 0)));
  std::vector<double> exact;
  for (int k = 0; k <= 5; ++k) exact.push_back(10.0 * (1.0 - std::exp(-5.0 * k)));
  out.writeln("exact    ", exact);
  out.writeln("each step is five time constants long; the exact curve has settled after the first");
}
```

Output

```text
trapezoid [0.00000, 14.28571, 8.16327, 10.78717, 9.66264, 10.14458]
gear2     [0.00000, 8.33333, 10.25641, 10.20710, 10.04400, 9.99761]
exact     [0.00000, 9.93262, 9.99955, 10.00000, 10.00000, 10.00000]
each step is five time constants long; the exact curve has settled after the first
```
### Example 4

Rejected arguments.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
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
  const pluss::Series time = vec(grid(10.0, 5));
  th::Net bare = net;
  bare.heats = {};
  th::Drive power;
  power.power = mat({1, 1, 1, 1, 1}, {5, 1});
  report([&] { (void)th::tran(bare, time, power); });          // a power drive with no heats
  th::Drive wrong;
  wrong.temperature = mat({20, 20, 20}, {3, 1});
  report([&] { (void)th::tran(net, time, wrong); });           // a drive with the wrong number of time samples
  th::Net open = net;
  open.stores = {};
  th::Start start;
  start.temperature = vec({1.0});
  report([&] { (void)th::tran(open, time, {}, start); });      // a start temperature with no stores
  th::Net heavy = net;
  heavy.stores.capacity = vec({-100.0});
  report([&] { (void)th::tran(heavy, time); });                // a capacity that is not positive
}
```

Output

```text
eng::thermal::tran: drive.power needs heats in the network
eng::thermal::tran: drive.temperature needs one row per time sample (5), got 3
eng::thermal::tran: start.temperature needs stores in the network
eng::thermal::tran: stores.capacity must be positive
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="steady.html">
    <span class="label">Previous</span>
    <span class="title">steady</span>
  </a>
</div>
<!-- apiref:nav:end -->
