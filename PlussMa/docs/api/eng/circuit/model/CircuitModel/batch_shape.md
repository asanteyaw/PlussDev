<!-- generated from batch_shape.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bab9125321f9 -->

# `circuit::Circuit::batch_shape`

[Back to `circuit::Circuit`](../CircuitModel.md)

The leading batch axes of the element values.

## Signature

```cpp
const std::vector<std::int64_t>& circuit::Circuit::batch_shape() const noexcept;
```

## What It Does

Returns the shape of the leading axes shared by the element values, empty for a single circuit. A circuit whose resistances have shape `{3, 2}` describes three circuits at once; every solution then carries a leading axis of length three.

## Returns

A reference to the batch shape; empty for a single circuit.

## Practical Examples

### Example 1

A resistor divider: a 10 V source into 1 kΩ and 3 kΩ in series.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace ckt = pluss::subsp::eng::circuit;
using namespace pluss::tils;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({10.0})};
  net.resistors = {{1, 2}, {2, 0}, vec({1000.0, 3000.0})};
  const ckt::Circuit obj = ckt::make(net);
  out.writeln("batch shape =", obj.batch_shape(), " unknowns per circuit", obj.unknowns());
}
```

Output

```text
batch shape = []  unknowns per circuit 3
```

### Example 2

Three divider scenarios in one circuit: the resistances carry a leading axis of length 3.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace ckt = pluss::subsp::eng::circuit;
using namespace pluss::tils;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({10.0})};
  net.resistors = {{1, 2}, {2, 0}, vec({1000, 1000, 1000, 2000, 1000, 4000}).reshape({3, 2})};
  const ckt::Circuit obj = ckt::make(net);
  out.writeln("batch shape =", obj.batch_shape(), " unknowns per circuit", obj.unknowns());
}
```

Output

```text
batch shape = [3]  unknowns per circuit 3
```

### Example 3

A circuit using every linear element: resistor, capacitor, inductor, both sources, both controlled sources and an op amp.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace ckt = pluss::subsp::eng::circuit;
using namespace pluss::tils;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
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
  const ckt::Circuit obj = ckt::make(net);
  out.writeln("batch shape =", obj.batch_shape(), " unknowns per circuit", obj.unknowns());
}
```

Output

```text
batch shape = []  unknowns per circuit 9
```

## See Also

- [`unknowns`](unknowns.md): unknowns of one circuit

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="diodes.html">
    <span class="label">Next</span>
    <span class="title">diodes</span>
  </a>
</div>
<!-- apiref:nav:end -->
