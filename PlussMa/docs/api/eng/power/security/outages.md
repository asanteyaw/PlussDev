<!-- generated from outages.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::outages`

Flows after every single-line outage.

## Signature

```cpp
power::OutageResult power::outages(const power::Grid& grid, double tolerance = 1e-10);
```

## What It Does

Solves the DC power flow of the grid and applies the outage factors, returning the flow on every line after the loss of every single line. Screening all N-1 contingencies is one matrix computation.

## Parameters

`grid`
: The compiled network.

`tolerance`
: The validity threshold of `lodf`.

## Returns

An `OutageResult`.

## Errors

Throws `std::invalid_argument` for a non-positive or non-finite tolerance.

## Interpretation and Use

Compare `flow` with ratings and read `valid` next to it. Confirm the critical outages with an AC power flow with the line's `active` flag false.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Flows after each outage of a ring, checked against a new DC solve.

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

Screening a five-bus network for overloads.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }


// five buses: a slack generator at bus 0, a second generator of 4 per unit at bus 3, and loads of 3, 3 and 4 per unit at buses 1, 2 and 4.
// Lines: 0: 0-1, 1: 0-3, 2: 0-4, 3: 1-2, 4: 2-3, 5: 3-4.
static power::Net five_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -3.0, -3.0, 0.0, -4.0});
  net.buses.q = vec({0.0, -0.6, -0.6, 0.0, -0.8});
  net.buses.vm = vec({1.0, 1.0, 1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.gens.bus = {3};
  net.gens.p = vec({4.0});
  net.gens.q = vec({0.0});
  net.lines.from = {0, 0, 0, 1, 2, 3};
  net.lines.to = {1, 3, 4, 2, 3, 4};
  net.lines.x = vec({0.028, 0.030, 0.006, 0.011, 0.030, 0.030});
  net.lines.r = vec({0.0028, 0.003, 0.0006, 0.0011, 0.003, 0.003});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const power::Grid grid = power::make(five_bus());
  const auto result = power::outages(grid);
  const std::vector<double> rating = {4.0, 3.0, 4.5, 4.0, 3.5, 4.1};
  const auto base = flat(result.base_flow);
  const auto after = flat(result.flow);

  out.writeln("base flows:  ", base);
  out.writeln("ratings:     ", rating);
  int violations = 0;
  for (int k = 0; k < 6; ++k)
    for (int m = 0; m < 6; ++m)
      if (m != k && std::abs(after[6 * k + m]) > rating[m]) {
        ++violations;
        out.writeln("losing line ", k, " overloads line ", m, ": flow ", after[6 * k + m], " against a rating of ", rating[m]);
      }
  out.writeln(violations, " overloads in all");
}
```

Output

```text
base flows:  [3.006, -0.185, 3.179, 0.006, -2.994, 0.821]
ratings:     [4.000, 3.000, 4.500, 4.000, 3.500, 4.100]
losing line 0 overloads line 2: flow 4.545 against a rating of 4.500
losing line 0 overloads line 4: flow -6.000 against a rating of 3.500
losing line 4 overloads line 0: flow 6.000 against a rating of 4.000
3 overloads in all
```
### Example 3

The pitfall of comparing against NaN.

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
  <a class="prev-link" href="lodf.html">
    <span class="label">Previous</span>
    <span class="title">lodf</span>
  </a>
</div>
<!-- apiref:nav:end -->
