<!-- generated from solve.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::solve`

Solves the model-predictive control problem from the current state.

## Signature

```cpp
control::mpc::Result control::mpc::solve(
    const control::mpc::Plan& plan,
    const pluss::Series& initial_state,
    const pluss::Series& reference,
    pluss::math::qp::WarmStart warm = {});

control::mpc::Result control::mpc::solve(
    const lti::StateSpace& system,
    const pluss::Series& initial_state,
    const pluss::Series& reference,
    const control::mpc::Weights& weights,
    const control::mpc::Bounds& bounds,
    control::mpc::Options options = {},
    pluss::math::qp::WarmStart warm = {});
```

## What It Does

Finds the sequence of inputs over the horizon that minimises the weighted tracking error and effort while respecting the bounds, and returns the whole plan. Only the first input is applied; the problem is solved again at the next sample from the new state (receding horizon). There is an overload on a built `Plan` and a one-shot overload that builds and solves.

## Parameters

`plan`
: A `Plan` from `build`; or, for the one-shot overload, `system`, `weights`, `bounds` and `options`.

`initial_state`
: The current state, `[states]`.

`reference`
: A target state `[states]` held over the horizon, or a trajectory `[horizon, states]`.

`warm`
: Optional previous primal and dual solution from `Result::optimization.warm()`.

## Returns

A `Result`.

## Errors

An initial state or reference of the wrong size, a non-finite initial state and the errors of `build` (for the one-shot overload) throw `pluss::Error`.

## Interpretation and Use

With a constant reference of 1 from the state 0 the plan moves 0.916 on the first step and then converges; a reference trajectory of 0.25, 0.5, 0.75 and 1 gives a gentler first move of 0.25 (the first example). Warm-starting from a previous solution of the same problem needs 10 iterations instead of 40, and 30 for a nearby problem (the second example). The third closes the loop: the state is rate-limited by the input bound and then settles on the reference.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A constant reference against a reference trajectory.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
namespace mpc = pluss::subsp::eng::control::mpc;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

// A discrete integrator x[k+1] = x[k] + u[k] to be driven from 0 to the reference 1.
static lti::StateSpace plant() { return lti::state(mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0); }
static mpc::Weights weights(double r = 0.1, double terminal = 2.0) { return {mat({1.0}, 1, 1), mat({r}, 1, 1), mat({terminal}, 1, 1)}; }
static mpc::Bounds generous() { return {pluss::make_series({-10.0}), pluss::make_series({10.0})}; }
static mpc::Options with_horizon(std::int64_t n) { mpc::Options o; o.horizon = n; o.qp.abs_tol = 1e-8; o.qp.rel_tol = 1e-8; return o; }
static mpc::Result run(double r = 0.1, const mpc::Bounds &b = generous(), std::int64_t n = 4, double terminal = 2.0) { return mpc::solve(plant(), pluss::make_series({0.0}), pluss::make_series({1.0}), weights(r, terminal), b, with_horizon(n)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // A constant target or a full reference trajectory.
  const mpc::Plan plan = mpc::build(plant(), weights(), generous(), with_horizon(4));
  const mpc::Result constant = mpc::solve(plan, pluss::make_series({0.0}), pluss::make_series({1.0}));
  const mpc::Result trajectory = mpc::solve(plan, pluss::make_series({0.0}), mat({0.25, 0.5, 0.75, 1.0}, 4, 1));
  out.writeln("constant reference 1:        first control", constant.control.items<double>()[0], " states", constant.states.flatten().items<double>());
  out.writeln("ramped reference 0.25 .. 1:  first control", trajectory.control.items<double>()[0], " states", trajectory.states.flatten().items<double>());
}
```

Output

```text
constant reference 1:        first control 0.9161  states [0.9161, 0.9930, 0.9994, 1.0000]
ramped reference 0.25 .. 1:  first control 0.2500  states [0.2500, 0.4999, 0.7490, 0.9880]
```
### Example 2

Cold and warm starts.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
namespace mpc = pluss::subsp::eng::control::mpc;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

// A discrete integrator x[k+1] = x[k] + u[k] to be driven from 0 to the reference 1.
static lti::StateSpace plant() { return lti::state(mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0); }
static mpc::Weights weights(double r = 0.1, double terminal = 2.0) { return {mat({1.0}, 1, 1), mat({r}, 1, 1), mat({terminal}, 1, 1)}; }
static mpc::Bounds generous() { return {pluss::make_series({-10.0}), pluss::make_series({10.0})}; }
static mpc::Options with_horizon(std::int64_t n) { mpc::Options o; o.horizon = n; o.qp.abs_tol = 1e-8; o.qp.rel_tol = 1e-8; return o; }
static mpc::Result run(double r = 0.1, const mpc::Bounds &b = generous(), std::int64_t n = 4, double terminal = 2.0) { return mpc::solve(plant(), pluss::make_series({0.0}), pluss::make_series({1.0}), weights(r, terminal), b, with_horizon(n)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  // Warm-starting from the previous solution helps most when the next problem is nearly the same, as in a receding-horizon loop that has settled.
  const mpc::Plan plan = mpc::build(plant(), weights(), {pluss::make_series({-0.4}), pluss::make_series({0.4})}, with_horizon(6));
  const mpc::Result first = mpc::solve(plan, pluss::make_series({0.0}), pluss::make_series({1.0}));
  const mpc::Result cold = mpc::solve(plan, pluss::make_series({0.0}), pluss::make_series({1.0}));
  const mpc::Result warm = mpc::solve(plan, pluss::make_series({0.0}), pluss::make_series({1.0}), first.optimization.warm());
  out.writeln("the same problem again: cold start", cold.optimization.iterations, "iterations, warm start", warm.optimization.iterations);
  out.writeln("the same first control either way:", std::abs(cold.control.items<double>()[0] - warm.control.items<double>()[0]) < 1e-6);
  const mpc::Result nearby_cold = mpc::solve(plan, pluss::make_series({0.001}), pluss::make_series({1.0}));
  const mpc::Result nearby_warm = mpc::solve(plan, pluss::make_series({0.001}), pluss::make_series({1.0}), first.optimization.warm());
  out.writeln("a nearby initial state (0.001): cold start", nearby_cold.optimization.iterations, "iterations, warm start", nearby_warm.optimization.iterations);
}
```

Output

```text
the same problem again: cold start 40 iterations, warm start 10
the same first control either way: true
a nearby initial state (0.001): cold start 40 iterations, warm start 30
```
### Example 3

A receding-horizon loop.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
namespace mpc = pluss::subsp::eng::control::mpc;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

// A discrete integrator x[k+1] = x[k] + u[k] to be driven from 0 to the reference 1.
static lti::StateSpace plant() { return lti::state(mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0); }
static mpc::Weights weights(double r = 0.1, double terminal = 2.0) { return {mat({1.0}, 1, 1), mat({r}, 1, 1), mat({terminal}, 1, 1)}; }
static mpc::Bounds generous() { return {pluss::make_series({-10.0}), pluss::make_series({10.0})}; }
static mpc::Options with_horizon(std::int64_t n) { mpc::Options o; o.horizon = n; o.qp.abs_tol = 1e-8; o.qp.rel_tol = 1e-8; return o; }
static mpc::Result run(double r = 0.1, const mpc::Bounds &b = generous(), std::int64_t n = 4, double terminal = 2.0) { return mpc::solve(plant(), pluss::make_series({0.0}), pluss::make_series({1.0}), weights(r, terminal), b, with_horizon(n)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // Receding horizon: apply the first control, move the plant, solve again.
  const mpc::Plan plan = mpc::build(plant(), weights(), {pluss::make_series({-0.4}), pluss::make_series({0.4})}, with_horizon(5));
  double x = 0.0;
  std::vector<double> path{x};
  for (int step = 0; step < 8; ++step) {
    x += mpc::solve(plan, pluss::make_series({x}), pluss::make_series({1.0})).control.items<double>()[0];
    path.push_back(x);
  }
  out.writeln("state after each step =", path, "(rate-limited by the input bound, then settling on the reference)");
}
```

Output

```text
state after each step = [0.0000, 0.4000, 0.8000, 0.9832, 0.9986, 0.9999, 1.0000, 1.0000, 1.0000] (rate-limited by the input bound, then settling on the reference)
```
### Example 4

Every refused argument.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
namespace mpc = pluss::subsp::eng::control::mpc;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

// A discrete integrator x[k+1] = x[k] + u[k] to be driven from 0 to the reference 1.
static lti::StateSpace plant() { return lti::state(mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0); }
static mpc::Weights weights(double r = 0.1, double terminal = 2.0) { return {mat({1.0}, 1, 1), mat({r}, 1, 1), mat({terminal}, 1, 1)}; }
static mpc::Bounds generous() { return {pluss::make_series({-10.0}), pluss::make_series({10.0})}; }
static mpc::Options with_horizon(std::int64_t n) { mpc::Options o; o.horizon = n; o.qp.abs_tol = 1e-8; o.qp.rel_tol = 1e-8; return o; }
static mpc::Result run(double r = 0.1, const mpc::Bounds &b = generous(), std::int64_t n = 4, double terminal = 2.0) { return mpc::solve(plant(), pluss::make_series({0.0}), pluss::make_series({1.0}), weights(r, terminal), b, with_horizon(n)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const mpc::Plan plan = mpc::build(plant(), weights(), generous(), with_horizon(4));
  refused(out, "an initial state with two entries", [&] { mpc::solve(plan, pluss::make_series({0.0, 0.0}), pluss::make_series({1.0})); });
  refused(out, "a reference trajectory of the wrong length", [&] { mpc::solve(plan, pluss::make_series({0.0}), mat({1.0, 1.0}, 2, 1)); });
  refused(out, "a non-finite initial state", [&] { mpc::solve(plan, pluss::make_series({std::nan("")}), pluss::make_series({1.0})); });
}
```

Output

```text
an initial state with two entries : eng::control::mpc::solve: initial_state must end in [states]
a reference trajectory of the wrong length : eng::control::mpc::solve: reference must be a vector or end in [horizon, width]
a non-finite initial state : math::qp::solve: q must be finite
```