<!-- generated from BusKind.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::BusKind`

Selects which pair of quantities a bus fixes.

## Signature

```cpp
power::BusKind
```

## What It Does

Every bus in a network is one of three kinds. The kind says which two of the four bus quantities (active and reactive injection, voltage magnitude and angle) are fixed and which are solved for.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `Slack` | Fixes the voltage magnitude and angle; its active and reactive injection are solved for and absorb the losses. Exactly one slack bus is required. |  |
| `PV` | Fixes the active injection and the voltage magnitude; its reactive injection and angle are solved for. Use for a bus where a generator or regulator holds the voltage. |  |
| `PQ` | Fixes the active and reactive injection; its voltage magnitude and angle are solved for. Use for load buses. |  |

## Returns

A scoped enumeration, stored in `Buses::kind`.

## Errors

A network with no slack bus or with more than one is rejected by `make`.

## Interpretation and Use

The injections of a bus are signed, with generation positive and load negative. A `PV` bus holds its voltage only while its reactive output is free; the library does not apply reactive limits in the power flow.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Generator buses as PV and as PQ.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a slack at bus 0, a generator bus at bus 1 (1.02 p.u., 0.6 p.u. generation, 0.2 p.u. load) and a load bus at bus 2
static power::Net three_bus(power::BusKind second = power::BusKind::PV) {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, second, power::BusKind::PQ};
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
  for (const auto kind : {power::BusKind::PV, power::BusKind::PQ}) {
    const auto r = power::ac(power::make(three_bus(kind)));
    out.writeln(kind == power::BusKind::PV ? "generator bus as PV: " : "generator bus as PQ: ", "voltage at bus 1 ", flat(r.magnitude)[1], ", reactive output at bus 1 ", flat(r.injection.imag())[1], ", reactive output of the slack ", flat(r.injection.imag())[0]);
  }
}
```

Output

```text
generator bus as PV: voltage at bus 1 1.02000, reactive output at bus 1 -0.19852, reactive output of the slack 0.32097
generator bus as PQ: voltage at bus 1 1.03611, reactive output at bus 1 0.00000, reactive output of the slack 0.11458
```
### Example 2

Network validation, including the slack bus rules.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <functional>

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
a load of 50 per unit (meant 0.5): accepted, converged 0, load-bus voltage -4.45537
```
### Example 3

A network with a voltage-controlled bus.

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Base.html">
    <span class="label">Next</span>
    <span class="title">Base</span>
  </a>
</div>
<!-- apiref:nav:end -->
