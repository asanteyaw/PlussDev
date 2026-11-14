<!-- generated from Net.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::Net`

The complete description of a circuit.

## Signature

```cpp
circuit::Net
```

## What It Does

The input of `make`: the number of nodes and a group for every kind of element.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `nodes` | The number of nodes including the ground, node `0`. | required |
| `resistors, capacitors, inductors` | Passive `Branches`. | empty |
| `current_sources, voltage_sources` | Independent source `Branches`. | empty |
| `vccs, vcvs` | Controlled sources, `Controls`. | empty |
| `amps` | Ideal `Amps`. | empty |
| `diodes` | Junction `Diodes`. | empty |
| `transistors` | Bipolar `Transistors`. | empty |
| `mosfets` | Enhancement `Mosfets`. | empty |

## Returns

A value; validated by `make`.

## Errors

Every inconsistency is reported by `make`.

## Interpretation and Use

Describe a circuit once and compile it; sweep component values through leading axes of the `value` arrays.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A divider.

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

Rejected descriptions.

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
  <a class="prev-link" href="Mosfets.html">
    <span class="label">Previous</span>
    <span class="title">Mosfets</span>
  </a>
  <a class="next-link" href="CircuitModel.html">
    <span class="label">Next</span>
    <span class="title">Circuit</span>
  </a>
</div>
<!-- apiref:nav:end -->
