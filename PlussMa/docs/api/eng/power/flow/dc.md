<!-- generated from dc.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::dc`

Solves the linear DC power flow.

## Signature

```cpp
power::DcResult power::dc(const power::Grid& grid);
```

## What It Does

Assumes unit voltage magnitudes, lossless lines and small angle differences, so that the active flow on a line is `(theta_i - theta_j) / x` and the power balance is the linear system `B theta = P`. One solve gives the angles, the injections and the flows of every scenario.

## Parameters

`grid`
: The compiled network.

## Returns

A `DcResult`.

## Errors

Throws `std::invalid_argument` for a malformed grid. The solve itself has no failure state for a connected network.

## Interpretation and Use

Use it where resistance is small against reactance and voltages are near nominal: screening, sensitivities, contingencies and dispatch. It cannot see voltages, reactive power or losses.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A ring with flows that can be found by hand.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// three buses in a ring: a slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 in between.
// The two paths from bus 0 to bus 2, directly (x = 0.3) and through bus 1 (x = 0.1 + 0.2), have equal reactance.
static power::Net ring(double resistance_scale = 1.0) {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01 * resistance_scale, 0.02 * resistance_scale, 0.03 * resistance_scale});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto r = power::dc(power::make(ring()));
  out.writeln("angles (rad):          ", flat(r.angle), "   by hand 0, -0.025, -0.075");
  out.writeln("flows from end (p.u.): ", flat(r.from_power), "   by hand 0.25 (0 to 1), 0.25 (1 to 2), 0.25 (0 to 2)");
  out.writeln("flows to end (p.u.):   ", flat(r.to_power), "   equal and opposite: the line is lossless");
  out.writeln("injections (p.u.):     ", flat(r.injection), "   the slack supplies the whole load");
  out.writeln("residual: ", flat(r.residual));
}
```

Output

```text
angles (rad):          [0.00000, -0.02500, -0.07500]   by hand 0, -0.025, -0.075
flows from end (p.u.): [0.25000, 0.25000, 0.25000]   by hand 0.25 (0 to 1), 0.25 (1 to 2), 0.25 (0 to 2)
flows to end (p.u.):   [-0.25000, -0.25000, -0.25000]   equal and opposite: the line is lossless
injections (p.u.):     [0.50000, 0.00000, -0.50000]   the slack supplies the whole load
residual: [0.00000]
```
### Example 2

Linearity: a sweep of load levels.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// three buses in a ring: a slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 in between.
// The two paths from bus 0 to bus 2, directly (x = 0.3) and through bus 1 (x = 0.1 + 0.2), have equal reactance.
static power::Net ring(double resistance_scale = 1.0) {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01 * resistance_scale, 0.02 * resistance_scale, 0.03 * resistance_scale});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  power::Net net = ring();
  net.buses.p = pluss::series::make(std::vector<double>{0, 0, -0.25, 0, 0, -0.5, 0, 0, -1.0}).reshape({3, 3});
  net.buses.q = pluss::series::make(std::vector<double>{0, 0, -0.05, 0, 0, -0.1, 0, 0, -0.2}).reshape({3, 3});
  const auto r = power::dc(power::make(net));
  const auto angle = flat(r.angle.select(-1, 2));
  out.writeln("load-bus angle for 0.25, 0.5 and 1.0 per unit (rad): ", angle);
  out.writeln("ratios to the first: ", angle[0] / angle[0], " ", angle[1] / angle[0], " ", angle[2] / angle[0]);
  out.writeln("shape of the angles: ", r.angle.sizes());
}
```

Output

```text
load-bus angle for 0.25, 0.5 and 1.0 per unit (rad): [-0.037500, -0.075000, -0.150000]
ratios to the first: 1.000000 2.000000 4.000000
shape of the angles: [3, 3]
```
### Example 3

Compared with the AC solution, and its lack of losses.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// three buses in a ring: a slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 in between.
// The two paths from bus 0 to bus 2, directly (x = 0.3) and through bus 1 (x = 0.1 + 0.2), have equal reactance.
static power::Net ring(double resistance_scale = 1.0) {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01 * resistance_scale, 0.02 * resistance_scale, 0.03 * resistance_scale});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto dc = power::dc(power::make(ring()));
  out.writeln("DC:                      direct-line flow ", flat(dc.from_power)[2], ", slack output ", flat(dc.injection)[0]);
  for (double scale : {1.0, 10.0}) {
    const auto ac = power::ac(power::make(ring(scale)));
    out.writeln("AC, resistance x ", scale, ":    direct-line flow ", flat(ac.from_power.real())[2], ", slack output ", flat(ac.injection.real())[0], ", converged ", ac.converged.item<bool>());
  }
}
```

Output

```text
DC:                      direct-line flow 0.25000, slack output 0.50000
AC, resistance x 1.00000:    direct-line flow 0.25206, slack output 0.50411, converged true
AC, resistance x 10.00000:    direct-line flow 0.27421, slack output 0.54842, converged true
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="AcResult.html">
    <span class="label">Previous</span>
    <span class="title">AcResult</span>
  </a>
  <a class="next-link" href="ac.html">
    <span class="label">Next</span>
    <span class="title">ac</span>
  </a>
</div>
<!-- apiref:nav:end -->
