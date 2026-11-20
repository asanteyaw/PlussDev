<!-- generated from batch_shape.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 404d26bd23cd -->

# `fluid::Network::batch_shape`

[Back to `fluid::Network`](../Network.md)

The leading scenario axes shared by the physical arrays.

## Signature

```cpp
const std::vector<std::int64_t>& fluid::Network::batch_shape() const noexcept;
```

## What It Does

Returns the shape of the leading axes of the physical arrays, empty for one network. Several demand patterns, roughness values or reservoir heads can be solved in one call by giving those arrays a leading axis.

## Returns

A reference to the batch shape; empty for a single network.

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
  out.writeln("batch shape =", obj.batch_shape(), " nodes", obj.nodes());
}
```

Output

```text
batch shape = []  nodes 2
```

### Example 2

Three demand scenarios on the one-pipe network: the demand carries a leading axis of length 3.

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
  net.nodes.demand = vec({0.0, 0.01, 0.0, 0.02, 0.0, 0.04}).reshape({3, 2});
  const auto obj = fluid::network(net);
  out.writeln("batch shape =", obj.batch_shape(), " nodes", obj.nodes());
}
```

Output

```text
batch shape = [3]  nodes 2
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
  out.writeln("batch shape =", obj.batch_shape(), " nodes", obj.nodes());
}
```

Output

```text
batch shape = []  nodes 5
```

## See Also

- [`nodes`](nodes.md): the node count

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="dtype.html">
    <span class="label">Next</span>
    <span class="title">dtype</span>
  </a>
</div>
<!-- apiref:nav:end -->
