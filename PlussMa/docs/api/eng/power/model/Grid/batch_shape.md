<!-- generated from batch_shape.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3f8f1fd1369a -->

# `power::Grid::batch_shape`

[Back to `power::Grid`](../Grid.md)

The leading scenario axes shared by all values.

## Signature

```cpp
const std::vector<std::int64_t>& power::Grid::batch_shape() const noexcept;
```

## What It Does

Returns the shape of the leading axes of the stored values, empty for one grid. Load or generation scenarios are solved in one call by giving the bus powers a leading axis.

## Returns

A reference to the batch shape; empty for a single grid.

## Practical Examples

### Example 1

Two buses joined by one line: a slack bus feeding a load bus.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace power = pluss::subsp::eng::power;
using namespace pluss::tils;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.5});
  net.buses.q = vec({0.0, -0.2});
  net.buses.vm = vec({1.0, 1.0});
  net.buses.va = vec({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = vec({0.01});
  net.lines.x = vec({0.1});
  const power::Grid obj = power::make(net);
  out.writeln("batch shape =", obj.batch_shape(), " buses", obj.buses());
}
```

Output

```text
batch shape = []  buses 2
```

### Example 2

Three load scenarios on the two-bus system: the bus powers carry a leading axis of length 3.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace power = pluss::subsp::eng::power;
using namespace pluss::tils;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.5});
  net.buses.q = vec({0.0, -0.2});
  net.buses.vm = vec({1.0, 1.0});
  net.buses.va = vec({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = vec({0.01});
  net.lines.x = vec({0.1});
  net.buses.p = vec({0.0, -0.3, 0.0, -0.5, 0.0, -0.7}).reshape({3, 2});
  net.buses.q = vec({0.0, -0.1, 0.0, -0.2, 0.0, -0.3}).reshape({3, 2});
  const power::Grid obj = power::make(net);
  out.writeln("batch shape =", obj.batch_shape(), " buses", obj.buses());
}
```

Output

```text
batch shape = [3]  buses 2
```

### Example 3

Three buses, one generator at bus 1 and three lines with line charging, on a 100 MVA, 230 kV base.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace power = pluss::subsp::eng::power;
using namespace pluss::tils;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.2, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.2});
  net.buses.vm = vec({1.04, 1.02, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {1};
  net.gens.p = vec({0.6});
  net.gens.q = vec({0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.015});
  net.lines.x = vec({0.1, 0.2, 0.25});
  net.lines.charge = vec({0.02, 0.05, 0.04});
  const power::Grid obj = power::make(net);
  out.writeln("batch shape =", obj.batch_shape(), " buses", obj.buses());
}
```

Output

```text
batch shape = []  buses 3
```

## See Also

- [`buses`](buses.md): the bus count

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="base_power.html">
    <span class="label">Previous</span>
    <span class="title">base_power</span>
  </a>
  <a class="next-link" href="buses.html">
    <span class="label">Next</span>
    <span class="title">buses</span>
  </a>
</div>
<!-- apiref:nav:end -->
