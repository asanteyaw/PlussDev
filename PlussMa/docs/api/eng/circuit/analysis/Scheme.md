<!-- generated from Scheme.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::Scheme`

Selects the time-stepping rule of a transient run.

## Signature

```cpp
circuit::Scheme
```

## What It Does

The two rules differ in how they treat a response that changes faster than the step can follow. Both are second order: halving the step divides the error of a smooth response by about four.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `Trapezoid` | Averages the equations at the two ends of each step. Exact for a straight-line response; conserves the energy of a lossless oscillation to rounding error; overshoots and rings when the step exceeds twice the fastest time constant. | The default of `TranOptions::scheme`. |
| `Gear2` | Extends the equations backwards over the two previous steps, starting with one backward-Euler step. Removes the fast part of a response within a few steps, overshooting by a few per cent at most; damps oscillations, more the fewer samples per period. |  |

## Returns

A choice of two rules, stored in `TranOptions::scheme`.

## Errors

None: both values are always valid.

## Interpretation and Use

Use `Trapezoid` for oscillators and resonant circuits, with at least twenty samples per period. Use `Gear2` for circuits whose time constants are far apart or when a long step cannot be avoided. When unsure, run both: agreement means either is right; a rule that rings or decays has told you the step is too long.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The energy of a lossless oscillator under each rule.

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
### Example 2

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
### Example 3

A step of ten time constants: the trapezoid rule rings and Gear2 does not.

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
  out.print_defaults(PrintOptions{}.fixed(5));
  // the RC step response with a step of ten time constants
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);
  const pluss::Series time = vec(grid(1e-2, 9));
  ckt::TranOptions gear;
  gear.scheme = ckt::Scheme::Gear2;
  out.writeln("Trapezoid", flat(ckt::tran(circuit, time).voltage.select(1, 2)));
  out.writeln("Gear2    ", flat(ckt::tran(circuit, time, {}, {}, gear).voltage.select(1, 2)));
}
```

Output

```text
Trapezoid [0.00000, 8.33333, 2.77778, 6.48148, 4.01235, 5.65844, 4.56104, 5.29264, 4.80491]
Gear2     [0.00000, 4.54545, 5.13834, 5.04382, 5.00161, 4.99837, 4.99965, 5.00001, 5.00002]
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="TranOptions.html">
    <span class="label">Previous</span>
    <span class="title">TranOptions</span>
  </a>
  <a class="next-link" href="SolveOptions.html">
    <span class="label">Next</span>
    <span class="title">SolveOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
