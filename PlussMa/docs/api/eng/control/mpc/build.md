<!-- generated from build.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::build`

Compiles a model-predictive control problem into a reusable plan.

## Signature

```cpp
control::mpc::Plan control::mpc::build(
    const lti::StateSpace& system,
    const control::mpc::Weights& weights,
    const control::mpc::Bounds& bounds,
    control::mpc::Options options = {});
```

## What It Does

Condenses the prediction of a discrete linear system over a horizon, together with the cost weights and the input and state bounds, into a quadratic program and prepares its solver. The result is a `Plan` that `solve` can run many times, once per sample, for different initial states and references.

## Parameters

`system`
: A discrete `StateSpace` (a positive sample time).

`weights`
: The `Weights` of the cost.

`bounds`
: The `Bounds` on the input and, optionally, the state.

`options`
: `Options`: the horizon and the quadratic-programming settings.

## Returns

A `Plan`.

## Errors

A continuous system, weights of the wrong size, a state weight that is not positive semidefinite, an input weight that is not positive definite, bounds of the wrong width or with a lower bound above the upper one, and a non-positive horizon throw `pluss::Error`.

## Interpretation and Use

Build once and solve every sample: the condensing happens in `build`, and a `Plan` is reused. The first example builds plans of two sizes and reads their dimensions, including a two-state plant; the second applies the same plan in a loop and the state climbs by the input bound (0.4) for two steps and then settles on the reference 1.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The dimensions of plans for a one-state and a two-state plant.

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
  for (const std::int64_t n : {3, 5}) {
    const mpc::Plan plan = mpc::build(plant(), weights(), generous(), with_horizon(n));
    out.writeln("horizon", n, ": plan horizon", plan.horizon(), " states", plan.states(), " inputs", plan.inputs());
  }
  // A two-state, one-input plant.
  const lti::StateSpace two = lti::state(mat({1.0, 1.0, 0.0, 1.0}, 2, 2), mat({0.5, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1), 1.0);
  const mpc::Plan p2 = mpc::build(two, {pluss::series::eye(2, pluss::ScalarType::Float64), mat({0.1}, 1, 1), pluss::series::eye(2, pluss::ScalarType::Float64)}, generous(), with_horizon(5));
  out.writeln("double integrator: states", p2.states(), " inputs", p2.inputs(), " horizon", p2.horizon());
}
```

Output

```text
horizon 3 : plan horizon 3  states 1  inputs 1
horizon 5 : plan horizon 5  states 1  inputs 1
double integrator: states 2  inputs 1  horizon 5
```
### Example 2

One plan solved at every step of a loop.

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
  // The expensive part is building; solving a built plan is cheap and may be repeated every sample.
  const mpc::Plan plan = mpc::build(plant(), weights(), {pluss::make_series({-0.4}), pluss::make_series({0.4})}, with_horizon(4));
  double x = 0.0;
  for (int step = 0; step < 6; ++step) {
    const mpc::Result res = mpc::solve(plan, pluss::make_series({x}), pluss::make_series({1.0}));
    x += res.control.items<double>()[0];
    out.writeln("step", step, ": control", res.control.items<double>()[0], " state", x);
  }
}
```

Output

```text
step 0 : control 0.4000  state 0.4000
step 1 : control 0.4000  state 0.8000
step 2 : control 0.1832  state 0.9832
step 3 : control 0.0154  state 0.9986
step 4 : control 0.0013  state 0.9999
step 5 : control 0.0001  state 1.0000
```
### Example 3

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
  refused(out, "weights with an input weight of the wrong size", [] { mpc::build(plant(), {mat({1.0}, 1, 1), pluss::series::eye(2, pluss::ScalarType::Float64), mat({1.0}, 1, 1)}, generous(), with_horizon(4)); });
  refused(out, "bounds of the wrong size", [] { mpc::build(plant(), weights(), {pluss::make_series({-1.0, -1.0}), pluss::make_series({1.0, 1.0})}, with_horizon(4)); });
}
```

Output

```text
weights with an input weight of the wrong size : eng::control::mpc::solve: weights.input must end in a square 1 by 1 matrix
bounds of the wrong size : eng::control::mpc::solve: bounds.input_lower has the wrong trailing width
```