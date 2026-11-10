<!-- generated from Lines.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::Lines`

The lines and transformers of a network.

## Signature

```cpp
power::Lines
```

## What It Does

Each line joins two buses with a series impedance and, optionally, line charging, an off-nominal tap and a phase shift, which together describe a line or a transformer.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `from` | The bus at the sending end of each line. | required |
| `to` | The bus at the receiving end of each line. | required |
| `r` | Series resistance, per unit. | required |
| `x` | Series reactance, per unit. | required |
| `charge` | Total line-charging susceptance, per unit, half at each end. | undefined (zero) |
| `tap` | Off-nominal transformer tap ratio (magnitude); `1` is a plain line. | undefined (1) |
| `shift` | Transformer phase shift, radians. | undefined (zero) |
| `active` | In-service status of each line. | empty (all in service) |

## Returns

A value; validated by `make`.

## Errors

A line from a bus to itself or to a bus outside the network, a zero series impedance, an `active` list of the wrong length, or lines that leave a bus disconnected from the slack are rejected.

## Interpretation and Use

A line out of service is a line with `active` false; the network without it must still connect every bus to the slack. Leading axes on `r`, `x`, `charge`, `tap` or `shift` describe scenarios.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Tap, charging and shunt effects on a load-bus voltage.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

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
### Example 2

A line taken out of service.

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
### Example 3

Validation of lines.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <functional>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

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

static void report(const char *what, const std::function<void()> &build) {
  try {
    build();
    std::cout << what << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << what << ": " << e.what() << "\n";
  }
}

int main() {
  report("no slack bus          ", [] { auto n = two_bus(); n.buses.kind = {power::BusKind::PQ, power::BusKind::PQ}; (void)power::make(n); });
  report("two slack buses       ", [] { auto n = two_bus(); n.buses.kind = {power::BusKind::Slack, power::BusKind::Slack}; (void)power::make(n); });
  report("an isolated bus       ", [] { auto n = two_bus(); n.lines.active = {false}; (void)power::make(n); });
  report("zero line impedance   ", [] { auto n = two_bus(); n.lines.r = vec({0.0}); n.lines.x = vec({0.0}); (void)power::make(n); });
  report("generator off the grid", [] { auto n = two_bus(); n.gens.bus = {7}; n.gens.p = vec({0.1}); n.gens.q = vec({0.0}); (void)power::make(n); });

  // a 50 MW load written as 50 where per unit was meant: accepted, and the answer is nonsense
  power::Net watts = two_bus();
  watts.buses.p = vec({0.0, -50.0});
  watts.buses.q = vec({0.0, -20.0});
  const auto r = power::ac(power::make(watts));
  std::cout << "a load of 50 per unit (meant 0.5): accepted, converged " << r.converged.item<bool>() << ", load-bus voltage " << flat(r.magnitude)[1] << "\n";
}
```

Output

```text
no slack bus          : eng::power::make: exactly one slack bus is required
two slack buses       : eng::power::make: exactly one slack bus is required
an isolated bus       : eng::power::make: every bus must connect to the slack island
zero line impedance   : eng::power::make: line series impedance cannot be zero
generator off the grid: eng::power::make: generator bus lies outside the network
a load of 50 per unit (meant 0.5): accepted, converged 0, load-bus voltage 4.48327
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Gens.html">
    <span class="label">Previous</span>
    <span class="title">Gens</span>
  </a>
  <a class="next-link" href="Net.html">
    <span class="label">Next</span>
    <span class="title">Net</span>
  </a>
</div>
<!-- apiref:nav:end -->
