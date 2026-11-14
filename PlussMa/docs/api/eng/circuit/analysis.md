<!-- generated from analysis.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Circuit analysis

This branch solves a compiled circuit. `op` gives the DC operating point, `ac` the phasor response at a vector of frequencies and `tran` the response in time to a source step or a waveform. `Drive`, `Start`, `TranOptions` and `Scheme` configure `tran`, and `SolveOptions` controls the Newton iteration that `op`, `ac` and `tran` use for circuits with diodes and transistors.

## Choosing A Surface

Use `op` for steady constant sources, `ac` for the settled response to a sinusoid at each frequency, and `tran` when you need how the circuit gets there: a switch-on, a pulse, a waveform of any shape, or an oscillation.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

The DC operating point of a divider.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({10.0})};
  net.resistors = {{1, 2}, {2, 0}, vec({1000.0, 3000.0})};
  const auto result = ckt::op(ckt::make(net));

  out.writeln("node voltages ", flat(result.voltage));
  out.writeln("source current", flat(result.source_current), " (negative: the source delivers 2.5 mA)");
  out.writeln("residual      ", result.residual.item<double>());
  out.writeln("axes of the empty groups: inductors", result.inductor_current.sizes(), " VCCS", result.vccs_current.sizes(), " VCVS", result.vcvs_current.sizes(), " amps", result.amp_current.sizes());
}
```

Output

```text
node voltages  [0.000000, 10.000000, 7.500000]
source current [-0.002500]  (negative: the source delivers 2.5 mA)
residual       0.000000
axes of the empty groups: inductors [0]  VCCS [0]  VCVS [0]  amps [0]
```
### Example 2

An RC low-pass in AC.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const pluss::Series omega = vec({0.0, 1.0, 1000.0, 100000.0});

  const auto result = ckt::ac(ckt::make(net), omega);
  const pluss::Series output = result.voltage.select(1, 2);   // the capacitor voltage, node 2
  out.writeln("voltage shape", result.voltage.sizes(), " element type", result.voltage.dtype().name);
  out.writeln("gain         ", flat(output.abs()));
  out.writeln("phase (deg)  ", flat(output.angle() * (180.0 / M_PI)));
  out.writeln("analytic gain", 1.0, 1.0 / std::sqrt(1.0 + 1e-6), 1.0 / std::sqrt(2.0), 1.0 / std::sqrt(1.0 + 1e4));
  out.writeln("source current magnitude", flat(result.source_current.abs()));
}
```

Output

```text
voltage shape [4, 3]  element type complex128
gain          [1.000000, 1.000000, 0.707107, 0.010000]
phase (deg)   [0.000000, -0.057296, -45.000000, -89.427061]
analytic gain 1.000000 1.000000 0.707107 0.010000
source current magnitude [0.000000, 0.000001, 0.000707, 0.001000]
```
### Example 3

A sweep of component values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({0.1e-6, 1e-6, 10e-6}).reshape({3, 1})};
  const auto result = ckt::ac(ckt::make(net), vec({100.0, 1000.0, 10000.0}));

  out.writeln("voltage shape", result.voltage.sizes(), " (capacitor value, frequency, node)");
  const pluss::Series gain = result.voltage.select(2, 2).abs();   // node 2, shape [3, 3]
  out.writeln("gain: rows are 0.1, 1 and 10 microfarad; columns 100, 1000 and 10000 rad/s");
  for (long row = 0; row < 3; ++row) out.writeln(flat(gain.select(0, row)));
}
```

Output

```text
voltage shape [3, 3, 3]  (capacitor value, frequency, node)
gain: rows are 0.1, 1 and 10 microfarad; columns 100, 1000 and 10000 rad/s
[0.99995, 0.99504, 0.70711]
[0.99504, 0.70711, 0.09950]
[0.70711, 0.09950, 0.01000]
```
### Example 4

A step response in time.

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
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto run = ckt::tran(ckt::make(net), vec(grid(1e-4, 51)));   // 5 ms in steps of 0.1 ms

  out.writeln("voltage shape", run.voltage.sizes(), " source current shape", run.source_current.sizes(), " residual shape", run.residual.sizes());
  out.writeln("first sample", flat(run.voltage.select(0, 0)));
  const std::vector<double> v = flat(run.voltage.select(1, 2));
  for (int ms : {1, 2, 5}) out.writeln("t =", ms, "ms  v =", v[ms * 10], " closed form", 5.0 * (1.0 - std::exp(-ms * 1e-3 / 1e-3)));
  out.writeln("largest step residual", run.residual.amax().item<double>() < 1e-12 ? "below 1e-12" : "too large");
}
```

Output

```text
voltage shape [51, 3]  source current shape [51, 1]  residual shape [51]
first sample [0.00000, 5.00000, 0.00000]
t = 1 ms  v = 3.16214  closed form 3.16060
t = 2 ms  v = 4.32445  closed form 4.32332
t = 5 ms  v = 4.96645  closed form 4.96631
largest step residual below 1e-12
```
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

analysis/Result
analysis/op
analysis/ac
analysis/tran
analysis/Drive
analysis/Start
analysis/TranOptions
analysis/Scheme
analysis/SolveOptions
```
<!-- apiref:toctree:end -->
