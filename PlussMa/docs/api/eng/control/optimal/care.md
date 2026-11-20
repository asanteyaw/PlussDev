<!-- generated from care.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::care`

Solves the continuous-time algebraic Riccati equation of the linear-quadratic regulator.

## Signature

```cpp
control::RiccatiResult control::care(
    const pluss::Series& a,
    const pluss::Series& b,
    const pluss::Series& q,
    const pluss::Series& r,
    pluss::ma::linearalg::MatrixEqOptions options = {});

control::RiccatiResult control::care(
    const pluss::Series& a,
    const pluss::Series& b,
    const pluss::Series& q,
    const pluss::Series& r,
    const pluss::Series& cross,
    pluss::ma::linearalg::MatrixEqOptions options = {});
```

## What It Does

Finds the stabilising `P` of `A'P + PA - (P B + N) R^{-1} (B'P + N') + Q = 0` and returns it with the optimal feedback `K = R^{-1}(B'P + N')` and the closed-loop matrix `A - B K`. With the cross-weight `N` omitted the equation is the classic `A'P + PA - P B R^{-1} B' P + Q = 0`. The control `u = -K x` minimises the cost `integral(x'Qx + 2x'Nu + u'Ru) dt`.

## Parameters

`a`, `b`
: The dynamics `[..., n, n]` and input matrix `[..., n, m]`.

`q`
: State weight `[..., n, n]`, symmetric and positive semidefinite.

`r`
: Input weight `[..., m, m]`, symmetric and positive definite.

`cross`
: Optional cross-weight `N`, `[..., n, m]`.

`options`
: `MatrixEqOptions` of the underlying solver.

## Returns

A `RiccatiResult` with `value` (`P`), `gain` (`K`) and `closed_loop` (`A - B K`).

## Errors

An `R` that is not positive definite, a cross-weight that makes the joint cost `[Q, N; N', R]` indefinite, matrices of inconsistent shape, and a pair with no stabilising solution (an unstable mode the input cannot reach, or one the weight `Q` does not see) throw `pluss::Error`. The last message names the conditions and appends the solver's own message.

## Interpretation and Use

For `a = b = q = r = 1` the equation is `2p - p^2 + 1 = 0` and the stabilising root is `1 + sqrt(2)`, which the first example reproduces; the closed loop is `-sqrt(2)`. The second example checks the Riccati equation itself for a double integrator: its residual is at rounding level and `P` is symmetric. A cross-weight shifts the gain (the third example).

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A scalar problem against its closed form.

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
  // a = 1, b = 1, q = 1, r = 1: the Riccati equation 2 p - p^2 + 1 = 0 has the stabilising root p = 1 + sqrt(2).
  const ctl::RiccatiResult r = ctl::care(mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1));
  out.writeln("P =", r.value.flatten().items<double>(), " exact 1 + sqrt(2) =", 1.0 + std::sqrt(2.0));
  out.writeln("K = R^-1 B' P =", r.gain.flatten().items<double>(), " closed loop a - b k =", r.closed_loop.flatten().items<double>(), " exact -sqrt(2) =", -std::sqrt(2.0));
}
```

Output

```text
P = [2.4142]  exact 1 + sqrt(2) = 2.4142
K = R^-1 B' P = [2.4142]  closed loop a - b k = [-1.4142]  exact -sqrt(2) = -1.4142
```
### Example 2

The residual of the Riccati equation for a double integrator.

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
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64);
  const pluss::Series r = mat({1.0}, 1, 1);
  const ctl::RiccatiResult sol = ctl::care(A2(), B2(), q, r);
  const pluss::Series &p = sol.value;
  const pluss::Series residual = A2().transpose(0, 1).matmul(p) + p.matmul(A2()) - p.matmul(B2()).matmul(B2().transpose(0, 1)).matmul(p) + q;
  out.writeln("P =", p.flatten().items<double>());
  out.writeln("largest entry of A'P + PA - P B R^-1 B' P + Q =", max_abs(residual));
  out.writeln("P is symmetric: largest |P - P'| =", max_abs(p - p.transpose(0, 1)));
  const auto k = sol.gain.flatten().items<double>();
  out.writeln("K =", k, " closed-loop trace =", sol.closed_loop.flatten().items<double>()[0] + sol.closed_loop.flatten().items<double>()[3], "(negative: both eigenvalues in the left half-plane for this system)");
}
```

Output

```text
P = [1.7321, 1.0000, 1.0000, 1.7321]
largest entry of A'P + PA - P B R^-1 B' P + Q = 0.0000
P is symmetric: largest |P - P'| = 0.0000
K = [1.0000, 1.7321]  closed-loop trace = -1.7321 (negative: both eigenvalues in the left half-plane for this system)
```
### Example 3

A cross-weight `N` and its effect on the gain.

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
  // A cross-weight N couples state and input in the cost x'Qx + 2 x'N u + u'Ru.
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64);
  const pluss::Series r = mat({1.0}, 1, 1);
  const pluss::Series n = mat({0.2, 0.1}, 2, 1);
  const ctl::RiccatiResult plain = ctl::care(A2(), B2(), q, r);
  const ctl::RiccatiResult coupled = ctl::care(A2(), B2(), q, r, n);
  out.writeln("gain without a cross term =", plain.gain.flatten().items<double>());
  out.writeln("gain with the cross term  =", coupled.gain.flatten().items<double>());
  const pluss::Series &p = coupled.value;
  const pluss::Series residual = A2().transpose(0, 1).matmul(p) + p.matmul(A2()) - (p.matmul(B2()) + n).matmul(B2().transpose(0, 1).matmul(p) + n.transpose(0, 1)) + q;
  out.writeln("largest entry of the cross-term Riccati residual =", max_abs(residual));
}
```

Output

```text
gain without a cross term = [1.0000, 1.7321]
gain with the cross term  = [1.0000, 1.6125]
largest entry of the cross-term Riccati residual = 0.0000
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
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64);
  refused(out, "R that is not positive definite", [&] { ctl::care(A2(), B2(), q, mat({-1.0}, 1, 1)); });
  refused(out, "a pair that cannot be stabilised", [&] { ctl::care(mat({1.0, 0.0, 0.0, 2.0}, 2, 2), mat({1.0, 0.0}, 2, 1), q, mat({1.0}, 1, 1)); });
  refused(out, "B with the wrong number of rows", [&] { ctl::care(A2(), mat({1.0, 0.0, 0.0}, 3, 1), q, mat({1.0}, 1, 1)); });
  refused(out, "a cross weight that makes the joint cost indefinite", [&] { ctl::care(A2(), B2(), q, mat({1.0}, 1, 1), mat({5.0, 5.0}, 2, 1)); });
  refused(out, "Q of the wrong size", [&] { ctl::care(A2(), B2(), mat({1.0}, 1, 1), mat({1.0}, 1, 1)); });
}
```

Output

```text
R that is not positive definite : eng::control::care: R must be positive definite
a pair that cannot be stabilised : eng::control::care: no stabilising solution; (A, B) must be stabilisable (every unstable mode reachable from the input) and the states weighted by Q must be detectable. Solver message: subsp::math::linalg::lu_solve: singular matrix
B with the wrong number of rows : eng::control::care: B rows must equal the state count
a cross weight that makes the joint cost indefinite : eng::control::care: the joint [Q, cross; cross^H, R] cost must be positive semidefinite
Q of the wrong size : eng::control::care: Q must have shape [..., states, states]
```