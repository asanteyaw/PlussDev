<!-- generated from op.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::op`

Solves the DC operating point of a circuit.

## Signature

```cpp
circuit::Result circuit::op(const circuit::Circuit& circuit, const circuit::SolveOptions& options = {});
```

## What It Does

Solves the DC equations: capacitors are open circuits and inductors are ideal shorts. A circuit of resistors, controlled sources and ideal sources is solved in one linear solve for all members of a batch. A circuit with diodes or transistors is solved by Newton iteration from a start with every voltage at zero: each iteration solves the circuit linearised about the previous junction voltages, with the change of each junction voltage limited so the exponential cannot overflow, until the update is below `options.tolerance`; a failed solve is retried with gmin stepping and source stepping. A batch iterates together until every member has converged.

## Parameters

`circuit`
: The compiled circuit.

`options`
: The controls of the Newton iteration; see `SolveOptions`. Circuits without diodes or transistors ignore them.

## Returns

A `Result` of real arrays. For a circuit with devices, `diode_current`, `collector_current` and `base_current` carry the device currents and `iterations` holds the number of Newton iterations, `[1]`; `residual` is the largest error of the nonlinear circuit equations, evaluated with the exact device currents. A circuit without devices has zero-width device arrays and `iterations` is zero.

## Errors

Throws `pluss::Error` when the equations are singular: the message names a node with no path to ground and a loop of ideal voltage sources and inductors, or two sources fixing the same voltage; for a circuit with devices, throws when the Newton iteration does not converge within `options.max_iterations` even with the fallbacks, when a device carries more than `options.max_current`, and for an invalid control.

## Interpretation and Use

Check the power balance, `residual`, and a hand value for a sub-circuit. The bias point of a circuit with devices is the starting point of `ac` around it. `residual` is absolute: read it against the size of the currents. A device current above `options.max_current` is refused: an ideal source directly across a junction has nothing to limit its current.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A divider and its result.

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

A bridge, current sources and an op amp.

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
  ckt::Net bridge;
  bridge.nodes = 4;
  bridge.voltage_sources = {{1}, {0}, vec({10.0})};
  bridge.resistors = {{1, 2, 1, 3, 2}, {2, 0, 3, 0, 3}, vec({100.0, 200.0, 300.0, 600.0, 50.0})};
  const auto balanced = flat(ckt::op(ckt::make(bridge)).voltage);
  out.writeln("balanced:   V2", balanced[2], " V3", balanced[3], " difference", balanced[2] - balanced[3]);

  bridge.resistors.value = vec({100.0, 200.0, 300.0, 500.0, 50.0});
  const auto skewed = flat(ckt::op(ckt::make(bridge)).voltage);
  out.writeln("unbalanced: V2", skewed[2], " V3", skewed[3], " difference", skewed[2] - skewed[3]);
}
```

Output

```text
balanced:   V2 6.666667  V3 6.666667  difference 0.000000
unbalanced: V2 6.575342  V3 6.506849  difference 0.068493
```
### Example 3

Singular circuits.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  report([&] {
    ckt::Net floating;
    floating.nodes = 3;
    floating.resistors = {{1}, {0}, vec({1.0})};   // node 2 is connected to nothing
    (void)ckt::op(ckt::make(floating));
  });
  report([&] {
    ckt::Net clash;
    clash.nodes = 3;
    clash.voltage_sources = {{1, 1}, {0, 0}, vec({1.0, 2.0})};   // 1 V and 2 V across the same nodes
    clash.resistors = {{1}, {2}, vec({1.0})};
    (void)ckt::op(ckt::make(clash));
  });
}
```

Output

```text
eng::circuit::op: the circuit equations are singular; check for a node with no path to ground, and for a loop of ideal voltage sources and inductors (or two sources fixing the same voltage). Solver message: subsp::math::linalg::lu_solve: singular matrix
eng::circuit::op: the circuit equations are singular; check for a node with no path to ground, and for a loop of ideal voltage sources and inductors (or two sources fixing the same voltage). Solver message: subsp::math::linalg::lu_solve: singular matrix
```
### Example 4

A diode behind a resistor.

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
  // 5 V source, 1 kohm, silicon diode to ground
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});

  const auto result = ckt::op(ckt::make(net));
  const double vd = flat(result.voltage)[2];
  const double id = flat(result.diode_current)[0];
  out.writeln("diode voltage     ", vd);
  out.writeln("diode current     ", id);
  out.writeln("resistor current  ", (5.0 - vd) / 1000.0);
  out.writeln("Newton iterations ", result.iterations.items<std::int64_t>()[0]);
  out.writeln("residual below 1e-9", result.residual.item<double>() < 1e-9);
}
```

Output

```text
diode voltage      0.692490
diode current      0.004308
resistor current   0.004308
Newton iterations  12
residual below 1e-9 true
```
### Example 5

A transistor stage.

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
  // NPN common emitter: 10 V supply, 1 kohm collector resistor, 5 V base drive through 100 kohm
  ckt::Net net;
  net.nodes = 5;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({10.0, 5.0})};
  net.resistors = {{1, 3}, {2, 4}, vec({1000.0, 100000.0})};
  net.transistors.collector = {2};
  net.transistors.base = {4};
  net.transistors.emitter = {0};
  net.transistors.polarity = {1};
  net.transistors.saturation = vec({1e-14});
  net.transistors.forward_gain = vec({100.0});

  const auto result = ckt::op(ckt::make(net));
  const std::vector<double> v = flat(result.voltage);
  const double ic = flat(result.collector_current)[0], ib = flat(result.base_current)[0];
  out.writeln("base voltage      ", v[4]);
  out.writeln("collector voltage ", v[2]);
  out.writeln("base current      ", ib);
  out.writeln("collector current ", ic);
  out.writeln("current gain Ic/Ib", ic / ib);
  out.writeln("emitter current   ", -(ic + ib));
}
```

Output

```text
base voltage       0.692490
collector voltage  5.692490
base current       0.000043
collector current  0.004308
current gain Ic/Ib 100.000000
emitter current    -0.004351
```
### Example 6

Controls of the iteration and the iteration count they produce.

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
  out.print_defaults(PrintOptions{}.fixed(8));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  const auto circuit = ckt::make(net);
  // a looser tolerance stops sooner; the answer changes only in the last digits
  for (double tolerance : {1e-3, 1e-6, 1e-10}) {
    ckt::SolveOptions options;
    options.tolerance = tolerance;
    const auto result = ckt::op(circuit, options);
    out.writeln("tolerance 1e", static_cast<int>(std::lround(std::log10(tolerance))), " iterations", result.iterations.items<std::int64_t>()[0], " diode voltage", flat(result.voltage)[2]);
  }
}
```

Output

```text
tolerance 1e -3  iterations 9  diode voltage 0.69249039
tolerance 1e -6  iterations 11  diode voltage 0.69249038
tolerance 1e -10  iterations 12  diode voltage 0.69249038
```
### Example 7

A swept source solved as a batch.

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
  // a swept source is a batch: each lane is its own solve
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({-1.0, 0.0, 0.5, 1.0, 2.0, 5.0}).reshape({6, 1})};
  net.resistors = {{1}, {2}, vec({100.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-12});

  const auto result = ckt::op(ckt::make(net));
  out.writeln("diode_current shape", result.diode_current.sizes(), " (source value, diode)");
  const std::vector<double> volts = flat(result.voltage.select(1, 2));
  const std::vector<double> amps = flat(result.diode_current);
  out.writeln("  source   diode V   diode I (uA)");
  const double drive[] = {-1.0, 0.0, 0.5, 1.0, 2.0, 5.0};
  for (int k = 0; k < 6; ++k) out.writeln(drive[k], volts[k], amps[k] * 1e6);
}
```

Output

```text
diode_current shape [6, 1]  (source value, diode)
  source   diode V   diode I (uA)
-1.000000 -1.000000 -0.000001
0.000000 0.000000 0.000000
0.500000 0.485600 143.997775
1.000000 0.573208 4267.919583
2.000000 0.603845 13961.550383
5.000000 0.633321 43666.787832
```
### Example 8

An iteration that cannot converge, and invalid controls.

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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  const auto circuit = ckt::make(net);
  ckt::SolveOptions few;
  few.max_iterations = 3;
  report([&] { (void)ckt::op(circuit, few); });
  ckt::SolveOptions cold;
  cold.thermal_voltage = 0.0;
  report([&] { (void)ckt::op(circuit, cold); });
  ckt::SolveOptions loose;
  loose.tolerance = -1.0;
  report([&] { (void)ckt::op(circuit, loose); });
  ckt::SolveOptions leaky;
  leaky.gmin = -1e-12;
  report([&] { (void)ckt::op(circuit, leaky); });
  ckt::SolveOptions none;
  none.max_iterations = 0;
  report([&] { (void)ckt::op(circuit, none); });
}
```

Output

```text
eng::circuit::op: the Newton iteration did not converge in 3 iterations (last update 4.59); raise max_iterations, loosen tolerance, or check that the bias is reachable (gmin stepping and source stepping were also tried)
eng::circuit::op: solver thermal_voltage must be positive
eng::circuit::op: solver tolerance must be positive
eng::circuit::op: solver gmin must be non-negative
eng::circuit::op: solver max_iterations must be at least 1
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Result.html">
    <span class="label">Previous</span>
    <span class="title">Result</span>
  </a>
  <a class="next-link" href="ac.html">
    <span class="label">Next</span>
    <span class="title">ac</span>
  </a>
</div>
<!-- apiref:nav:end -->
