<!-- generated from ptdf.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::ptdf`

Power transfer distribution factors of a grid.

## Signature

```cpp
pluss::Series power::ptdf(const power::Grid& grid);
```

## What It Does

Returns the matrix whose entry for a line and a bus is the change of the DC active flow on the line, from its first bus to its second, per unit of power injected at the bus and withdrawn at the slack. The matrix depends only on the network, so one matrix gives the effect of any change of injection: `flow + ptdf * delta_p`.

## Parameters

`grid`
: The compiled network.

## Returns

A `Series` of shape `[..., line, bus]`; the column of the slack bus and the row of an out-of-service line are zero.

## Errors

Throws `std::invalid_argument` for a malformed grid.

## Interpretation and Use

A transfer between two buses is the difference of their columns. The factors are exact for the DC model and say nothing about losses or voltages.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The factors of a ring against hand values.

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
  const auto factors = power::ptdf(grid);
  const auto f = flat(factors);
  out.writeln("shape of the PTDF matrix: ", factors.sizes(), " (line, bus)");
  for (int line = 0; line < 3; ++line) out.writeln("line ", line, ": ", f[3 * line], " ", f[3 * line + 1], " ", f[3 * line + 2]);
  out.writeln("by hand: bus 1 column -5/6, +1/6, -1/6 (the reactance ratio 0.5 : 0.1 of the two paths from bus 1 to the slack); bus 2 column -1/2, -1/2, -1/2");
}
```

Output

```text
shape of the PTDF matrix: [3, 3] (line, bus)
line 0: 0.00000 -0.83333 -0.50000
line 1: 0.00000 0.16667 -0.50000
line 2: 0.00000 -0.16667 -0.50000
by hand: bus 1 column -5/6, +1/6, -1/6 (the reactance ratio 0.5 : 0.1 of the two paths from bus 1 to the slack); bus 2 column -1/2, -1/2, -1/2
```
### Example 2

Predicting the effect of a change of load, checked against a new solve.

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
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto base = flat(power::dc(grid).from_power);
  const auto f = flat(power::ptdf(grid));

  const double change = -0.3;   // the injection at bus 2 falls by 0.3: a larger load
  std::vector<double> predicted(3);
  for (int line = 0; line < 3; ++line) predicted[line] = base[line] + f[3 * line + 2] * change;

  power::Net heavier = ring();
  heavier.buses.p = vec({0.0, 0.0, -0.8});
  const auto resolved = flat(power::dc(power::make(heavier)).from_power);
  out.writeln("base flows:               ", base);
  out.writeln("predicted with the PTDF:  ", predicted);
  out.writeln("re-solved flows:          ", resolved);
  double worst = 0.0;
  for (int line = 0; line < 3; ++line) worst = std::max(worst, std::abs(predicted[line] - resolved[line]));
  out.writeln("largest difference: ", worst);
}
```

Output

```text
base flows:               [0.250000, 0.250000, 0.250000]
predicted with the PTDF:  [0.400000, 0.400000, 0.400000]
re-solved flows:          [0.400000, 0.400000, 0.400000]
largest difference: 0.000000
```
### Example 3

A transfer between two buses, and the wrong reading of one column.

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
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto f = flat(power::ptdf(grid));
  const int line = 0;
  const double shift = 0.1;   // generation moves from bus 1 to bus 2: the injection falls by 0.1 at bus 1 and rises by 0.1 at bus 2

  const double one_column = f[3 * line + 2] * shift;                           // reading the column of bus 2 alone
  const double difference = (f[3 * line + 2] - f[3 * line + 1]) * shift;        // the transfer from bus 1 to bus 2

  power::Net moved = ring();
  moved.buses.p = vec({0.0, -0.1, -0.4});
  const double before = flat(power::dc(grid).from_power)[line];
  const double after = flat(power::dc(power::make(moved)).from_power)[line];
  out.writeln("change of the flow on line 0 from re-solving:       ", after - before);
  out.writeln("column of bus 2 alone (wrong):                      ", one_column);
  out.writeln("difference of the columns of bus 2 and bus 1:       ", difference);
}
```

Output

```text
change of the flow on line 0 from re-solving:       0.033333
column of bus 2 alone (wrong):                      -0.050000
difference of the columns of bus 2 and bus 1:       0.033333
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="OutageResult.html">
    <span class="label">Previous</span>
    <span class="title">OutageResult</span>
  </a>
  <a class="next-link" href="lodf.html">
    <span class="label">Next</span>
    <span class="title">lodf</span>
  </a>
</div>
<!-- apiref:nav:end -->
