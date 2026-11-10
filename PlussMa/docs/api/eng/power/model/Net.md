<!-- generated from Net.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::Net`

The complete description of a power network.

## Signature

```cpp
power::Net
```

## What It Does

A `Net` is the input of `make`: the per-unit base, the buses, the generators and the lines.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `base` | The per-unit `Base`. | required |
| `buses` | The `Buses`. | required |
| `gens` | The `Gens`. | empty (none) |
| `lines` | The `Lines`. | required |

## Returns

A value; validated by `make`.

## Errors

Every inconsistency in the contained values is reported by `make`.

## Interpretation and Use

Build a `Net` once, compile it with `make`, and vary scenarios through leading axes or `active` flags rather than by rebuilding.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A minimal two-bus network.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

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
### Example 2

A network with a generator.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

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
### Example 3

Batches and outages.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static power::Net three_bus() {
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
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));

  const auto base = power::ac(power::make(three_bus()));
  power::Net outage = three_bus();
  outage.lines.active = {true, true, false};   // line 0 - 2 out of service
  const auto cut = power::ac(power::make(outage));
  out.writeln("all lines in:         load-bus voltage ", flat(base.magnitude)[2], ", slack output (p.u.) ", flat(base.injection.real())[0]);
  out.writeln("line 0-2 out:         load-bus voltage ", flat(cut.magnitude)[2], ", slack output (p.u.) ", flat(cut.injection.real())[0]);

  // three load levels in one call: a leading axis of three on the injections
  power::Net sweep;
  sweep.base = {100e6, 230e3};
  sweep.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  sweep.buses.p = pluss::series::make(std::vector<double>{0, -0.2, 0, -0.5, 0, -1.0}).reshape({3, 2});
  sweep.buses.q = pluss::series::make(std::vector<double>{0, -0.08, 0, -0.2, 0, -0.4}).reshape({3, 2});
  sweep.buses.vm = vec({1.0, 1.0});
  sweep.buses.va = vec({0.0, 0.0});
  sweep.lines.from = {0};
  sweep.lines.to = {1};
  sweep.lines.r = vec({0.01});
  sweep.lines.x = vec({0.1});
  const power::Grid grid = power::make(sweep);
  const auto r = power::ac(grid);
  out.writeln("batch shape of the grid: ", grid.batch_shape(), "; magnitude shape ", r.magnitude.sizes());
  out.writeln("load-bus voltage at 0.2, 0.5 and 1.0 per unit of load: ", flat(r.magnitude.select(-1, 1)));
}
```

Output

```text
all lines in:         load-bus voltage 1.00575, slack output (p.u.) 0.10314
line 0-2 out:         load-bus voltage 0.96833, slack output (p.u.) 0.10646
batch shape of the grid: [3]; magnitude shape [3, 2]
load-bus voltage at 0.2, 0.5 and 1.0 per unit of load: [0.98971, 0.97309, 0.94169]
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Lines.html">
    <span class="label">Previous</span>
    <span class="title">Lines</span>
  </a>
  <a class="next-link" href="Grid.html">
    <span class="label">Next</span>
    <span class="title">Grid</span>
  </a>
</div>
<!-- apiref:nav:end -->
