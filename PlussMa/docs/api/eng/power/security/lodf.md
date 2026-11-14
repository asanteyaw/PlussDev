<!-- generated from lodf.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::lodf`

Line outage distribution factors of a grid.

## Signature

```cpp
power::LodfResult power::lodf(const power::Grid& grid, double tolerance = 1e-10);
```

## What It Does

For every line taken as an outage, returns how its pre-outage flow redistributes over the other lines. The factors follow from the transfer factors of the line against itself, and are exact for the DC model.

## Parameters

`grid`
: The compiled network.

`tolerance`
: The smallest denominator `|1 - PTDF-of-the-line-against-itself|` that is accepted; an outage with a smaller one is invalid.

## Returns

A `LodfResult`.

## Errors

Throws `std::invalid_argument` for a non-positive or non-finite tolerance.

## Interpretation and Use

An outage that splits the network, or of a line already out of service, is invalid and returns `NaN`. Leave the tolerance at its default unless you want to exclude lines that are nearly bridges.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The factors of a ring.

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

A spur outage and the effect of the tolerance.

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

Factors against an AC solution.

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
  const auto dc = flat(power::outages(grid).flow);   // [outage][monitored]
  power::Net cut = ring();
  cut.lines.active = {true, true, false};
  const auto ac = power::ac(power::make(cut));
  out.writeln("losing line 2: DC flows ", dc[6], " ", dc[7], " ", dc[8], "   AC flows ", flat(ac.from_power.real())[0], " ", flat(ac.from_power.real())[1], " ", flat(ac.from_power.real())[2]);
  out.writeln("AC converged ", ac.converged.item<bool>(), ", AC voltage at the load bus ", flat(ac.magnitude)[2]);
}
```

Output

```text
losing line 2: DC flows 0.50000 0.50000 0.00000   AC flows 0.50883 0.50589 0.00000
AC converged true, AC voltage at the load bus 0.93981
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ptdf.html">
    <span class="label">Previous</span>
    <span class="title">ptdf</span>
  </a>
  <a class="next-link" href="outages.html">
    <span class="label">Next</span>
    <span class="title">outages</span>
  </a>
</div>
<!-- apiref:nav:end -->
