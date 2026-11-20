<!-- generated from lqr.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::lqr`

Computes the optimal state-feedback gain of a state-space system.

## Signature

```cpp
control::RiccatiResult control::lqr(
    const lti::StateSpace& system,
    const pluss::Series& q,
    const pluss::Series& r,
    pluss::ma::linearalg::MatrixEqOptions options = {});

control::RiccatiResult control::lqr(
    const lti::StateSpace& system,
    const pluss::Series& q,
    const pluss::Series& r,
    const pluss::Series& cross,
    pluss::ma::linearalg::MatrixEqOptions options = {});
```

## What It Does

Chooses `care` for a continuous system and `dare` for a discrete one from the sample time of `system`, and returns the same `RiccatiResult`. It is the convenience entry point: pass the model and the two weights.

## Parameters

`system`
: A `StateSpace`; continuous systems solve the continuous equation and discrete ones the discrete equation.

`q`, `r`
: The state and input weights.

`cross`
: Optional cross-weight `N`.

`options`
: `MatrixEqOptions` of the underlying solver.

## Returns

A `RiccatiResult`.

## Errors

The errors of `care` and `dare`.

## Interpretation and Use

The first two examples show that `lqr` gives exactly the `care` gain for a continuous system and the `dare` gain for a discrete one. The third checks optimality directly: from the initial state `(1, 0)` the integrated closed-loop cost is `1.7321`, equal to `x0' P x0`. Choose `Q` and `R` by what each state and each unit of effort should cost.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A continuous system against `care`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const lti::StateSpace system = lti::state(A2(), B2(), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1));
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64), r = mat({1.0}, 1, 1);
  const ctl::RiccatiResult from_system = ctl::lqr(system, q, r);
  const ctl::RiccatiResult from_matrices = ctl::care(A2(), B2(), q, r);
  out.writeln("lqr gain  =", from_system.gain.flatten().items<double>());
  out.writeln("care gain =", from_matrices.gain.flatten().items<double>());
  out.writeln("largest difference =", max_abs(from_system.gain - from_matrices.gain), " (a continuous system solves the continuous Riccati equation)");
}
```

Output

```text
lqr gain  = [1.0000, 1.7321]
care gain = [1.0000, 1.7321]
largest difference = 0.0000  (a continuous system solves the continuous Riccati equation)
```
### Example 2

A discrete system against `dare`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series a = mat({1.0, 0.1, 0.0, 1.0}, 2, 2), b = mat({0.005, 0.1}, 2, 1);
  const lti::StateSpace system = lti::state(a, b, mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1), 0.1);
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64), r = mat({1.0}, 1, 1);
  const ctl::RiccatiResult from_system = ctl::lqr(system, q, r);
  const ctl::RiccatiResult from_matrices = ctl::dare(a, b, q, r);
  out.writeln("lqr gain  =", from_system.gain.flatten().items<double>());
  out.writeln("dare gain =", from_matrices.gain.flatten().items<double>());
  out.writeln("largest difference =", max_abs(from_system.gain - from_matrices.gain), " (a discrete system solves the discrete Riccati equation)");
}
```

Output

```text
lqr gain  = [0.9171, 1.6356]
dare gain = [0.9171, 1.6356]
largest difference = 0.0000  (a discrete system solves the discrete Riccati equation)
```
### Example 3

The simulated closed-loop cost against `x0' P x0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // The optimal cost from x0 is x0' P x0. Simulate the closed loop and integrate the cost to check it.
  const lti::StateSpace system = lti::state(A2(), B2(), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1));
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64), r = mat({1.0}, 1, 1);
  const ctl::RiccatiResult sol = ctl::lqr(system, q, r);
  const lti::StateSpace closed = lti::state(sol.closed_loop, B2(), pluss::series::eye(2, pluss::ScalarType::Float64), pluss::series::zeros({2, 1}, pluss::ScalarType::Float64));
  const std::int64_t n = 8001;
  std::vector<double> t(n);
  for (std::int64_t i = 0; i < n; ++i) t[i] = 0.005 * i;
  const lti::SimRun run = lti::sim(closed, pluss::series::zeros({n, 1}, pluss::ScalarType::Float64), pluss::make_series(t), pluss::make_series({1.0, 0.0}));
  const pluss::Series x = run.states;
  const pluss::Series u = -x.matmul(sol.gain.transpose(0, 1));
  const pluss::Series stage = (x * x).sum(1) + (u * u).sum(1);
  const double cost = ((stage.slice(0, 0, n - 1) + stage.slice(0, 1, n)) * 0.5 * 0.005).sum().item<double>();
  out.writeln("x0 = (1, 0): cost integrated over 40 s =", cost, " x0' P x0 =", sol.value.flatten().items<double>()[0]);
}
```

Output

```text
x0 = (1, 0): cost integrated over 40 s = 1.7321  x0' P x0 = 1.7321
```
### Example 4

Every refused problem.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const lti::StateSpace system = lti::state(A2(), B2(), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1));
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64);
  refused(out, "R that is not positive definite", [&] { ctl::lqr(system, q, mat({0.0}, 1, 1)); });
  refused(out, "Q of the wrong size", [&] { ctl::lqr(system, mat({1.0}, 1, 1), mat({1.0}, 1, 1)); });
  const lti::StateSpace stuck = lti::state(mat({1.0, 0.0, 0.0, 2.0}, 2, 2), mat({1.0, 0.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1));
  refused(out, "a system that cannot be stabilised", [&] { ctl::lqr(stuck, q, mat({1.0}, 1, 1)); });
}
```

Output

```text
R that is not positive definite : eng::control::care: R must be positive definite
Q of the wrong size : eng::control::care: Q must have shape [..., states, states]
a system that cannot be stabilised : eng::control::care: no stabilising solution; (A, B) must be stabilisable (every unstable mode reachable from the input) and the states weighted by Q must be detectable. Solver message: subsp::math::linalg::lu_solve: singular matrix
```