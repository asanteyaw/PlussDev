<!-- generated from TranOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::TranOptions`

Choices of the transient solver.

## Signature

```cpp
circuit::TranOptions
```

## What It Does

Selects the time-stepping rule of `tran` and the controls of its Newton iteration.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `scheme` | The time-stepping rule, `Scheme::Trapezoid` or `Scheme::Gear2`. | `Scheme::Trapezoid` |
| `solver` | The `SolveOptions` of the Newton iteration used when the circuit has diodes or transistors: `thermal_voltage`, `tolerance`, `max_iterations` and `gmin`. | `SolveOptions` defaults |

## Returns

A value; read by `tran`.

## Errors

Every value of `scheme` is accepted; an invalid `solver` control is reported by `tran`.

## Interpretation and Use

The trapezoid rule keeps the energy of an oscillation and is the more accurate rule at a given step; `Scheme::Gear2` damps the fast part of a response that a long step cannot resolve, at the price of damping oscillations. See `Scheme` for the choice.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Omitted options are the trapezoid rule.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
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
  out.print_defaults(PrintOptions{}.fixed(6));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);
  const pluss::Series time = vec(grid(1e-4, 21));
  ckt::TranOptions trapezoid;
  trapezoid.scheme = ckt::Scheme::Trapezoid;
  ckt::TranOptions gear;
  gear.scheme = ckt::Scheme::Gear2;
  const auto omitted = flat(ckt::tran(circuit, time).voltage.select(1, 2));
  const auto explicit_trapezoid = flat(ckt::tran(circuit, time, {}, {}, trapezoid).voltage.select(1, 2));
  const auto explicit_gear = flat(ckt::tran(circuit, time, {}, {}, gear).voltage.select(1, 2));
  double same = 0.0, differs = 0.0;
  for (std::size_t k = 0; k < omitted.size(); ++k) {
    same = std::max(same, std::abs(omitted[k] - explicit_trapezoid[k]));
    differs = std::max(differs, std::abs(omitted[k] - explicit_gear[k]));
  }
  out.writeln("options omitted vs Trapezoid, largest difference", same);
  out.writeln("options omitted vs Gear2,     largest difference", differs);
}
```

Output

```text
options omitted vs Trapezoid, largest difference 0.000000
options omitted vs Gear2,     largest difference 0.026348
```
### Example 2

Each rule on an oscillator that must keep its energy.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
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
  out.print_defaults(PrintOptions{}.fixed(6));
  // a lossless LC oscillator: 1 mH, 1 uF, the capacitor starts at 1 V
  ckt::Net lc;
  lc.nodes = 2;
  lc.inductors = {{1}, {0}, vec({1e-3})};
  lc.capacitors = {{1}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(lc);
  ckt::Start start;
  start.capacitor_voltage = vec({1.0});
  const double period = 2.0 * M_PI * std::sqrt(1e-3 * 1e-6);
  const std::vector<double> time = grid(period / 40.0, 161);   // four periods at 40 samples per period

  for (auto scheme : {ckt::Scheme::Trapezoid, ckt::Scheme::Gear2}) {
    ckt::TranOptions options;
    options.scheme = scheme;
    const auto run = ckt::tran(circuit, vec(time), {}, start, options);
    const double v = flat(run.voltage.select(1, 1)).back(), i = flat(run.inductor_current).back();
    out.writeln(scheme == ckt::Scheme::Trapezoid ? "Trapezoid" : "Gear2    ", "energy left after four periods", (0.5e-6 * v * v + 0.5e-3 * i * i) / 0.5e-6);
  }
}
```

Output

```text
Trapezoid energy left after four periods 1.000000
Gear2     energy left after four periods 0.921931
```
### Example 3

The error of each rule as the step is halved.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
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
  out.print_defaults(PrintOptions{}.fixed(6));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);
  for (auto scheme : {ckt::Scheme::Trapezoid, ckt::Scheme::Gear2}) {
    ckt::TranOptions options;
    options.scheme = scheme;
    out.writeln(scheme == ckt::Scheme::Trapezoid ? "Trapezoid" : "Gear2    ", "largest error at steps of 0.2, 0.1, 0.05 ms:");
    for (double step : {2e-4, 1e-4, 5e-5}) {
      const std::vector<double> time = grid(step, static_cast<int>(std::round(5e-3 / step)) + 1);
      const std::vector<double> v = flat(ckt::tran(circuit, vec(time), {}, {}, options).voltage.select(1, 2));
      double worst = 0.0;
      for (std::size_t k = 0; k < time.size(); ++k) worst = std::max(worst, std::abs(v[k] - 5.0 * (1.0 - std::exp(-time[k] / 1e-3))));
      out.writeln("  step", step * 1e3, "ms  error", worst);
    }
  }
}
```

Output

```text
Trapezoid largest error at steps of 0.2, 0.1, 0.05 ms:
  step 0.200000 ms  error 0.006158
  step 0.100000 ms  error 0.001534
  step 0.050000 ms  error 0.000383
Gear2     largest error at steps of 0.2, 0.1, 0.05 ms:
  step 0.200000 ms  error 0.079772
  step 0.100000 ms  error 0.025664
  step 0.050000 ms  error 0.007446
```
### Example 4

The iteration controls on a rectifier.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.diodes.positive = {1};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-12});
  net.resistors = {{2}, {0}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);
  const int samples = 81;
  std::vector<double> time, wave;
  for (int k = 0; k < samples; ++k) {
    time.push_back(k * 5e-5);
    wave.push_back(5.0 * std::sin(2.0 * M_PI * 500.0 * k * 5e-5));
  }
  ckt::Drive drive;
  drive.voltage = vec(wave).reshape({samples, 1});
  // a looser tolerance gives Newton fewer iterations per step
  for (double tolerance : {1e-4, 1e-8, 1e-12}) {
    ckt::TranOptions options;
    options.solver.tolerance = tolerance;
    const auto run = ckt::tran(circuit, vec(time), drive, {}, options);
    std::int64_t total = 0;
    for (std::int64_t count : run.iterations.items<std::int64_t>()) total += count;
    out.writeln("tolerance", tolerance, " total Newton iterations", total, " highest load voltage", run.voltage.select(1, 2).amax().item<double>());
  }
}
```

Output

```text
tolerance 0.0001  total Newton iterations 1101  highest load voltage 4.418159163704423
tolerance 1e-08  total Newton iterations 1477  highest load voltage 4.418159163683493
tolerance 1e-12  total Newton iterations 1671  highest load voltage 4.418159163683493
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Start.html">
    <span class="label">Previous</span>
    <span class="title">Start</span>
  </a>
  <a class="next-link" href="Scheme.html">
    <span class="label">Next</span>
    <span class="title">Scheme</span>
  </a>
</div>
<!-- apiref:nav:end -->
