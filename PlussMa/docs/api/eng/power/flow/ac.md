<!-- generated from ac.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::ac`

Solves the AC power flow by Newton's method.

## Signature

```cpp
power::AcResult power::ac(const power::Grid& grid, const power::Options& options = {});
```

## What It Does

Finds the complex bus voltages that satisfy `S = V conj(Y V)` at every bus, starting from the voltages of the network description and applying Newton's method with the exact Jacobian until the largest power mismatch is below the tolerance. Every scenario of a batch is solved at once, each with its own `converged`.

## Parameters

`grid`
: The compiled network.

`options`
: The tolerance and iteration budget.

## Returns

An `AcResult`.

## Errors

Throws `std::invalid_argument` for a non-positive tolerance or a budget below one. A network with no solution, or one beyond the loadability limit, does not throw: it returns with `converged` false and the last iterate.

## Interpretation and Use

Check `converged` before using a result. Newton converges up to the nose point of the load curve and fails beyond it, so a failure on a network that should be solvable usually means a data error, for example in the per-unit conversion.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A three-bus network checked against its defining equation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

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

Tolerance and iteration budget.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

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
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const power::Grid grid = power::make(two_bus(0.5));
  struct Run { const char *name; double tolerance; std::int64_t iterations; };
  for (const Run &run : {Run{"defaults (1e-10, 20)  ", 1e-10, 20}, Run{"tolerance 1e-3        ", 1e-3, 20}, Run{"tolerance 1e-14       ", 1e-14, 20}, Run{"max_iterations 2      ", 1e-10, 2}}) {
    power::Options options;
    options.tolerance = run.tolerance;
    options.max_iterations = run.iterations;
    const auto r = power::ac(grid, options);
    out.writeln(run.name, ": iterations ", r.iterations, ", residual ", r.residual.item<double>(), ", converged ", r.converged.item<bool>());
  }
}
```

Output

```text
defaults (1e-10, 20)  : iterations 4, residual 2.22e-16, converged true
tolerance 1e-3        : iterations 2, residual 4.08e-05, converged true
tolerance 1e-14       : iterations 4, residual 2.22e-16, converged true
max_iterations 2      : iterations 2, residual 4.08e-05, converged false
```
### Example 3

The loadability limit of a line, against its closed form.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

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

// the largest load the line can serve, from the radicand of the closed form
static double closed_form_limit() {
  const auto radicand = [](double P) {
    const double Q = 0.4 * P, r = 0.01, x = 0.1;
    const double b = 0.5 - (r * P + x * Q);
    return b * b - (r * r + x * x) * (P * P + Q * Q);
  };
  double lo = 0.1, hi = 10.0;
  for (int i = 0; i < 80; ++i) {
    const double mid = 0.5 * (lo + hi);
    (radicand(mid) >= 0.0 ? lo : hi) = mid;
  }
  return lo;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  power::Options options;
  options.max_iterations = 60;
  double lo = 0.5, hi = 6.0;   // converged at 0.5, not at 6
  for (int i = 0; i < 40; ++i) {
    const double mid = 0.5 * (lo + hi);
    (power::ac(power::make(two_bus(mid)), options).converged.item<bool>() ? lo : hi) = mid;
  }
  out.writeln("largest load for which the power flow converges (p.u.): ", lo);
  out.writeln("closed-form nose point (p.u.):                          ", closed_form_limit());
  for (double load : {1.0, 3.0, lo, 3.2}) {
    const auto r = power::ac(power::make(two_bus(load)), options);
    out.writeln("load ", load, ": converged ", r.converged.item<bool>(), ", load-bus voltage ", flat(r.magnitude)[1], ", residual ", r.residual.item<double>());
  }
}
```

Output

```text
largest load for which the power flow converges (p.u.): 3.15975
closed-form nose point (p.u.):                          3.15975
load 1.00000: converged true, load-bus voltage 0.94169, residual 0.00000
load 3.00000: converged true, load-bus voltage 0.69325, residual 0.00000
load 3.15975: converged true, load-bus voltage 0.58482, residual 0.00000
load 3.20000: converged false, load-bus voltage 0.47115, residual 0.20591
```
### Example 4

A scenario beyond the limit.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

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
  <a class="prev-link" href="dc.html">
    <span class="label">Previous</span>
    <span class="title">dc</span>
  </a>
</div>
<!-- apiref:nav:end -->
