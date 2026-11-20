<!-- generated from dtype.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 74f6abf28d49 -->

# `circuit::Circuit::dtype`

[Back to `circuit::Circuit`](../CircuitModel.md)

The floating dtype of the stored element values.

## Signature

```cpp
pluss::ScalarType circuit::Circuit::dtype() const noexcept;
```

## What It Does

Returns the element type the circuit was compiled in, taken from the element values: `Float64` for double-precision data. Solutions are returned in this type.

## Returns

The `ScalarType` of the stored values.

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
  out.writeln("dtype =", dtype_of(obj.dtype()).name);
}
```

Output

```text
dtype = float64
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
  out.writeln("dtype =", dtype_of(obj.dtype()).name);
}
```

Output

```text
dtype = float64
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
  out.writeln("dtype =", dtype_of(obj.dtype()).name);
}
```

Output

```text
dtype = float64
```

## See Also

- [`platform`](platform.md): the device

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diodes.html">
    <span class="label">Previous</span>
    <span class="title">diodes</span>
  </a>
  <a class="next-link" href="mosfets.html">
    <span class="label">Next</span>
    <span class="title">mosfets</span>
  </a>
</div>
<!-- apiref:nav:end -->
