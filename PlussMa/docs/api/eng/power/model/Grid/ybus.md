<!-- generated from ybus.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3ccffeb85b1a -->

# `power::Grid::ybus`

[Back to `power::Grid`](../Grid.md)

The complex bus admittance matrix.

## Signature

```cpp
const pluss::Series& power::Grid::ybus() const noexcept;
```

## What It Does

Returns the complex bus admittance matrix `Y`, `buses x buses` (with the batch axes in front), in per unit. The diagonal holds each bus's total admittance, the off-diagonals the negative of the admittance of the lines between buses. `Y` multiplies a vector of bus voltages to give the injected currents; AC power flow is built on it.

## Returns

A reference to the stored complex admittance matrix.

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
  const pluss::Series re = obj.ybus().real(), im = obj.ybus().imag();
  out.writeln("shape", re.sizes());
  out.writeln("real", re.flatten().items<double>());
  out.writeln("imag", im.flatten().items<double>());
}
```

Output

```text
shape [2, 2]
real [0.9901, -0.9901, -0.9901, 0.9901]
imag [-9.9010, 9.9010, 9.9010, -9.9010]
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
  const pluss::Series re = obj.ybus().real(), im = obj.ybus().imag();
  out.writeln("shape", re.sizes());
  out.writeln("real", re.flatten().items<double>());
  out.writeln("imag", im.flatten().items<double>());
}
```

Output

```text
shape [3, 3]
real [1.2292, -0.9901, -0.2391, -0.9901, 1.4851, -0.4950, -0.2391, -0.4950, 0.7342]
imag [-13.8566, 9.9010, 3.9857, 9.9010, -14.8165, 4.9505, 3.9857, 4.9505, -8.8911]
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
  const pluss::Series re = obj.ybus().real(), im = obj.ybus().imag();
  out.writeln("shape", re.sizes());
  out.writeln("real", re.flatten().items<double>());
  out.writeln("imag", im.flatten().items<double>());
}
```

Output

```text
shape [3, 2, 2]
real [0.9901, -0.9901, -0.9901, 0.9901, 0.9901, -0.9901, -0.9901, 0.9901, 0.9901, -0.9901, -0.9901, 0.9901]
imag [-9.9010, 9.9010, 9.9010, -9.9010, -9.9010, 9.9010, 9.9010, -9.9010, -9.9010, 9.9010, 9.9010, -9.9010]
```

## See Also

- [`yfrom`](yfrom.md): from-end line admittances
- [`yto`](yto.md): to-end line admittances

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="platform.html">
    <span class="label">Previous</span>
    <span class="title">platform</span>
  </a>
  <a class="next-link" href="yfrom.html">
    <span class="label">Next</span>
    <span class="title">yfrom</span>
  </a>
</div>
<!-- apiref:nav:end -->
