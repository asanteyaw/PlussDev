<!-- generated from TraceStop.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::TraceStop`

Where a continuation trace ends.

## Signature

```cpp
power::TraceStop
```

## What It Does

Chooses the end of a trace: `Target` follows the curve until the loading factor equals the target, `Nose` follows it until the factor reaches its maximum, the voltage-collapse point.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `Target` | Stop when the loading factor equals `TraceOptions::target`. | default |
| `Nose` | Stop at the maximum of the loading factor, the nose of the voltage curve. |  |

## Returns

An enumeration used in `TraceOptions::stop`.

## Errors

No validation error.

## Interpretation and Use

With `Target` a target beyond the nose cannot be reached: the trace ends at the nose and `reached` is false.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The nose of a line against its closed form.

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

A target beyond the nose.

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
### Example 3

The nose of a network with a generator.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the three-bus network of chapter 1
static power::Net three_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.2, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.2});
  net.buses.vm = vec({1.04, 1.02, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {1};
  net.gens.p = vec({0.6});
  net.gens.q = vec({0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.015});
  net.lines.x = vec({0.1, 0.2, 0.25});
  net.lines.charge = vec({0.02, 0.05, 0.04});
  return net;
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Grid grid = power::make(three_bus());
  power::LoadPath path;
  path.p = vec({0.0, 0.4, -1.0});    // bus 1 keeps its net injection of 0.4; the load at bus 2 grows to 1.0
  path.q = vec({0.0, 0.0, -0.4});
  power::TraceOptions options;
  options.stop = power::TraceStop::Nose;
  const auto curve = power::trace(grid, path, options);
  out.writeln("trace: nose reached ", curve.reached.item<bool>(), " at loading factor ", flat(curve.factor).back(), " after ", curve.points, " points, load-bus voltage ", flat(curve.magnitude.select(-1, 2)).back());

  // bisection on the ordinary power flow: the load at bus 2 is 0.5 + factor * 0.5 and 0.2 + factor * 0.2
  double low = 0.0, high = 20.0;
  const auto converges = [&](double factor) {
    power::Net net = three_bus();
    net.buses.p = vec({0.0, -0.2, -0.5 - 0.5 * factor});
    net.buses.q = vec({0.0, 0.0, -0.2 - 0.2 * factor});
    power::Options newton;
    newton.max_iterations = 60;
    return power::ac(power::make(net), newton).converged.item<bool>();
  };
  for (int i = 0; i < 40; ++i) (converges(0.5 * (low + high)) ? low : high) = 0.5 * (low + high);
  out.writeln("largest factor at which the ordinary power flow converges: ", low);
}
```

Output

```text
trace: nose reached true at loading factor 5.11481 after 36 points, load-bus voltage 0.60798
largest factor at which the ordinary power flow converges: 5.11481
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="LoadPath.html">
    <span class="label">Next</span>
    <span class="title">LoadPath</span>
  </a>
</div>
<!-- apiref:nav:end -->
