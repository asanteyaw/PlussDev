<!-- generated from Controls.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::Controls`

Four-terminal voltage-controlled elements.

## Signature

```cpp
circuit::Controls
```

## What It Does

A group of voltage-controlled current sources (`Net::vccs`) or voltage sources (`Net::vcvs`). The controlled element runs from `positive` to `negative` and is controlled by the voltage `control_positive - control_negative`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `positive` | The positive output node of each element. | empty |
| `negative` | The negative output node. | empty |
| `control_positive` | The positive control node. | empty |
| `control_negative` | The negative control node. | empty |
| `gain` | Transconductance in siemens for a VCCS, voltage gain for a VCVS; last axis the elements. | undefined |

## Returns

A value; validated by `make`.

## Errors

Mismatched list lengths, nodes outside the circuit or a non-finite gain are rejected by `make`.

## Interpretation and Use

A VCCS draws its current from its positive node. A VCVS adds a branch-current unknown, reported as `vcvs_current`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A VCCS and a VCVS in one circuit.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  ckt::Net net;
  net.nodes = 6;
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  net.inductors = {{2}, {3}, vec({1e-3})};
  net.current_sources = {{0}, {3}, vec({0.001})};
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.vccs = {{4}, {0}, {1}, {0}, vec({0.002})};
  net.vcvs = {{5}, {0}, {1}, {0}, vec({2.0})};
  net.amps = {{4}, {0}, {0}, {3}};

  const ckt::Circuit circuit = ckt::make(net);
  out.writeln("nodes", circuit.nodes(), " unknowns", circuit.unknowns(), " (5 node voltages + voltage source + inductor + VCVS + op amp)");
}
```

Output

```text
nodes 6  unknowns 9  (5 node voltages + voltage source + inductor + VCVS + op amp)
```
### Example 2

A VCVS of gain 5 driving a load.

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
  ckt::Net amplifier;
  amplifier.nodes = 3;
  amplifier.voltage_sources = {{1}, {0}, vec({2.0})};
  amplifier.resistors = {{2}, {0}, vec({1000.0})};
  amplifier.vcvs = {{2}, {0}, {1}, {0}, vec({5.0})};
  const auto voltage_gain = ckt::op(ckt::make(amplifier));
  out.writeln("VCVS, gain 5: node voltages", flat(voltage_gain.voltage), " VCVS current", flat(voltage_gain.vcvs_current));

  ckt::Net transconductor;
  transconductor.nodes = 3;
  transconductor.voltage_sources = {{1}, {0}, vec({1.0})};
  transconductor.resistors = {{2}, {0}, vec({1000.0})};
  transconductor.vccs = {{2}, {0}, {1}, {0}, vec({0.002})};
  const auto transconductance = ckt::op(ckt::make(transconductor));
  out.writeln("VCCS, 2 mS : node voltages", flat(transconductance.voltage), " VCCS current", flat(transconductance.vccs_current));
}
```

Output

```text
VCVS, gain 5: node voltages [0.000000, 2.000000, 10.000000]  VCVS current [-0.010000]
VCCS, 2 mS : node voltages [0.000000, 1.000000, -2.000000]  VCCS current [0.002000]
```
### Example 3

Singular circuits that controlled sources can create.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Branches.html">
    <span class="label">Previous</span>
    <span class="title">Branches</span>
  </a>
  <a class="next-link" href="Amps.html">
    <span class="label">Next</span>
    <span class="title">Amps</span>
  </a>
</div>
<!-- apiref:nav:end -->
