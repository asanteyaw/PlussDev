<!-- generated from tanks.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 23ba968c6b46 -->

# `fluid::Network::tanks`

[Back to `fluid::Network`](../Network.md)

The number of tanks.

## Signature

```cpp
std::int64_t fluid::Network::tanks() const noexcept;
```

## What It Does

Returns how many nodes are storage tanks, whose level evolves in a dynamic `run` and is held fixed in a steady solve.

## Returns

The number of tanks.

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
  out.writeln("tanks =", obj.tanks(), " nodes", obj.nodes());
}
```

Output

```text
tanks = 0  nodes 2
```

### Example 2

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
  out.writeln("tanks =", obj.tanks(), " nodes", obj.nodes());
}
```

Output

```text
tanks = 1  nodes 3
```

### Example 3

A looped network: a reservoir feeding four junctions through six pipes.

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
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction, fluid::NodeKind::Junction, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 10.0, 12.0, 8.0, 15.0});
  net.nodes.demand = vec({0.0, 0.010, 0.015, 0.012, 0.008});
  net.nodes.head = vec({60.0, 55.0, 55.0, 55.0, 55.0});
  net.pipes.from = {0, 1, 1, 2, 3, 2};
  net.pipes.to = {1, 2, 3, 4, 4, 3};
  net.pipes.length = vec({800.0, 500.0, 600.0, 450.0, 550.0, 300.0});
  net.pipes.diameter = vec({0.30, 0.20, 0.20, 0.15, 0.15, 0.10});
  net.pipes.roughness = vec({1e-4, 1e-4, 1e-4, 1e-4, 1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  const auto obj = fluid::network(net);
  out.writeln("tanks =", obj.tanks(), " nodes", obj.nodes());
}
```

Output

```text
tanks = 0  nodes 5
```

## See Also

- [`reservoirs`](reservoirs.md): fixed-head nodes
- [`junctions`](junctions.md): free-head nodes

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reservoirs.html">
    <span class="label">Previous</span>
    <span class="title">reservoirs</span>
  </a>
  <a class="next-link" href="valves.html">
    <span class="label">Next</span>
    <span class="title">valves</span>
  </a>
</div>
<!-- apiref:nav:end -->
