<!-- generated from Gens.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::Gens`

The generators of a network.

## Signature

```cpp
power::Gens
```

## What It Does

Generators are listed separately from buses, so several may share a bus and each may be taken out of service.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `bus` | The bus index of each generator. | empty (none) |
| `p` | Active generation of each generator, per unit. | required with `bus` |
| `q` | Reactive generation or initial set point, per unit. | required with `bus` |
| `active` | In-service status of each generator; false removes it from the injections. | empty (all active) |

## Returns

A value; validated by `make`.

## Errors

A generator on a bus outside the network, an `active` list that does not match the generator count, or generator values without generator buses are rejected.

## Interpretation and Use

The injection of a bus is its own `p` plus the `p` of its active generators; the load of a generator bus therefore goes into `Buses::p` and the generation into `Gens::p`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A generator on a voltage-controlled bus.

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
### Example 2

Generator-off-grid validation.

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
### Example 3

Generation moved from the slack to a generator in the DC flow.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// three buses in a ring: a slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 in between.
// The two paths from bus 0 to bus 2, directly (x = 0.3) and through bus 1 (x = 0.1 + 0.2), have equal reactance.
static power::Net ring(double resistance_scale = 1.0) {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01 * resistance_scale, 0.02 * resistance_scale, 0.03 * resistance_scale});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  power::Net net = ring();
  net.buses.kind[1] = power::BusKind::PV;
  net.gens.bus = {1};
  net.gens.p = vec({0.3});
  net.gens.q = vec({0.0});
  const auto r = power::dc(power::make(net));
  out.writeln("injections (p.u.): ", flat(r.injection), "   the slack supplies 0.5 - 0.3 = 0.2");
  out.writeln("angles (rad):      ", flat(r.angle));
  auto flows = flat(r.from_power);
  for (double &flow : flows) flow += 0.0;   // turns a rounding-level -0 into 0
  out.writeln("flows (p.u.):      ", flows, "   lines 0-1, 1-2, 0-2");
}
```

Output

```text
injections (p.u.): [0.20000, 0.30000, -0.50000]   the slack supplies 0.5 - 0.3 = 0.2
angles (rad):      [0.00000, 0.00000, -0.06000]
flows (p.u.):      [-0.00000, 0.30000, 0.20000]   lines 0-1, 1-2, 0-2
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Buses.html">
    <span class="label">Previous</span>
    <span class="title">Buses</span>
  </a>
  <a class="next-link" href="Lines.html">
    <span class="label">Next</span>
    <span class="title">Lines</span>
  </a>
</div>
<!-- apiref:nav:end -->
