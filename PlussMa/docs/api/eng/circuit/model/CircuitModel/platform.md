<!-- generated from platform.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8e63498e974a -->

# `circuit::Circuit::platform`

[Back to `circuit::Circuit`](../CircuitModel.md)

The device the stored element values live on.

## Signature

```cpp
pluss::Platform circuit::Circuit::platform() const noexcept;
```

## What It Does

Returns the platform of the compiled data, the CPU for ordinary arrays. Solving happens there.

## Returns

The `Platform` of the circuit.

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
  out.writeln("on the CPU =", obj.platform() == pluss::Platform::CPU);
}
```

Output

```text
on the CPU = true
```

### Example 2

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
  out.writeln("on the CPU =", obj.platform() == pluss::Platform::CPU);
}
```

Output

```text
on the CPU = true
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
  out.writeln("on the CPU =", obj.platform() == pluss::Platform::CPU);
}
```

Output

```text
on the CPU = true
```

## See Also

- [`dtype`](dtype.md): the floating dtype

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nodes.html">
    <span class="label">Previous</span>
    <span class="title">nodes</span>
  </a>
  <a class="next-link" href="transistors.html">
    <span class="label">Next</span>
    <span class="title">transistors</span>
  </a>
</div>
<!-- apiref:nav:end -->
