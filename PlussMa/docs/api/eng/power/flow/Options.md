<!-- generated from Options.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::Options`

Controls of the AC Newton iteration.

## Signature

```cpp
power::Options
```

## What It Does

Controls for `ac`: how exactly the power balance must be met and how many Newton iterations are allowed.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `tolerance` | The largest power mismatch, per unit, at which the iteration stops. | 1e-10 |
| `max_iterations` | The Newton iteration budget. | 20 |

## Returns

A value; validated by `ac`.

## Errors

A non-positive or non-finite tolerance, or a budget below one, is rejected.

## Interpretation and Use

Newton's method converges quadratically, so a tighter tolerance costs about one iteration per further digit. Check `converged` whenever the budget is cut.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The residual after one to four iterations: quadratic convergence.

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
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const power::Grid grid = power::make(two_bus(0.5));
  for (std::int64_t budget = 1; budget <= 4; ++budget) {
    power::Options options;
    options.tolerance = 1e-30;   // never met, so the run uses its whole budget
    options.max_iterations = budget;
    const auto r = power::ac(grid, options);
    out.writeln("after ", budget, " iteration(s): residual ", r.residual.item<double>());
  }
}
```

Output

```text
after 1 iteration(s): residual 1.61e-02
after 2 iteration(s): residual 4.08e-05
after 3 iteration(s): residual 2.46e-10
after 4 iteration(s): residual 2.22e-16
```
### Example 2

Default, loose and tight tolerances and a short budget.

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

The bisection of the loadability limit with a larger budget.

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

An unconverged run.

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
  <a class="next-link" href="DcResult.html">
    <span class="label">Next</span>
    <span class="title">DcResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
