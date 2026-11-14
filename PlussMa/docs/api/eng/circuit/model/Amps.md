<!-- generated from Amps.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::Amps`

Ideal operational amplifiers.

## Signature

```cpp
circuit::Amps
```

## What It Does

An ideal op amp has infinite gain: its output and reference nodes supply whatever current is needed to make its two inputs equal.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `output` | The output node of each amplifier. | empty |
| `reference` | The node the output current returns to. | empty |
| `positive_input` | The non-inverting input node. | empty |
| `negative_input` | The inverting input node. | empty |

## Returns

A value; validated by `make`.

## Errors

Mismatched list lengths or nodes outside the circuit are rejected by `make`.

## Interpretation and Use

The amplifier carries no input current and enforces `V(positive_input) = V(negative_input)`; its supplied current is `amp_current`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

An inverting amplifier of gain -5.

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
  net.nodes = 4;
  net.voltage_sources = {{1}, {0}, vec({0.5})};
  net.resistors = {{1, 2}, {2, 3}, vec({1000.0, 10000.0})};
  net.amps = {{3}, {0}, {0}, {2}};   // output 3, reference 0, positive input 0, negative input 2
  const auto result = ckt::op(ckt::make(net));
  out.writeln("node voltages", flat(result.voltage), " (node 2 is the virtual ground; node 3 the output)");
  out.writeln("op-amp current", flat(result.amp_current));
}
```

Output

```text
node voltages [0.000000, 0.500000, -0.000000, -5.000000]  (node 2 is the virtual ground; node 3 the output)
op-amp current [0.000500]
```
### Example 2

An op amp in all-elements circuit.

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

An integrator at AC.

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
  net.nodes = 4;
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {3}, vec({1e-6})};
  net.amps = {{3}, {0}, {0}, {2}};
  const auto result = ckt::ac(ckt::make(net), vec({100.0, 1000.0, 10000.0}));
  const pluss::Series output = result.voltage.select(1, 3);
  out.writeln("gain       ", flat(output.abs()), " expected 10, 1, 0.1");
  out.writeln("phase (deg)", flat(output.angle() * (180.0 / M_PI)));
}
```

Output

```text
gain        [10.000000, 1.000000, 0.100000]  expected 10, 1, 0.1
phase (deg) [90.000000, 90.000000, 90.000000]
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Controls.html">
    <span class="label">Previous</span>
    <span class="title">Controls</span>
  </a>
  <a class="next-link" href="Diodes.html">
    <span class="label">Next</span>
    <span class="title">Diodes</span>
  </a>
</div>
<!-- apiref:nav:end -->
