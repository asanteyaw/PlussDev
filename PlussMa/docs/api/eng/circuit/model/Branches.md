<!-- generated from Branches.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::Branches`

Two-terminal elements of one kind: their nodes and values.

## Signature

```cpp
circuit::Branches
```

## What It Does

A group of two-terminal elements, a resistor group or a source group. Element `i` joins node `positive[i]` to node `negative[i]` and has value `value[..., i]`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `positive` | The positive node of each element. Node `0` is the ground. | empty |
| `negative` | The negative node of each element. | empty |
| `value` | The element values, last axis the elements; leading axes are a batch of circuits with the same topology. Resistance in ohms, capacitance in farads, inductance in henries, source current in amperes, source voltage in volts. | undefined |

## Returns

A value; validated by `make`.

## Errors

Lists of different lengths, a node outside `[0, nodes)`, an element from a node to itself, or a non-positive resistance or inductance is rejected by `make`.

## Interpretation and Use

The orientation is the same for every group: positive to negative. A voltage source fixes `V(positive) - V(negative)`; a current source delivers into its negative node.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A divider built from two resistor branches and a voltage source.

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

One of every element kind.

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

A batched resistor value.

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
  net.resistors = {{1, 2}, {2, 0}, vec({1000, 1000, 1000, 2000, 1000, 4000}).reshape({3, 2})};

  const ckt::Circuit circuit = ckt::make(net);
  out.writeln("batch shape", circuit.batch_shape(), " unknowns per circuit", circuit.unknowns());
}
```

Output

```text
batch shape [3]  unknowns per circuit 3
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Controls.html">
    <span class="label">Next</span>
    <span class="title">Controls</span>
  </a>
</div>
<!-- apiref:nav:end -->
