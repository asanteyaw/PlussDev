<!-- generated from Buses.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::Buses`

The buses of a network: kind, fixed injections, voltage set points and shunts.

## Signature

```cpp
power::Buses
```

## What It Does

The bus data of a network. Every `Series` has the buses on its last axis, and leading axes describe a batch of scenarios.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `kind` | The `BusKind` of each bus. | required |
| `p` | Fixed active injection, per unit; loads are negative. | required |
| `q` | Fixed reactive injection, per unit; loads are negative. | required |
| `vm` | Voltage magnitude, per unit: the set point of slack and `PV` buses and the starting guess of `PQ` buses. | required |
| `va` | Voltage angle, radians: the value at the slack bus and the starting guess elsewhere. | required |
| `g` | Shunt conductance to ground at unit voltage, per unit. | undefined (zero) |
| `b` | Shunt susceptance to ground at unit voltage, per unit; positive is capacitive. | undefined (zero) |

## Returns

A value; validated by `make`.

## Errors

Entries of the wrong count, a non-finite or non-positive `vm`, or arrays that differ in dtype, platform or batch axes are rejected by `make`.

## Interpretation and Use

The net injection of a bus is `p` plus the `p` of the active generators on it. A `PQ` bus has no use for `vm` and `va` except as the starting guess.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A network with PV and PQ buses.

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
### Example 2

A shunt capacitor on a bus.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static power::Net two_bus() {
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
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto voltage = [&](const power::Net &net) { return flat(power::ac(power::make(net)).magnitude)[1]; };

  out.writeln("plain line:                    load-bus voltage ", voltage(two_bus()));
  power::Net tapped = two_bus();
  tapped.lines.tap = vec({1.05});
  out.writeln("tap ratio 1.05:                load-bus voltage ", voltage(tapped));
  power::Net charged = two_bus();
  charged.lines.charge = vec({0.4});
  out.writeln("line charging 0.4:             load-bus voltage ", voltage(charged));
  power::Net shunted = two_bus();
  shunted.buses.b = vec({0.0, 0.3});
  out.writeln("shunt capacitor 0.3 at bus 1:  load-bus voltage ", voltage(shunted));
}
```

Output

```text
plain line:                    load-bus voltage 0.97309
tap ratio 1.05:                load-bus voltage 0.92390
line charging 0.4:             load-bus voltage 0.99344
shunt capacitor 0.3 at bus 1:  load-bus voltage 1.00392
```
### Example 3

A shunt conductance at the load bus.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

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
  const auto report = [&](const char *label, const power::Net &net) {
    const auto r = power::ac(power::make(net));
    out.writeln(label, ": slack output (p.u.) ", flat(r.injection.real())[0], ", flow on line 0-1 (p.u.) ", flat(r.from_power.real())[0], ", load-bus voltage ", flat(r.magnitude)[2]);
  };
  report("as described              ", three_bus());

  power::Net unit_out = three_bus();
  unit_out.gens.active = {false};              // the generator at bus 1 is out of service
  report("generator out of service  ", unit_out);

  power::Net conductance = three_bus();
  conductance.buses.g = vec({0.0, 0.0, 0.2});  // a resistive shunt at the load bus
  report("conductance 0.2 at bus 2  ", conductance);

  power::Net shifted = three_bus();
  shifted.lines.shift = vec({0.05, 0.0, 0.0}); // a phase shifter on line 0-1
  report("phase shift 0.05 on line 0", shifted);
}
```

Output

```text
as described              : slack output (p.u.) 0.10314, flow on line 0-1 (p.u.) -0.09867, load-bus voltage 1.00575
generator out of service  : slack output (p.u.) 0.70407, flow on line 0-1 (p.u.) 0.39463, load-bus voltage 1.00609
conductance 0.2 at bus 2  : slack output (p.u.) 0.30611, flow on line 0-1 (p.u.) -0.00692, load-bus voltage 1.00250
phase shift 0.05 on line 0: slack output (p.u.) 0.10317, flow on line 0-1 (p.u.) -0.19275, load-bus voltage 1.00608
```
### Example 4

A batch of load levels in one call.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

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
  <a class="prev-link" href="Base.html">
    <span class="label">Previous</span>
    <span class="title">Base</span>
  </a>
  <a class="next-link" href="Gens.html">
    <span class="label">Next</span>
    <span class="title">Gens</span>
  </a>
</div>
<!-- apiref:nav:end -->
