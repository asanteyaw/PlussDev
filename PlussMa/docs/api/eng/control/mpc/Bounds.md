<!-- generated from Bounds.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::Bounds`

The input and state limits of the predictive-control problem.

## Signature

```cpp
control::mpc::Bounds
```

## What It Does

Limits the input at every step and, optionally, the state. The optimiser keeps every predicted input and state inside them.

## Fields

| Field | Meaning |
|---|---|
| `input_lower`, `input_upper` | Per-input limits, `[inputs]`, with lower not above upper. |
| `state_lower`, `state_upper` | Optional per-state limits, `[states]`. |

## Returns

Member of the argument list of `build` and `solve`.

## Errors

A lower bound above the upper bound and bounds of the wrong width throw `pluss::Error`.

## Interpretation and Use

The first example limits the input to `[-0.4, 0.4]`: the plan that would have moved 0.92 at once now moves 0.4 twice and then eases in (0.18, 0.016). A state bound of 0.5 keeps every predicted state at or below 0.5 (the second example). The third combines both bounds over a longer horizon and the problem remains solvable: the plan reaches the state limit and holds it. Choose bounds from the actuator and the physical limits, not as a way to shape the response.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

An input bound against no bound.

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
  const mpc::Result free = run(0.1, generous());
  const mpc::Result clipped = run(0.1, {pluss::make_series({-0.4}), pluss::make_series({0.4})});
  out.writeln("without a tight bound: controls", free.controls.flatten().items<double>());
  out.writeln("with the input in [-0.4, 0.4]: controls", clipped.controls.flatten().items<double>());
}
```

Output

```text
without a tight bound: controls [0.9161, 0.0769, 0.0065, 0.0006]
with the input in [-0.4, 0.4]: controls [0.4000, 0.4000, 0.1833, 0.0159]
```
### Example 2

A state bound.

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
  const mpc::Bounds limited{pluss::make_series({-10.0}), pluss::make_series({10.0}), pluss::make_series({-2.0}), pluss::make_series({0.5})};
  const mpc::Result res = run(0.1, limited);
  const auto s = res.states.flatten().items<double>();
  out.writeln("states with an upper bound of 0.5 =", s, " largest", *std::max_element(s.begin(), s.end()));
  out.writeln("the free solution reaches", run(0.1, generous()).states.flatten().items<double>().back());
}
```

Output

```text
states with an upper bound of 0.5 = [0.5000, 0.5000, 0.5000, 0.5000]  largest 0.5000
the free solution reaches 1.0000
```
### Example 3

Input and state bounds together.

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
  // x cannot reach 1 in one step when the input is limited to 0.4 and the state to 0.5, but the problem is still solvable: the bounds hold.
  const mpc::Bounds tight{pluss::make_series({-0.4}), pluss::make_series({0.4}), pluss::make_series({-0.5}), pluss::make_series({0.5})};
  const mpc::Result res = run(0.1, tight, 6);
  out.writeln("converged:", res.optimization.converged, " controls", res.controls.flatten().items<double>());
  out.writeln("states", res.states.flatten().items<double>());
}
```

Output

```text
converged: true  controls [0.4000, 0.1000, -0.0000, 0.0000, -0.0000, 0.0000]
states [0.4000, 0.5000, 0.5000, 0.5000, 0.5000, 0.5000]
```
### Example 4

Every refused bound.

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
  refused(out, "a lower input bound above the upper one", [] { mpc::build(plant(), weights(), {pluss::make_series({0.5}), pluss::make_series({-0.5})}, with_horizon(4)); });
  refused(out, "input bounds with two entries for one input", [] { mpc::build(plant(), weights(), {pluss::make_series({-1.0, -1.0}), pluss::make_series({1.0, 1.0})}, with_horizon(4)); });
  refused(out, "a lower state bound above the upper one", [] { mpc::build(plant(), weights(), {pluss::make_series({-1.0}), pluss::make_series({1.0}), pluss::make_series({2.0}), pluss::make_series({1.0})}, with_horizon(4)); });
}
```

Output

```text
a lower input bound above the upper one : eng::control::mpc::solve: bounds.input_lower must not exceed bounds.input_upper
input bounds with two entries for one input : eng::control::mpc::solve: bounds.input_lower has the wrong trailing width
a lower state bound above the upper one : eng::control::mpc::solve: bounds.state_lower must not exceed bounds.state_upper
```