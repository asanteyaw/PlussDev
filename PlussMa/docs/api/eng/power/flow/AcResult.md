<!-- generated from AcResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::AcResult`

Voltages, injections and flows of the AC power flow.

## Signature

```cpp
power::AcResult
```

## What It Does

The result of `ac`. Arrays keep the leading scenario axes of the grid.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `voltage` | Complex bus voltages, per unit, `[..., bus]`. |  |
| `magnitude` | Voltage magnitudes, `[..., bus]`. |  |
| `angle` | Voltage angles in radians, `[..., bus]`. |  |
| `injection` | Complex power injected at each bus, `[..., bus]`, including the slack. |  |
| `from_power` | Complex power entering each line at its sending end, `[..., line]`. |  |
| `to_power` | Complex power entering each line at its receiving end, `[..., line]`; the sum of the two ends is the loss of the line. |  |
| `residual` | The largest power mismatch after the last iteration, `[...]`. |  |
| `converged` | True where the residual met the tolerance, `[...]`. |  |
| `iterations` | Newton iterations used, the largest over the scenarios. |  |

## Returns

Returned by `ac`.

## Errors

Reading a field does not solve again.

## Interpretation and Use

A scenario that did not converge has no solution in the iteration budget; its numbers are the last iterate, not an answer. Read `converged` and `residual` first.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Every field, checked against the defining equation.

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
### Example 2

Voltage-controlled and uncontrolled buses compared.

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
### Example 3

An unconverged scenario.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a slack and a load bus joined by a line of 0.01 + 0.1j per unit; the load has a power factor of about 0.93
static power::Net two_bus(double active) {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -active});
  net.buses.q = vec({0.0, -0.4 * active});
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
  power::Options options;
  options.max_iterations = 60;
  const auto r = power::ac(power::make(two_bus(3.2)), options);
  out.writeln("converged: ", r.converged.item<bool>(), ", residual (p.u.): ", r.residual.item<double>());
  out.writeln("load-bus voltage ", flat(r.magnitude)[1], ", load-bus angle ", flat(r.angle)[1]);
  out.writeln("reported load-bus injection: ", flat(r.injection.real())[1], " + ", flat(r.injection.imag())[1], "j   requested: -3.2 - 1.28j");
}
```

Output

```text
converged: false, residual (p.u.): 0.20591
load-bus voltage 0.47115, load-bus angle 43.32663
reported load-bus injection: -2.99409 + -1.21528j   requested: -3.2 - 1.28j
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DcResult.html">
    <span class="label">Previous</span>
    <span class="title">DcResult</span>
  </a>
  <a class="next-link" href="dc.html">
    <span class="label">Next</span>
    <span class="title">dc</span>
  </a>
</div>
<!-- apiref:nav:end -->
