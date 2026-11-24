# Integrating stiff systems

[Back to Methods 6: Stiff systems](../meth_06_stiff_systems.md)

A system is stiff when it has processes on wildly different time scales: a reaction that finishes in a microsecond sitting beside one that takes an hour, or a very strongly damped oscillator. An ordinary explicit integrator must take steps as small as the fastest process even after that process has stopped mattering, and the run takes forever. This page integrates such systems with `stiff::integrate`, an implicit method whose step is set by the accuracy you ask for, not by the fastest rate.

## How it works

You give `integrate(f, y0, t0, t1, options, jacobian)` a rate function `f` that maps a state `y` of shape `[..., n]` to `dy/dt` of the same shape, an initial state, and the interval. The system must be **autonomous**: `f` receives only `y`. The method is a second-order Rosenbrock scheme with an embedded third-order error estimate, which solves one linear system per step instead of a nonlinear one:

```{math}
(I - h d J)\, k_1 = f(y_n), \qquad d = \frac{1}{2 + \sqrt 2}, \qquad y_{n+1} = y_n + h\, k_2 ,
```

with `J = df/dy` the Jacobian at the start of the step and `k_2`, `k_3` the further stages. The step is accepted when the scaled error estimate is below `rtol`, with the error measured relative to `max(|y|, |y_new|, atol/rtol)`, and the next step is chosen from that error. If you do not give a Jacobian it is built by finite differences, one column per state variable, in a single call to `f` for all lanes. If you give one, as a function from `y` to an array of shape `[..., n, n]`, the rate function is called far less often.

The result carries `y` and `t` (where the run stopped), `steps`, `rejected`, `rhs_evaluations`, `jacobian_evaluations`, and `converged`, which is `false` when the step budget `max_steps` ran out or the step shrank below machine precision. All lanes of a batched state share one step size, chosen from the worst lane. `integrate_at(f, y0, times, options, jacobian)` returns the state at a list of increasing times and throws if any interval fails.

## A chemical system with three rates

Robertson's problem is the standard stiff chemistry example. A turns slowly into B at rate `0.04`; B reacts with itself at the very fast rate `3e7` to make C; and B and C make A and C at the fast rate `1e4`. The three species always sum to one. The program integrates from `(1, 0, 0)` to `t = 40` with the default tolerances, prints the state, checks the mass balance, and reports how much work was done.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace stiff = pluss::ma::natsci::common::stiff;

// Robertson's chemical kinetics problem: A -> B (slow, 0.04), 2 B -> B + C (very fast, 3e7), B + C -> A + C (fast, 1e4)
static pluss::Series robertson(const pluss::Series &y) {
  const pluss::Series a = y.select(-1, 0), b = y.select(-1, 1), c = y.select(-1, 2);
  const pluss::Series bc = b.mul(c).mul(1.0e4);
  const pluss::Series bb = b.square().mul(3.0e7);
  const pluss::Series da = a.mul(-0.04).add(bc);
  const pluss::Series db = a.mul(0.04).sub(bc).sub(bb);
  return da.unsqueeze(-1).unite(std::vector<pluss::Series>{db.unsqueeze(-1), bb.unsqueeze(-1)}, -1);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(5).with_sep(""));
  const pluss::Series y0 = pluss::make_series(std::vector<double>{1.0, 0.0, 0.0}, {3}).to(pluss::ScalarType::Float64);
  const auto r = stiff::integrate(robertson, y0, 0.0, 40.0);
  const auto y = r.y.host<double>();
  out.writeln("y(40) = [", y[0], ", ", y[1], ", ", y[2], "]   (reference 7.158e-01, 9.185e-06, 2.842e-01)");
  out.print_defaults(PrintOptions{}.fixed(10).with_sep(""));
  out.writeln("mass balance y1 + y2 + y3 - 1 = ", y[0] + y[1] + y[2] - 1.0);
  out.writeln("converged: ", r.converged ? "yes" : "no", ", accepted steps ", r.steps, ", rejected steps ", r.rejected);
  out.writeln("rate evaluations ", r.rhs_evaluations, ", Jacobian evaluations ", r.jacobian_evaluations);
}
```

Output

```text
y(40) = [7.15827e-01, 9.18553e-06, 2.84164e-01]   (reference 7.158e-01, 9.185e-06, 2.842e-01)
mass balance y1 + y2 + y3 - 1 = 0.0000000000
converged: yes, accepted steps 582, rejected steps 2
rate evaluations 5242, Jacobian evaluations 582
```

The state at `t = 40` is `(0.715827, 9.18553e-06, 0.284164)`, matching the published values. B is tiny because it is produced as fast as it is consumed. The sum of the three species is one to ten digits. The run took 582 steps and 5242 rate evaluations, with 2 rejected steps; an explicit method has to keep its step tied to the fastest rates, which are of order `1e4` per unit time.

## Telling the method the Jacobian

Finite-difference Jacobians cost `n` extra calls to `f` per step. When you can write the Jacobian down, pass it as the last argument. The program integrates the van der Pol oscillator with `mu = 100`, a strongly damped oscillation whose Jacobian is `[[0, 1], [-2 mu y1 y2 - 1, mu (1 - y1^2)]]`, to `t = 200`, a little more than one relaxation cycle, once with each choice.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace stiff = pluss::ma::natsci::common::stiff;

static const double mu = 100.0;

// van der Pol oscillator y1' = y2, y2' = mu (1 - y1^2) y2 - y1; the damping is large, so the system is stiff
static pluss::Series vdp(const pluss::Series &y) {
  const pluss::Series y1 = y.select(-1, 0), y2 = y.select(-1, 1);
  const pluss::Series d2 = y1.square().neg().add(1.0).mul(y2).mul(mu).sub(y1);
  return y2.unsqueeze(-1).unite(std::vector<pluss::Series>{d2.unsqueeze(-1)}, -1);
}
// its Jacobian [[0, 1], [-2 mu y1 y2 - 1, mu (1 - y1^2)]]
static pluss::Series vdp_jacobian(const pluss::Series &y) {
  const pluss::Series y1 = y.select(-1, 0), y2 = y.select(-1, 1);
  const pluss::Series zero = y1.mul(0.0), one = zero.add(1.0);
  const pluss::Series row0 = zero.unsqueeze(-1).unite(std::vector<pluss::Series>{one.unsqueeze(-1)}, -1);
  const pluss::Series row1 = y1.mul(y2).mul(-2.0 * mu).sub(1.0).unsqueeze(-1).unite(std::vector<pluss::Series>{y1.square().neg().add(1.0).mul(mu).unsqueeze(-1)}, -1);
  return row0.unsqueeze(-2).unite(std::vector<pluss::Series>{row1.unsqueeze(-2)}, -2);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const pluss::Series y0 = pluss::make_series(std::vector<double>{2.0, 0.0}, {2}).to(pluss::ScalarType::Float64);
  const auto numeric = stiff::integrate(vdp, y0, 0.0, 200.0);
  const auto analytic = stiff::integrate(vdp, y0, 0.0, 200.0, stiff::Options{}, vdp_jacobian);
  const auto a = numeric.y.host<double>(), b = analytic.y.host<double>();
  out.writeln("finite-difference Jacobian: y(200) = [", a[0], ", ", a[1], "], ", numeric.steps, " steps, ", numeric.rhs_evaluations, " rate evaluations, ", numeric.jacobian_evaluations, " Jacobians");
  out.writeln("analytic Jacobian:         y(200) = [", b[0], ", ", b[1], "], ", analytic.steps, " steps, ", analytic.rhs_evaluations, " rate evaluations, ", analytic.jacobian_evaluations, " Jacobians");
  out.writeln("largest difference between the two answers: ", numeric.y.sub(analytic.y).abs().max().item<double>());
}
```

Output

```text
finite-difference Jacobian: y(200) = [1.718605, -0.008797], 5475 steps, 38333 rate evaluations, 5475 Jacobians
analytic Jacobian:         y(200) = [1.718605, -0.008797], 5475 steps, 16433 rate evaluations, 5475 Jacobians
largest difference between the two answers: 0.000000
```

The two runs take the same 5475 steps and reach the same state, `(1.718605, -0.008797)`, to six digits. The difference is the cost: 38333 rate evaluations with finite differences against 16433 with the analytic Jacobian. Both do 5475 Jacobian evaluations, since one is needed per step either way; the analytic one just costs less per evaluation.

## The state at a list of times

To follow a system across many decades of time, ask for the state at chosen times with `integrate_at`; the step size carries over from one interval to the next, so you do not pay again for the warm-up. The program asks for Robertson's problem at `0, 0.4, 4, 40, 400` and `4000`, which are the times at which the published values are usually quoted.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace stiff = pluss::ma::natsci::common::stiff;

static pluss::Series robertson(const pluss::Series &y) {
  const pluss::Series a = y.select(-1, 0), b = y.select(-1, 1), c = y.select(-1, 2);
  const pluss::Series bc = b.mul(c).mul(1.0e4);
  const pluss::Series bb = b.square().mul(3.0e7);
  return a.mul(-0.04).add(bc).unsqueeze(-1).unite(std::vector<pluss::Series>{a.mul(0.04).sub(bc).sub(bb).unsqueeze(-1), bb.unsqueeze(-1)}, -1);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));
  const pluss::Series y0 = pluss::make_series(std::vector<double>{1.0, 0.0, 0.0}, {3}).to(pluss::ScalarType::Float64);
  const std::vector<double> times{0.0, 0.4, 4.0, 40.0, 400.0, 4000.0};
  const pluss::Series path = stiff::integrate_at(robertson, y0, times);  // [6, 3]: the state at each requested time
  out.writeln("     t        y1          y2          y3");
  for (std::size_t i = 0; i < times.size(); ++i)
    out.writeln(times[i], "   ", path.select(0, static_cast<std::int64_t>(i)).select(0, 0).item<double>(), "   ", path.select(0, static_cast<std::int64_t>(i)).select(0, 1).item<double>(), "   ",
                path.select(0, static_cast<std::int64_t>(i)).select(0, 2).item<double>());
}
```

Output

```text
     t        y1          y2          y3
0.0000e+00   1.0000e+00   0.0000e+00   0.0000e+00
4.0000e-01   9.8517e-01   3.3864e-05   1.4794e-02
4.0000e+00   9.0552e-01   2.2405e-05   9.4459e-02
4.0000e+01   7.1583e-01   9.1855e-06   2.8416e-01
4.0000e+02   4.5052e-01   3.2229e-06   5.4948e-01
4.0000e+03   1.8320e-01   8.9423e-07   8.1680e-01
```

The table follows the slow decay of A, `1 -> 0.985 -> 0.906 -> 0.716 -> 0.451 -> 0.183`, while B stays near `1e-5` throughout. Each row is a factor of ten later than the one before it, and the step size carries over between rows.

## What the tolerance buys

`rtol` and `atol` control the error of each step, and the number of steps grows quickly as they tighten. The program computes a reference solution with `rtol = 1e-11`, then runs Robertson's problem to `t = 40` with `rtol` of `1e-3`, `1e-6` and `1e-9` (and `atol` four orders below), and prints the steps taken and the largest error against the reference.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace stiff = pluss::ma::natsci::common::stiff;

static pluss::Series robertson(const pluss::Series &y) {
  const pluss::Series a = y.select(-1, 0), b = y.select(-1, 1), c = y.select(-1, 2);
  const pluss::Series bc = b.mul(c).mul(1.0e4);
  const pluss::Series bb = b.square().mul(3.0e7);
  return a.mul(-0.04).add(bc).unsqueeze(-1).unite(std::vector<pluss::Series>{a.mul(0.04).sub(bc).sub(bb).unsqueeze(-1), bb.unsqueeze(-1)}, -1);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const pluss::Series y0 = pluss::make_series(std::vector<double>{1.0, 0.0, 0.0}, {3}).to(pluss::ScalarType::Float64);
  stiff::Options tight;
  tight.rtol = 1e-11;
  tight.atol = 1e-14;
  const auto reference = stiff::integrate(robertson, y0, 0.0, 40.0, tight);
  out.writeln("reference (rtol 1e-11): ", reference.steps, " steps");
  for (double rtol : {1e-3, 1e-6, 1e-9}) {
    stiff::Options options;
    options.rtol = rtol;
    options.atol = rtol * 1e-4;
    const auto r = stiff::integrate(robertson, y0, 0.0, 40.0, options);
    out.writeln("rtol ", rtol, ": ", r.steps, " steps (", r.rejected, " rejected), largest error ", r.y.sub(reference.y).abs().max().item<double>());
  }
}
```

Output

```text
reference (rtol 1e-11): 31143 steps
rtol 1.00e-03: 35 steps (2 rejected), largest error 7.11e-06
rtol 1.00e-06: 582 steps (2 rejected), largest error 2.28e-08
rtol 1.00e-09: 12376 steps (3 rejected), largest error 2.76e-11
```

Going from `1e-3` to `1e-6` costs 35 to 582 steps and takes the error from `7.1e-06` to `2.3e-08`; going to `1e-9` costs 12376 steps for an error of `2.8e-11`. The method is second order, so each extra three digits cost roughly 17 to 21 times as many steps. Use a loose tolerance for exploration and tighten it for results you will report.

## Decisions

- Use `stiff::integrate` when the state has components on very different time scales, or when an explicit integrator's step is forced far below what the accuracy needs. For smooth, non-stiff systems the explicit solvers in the other chapters are cheaper per step.
- Pass a Jacobian when you have one and the system is large or the rate function is expensive. For a handful of species the finite-difference default is fine.
- Set `rtol` to the number of correct digits you need in the result, and set `atol` to the smallest value of a component that still matters; components below `atol` are not controlled. The defaults, `1e-6` and `1e-10`, suit concentrations of order one.
- Use `integrate_at` when you want the state at several times. Use a single `integrate` call when you only need the end state.
- A batched state of shape `[lanes, n]` integrates all lanes together with one step size; group lanes that have similar time scales, because the stiffest lane sets the step for all.

## Pitfalls

The rate function receives only the state, never the time, so a forcing that depends on time must be turned into part of the state. The program integrates `y' = -y + sin t` by adding a clock variable `t` with `t' = 1`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace stiff = pluss::ma::natsci::common::stiff;

static pluss::Series robertson(const pluss::Series &y) {
  const pluss::Series a = y.select(-1, 0), b = y.select(-1, 1), c = y.select(-1, 2);
  const pluss::Series bc = b.mul(c).mul(1.0e4);
  const pluss::Series bb = b.square().mul(3.0e7);
  return a.mul(-0.04).add(bc).unsqueeze(-1).unite(std::vector<pluss::Series>{a.mul(0.04).sub(bc).sub(bb).unsqueeze(-1), bb.unsqueeze(-1)}, -1);
}
static pluss::Series state(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(pluss::ScalarType::Float64); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // y' = -y + sin t has a forcing that depends on time, but the rate function sees only the state.
  // Make time a state variable: s = (y, t) with s' = (-y + sin t, 1).
  const auto forced = [](const pluss::Series &s) {
    const pluss::Series y = s.select(-1, 0), t = s.select(-1, 1);
    return y.neg().add(t.sin()).unsqueeze(-1).unite(std::vector<pluss::Series>{t.mul(0.0).add(1.0).unsqueeze(-1)}, -1);
  };
  const auto r = stiff::integrate(forced, state({0.0, 0.0}), 0.0, 10.0);
  const double exact = 0.5 * (std::sin(10.0) - std::cos(10.0) + std::exp(-10.0));
  out.writeln("y(10) = ", r.y.select(-1, 0).item<double>(), ", exact ", exact);
  out.writeln("the clock variable ends at ", r.y.select(-1, 1).item<double>());
}
```

Output

```text
y(10) = 0.147556, exact 0.147548
the clock variable ends at 10.000000
```

The augmented system gives `y(10) = 0.147556` against the exact `0.147548`, and the clock ends at exactly `10`. A rate function that tried to read time from outside the call would see a single frozen value.

`converged` is `false` when the step budget is spent, and the returned state is the last accepted one, not the state at `t1`. The program gives Robertson's problem a budget of 100 steps for a run to `t = 4000`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace stiff = pluss::ma::natsci::common::stiff;

static pluss::Series robertson(const pluss::Series &y) {
  const pluss::Series a = y.select(-1, 0), b = y.select(-1, 1), c = y.select(-1, 2);
  const pluss::Series bc = b.mul(c).mul(1.0e4);
  const pluss::Series bb = b.square().mul(3.0e7);
  return a.mul(-0.04).add(bc).unsqueeze(-1).unite(std::vector<pluss::Series>{a.mul(0.04).sub(bc).sub(bb).unsqueeze(-1), bb.unsqueeze(-1)}, -1);
}
static pluss::Series state(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(pluss::ScalarType::Float64); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  stiff::Options small;
  small.max_steps = 100;
  const auto r = stiff::integrate(robertson, state({1.0, 0.0, 0.0}), 0.0, 4000.0, small);
  out.writeln("integrate: converged ", r.converged ? "yes" : "no", ", stopped at t = ", r.t, " after ", r.steps, " steps");
  try {
    stiff::integrate_at(robertson, state({1.0, 0.0, 0.0}), {0.0, 40.0, 4000.0}, small);
  } catch (const pluss::Error &e) { out.writeln("integrate_at: ", e.message()); }
}
```

Output

```text
integrate: converged no, stopped at t = 0.0175 after 98 steps
integrate_at: natsci: stiff: step budget exhausted or step size underflow
```

The run stops at `t = 0.0175` after 98 steps with `converged no`: the first steps of a stiff problem are tiny while the fast transient settles. `integrate_at` turns the same condition into an error, because it cannot return a state at a time it did not reach. Raise `max_steps`, loosen `rtol`, or check the rate function for a runaway.

Invalid arguments are rejected before any work is done: a backwards interval, a non-positive tolerance, a NaN in the starting state, and fewer than two output times.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace stiff = pluss::ma::natsci::common::stiff;

static pluss::Series robertson(const pluss::Series &y) {
  const pluss::Series a = y.select(-1, 0), b = y.select(-1, 1), c = y.select(-1, 2);
  const pluss::Series bc = b.mul(c).mul(1.0e4);
  const pluss::Series bb = b.square().mul(3.0e7);
  return a.mul(-0.04).add(bc).unsqueeze(-1).unite(std::vector<pluss::Series>{a.mul(0.04).sub(bc).sub(bb).unsqueeze(-1), bb.unsqueeze(-1)}, -1);
}
static pluss::Series state(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(pluss::ScalarType::Float64); }

int main() {
  OutputEngine out;
  const pluss::Series y0 = state({1.0, 0.0, 0.0});
  try { stiff::integrate(robertson, y0, 40.0, 0.0); } catch (const pluss::Error &e) { out.writeln("backwards interval: ", e.message()); }
  stiff::Options zero;
  zero.rtol = 0.0;
  try { stiff::integrate(robertson, y0, 0.0, 40.0, zero); } catch (const pluss::Error &e) { out.writeln("zero tolerance: ", e.message()); }
  try { stiff::integrate(robertson, state({1.0, std::nan(""), 0.0}), 0.0, 40.0); } catch (const pluss::Error &e) { out.writeln("NaN start: ", e.message()); }
  try { stiff::integrate_at(robertson, y0, {0.0}); } catch (const pluss::Error &e) { out.writeln("one output time: ", e.message()); }
  try { stiff::integrate_at(robertson, y0, {0.0, 10.0, 5.0}); } catch (const pluss::Error &e) { out.writeln("decreasing times: ", e.message()); }
}
```

Output

```text
backwards interval:  natsci: stiff: need finite t0 <= t1
zero tolerance:  natsci: stiff: tolerances must be positive
NaN start:  natsci: stiff: initial state must be finite
one output time:  natsci: stiff: need at least two output times
decreasing times:  natsci: stiff: need finite t0 <= t1
```

Each message names the argument. A decreasing list of output times is reported as a bad interval, because `integrate_at` hands each consecutive pair to `integrate`.

## Where next

[Methods 4](../meth_04_stochastic_kinetics/gillespie_ensembles.md) simulates the same kinds of reaction networks one molecule at a time; the rates there are the propensities whose averages obey the rate equations integrated here. [Biology 25](../../biology/bio_25_networks/mass_action_networks.md) builds reaction networks from mass-action rate laws, the kind of right-hand side integrated here. The reference pages for `natsci::common::stiff::integrate`, `integrate_at`, `Options` and `Result` list every field.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
