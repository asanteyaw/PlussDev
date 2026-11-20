<!-- generated from lines.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9782fcef2062 -->

# `power::Grid::lines`

[Back to `power::Grid`](../Grid.md)

The number of lines.

## Signature

```cpp
std::int64_t power::Grid::lines() const noexcept;
```

## What It Does

Returns the number of branches, lines and transformers, that connect buses. It is the length of the branch axis of flow and loading results and of [`yfrom`](yfrom.md) and [`yto`](yto.md).

## Returns

The number of lines.

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
  out.writeln("lines =", obj.lines(), " buses", obj.buses());
}
```

Output

```text
lines = 1  buses 2
```

### Example 2

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
  out.writeln("lines =", obj.lines(), " buses", obj.buses());
}
```

Output

```text
lines = 3  buses 3
```

### Example 3

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
  out.writeln("lines =", obj.lines(), " buses", obj.buses());
}
```

Output

```text
lines = 1  buses 2
```

## See Also

- [`buses`](buses.md): the bus count
- [`gens`](gens.md): the generator count

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gens.html">
    <span class="label">Previous</span>
    <span class="title">gens</span>
  </a>
  <a class="next-link" href="platform.html">
    <span class="label">Next</span>
    <span class="title">platform</span>
  </a>
</div>
<!-- apiref:nav:end -->
