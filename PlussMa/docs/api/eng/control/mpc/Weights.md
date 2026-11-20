<!-- generated from Weights.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::Weights`

The cost weights of the predictive-control problem.

## Signature

```cpp
control::mpc::Weights
```

## What It Does

The cost over the horizon is the sum of `(x - r)' Q (x - r) + u' R u` over the steps plus a terminal term `(x_N - r)' P (x_N - r)`. `Weights` holds `Q`, `R` and `P`.

## Fields

| Field | Meaning |
|---|---|
| `state` | `Q`, `[..., n, n]`, symmetric positive semidefinite: the cost of tracking error. |
| `input` | `R`, `[..., m, m]`, symmetric positive definite: the cost of control effort. |
| `terminal` | `P`, `[..., n, n]`, optional: the cost of the final tracking error; when omitted the state weight `Q` is used. |

## Returns

Member of the argument list of `build` and `solve`.

## Errors

A state weight that is not positive semidefinite, an input weight that is not positive definite and weights of the wrong size throw `pluss::Error` when the plan is built.

## Interpretation and Use

A heavier `R` makes effort costly: with `R = 10` the first move is 0.25 and the state reaches only 0.62 by the end of the horizon, against 0.99 and 1.00 for `R = 0.01` (the first example). A heavier `Q` tracks harder (the third example): the first move rises from 0.62 to 0.92 to 0.99 as `Q` goes from 0.1 to 1 to 10. The terminal weight pulls the end of the horizon onto the reference (the second example).

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The effect of the input weight.

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
  // A heavier input weight makes effort costly: a gentler first move.
  for (const double r : {0.01, 0.1, 10.0}) {
    const mpc::Result res = run(r);
    out.writeln("input weight", r, ": first control", res.control.items<double>()[0], " final predicted state", res.states.flatten().items<double>().back());
  }
}
```

Output

```text
input weight 0.0100 : first control 0.9902  final predicted state 1.0000
input weight 0.1000 : first control 0.9161  final predicted state 1.0000
input weight 10.0000 : first control 0.2520  final predicted state 0.6245
```
### Example 2

The effect of the terminal weight.

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
  // The terminal weight pulls the end of the horizon toward the reference.
  for (const double t : {0.0, 2.0, 50.0}) {
    const mpc::Result res = run(0.1, generous(), 4, t);
    out.writeln("terminal weight", t, ": state at the end of the horizon", res.states.flatten().items<double>().back(), "(reference 1)");
  }
}
```

Output

```text
terminal weight 0.0000 : state at the end of the horizon 0.9994 (reference 1)
terminal weight 2.0000 : state at the end of the horizon 1.0000 (reference 1)
terminal weight 50.0000 : state at the end of the horizon 1.0000 (reference 1)
```
### Example 3

The effect of the state weight.

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
  // A heavier state weight tracks the reference harder.
  for (const double q : {0.1, 1.0, 10.0}) {
    const mpc::Weights wts{mat({q}, 1, 1), mat({0.1}, 1, 1), mat({0.0}, 1, 1)};
    const mpc::Result res = mpc::solve(plant(), pluss::make_series({0.0}), pluss::make_series({1.0}), wts, generous(), with_horizon(4));
    out.writeln("state weight", q, ": first control", res.control.items<double>()[0], " states", res.states.flatten().items<double>());
  }
}
```

Output

```text
state weight 0.1000 : first control 0.6154  states [0.6154, 0.8462, 0.9231, 0.9231]
state weight 1.0000 : first control 0.9161  states [0.9161, 0.9930, 0.9994, 0.9994]
state weight 10.0000 : first control 0.9902  states [0.9902, 0.9999, 1.0000, 1.0000]
```
### Example 4

Every refused weight.

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
  refused(out, "a negative state weight", [] { mpc::build(plant(), {mat({-1.0}, 1, 1), mat({0.1}, 1, 1), mat({0.0}, 1, 1)}, generous(), with_horizon(4)); });
  refused(out, "an input weight that is not positive definite", [] { mpc::build(plant(), {mat({1.0}, 1, 1), mat({0.0}, 1, 1), mat({0.0}, 1, 1)}, generous(), with_horizon(4)); });
  refused(out, "a state weight of the wrong size", [] { mpc::build(plant(), {pluss::series::eye(2, pluss::ScalarType::Float64), mat({0.1}, 1, 1), mat({0.0}, 1, 1)}, generous(), with_horizon(4)); });
}
```

Output

```text
a negative state weight : eng::control::mpc::solve: weights.state must be positive semidefinite
an input weight that is not positive definite : eng::control::mpc::solve: weights.input must be positive definite
a state weight of the wrong size : eng::control::mpc::solve: weights.state must end in a square 1 by 1 matrix
```