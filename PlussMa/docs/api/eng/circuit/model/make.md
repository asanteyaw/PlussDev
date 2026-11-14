<!-- generated from make.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::make`

Validates a circuit description and compiles it.

## Signature

```cpp
circuit::Circuit circuit::make(const circuit::Net& net);
```

## What It Does

Checks the topology and values of a `Net` and compiles the incidence matrices that `op` and `ac` solve. All non-empty value groups must share a dtype and platform and compatible leading batch axes.

## Parameters

`net`
: The description of the circuit.

## Returns

An immutable `circuit::Circuit`.

## Errors

Throws `pluss::Error` for fewer than two nodes, a net with no elements, a node outside the circuit, an element from a node to itself, non-positive resistance or inductance, negative capacitance, mismatched list lengths, a diode or transistor with a terminal outside the circuit, two terminals on the same node, a polarity other than `1` or `-1`, a non-positive device value, or values that differ in dtype, platform or batch axes.

## Interpretation and Use

Validation is done here once; the analyses do not repeat it. Singular circuits (a floating node, contradicting ideal sources) are accepted here and reported by the analyses.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A divider compiled and solved.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({10.0})};
  net.resistors = {{1, 2}, {2, 0}, vec({1000.0, 3000.0})};

  const ckt::Circuit circuit = ckt::make(net);
  out.writeln("nodes", circuit.nodes(), " unknowns", circuit.unknowns());
  out.writeln("element type", dtype_of(circuit.dtype()).name, " batch shape", circuit.batch_shape());
}
```

Output

```text
nodes 3  unknowns 3
element type float64  batch shape []
```
### Example 2

Every element kind.

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
### Example 3

Every validation error.

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
  report([&] { ckt::Net net; net.nodes = 1; (void)ckt::make(net); });
  report([&] { ckt::Net net; net.nodes = 3; (void)ckt::make(net); });
  report([&] { ckt::Net net; net.nodes = 3; net.resistors = {{1}, {1}, vec({1.0})}; (void)ckt::make(net); });
  report([&] { ckt::Net net; net.nodes = 3; net.resistors = {{1}, {5}, vec({1.0})}; (void)ckt::make(net); });
  report([&] { ckt::Net net; net.nodes = 3; net.resistors = {{1}, {0}, vec({-1.0})}; (void)ckt::make(net); });
  report([&] { ckt::Net net; net.nodes = 3; net.resistors = {{1, 2}, {0, 0}, vec({1.0})}; (void)ckt::make(net); });
  report([&] { ckt::Net net; net.nodes = 3; net.resistors = {{1}, {0, 2}, vec({1.0})}; (void)ckt::make(net); });
  report([&] {
    ckt::Net net;
    net.nodes = 3;
    net.resistors = {{1}, {0}, vec({1.0})};
    net.capacitors = {{2}, {0}, pluss::series::make(std::vector<float>{1.0f})};
    (void)ckt::make(net);
  });
}
```

Output

```text
eng::circuit::make: nodes must include ground and at least one unknown node
eng::circuit::make: at least one non-empty element group is required
eng::circuit::make: resistors cannot connect a node to itself
eng::circuit::make: resistors contains a node outside [0, nodes)
eng::circuit::make: resistors values must be positive
eng::circuit::make: resistors values must be real floating [..., elements]
eng::circuit::make: resistors endpoint arrays must have equal length
eng::circuit::make: every value group must share dtype and platform
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="CircuitModel.html">
    <span class="label">Previous</span>
    <span class="title">Circuit</span>
  </a>
</div>
<!-- apiref:nav:end -->
