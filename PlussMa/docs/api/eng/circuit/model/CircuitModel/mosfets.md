<!-- generated from mosfets.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 98c8599bdbae -->

# `circuit::Circuit::mosfets`

[Back to `circuit::Circuit`](../CircuitModel.md)

The number of MOSFETs.

## Signature

```cpp
std::int64_t circuit::Circuit::mosfets() const noexcept;
```

## What It Does

Returns how many MOSFETs the circuit contains. Any MOSFET makes the operating point a Newton solve.

## Returns

The number of MOSFETs.

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
  out.writeln("mosfets =", obj.mosfets());
}
```

Output

```text
mosfets = 0
```

### Example 2

An enhancement MOSFET with a 1 kΩ source resistor, gate at 5 V and drain at 10 V.

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
  net.nodes = 4;
  net.voltage_sources = {{1, 2}, {0, 0}, vec({10.0, 5.0})};
  net.resistors = {{3}, {0}, vec({1000.0})};
  net.mosfets.drain = {1};
  net.mosfets.gate = {2};
  net.mosfets.source = {3};
  net.mosfets.polarity = {1};
  net.mosfets.bulk = {0};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  const ckt::Circuit obj = ckt::make(net);
  out.writeln("mosfets =", obj.mosfets());
}
```

Output

```text
mosfets = 1
```

### Example 3

A diode in series with a 1 kΩ resistor across a 5 V source.

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
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  const ckt::Circuit obj = ckt::make(net);
  out.writeln("mosfets =", obj.mosfets());
}
```

Output

```text
mosfets = 0
```

## See Also

- [`diodes`](diodes.md): the number of diodes
- [`transistors`](transistors.md): the number of transistors

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dtype.html">
    <span class="label">Previous</span>
    <span class="title">dtype</span>
  </a>
  <a class="next-link" href="nodes.html">
    <span class="label">Next</span>
    <span class="title">nodes</span>
  </a>
</div>
<!-- apiref:nav:end -->
