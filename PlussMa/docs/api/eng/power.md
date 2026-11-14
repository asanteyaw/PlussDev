<!-- generated from power.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Power systems

Transmission networks described in per unit, compiled into an immutable grid, and analysed by linear and exact power flow, sensitivity and outage analysis, economic dispatch, optimal power flow, state estimation and continuation. A network is described once, as a `Net`, compiled by `make` into a `Grid`, and analysed under any number of scenarios.

## Choosing A Surface

| Task | Branch |
|---|---|
| Describe a network, convert units and compile a grid | [Network model](power/model.md) |
| DC and AC power flow | [Power flow](power/flow.md) |
| Power transfer and line outage distribution factors | [Sensitivity and outage factors](power/security.md) |
| Least-cost generation, with and without security | [Economic dispatch](power/dispatch.md) |
| Exact AC least-cost operation | [Optimal power flow](power/opf.md) |
| Voltages from noisy measurements | [State estimation](power/estimation.md) |
| Voltage collapse and loading margin | [Continuation power flow](power/continuation.md) |

## Practical Examples

The examples build a two-bus network, solve a ring with the DC power flow and a three-bus network with the AC power flow.

### Example 1

Compile a two-bus network and compare its admittance matrix with a hand value.

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
### Example 2

Solve a ring with the DC power flow against hand values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

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
  const auto r = power::dc(power::make(ring()));
  out.writeln("angles (rad):          ", flat(r.angle), "   by hand 0, -0.025, -0.075");
  out.writeln("flows from end (p.u.): ", flat(r.from_power), "   by hand 0.25 (0 to 1), 0.25 (1 to 2), 0.25 (0 to 2)");
  out.writeln("flows to end (p.u.):   ", flat(r.to_power), "   equal and opposite: the line is lossless");
  out.writeln("injections (p.u.):     ", flat(r.injection), "   the slack supplies the whole load");
  out.writeln("residual: ", flat(r.residual));
}
```

Output

```text
angles (rad):          [0.00000, -0.02500, -0.07500]   by hand 0, -0.025, -0.075
flows from end (p.u.): [0.25000, 0.25000, 0.25000]   by hand 0.25 (0 to 1), 0.25 (1 to 2), 0.25 (0 to 2)
flows to end (p.u.):   [-0.25000, -0.25000, -0.25000]   equal and opposite: the line is lossless
injections (p.u.):     [0.50000, 0.00000, -0.50000]   the slack supplies the whole load
residual: [0.00000]
```
### Example 3

Solve a three-bus network with the AC power flow and check it against its defining equation.

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
  const power::Grid grid = power::make(three_bus());
  const auto r = power::ac(grid);
  out.writeln("voltage magnitudes: ", flat(r.magnitude), ", angles (rad): ", flat(r.angle));
  out.writeln("converged ", r.converged.item<bool>(), " in ", r.iterations, " iterations, residual ", r.residual.item<double>());

  // the defining equation, rebuilt from the voltages
  const auto vr = flat(r.voltage.real()), vi = flat(r.voltage.imag()), yr = flat(grid.ybus().real()), yi = flat(grid.ybus().imag());
  const auto pr = flat(r.injection.real()), pi = flat(r.injection.imag());
  double worst = 0.0;
  for (int i = 0; i < 3; ++i) {
    std::complex<double> current = 0.0;
    for (int j = 0; j < 3; ++j) current += std::complex<double>(yr[3 * i + j], yi[3 * i + j]) * std::complex<double>(vr[j], vi[j]);
    const std::complex<double> power = std::complex<double>(vr[i], vi[i]) * std::conj(current);
    worst = std::max(worst, std::abs(power - std::complex<double>(pr[i], pi[i])));
    out.writeln("bus ", i, ": reported ", pr[i], " + ", pi[i], "j    V conj(Y V) = ", power.real(), " + ", power.imag(), "j");
  }
  out.writeln("largest difference: ", worst);

  // losses: the sum of the two ends of every line
  const auto from = flat(r.from_power.real()), to = flat(r.to_power.real());
  double loss = 0.0, injected = 0.0;
  for (std::size_t line = 0; line < from.size(); ++line) loss += from[line] + to[line];
  for (double v : pr) injected += v;
  out.writeln("active losses from the line ends: ", loss, "   sum of the injections: ", injected);
}
```

Output

```text
voltage magnitudes: [1.04000, 1.02000, 1.00575], angles (rad): [0.00000, 0.01136, -0.04632]
converged true in 3 iterations, residual 0.00000
bus 0: reported 0.10314 + 0.32097j    V conj(Y V) = 0.10314 + 0.32097j
bus 1: reported 0.40000 + -0.19852j    V conj(Y V) = 0.40000 + -0.19852j
bus 2: reported -0.50000 + -0.20000j    V conj(Y V) = -0.50000 + -0.20000j
largest difference: 0.00000
active losses from the line ends: 0.00314   sum of the injections: 0.00314
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

power/model
power/flow
power/security
power/dispatch
power/opf
power/estimation
power/continuation
```
<!-- apiref:toctree:end -->
