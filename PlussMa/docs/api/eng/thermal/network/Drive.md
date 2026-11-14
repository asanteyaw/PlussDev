<!-- generated from Drive.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::Drive`

Time-varying boundary temperatures and heat inflows.

## Signature

```cpp
thermal::Drive
```

## What It Does

Replaces the stored values of `Fixed::temperature` and `Heats::power` by arrays that change from sample to sample of a transient run. A member left undefined holds the stored value constant over the run.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `temperature` | Boundary temperatures `[..., time, fixed]`, one row per sample of the time vector. | undefined (the stored temperatures) |
| `power` | Heat inflows `[..., time, heats]`, one row per sample. | undefined (the stored powers) |

## Returns

A value; read by `tran`.

## Errors

`tran` throws `pluss::Error` for a member without the matching group in the network, for a last axis that does not have one value per element, for a number of rows that differs from the number of time samples, and for a value that is not finite.

## Interpretation and Use

The value in row `k` applies at time sample `k`; a jump on a sample instant is a one-step ramp, so give the edge sample the average of the two levels if you need the exact energy.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A step in the surroundings and a heater pulse.

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
### Example 2

Stored values against driven values.

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
  // An undefined Drive member keeps the stored values; a defined one replaces them sample by sample.
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({0.5})};
  net.stores = {{0}, vec({100.0})};
  net.fixed = {{1}, vec({20.0})};
  th::Start warm;
  warm.temperature = vec({20.0});
  const pluss::Series time = vec(grid(100.0, 4));
  const auto held = th::tran(net, time, {}, warm);
  th::Drive hot;
  hot.temperature = mat({20, 60, 60, 60}, {4, 1});
  const auto driven = th::tran(net, time, hot, warm);
  out.writeln("stored ambient   ", flat(held.temperature.select(1, 0)));
  out.writeln("ambient steps up ", flat(driven.temperature.select(1, 0)));
}
```

Output

```text
stored ambient    [20.00000, 20.00000, 20.00000, 20.00000]
ambient steps up  [20.00000, 28.00000, 40.80000, 48.48000]
```
### Example 3

Rejected drives.

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
  <a class="prev-link" href="Result.html">
    <span class="label">Previous</span>
    <span class="title">Result</span>
  </a>
  <a class="next-link" href="Start.html">
    <span class="label">Next</span>
    <span class="title">Start</span>
  </a>
</div>
<!-- apiref:nav:end -->
