<!-- generated from liquid.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 398559682c44 -->

# `fluid::Network::liquid`

[Back to `fluid::Network`](../Network.md)

The `Liquid` the network was compiled with.

## Signature

```cpp
const fluid::Liquid& fluid::Network::liquid() const noexcept;
```

## What It Does

Returns the liquid properties, density, kinematic viscosity and gravity, that `network` was given; water at 20 degrees Celsius when none was passed. They fix the friction losses and the pressure from head.

## Returns

A reference to the stored `Liquid`.

## Practical Examples

### Example 1

Water at the default properties.

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
  out.writeln("density", obj.liquid().density, " viscosity (mm^2/s)", obj.liquid().kinematic_viscosity * 1e6, " gravity", obj.liquid().gravity);
}
```

Output

```text
density 998.2000  viscosity (mm^2/s) 1.0038  gravity 9.8066
```

### Example 2

The same defaults on a looped network.

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
  out.writeln("density", obj.liquid().density, " viscosity (mm^2/s)", obj.liquid().kinematic_viscosity * 1e6, " gravity", obj.liquid().gravity);
}
```

Output

```text
density 998.2000  viscosity (mm^2/s) 1.0038  gravity 9.8066
```

### Example 3

A heavier, more viscous liquid passed to `network`.

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
  fluid::Liquid oil;
  oil.density = 850.0;
  oil.kinematic_viscosity = 3.0e-5;
  const auto obj = fluid::network(net, oil);
  out.writeln("density", obj.liquid().density, " viscosity (mm^2/s)", obj.liquid().kinematic_viscosity * 1e6, " gravity", obj.liquid().gravity);
}
```

Output

```text
density 850.0000  viscosity (mm^2/s) 30.0000  gravity 9.8066
```

## See Also

- [`dtype`](dtype.md): the precision

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="links.html">
    <span class="label">Previous</span>
    <span class="title">links</span>
  </a>
  <a class="next-link" href="nodes.html">
    <span class="label">Next</span>
    <span class="title">nodes</span>
  </a>
</div>
<!-- apiref:nav:end -->
