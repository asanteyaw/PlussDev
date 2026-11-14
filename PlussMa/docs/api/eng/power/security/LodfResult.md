<!-- generated from LodfResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::LodfResult`

Line outage distribution factors and their validity.

## Signature

```cpp
power::LodfResult
```

## What It Does

The result of `lodf`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `factor` | The factor of every monitored line for every outage, `[..., monitored line, outage line]`. A line monitored against its own outage has factor `-1`; the column of an invalid outage is `NaN`. |  |
| `valid` | True for each outage line that is a single-line contingency of a connected network, `[..., outage line]`. |  |

## Returns

Returned by `lodf`.

## Errors

Reading a field does not compute again.

## Interpretation and Use

The flow on line `m` after the loss of line `k` is `flow[m] + factor[m, k] * flow[k]`. An outage is invalid when the line is already out of service or its loss splits the network.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The factors of a ring and the flows they give.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: the slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 between them.
// Lines: 0 joins buses 0-1 (x = 0.1), line 1 joins 1-2 (x = 0.2), line 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto factors = power::lodf(grid);
  const auto f = flat(factors.factor);
  out.writeln("LODF [monitored line][outage line], shape ", factors.factor.sizes(), ":");
  for (int m = 0; m < 3; ++m) out.writeln("  line ", m, ": ", f[3 * m], " ", f[3 * m + 1], " ", f[3 * m + 2]);
  out.writeln("valid outages: ", flat(factors.valid));

  const auto result = power::outages(grid);
  const auto after = flat(result.flow);
  out.writeln("base flows: ", flat(result.base_flow));
  for (int k = 0; k < 3; ++k) out.writeln("flows after losing line ", k, ": ", after[3 * k], " ", after[3 * k + 1], " ", after[3 * k + 2]);

  // the same outage by taking line 2 out of service and solving again
  power::Net direct = ring();
  direct.lines.active = {true, true, false};
  out.writeln("flows from a new DC solve without line 2: ", flat(power::dc(power::make(direct)).from_power));
}
```

Output

```text
LODF [monitored line][outage line], shape [3, 3]:
  line 0: -1.00000 -1.00000 1.00000
  line 1: -1.00000 -1.00000 1.00000
  line 2: 1.00000 1.00000 -1.00000
valid outages: [1.00000, 1.00000, 1.00000]
base flows: [0.25000, 0.25000, 0.25000]
flows after losing line 0: 0.00000 0.00000 0.50000
flows after losing line 1: 0.00000 0.00000 0.50000
flows after losing line 2: 0.50000 0.50000 0.00000
flows from a new DC solve without line 2: [0.50000, 0.50000, 0.00000]
```
### Example 2

A spur line whose outage is invalid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: the slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 between them.
// Lines: 0 joins buses 0-1 (x = 0.1), line 1 joins 1-2 (x = 0.2), line 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  power::Net spur = ring();
  spur.buses.kind.push_back(power::BusKind::PQ);   // a fourth bus on a spur from bus 2
  spur.buses.p = vec({0.0, 0.0, -0.5, -0.1});
  spur.buses.q = vec({0.0, 0.0, -0.1, -0.02});
  spur.buses.vm = vec({1.0, 1.0, 1.0, 1.0});
  spur.buses.va = vec({0.0, 0.0, 0.0, 0.0});
  spur.lines.from.push_back(2);
  spur.lines.to.push_back(3);
  spur.lines.r = vec({0.01, 0.02, 0.03, 0.01});
  spur.lines.x = vec({0.1, 0.2, 0.3, 0.1});
  const power::Grid grid = power::make(spur);

  const auto result = power::outages(grid);
  out.writeln("valid outages (one entry per line): ", flat(result.valid));
  const auto after = flat(result.flow);
  for (int k = 0; k < 4; ++k) out.writeln("flows after losing line ", k, ": ", after[4 * k], " ", after[4 * k + 1], " ", after[4 * k + 2], " ", after[4 * k + 3]);
  out.writeln("with tolerance 0.9, valid outages: ", flat(power::lodf(grid, 0.9).valid));
}
```

Output

```text
valid outages (one entry per line): [1.000, 1.000, 1.000, 0.000]
flows after losing line 0: 0.000 0.000 0.600 0.100
flows after losing line 1: 0.000 0.000 0.600 0.100
flows after losing line 2: 0.600 0.600 0.000 0.100
flows after losing line 3: nan nan nan nan
with tolerance 0.9, valid outages: [0.000, 0.000, 0.000, 0.000]
```
### Example 3

Reading the factors next to the validity.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: the slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 between them.
// Lines: 0 joins buses 0-1 (x = 0.1), line 1 joins 1-2 (x = 0.2), line 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  power::Net spur = ring();
  spur.buses.kind.push_back(power::BusKind::PQ);
  spur.buses.p = vec({0.0, 0.0, -0.5, -0.1});
  spur.buses.q = vec({0.0, 0.0, -0.1, -0.02});
  spur.buses.vm = vec({1.0, 1.0, 1.0, 1.0});
  spur.buses.va = vec({0.0, 0.0, 0.0, 0.0});
  spur.lines.from.push_back(2);
  spur.lines.to.push_back(3);
  spur.lines.r = vec({0.01, 0.02, 0.03, 0.01});
  spur.lines.x = vec({0.1, 0.2, 0.3, 0.1});
  const auto result = power::outages(power::make(spur));
  const auto after = flat(result.flow);
  const auto valid = flat(result.valid);
  const double rating = 0.45;   // a tight rating on every line

  int naive = 0, skipped = 0;
  for (int k = 0; k < 4; ++k) {
    if (valid[k] < 0.5) ++skipped;
    for (int m = 0; m < 4; ++m)
      if (std::abs(after[4 * k + m]) > rating) ++naive;
  }
  out.writeln("overloads counted by comparing every entry: ", naive);
  out.writeln("outages that were not valid contingencies and were silently skipped: ", skipped, " (the spur line: its post-outage row is NaN, and a comparison with NaN is false)");
}
```

Output

```text
overloads counted by comparing every entry: 4
outages that were not valid contingencies and were silently skipped: 1 (the spur line: its post-outage row is NaN, and a comparison with NaN is false)
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="OutageResult.html">
    <span class="label">Next</span>
    <span class="title">OutageResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
