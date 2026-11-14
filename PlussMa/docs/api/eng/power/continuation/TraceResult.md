<!-- generated from TraceResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::TraceResult`

The points of a continuation curve.

## Signature

```cpp
power::TraceResult
```

## What It Does

The result of `trace`: one entry for every point on the curve, with the voltages, powers and loading factor at each.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `voltage` | Complex bus voltages at each point, `[..., point, bus]`. |  |
| `magnitude` | Voltage magnitudes, `[..., point, bus]`. |  |
| `angle` | Voltage angles, `[..., point, bus]`. |  |
| `injection` | Complex net bus injections, `[..., point, bus]`. |  |
| `from_power` | Complex power entering each line at its sending end, `[..., point, line]`. |  |
| `to_power` | The power at the receiving end, `[..., point, line]`. |  |
| `factor` | The loading factor of each point, `[..., point]`. |  |
| `tangent` | The unit tangent of the curve at each point, `[..., point, state + 1]`. |  |
| `residual` | The power mismatch at each point, `[..., point]`. |  |
| `valid` | False for the points after a scenario has stopped, `[..., point]`. |  |
| `reached` | True when the requested target or the nose was reached, `[...]`. |  |
| `points` | The number of points on the curve. |  |

## Returns

Returned by `trace`.

## Errors

Reading a field does not trace again.

## Interpretation and Use

Read `reached` before using the last point as the target or the nose. In a batch every scenario stops at its own end and the later points are marked invalid.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The curve of a line to its nose.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a slack and a load bus joined by a line of 0.01 + 0.1j per unit, with the given load
static power::Net two_bus(double active, double reactive) {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -active});
  net.buses.q = vec({0.0, -reactive});
  net.buses.vm = vec({1.0, 1.0});
  net.buses.va = vec({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = vec({0.01});
  net.lines.x = vec({0.1});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Grid grid = power::make(two_bus(0.0, 0.0));
  power::LoadPath path;
  path.p = vec({0.0, -1.0});
  path.q = vec({0.0, -0.4});
  power::TraceOptions options;
  options.stop = power::TraceStop::Nose;
  const auto curve = power::trace(grid, path, options);

  const auto factor = flat(curve.factor), voltage = flat(curve.magnitude.select(-1, 1));
  out.writeln(curve.points, " points; the nose was reached: ", curve.reached.item<bool>());
  out.writeln("load (p.u.)  load-bus voltage");
  for (std::size_t i = 0; i < factor.size(); i += 4) out.writeln(factor[i], "      ", voltage[i]);
  out.writeln("last point: load ", factor.back(), ", voltage ", voltage.back());
  out.writeln("closed form: load 3.15975, voltage 0.58482;  largest residual on the curve ", curve.residual.max().item<double>());
}
```

Output

```text
29 points; the nose was reached: true
load (p.u.)  load-bus voltage
0.00000      1.00000
0.54653      0.97038
1.34002      0.91705
2.12982      0.84405
2.72728      0.75893
3.03128      0.68243
3.14588      0.61759
3.15975      0.58482
last point: load 3.15975, voltage 0.58482
closed form: load 3.15975, voltage 0.58482;  largest residual on the curve 0.00000
```
### Example 2

Shapes and the end points of a trace to a target.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a slack and a load bus joined by a line of 0.01 + 0.1j per unit, with the given load
static power::Net two_bus(double active, double reactive) {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -active});
  net.buses.q = vec({0.0, -reactive});
  net.buses.vm = vec({1.0, 1.0});
  net.buses.va = vec({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = vec({0.01});
  net.lines.x = vec({0.1});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Grid base = power::make(two_bus(0.5, 0.2));
  power::LoadPath path;
  path.p = vec({0.0, -1.5});
  path.q = vec({0.0, -0.6});
  const auto curve = power::trace(base, path);   // the default options stop at the target, factor 1

  const auto factor = flat(curve.factor), voltage = flat(curve.magnitude.select(-1, 1));
  out.writeln("factors: ", factor);
  out.writeln("shapes: voltage ", curve.magnitude.sizes(), ", power entering the line ", curve.from_power.sizes(), ", tangent ", curve.tangent.sizes());
  out.writeln("first point ", voltage.front(), "   ordinary power flow at the base load ", flat(power::ac(base).magnitude)[1]);
  out.writeln("last point  ", voltage.back(), "   ordinary power flow at the target load ", flat(power::ac(power::make(two_bus(1.5, 0.6))).magnitude)[1]);
  out.writeln("target reached: ", curve.reached.item<bool>());
}
```

Output

```text
factors: [-0.00000, 0.04966, 0.14896, 0.34747, 0.54586, 0.74409, 0.94214, 1.00000]
shapes: voltage [8, 2], power entering the line [8, 1], tangent [8, 3]
first point 0.97309   ordinary power flow at the base load 0.97309
last point  0.90430   ordinary power flow at the target load 0.90430
target reached: true
```
### Example 3

A trace that did not reach its target.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a slack and a load bus joined by a line of 0.01 + 0.1j per unit, with the given load
static power::Net two_bus(double active, double reactive) {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -active});
  net.buses.q = vec({0.0, -reactive});
  net.buses.vm = vec({1.0, 1.0});
  net.buses.va = vec({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = vec({0.01});
  net.lines.x = vec({0.1});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Grid grid = power::make(two_bus(0.0, 0.0));
  power::LoadPath path;
  path.p = vec({0.0, -1.0});
  path.q = vec({0.0, -0.4});
  power::TraceOptions beyond;
  beyond.target = 4.0;   // the nose is at 3.15975
  const auto curve = power::trace(grid, path, beyond);
  out.writeln("target 4.0: reached ", curve.reached.item<bool>(), ", the curve ends at the nose, load ", flat(curve.factor).back(), ", valid points ", curve.valid.all().item<bool>());

  power::Options newton;
  newton.max_iterations = 60;
  const auto direct = power::ac(power::make(two_bus(4.0, 1.6)), newton);
  out.writeln("ordinary power flow at that load: converged ", direct.converged.item<bool>(), ", residual ", direct.residual.item<double>(), " (no voltage exists there, and it does not say where the limit is)");
}
```

Output

```text
target 4.0: reached false, the curve ends at the nose, load 3.15975, valid points true
ordinary power flow at that load: converged false, residual 3.49984 (no voltage exists there, and it does not say where the limit is)
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="TraceOptions.html">
    <span class="label">Previous</span>
    <span class="title">TraceOptions</span>
  </a>
  <a class="next-link" href="trace.html">
    <span class="label">Next</span>
    <span class="title">trace</span>
  </a>
</div>
<!-- apiref:nav:end -->
