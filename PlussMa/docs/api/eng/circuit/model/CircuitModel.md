<!-- generated from CircuitModel.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::Circuit`

An immutable compiled circuit.

## Signature

```cpp
std::int64_t circuit::Circuit::nodes() const noexcept;
std::int64_t circuit::Circuit::unknowns() const noexcept;
std::int64_t circuit::Circuit::diodes() const noexcept;
std::int64_t circuit::Circuit::transistors() const noexcept;
std::int64_t circuit::Circuit::mosfets() const noexcept;
const std::vector<std::int64_t>& circuit::Circuit::batch_shape() const noexcept;
```

## What It Does

What `circuit::make` returns: the validated topology compiled into incidence matrices, ready to be solved by `op`, `ac` and `tran` any number of times.

## Methods

`nodes()`
: The number of nodes including the ground.

`unknowns()`
: The number of unknowns of the nodal equations: the non-ground node voltages plus a current for every voltage source, inductor, VCVS and op amp.

`diodes(), transistors(), mosfets()`
: The number of diodes, of transistors and of MOSFETs; a circuit with either is solved by Newton iteration.

`batch_shape()`
: The leading batch axes of the element values.

`dtype(), platform()`
: The element type and the device of the stored data.

## Returns

Accessors return counts and the batch shape.

## Errors

Accessors do not fail. Invalid descriptions are rejected by `make`.

## Changelog

- **0.1** — Initial public contract.

## Interpretation and Use

A `Circuit` cannot be default-constructed; build it with `make`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The counts of a divider.

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

The batch shape of a sweep.

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
### Example 3

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Net.html">
    <span class="label">Previous</span>
    <span class="title">Net</span>
  </a>
  <a class="next-link" href="make.html">
    <span class="label">Next</span>
    <span class="title">make</span>
  </a>
</div>
<!-- apiref:nav:end -->

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
