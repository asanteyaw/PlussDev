<!-- generated from diodes.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a896ece8ea5f -->

# `circuit::Circuit::diodes`

[Back to `circuit::Circuit`](../CircuitModel.md)

The number of diodes.

## Signature

```cpp
std::int64_t circuit::Circuit::diodes() const noexcept;
```

## What It Does

Returns how many diodes the circuit contains. A circuit with any diode, transistor or MOSFET is non-linear and its operating point is found by Newton iteration; a purely linear circuit is solved directly.

## Returns

The number of diodes.

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
  out.writeln("diodes =", obj.diodes());
}
```

Output

```text
diodes = 0
```

### Example 2

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
  out.writeln("diodes =", obj.diodes());
}
```

Output

```text
diodes = 1
```

### Example 3

An NPN common-emitter stage: 10 V supply, 1 kΩ collector resistor, 5 V base drive through 100 kΩ.

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
  net.nodes = 5;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({10.0, 5.0})};
  net.resistors = {{1, 3}, {2, 4}, vec({1000.0, 100000.0})};
  net.transistors.collector = {2};
  net.transistors.base = {4};
  net.transistors.emitter = {0};
  net.transistors.polarity = {1};
  net.transistors.saturation = vec({1e-14});
  net.transistors.forward_gain = vec({100.0});
  const ckt::Circuit obj = ckt::make(net);
  out.writeln("diodes =", obj.diodes());
}
```

Output

```text
diodes = 0
```

## See Also

- [`transistors`](transistors.md): the number of transistors
- [`mosfets`](mosfets.md): the number of MOSFETs

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="batch_shape.html">
    <span class="label">Previous</span>
    <span class="title">batch_shape</span>
  </a>
  <a class="next-link" href="dtype.html">
    <span class="label">Next</span>
    <span class="title">dtype</span>
  </a>
</div>
<!-- apiref:nav:end -->
