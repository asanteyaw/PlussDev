<!-- generated from Result.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::Result`

Voltages and currents of an operating point, an AC response or a transient run.

## Signature

```cpp
circuit::Result
```

## What It Does

The result of `op`, `ac` and `tran`. For `ac` the arrays are complex phasors and gain a frequency axis immediately before the final element axis: `[..., frequency, nodes]`. For `tran` they are real and gain a time axis in the same place: `[..., time, nodes]`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `voltage` | Node voltages, `[..., nodes]`; the entry for node `0` is zero. |  |
| `source_current` | Current through each voltage source, positive to negative; a delivering source has a negative current. |  |
| `inductor_current` | Current through each inductor, positive to negative. |  |
| `vccs_current` | Current each VCCS carries, positive to negative. |  |
| `vcvs_current` | Current through each VCVS, positive to negative. |  |
| `amp_current` | Current each op amp supplies from its output node to its reference node. |  |
| `diode_current` | Current through each diode from anode to cathode, `[..., diodes]`. |  |
| `collector_current` | Current into the collector terminal of each transistor, `[..., transistors]`. |  |
| `base_current` | Current into the base terminal of each transistor, `[..., transistors]`; the emitter current is `-(collector + base)`. |  |
| `drain_current` | Current into the drain terminal of each MOSFET, `[..., mosfets]`. |  |
| `iterations` | The Newton iterations used, as an `Int64` vector: `[1]` for `op`, `[time]` for `tran`; zero for a circuit without devices. |  |
| `refinements` | The sub-steps a `tran` sample needed where its step was subdivided, an `Int64` vector `[time]`; zero elsewhere and for circuits without devices. |  |
| `residual` | The largest error of the solved equations, per circuit (per frequency for `ac`, per time sample for `tran`). |  |

## Returns

Returned by `op`, `ac` and `tran`.

## Errors

Reading a field does not solve again.

## Interpretation and Use

A group with no elements gives an axis of length zero. Read `residual` as a check on the solution.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Every field of a divider.

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

Phasors of an RC low-pass.

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

Power balance from the result.

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
  const std::vector<double> v = flat(result.voltage);

  const double dissipated = (v[1] - v[2]) * (v[1] - v[2]) / 1000.0 + v[2] * v[2] / 3000.0;
  const double delivered = -10.0 * flat(result.source_current)[0];
  out.writeln("power dissipated in the resistors", dissipated);
  out.writeln("power delivered by the source    ", delivered);
}
```

Output

```text
power dissipated in the resistors 0.025000
power delivered by the source     0.025000
```
### Example 4

The shape of every field of a transient result.

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
  // node 1 source, node 2 an RC low-pass, node 3 a VCVS of gain 2 with an inductor, nodes 4 and 5 an op-amp inverter with a transconductance draw
  ckt::Net net;
  net.nodes = 6;
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1, 3, 4, 5}, {2, 4, 5, 0}, vec({1000.0, 1000.0, 1000.0, 2000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  net.inductors = {{3}, {0}, vec({0.5})};
  net.vcvs = {{3}, {0}, {2}, {0}, vec({2.0})};
  net.vccs = {{5}, {0}, {2}, {0}, vec({1e-3})};
  net.amps = {{5}, {0}, {0}, {4}};
  const auto run = ckt::tran(ckt::make(net), vec(grid(1e-4, 11)));
  out.writeln("voltage         ", run.voltage.sizes());
  out.writeln("source_current  ", run.source_current.sizes());
  out.writeln("inductor_current", run.inductor_current.sizes());
  out.writeln("vccs_current    ", run.vccs_current.sizes());
  out.writeln("vcvs_current    ", run.vcvs_current.sizes());
  out.writeln("amp_current     ", run.amp_current.sizes());
  out.writeln("residual        ", run.residual.sizes());
}
```

Output

```text
voltage          [11, 6]
source_current   [11, 1]
inductor_current [11, 1]
vccs_current     [11, 1]
vcvs_current     [11, 1]
amp_current      [11, 1]
residual         [11]
```
### Example 5

The device fields of a diode circuit.

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
### Example 6

The device fields of a transistor stage.

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
### Example 7

The device fields of a MOSFET stage.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net common_source(double vdd, double vgs, double rd, double lambda = 0.0, std::int64_t polarity = 1) {
  // nodes: 1 supply, 2 drain, 3 gate drive; source grounded
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({vdd, vgs})};
  net.resistors = {{1}, {2}, vec({rd})};
  net.mosfets.drain = {2};
  net.mosfets.gate = {3};
  net.mosfets.source = {0};
  net.mosfets.polarity = {polarity};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  if (lambda > 0.0) net.mosfets.channel_modulation = vec({lambda});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // kp = 2 mA/V^2, Vth = 1 V, Rd = 500 ohm: at vgs = 3 V the overdrive is 2 V and the saturation current kp vov^2 / 2 = 4 mA
  const auto circuit = ckt::make(common_source(10.0, 3.0, 500.0));
  const auto result = ckt::op(circuit);
  out.writeln("MOSFETs in the circuit", circuit.mosfets());
  out.writeln("drain voltage", flat(result.voltage)[2], " drain current", flat(result.drain_current)[0]);
  out.writeln("saturated: the drain is above the overdrive", flat(result.voltage)[2] > 2.0);
  out.writeln("residual below 1e-9", result.residual.item<double>() < 1e-9);
}
```

Output

```text
MOSFETs in the circuit 1
drain voltage 8.000000  drain current 0.004000
saturated: the drain is above the overdrive true
residual below 1e-9 true
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="op.html">
    <span class="label">Next</span>
    <span class="title">op</span>
  </a>
</div>
<!-- apiref:nav:end -->
