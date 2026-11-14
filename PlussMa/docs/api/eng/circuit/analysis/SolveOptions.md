<!-- generated from SolveOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::SolveOptions`

Controls of the Newton iteration for circuits with diodes and transistors.

## Signature

```cpp
circuit::SolveOptions
```

## What It Does

Sets the thermal voltage of the device models, the leakage conductance across every junction, when the Newton iteration stops, the largest device current that is accepted, and how `tran` subdivides a time step it cannot resolve. A solve that does not converge is retried automatically by gmin stepping (a large junction conductance stepped down to `gmin`) and, for operating points and starting states, by source stepping (the sources ramped up from a tenth), before an error is reported. `op` and the overload of `ac` that takes an operating point read it directly; `tran` reads it from `TranOptions::solver`. A circuit without diodes or transistors does not iterate and ignores these values apart from validating them.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `thermal_voltage` | `Vt = k T / q` in volts, positive; `0.02585` is room temperature (`300 K`). | `0.02585` |
| `tolerance` | The iteration stops when the largest change of any unknown and of any limited junction voltage is below `tolerance` times one plus the largest unknown. It is raised to a few hundred times the machine epsilon of the circuit's dtype if it is smaller. | `1e-10` |
| `max_iterations` | The largest number of Newton iterations per solve (per time sample in `tran`), at least `1`. | `100` |
| `gmin` | A conductance in siemens, not negative, across every junction. The default is far below the leakage of any real junction, so a reverse-biased diode carries its saturation current; raise it to model a leaky junction. | `1e-18` |
| `max_current` | The largest device current in amperes that the solution may carry, positive. A larger current is an error: it means nothing in the circuit limits it. | `1e6` |
| `step_tolerance` | `tran` subdivides a time step when its solution differs from two half steps by more than this relative amount, positive. | `1e-3` |
| `max_refinement` | The largest number of halvings of a time step in `tran`, not negative; `0` turns the subdivision off. | `8` |

## Returns

A value; read by `op`, `ac` and `tran`.

## Errors

`op`, `ac` and `tran` throw `pluss::Error` for a non-positive or non-finite `thermal_voltage` or `tolerance`, a `max_iterations` below `1`, or a negative or non-finite `gmin`; and, when the iteration does not converge within `max_iterations`, with a message that gives the iteration count and the last update and says which fallbacks were tried; and when a device current exceeds `max_current`.

## Interpretation and Use

The defaults suit silicon devices at room temperature. Newton converges quadratically near the answer, so tightening `tolerance` adds only an iteration or two; a circuit that still does not converge after the fallbacks usually has no reachable bias. Before loosening `tolerance` raise `max_iterations`. `gmin` zero is allowed; the fallbacks add their own larger values temporarily. Lowering `max_refinement` trades accuracy at switching edges for speed.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A looser tolerance stops sooner.

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
### Example 2

The leakage conductance `gmin` carries the reverse current.

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
  // gmin is a leakage conductance across every junction: it dominates a reverse-biased diode's current
  for (double gmin : {0.0, 1e-12, 1e-9}) {
    ckt::Net net;
    net.nodes = 3;
    net.voltage_sources = {{1}, {0}, vec({-5.0})};
    net.resistors = {{1}, {2}, vec({1000.0})};
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = vec({1e-14});
    ckt::SolveOptions options;
    options.gmin = gmin;
    const auto result = ckt::op(ckt::make(net), options);
    out.writeln("gmin", gmin, " reverse diode current", flat(result.diode_current)[0]);
  }
}
```

Output

```text
gmin 0  reverse diode current -1e-14
gmin 1e-12  reverse diode current -5.00999999499e-12
gmin 1e-09  reverse diode current -5.0000049999950006e-09
```
### Example 3

The thermal voltage moves the junction drop.

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
  out.print_defaults(PrintOptions{}.fixed(5));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  const auto circuit = ckt::make(net);
  // thermal voltage k T / q: 25.85 mV at 300 K, about 34.5 mV at 400 K, 21.5 mV at 250 K
  for (double vt : {0.02154, 0.02585, 0.03447}) {
    ckt::SolveOptions options;
    options.thermal_voltage = vt;
    out.writeln("thermal voltage", vt, " diode voltage", flat(ckt::op(circuit, options).voltage)[2]);
  }
}
```

Output

```text
thermal voltage 0.02154  diode voltage 0.57760
thermal voltage 0.02585  diode voltage 0.69249
thermal voltage 0.03447  diode voltage 0.92153
```
### Example 4

`max_iterations` and the iteration that is needed.

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
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  const auto circuit = ckt::make(net);
  const std::int64_t needed = ckt::op(circuit).iterations.items<std::int64_t>()[0];
  out.writeln("iterations the default solve needs", needed);
  for (std::int64_t limit : {needed, needed - 1, std::int64_t{3}}) {
    ckt::SolveOptions options;
    options.max_iterations = limit;
    try {
      const auto result = ckt::op(circuit, options);
      out.writeln("limit", limit, " converged in", result.iterations.items<std::int64_t>()[0]);
    } catch (const pluss::Error &e) {
      out.writeln("limit", limit, "failed:", e.message());
    }
  }
}
```

Output

```text
iterations the default solve needs 12
limit 12  converged in 12
limit 11  converged in 53
limit 3 failed: eng::circuit::op: the Newton iteration did not converge in 3 iterations (last update 4.59); raise max_iterations, loosen tolerance, or check that the bias is reachable (gmin stepping and source stepping were also tried)
```
### Example 5

A device current that nothing limits.

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
  // an ideal source straight across a diode: nothing limits the current
  ckt::Net net;
  net.nodes = 2;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.diodes.positive = {1};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  const auto circuit = ckt::make(net);
  try {
    (void)ckt::op(circuit);
  } catch (const pluss::Error &e) {
    out.writeln(e.message());
  }
  // a series resistance makes it an ordinary bias point; or accept the model's answer by raising max_current
  ckt::SolveOptions allowed;
  allowed.max_current = 1e80;
  out.writeln("with max_current raised, the diode current", flat(ckt::op(circuit, allowed).diode_current)[0]);
}
```

Output

```text
eng::circuit::op: a device carries 1.01e+70 A, more than max_current (1e+06 A); a junction driven directly by an ideal source has nothing to limit its current, so add a series resistance (or raise max_current)
with max_current raised, the diode current 1.0064707126039922e+70
```
### Example 6

Rejected controls.

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
  <a class="prev-link" href="Scheme.html">
    <span class="label">Previous</span>
    <span class="title">Scheme</span>
  </a>
</div>
<!-- apiref:nav:end -->
