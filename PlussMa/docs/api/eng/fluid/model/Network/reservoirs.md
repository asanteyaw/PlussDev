<!-- generated from reservoirs.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: df1d04444ba6 -->

# `fluid::Network::reservoirs`

[Back to `fluid::Network`](../Network.md)

The number of reservoirs.

## Signature

```cpp
std::int64_t fluid::Network::reservoirs() const noexcept;
```

## What It Does

Returns how many nodes are reservoirs: fixed-head sources and sinks. A network needs at least one fixed-head node (a reservoir or a tank) to have a defined solution.

## Returns

The number of reservoirs.

## Practical Examples

### Example 1

One pipe from a reservoir to a demand junction.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fluid = pluss::subsp::eng::fluid;
using namespace pluss::tils;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, 0.02});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.30});
  net.pipes.roughness = vec({1.0e-4});
  net.pipes.minor = vec({0.0});
  const auto obj = fluid::network(net);
  out.writeln("reservoirs =", obj.reservoirs(), " nodes", obj.nodes());
}
```

Output

```text
reservoirs = 1  nodes 2
```

### Example 2

A pump lifting water from one reservoir to a junction that drains to a second reservoir.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fluid = pluss::subsp::eng::fluid;
using namespace pluss::tils;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Reservoir};
  net.nodes.elevation = vec({0.0, 5.0, 0.0});
  net.nodes.demand = vec({0.0, 0.0, 0.0});
  net.nodes.head = vec({10.0, 40.0, 40.0});
  net.pumps.from = {0};
  net.pumps.to = {1};
  net.pumps.shutoff_head = vec({60.0});
  net.pumps.resistance = vec({4000.0});
  net.pumps.exponent = vec({2.0});
  net.pumps.speed = vec({1.0});
  net.pipes.from = {1};
  net.pipes.to = {2};
  net.pipes.length = vec({500.0});
  net.pipes.diameter = vec({0.20});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  const auto obj = fluid::network(net);
  out.writeln("reservoirs =", obj.reservoirs(), " nodes", obj.nodes());
}
```

Output

```text
reservoirs = 2  nodes 3
```

### Example 3

A reservoir feeding a demand junction and a storage tank.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fluid = pluss::subsp::eng::fluid;
using namespace pluss::tils;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Tank};
  net.nodes.elevation = vec({0.0, 10.0, 20.0});
  net.nodes.demand = vec({0.0, 0.04, 0.0});
  net.nodes.head = vec({60.0, 50.0, 23.0});
  net.tanks.area = vec({400.0});
  net.tanks.min_level = vec({0.5});
  net.tanks.max_level = vec({6.0});
  net.tanks.overflow = {false};
  net.pipes.from = {0, 1};
  net.pipes.to = {1, 2};
  net.pipes.length = vec({800.0, 400.0});
  net.pipes.diameter = vec({0.15, 0.20});
  net.pipes.roughness = vec({1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0});
  const auto obj = fluid::network(net);
  out.writeln("reservoirs =", obj.reservoirs(), " nodes", obj.nodes());
}
```

Output

```text
reservoirs = 1  nodes 3
```

## See Also

- [`junctions`](junctions.md): free-head nodes
- [`tanks`](tanks.md): storage nodes

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pumps.html">
    <span class="label">Previous</span>
    <span class="title">pumps</span>
  </a>
  <a class="next-link" href="tanks.html">
    <span class="label">Next</span>
    <span class="title">tanks</span>
  </a>
</div>
<!-- apiref:nav:end -->
