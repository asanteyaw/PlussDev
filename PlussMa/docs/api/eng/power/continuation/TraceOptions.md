<!-- generated from TraceOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::TraceOptions`

Stopping rule and stepping controls of the continuation.

## Signature

```cpp
power::TraceOptions
```

## What It Does

Controls for `trace`: where to stop, how large the first step is and how it may change, and the corrector limits.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `stop` | Whether the trace ends at the target or at the nose. | TraceStop::Target |
| `target` | The loading factor at which a `Target` trace ends. | 1 |
| `step` | The first step in the loading factor. | 0.05 |
| `step_min` | The smallest step the adaptation may use. | 1e-4 |
| `step_max` | The largest step the adaptation may use. | 0.2 |
| `adapt_tolerance` | The predictor error that the step adaptation aims at. | 1e-3 |
| `adapt_damping` | The damping of the step adaptation. | 0.7 |
| `tolerance` | The residual tolerance of each corrector. | 1e-9 |
| `max_steps` | The largest number of points on the curve. | 400 |
| `max_iterations` | The Newton iterations allowed per corrector. | 12 |
| `adaptive` | Let the step grow and shrink with the curvature. | true |

## Returns

A value; validated by `trace`.

## Errors

Step bounds, tolerance or iteration limits that are not positive and ordered are rejected.

## Interpretation and Use

An adaptive trace takes larger steps where the curve is nearly straight and shortens them near the nose. A small `max_steps` ends the trace early with `reached` false.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Adaptive, fixed and capped steps.

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
  const auto run = [&](const char *label, power::TraceOptions options) {
    options.target = 2.0;
    try {
      const auto r = power::trace(grid, path, options);
      out.writeln(label, ": ", r.points, " points, reached ", r.reached.item<bool>(), ", last factor ", flat(r.factor).back(), ", voltage ", flat(r.magnitude.select(-1, 1)).back());
    } catch (const std::exception &e) {
      std::cout << label << ": " << e.what() << "\n";
    }
  };
  run("defaults (adaptive)        ", power::TraceOptions{});
  power::TraceOptions fixed;
  fixed.adaptive = false;
  fixed.step = 0.1;
  run("fixed step 0.1             ", fixed);
  power::TraceOptions wide;
  wide.step_max = 0.5;
  run("step_max 0.5               ", wide);
  power::TraceOptions oversized;
  oversized.step = 0.5;
  run("first step above step_max  ", oversized);
  power::TraceOptions few;
  few.max_steps = 3;
  run("max_steps 3                ", few);
}
```

Output

```text
defaults (adaptive)        : 13 points, reached true, last factor 2.00000, voltage 0.85811
fixed step 0.1             : 22 points, reached true, last factor 2.00000, voltage 0.85811
step_max 0.5               : 10 points, reached true, last factor 2.00000, voltage 0.85811
first step above step_max  : eng::power::trace: step bounds, tolerance, and iteration limits must be positive and ordered
max_steps 3                : 4 points, reached false, last factor 0.34786, voltage 0.98170
```
### Example 2

The default trace.

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

A trace to the nose.

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="LoadPath.html">
    <span class="label">Previous</span>
    <span class="title">LoadPath</span>
  </a>
  <a class="next-link" href="TraceResult.html">
    <span class="label">Next</span>
    <span class="title">TraceResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
