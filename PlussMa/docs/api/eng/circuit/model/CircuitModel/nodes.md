<!-- generated from nodes.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d1b141a070b6 -->

# `circuit::Circuit::nodes`

[Back to `circuit::Circuit`](../CircuitModel.md)

The number of nodes, ground included.

## Signature

```cpp
std::int64_t circuit::Circuit::nodes() const noexcept;
```

## What It Does

Returns the node count of the netlist, counting the ground node `0`. Node numbers in the element lists run from `0` to `nodes() - 1`. The nodal equations have one unknown fewer, because the ground voltage is fixed: see [`unknowns`](unknowns.md).

## Returns

The number of nodes, at least `1`.

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
  out.writeln("nodes =", obj.nodes(), " unknowns", obj.unknowns());
}
```

Output

```text
nodes = 3  unknowns 3
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
  out.writeln("nodes =", obj.nodes(), " unknowns", obj.unknowns());
}
```

Output

```text
nodes = 6  unknowns 9
```

### Example 3

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
  out.writeln("nodes =", obj.nodes(), " unknowns", obj.unknowns());
}
```

Output

```text
nodes = 3  unknowns 3
```

## See Also

- [`unknowns`](unknowns.md): the size of the equation system

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mosfets.html">
    <span class="label">Previous</span>
    <span class="title">mosfets</span>
  </a>
  <a class="next-link" href="platform.html">
    <span class="label">Next</span>
    <span class="title">platform</span>
  </a>
</div>
<!-- apiref:nav:end -->
