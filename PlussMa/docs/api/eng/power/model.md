<!-- generated from model.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Network model

This branch describes a power network. A `Net` collects a per-unit `Base`, the `Buses`, the generators `Gens` and the `Lines`; `make` validates it and compiles an immutable `Grid` that every analysis takes. `pu` and `si` convert values to and from per unit.

## Choosing A Surface

Write every quantity in per unit on one `Base`. Use the three bus kinds to say which pair of quantities each bus fixes. Use leading axes on any value to describe a batch of scenarios, and `active` flags to switch lines and generators.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

Per-unit conversion on a 100 MVA, 230 kV base.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Base base{100e6, 230e3};   // 100 MVA, 230 kV line to line
  out.writeln("impedance base (ohm): ", power::zbase(base));
  out.writeln("admittance base (S):  ", power::ybase(base));
  out.writeln("current base (A):     ", power::ibase(base));
  out.writeln("50 MW in per unit: ", flat(power::pu(vec({50e6}), base.power)), "   0.5 per unit in watts: ", flat(power::si(vec({0.5}), base.power)));
  out.writeln("52.9 ohm in per unit: ", flat(power::pu(vec({52.9}), power::zbase(base))));
}
```

Output

```text
impedance base (ohm): 529.00000
admittance base (S):  0.00189
current base (A):     251.02186
50 MW in per unit: [0.50000]   0.5 per unit in watts: [50000000.00000]
52.9 ohm in per unit: [0.10000]
```
### Example 2

Compile a two-bus network.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.5});       // the load bus draws 0.5 per unit of active power
  net.buses.q = vec({0.0, -0.2});       // and 0.2 of reactive power
  net.buses.vm = vec({1.0, 1.0});       // set point at the slack, starting guess at the load bus
  net.buses.va = vec({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = vec({0.01});
  net.lines.x = vec({0.1});

  const power::Grid grid = power::make(net);
  out.writeln("buses ", grid.buses(), ", generators ", grid.gens(), ", lines ", grid.lines(), ", base power (W) ", grid.base_power());
  const auto re = flat(grid.ybus().real()), im = flat(grid.ybus().imag());
  out.writeln("Y[0][0] from the grid: ", re[0], " ", im[0], "j   Y[0][1]: ", re[1], " ", im[1], "j");
  const std::complex<double> y = 1.0 / std::complex<double>(0.01, 0.1);
  out.writeln("1 / (r + jx) by hand:  ", y.real(), " ", y.imag(), "j");
}
```

Output

```text
buses 2, generators 0, lines 1, base power (W) 100000000.000000
Y[0][0] from the grid: 0.990099 -9.900990j   Y[0][1]: -0.990099 9.900990j
1 / (r + jx) by hand:  0.990099 -9.900990j
```
### Example 3

Generators and voltage-controlled buses.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.2, -0.5});      // the load at bus 1 is part of the bus; its generator is listed separately
  net.buses.q = vec({0.0, 0.0, -0.2});
  net.buses.vm = vec({1.04, 1.02, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {1};
  net.gens.p = vec({0.6});
  net.gens.q = vec({0.0});
  net.gens.active = {true};
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.015});
  net.lines.x = vec({0.1, 0.2, 0.25});
  net.lines.charge = vec({0.02, 0.05, 0.04});

  const auto r = power::ac(power::make(net));
  out.writeln("voltage magnitudes: ", flat(r.magnitude), "   angles (rad): ", flat(r.angle));
  out.writeln("net active injections: ", flat(r.injection.real()), "   (bus 1: -0.2 load + 0.6 generator = 0.4)");
  out.writeln("net reactive injections: ", flat(r.injection.imag()));
  out.writeln("converged ", r.converged.item<bool>(), " in ", r.iterations, " iterations");
}
```

Output

```text
voltage magnitudes: [1.04000, 1.02000, 1.00575]   angles (rad): [0.00000, 0.01136, -0.04632]
net active injections: [0.10314, 0.40000, -0.50000]   (bus 1: -0.2 load + 0.6 generator = 0.4)
net reactive injections: [0.32097, -0.19852, -0.20000]
converged true in 3 iterations
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

model/BusKind
model/Base
model/Buses
model/Gens
model/Lines
model/Net
model/Grid
model/make
model/zbase
model/ybase
model/ibase
model/pu
model/si
```
<!-- apiref:toctree:end -->
