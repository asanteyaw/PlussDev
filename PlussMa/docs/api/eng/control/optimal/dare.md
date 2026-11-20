<!-- generated from dare.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::dare`

Solves the discrete-time algebraic Riccati equation of the linear-quadratic regulator.

## Signature

```cpp
control::RiccatiResult control::dare(
    const pluss::Series& a,
    const pluss::Series& b,
    const pluss::Series& q,
    const pluss::Series& r,
    pluss::ma::linearalg::MatrixEqOptions options = {});

control::RiccatiResult control::dare(
    const pluss::Series& a,
    const pluss::Series& b,
    const pluss::Series& q,
    const pluss::Series& r,
    const pluss::Series& cross,
    pluss::ma::linearalg::MatrixEqOptions options = {});
```

## What It Does

Finds the stabilising `P` of `P = A'PA - (A'PB + N)(R + B'PB)^{-1}(B'PA + N') + Q` and returns it with the optimal feedback `K = (R + B'PB)^{-1}(B'PA + N')` and the closed-loop matrix `A - B K`. The control `u[k] = -K x[k]` minimises `sum(x'Qx + 2x'Nu + u'Ru)`.

## Parameters

`a`, `b`
: The dynamics `[..., n, n]` and input matrix `[..., n, m]`.

`q`
: State weight, symmetric and positive semidefinite.

`r`
: Input weight, symmetric and positive definite.

`cross`
: Optional cross-weight `N`, `[..., n, m]`.

`options`
: `MatrixEqOptions` of the underlying solver.

## Returns

A `RiccatiResult` with `value`, `gain` and `closed_loop`.

## Errors

An `R` that is not positive definite, inconsistent shapes and a pair with no stabilising solution throw `pluss::Error`.

## Interpretation and Use

For `a = b = q = r = 1` the solution is the golden ratio `(1 + sqrt(5)) / 2`, the gain `p / (1 + p)` and the closed loop `1 / (1 + p)`, all of which the first example prints against their closed forms. The closed loop lies inside the unit circle. The second example checks the discrete Riccati equation for a sampled double integrator.

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
  // a = 1, b = 1, q = 1, r = 1: p = 1 + p - p^2 / (1 + p) gives p = (1 + sqrt(5)) / 2, the golden ratio.
  const ctl::RiccatiResult r = ctl::dare(mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1));
  const double golden = (1.0 + std::sqrt(5.0)) / 2.0;
  out.writeln("P =", r.value.flatten().items<double>(), " exact", golden);
  out.writeln("K =", r.gain.flatten().items<double>(), " exact p / (1 + p) =", golden / (1.0 + golden));
  out.writeln("closed loop a - b k =", r.closed_loop.flatten().items<double>(), " exact 1 / (1 + p) =", 1.0 / (1.0 + golden), "(inside the unit circle)");
}
```

Output

```text
P = [1.6180]  exact 1.6180
K = [0.6180]  exact p / (1 + p) = 0.6180
closed loop a - b k = [0.3820]  exact 1 / (1 + p) = 0.3820 (inside the unit circle)
```
### Example 2

The residual of the discrete Riccati equation for a sampled double integrator.

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
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64), r = mat({1.0}, 1, 1);
  const ctl::RiccatiResult sol = ctl::dare(a, b, q, r);
  const pluss::Series &p = sol.value;
  const pluss::Series s = r + b.transpose(0, 1).matmul(p).matmul(b);
  const pluss::Series residual = a.transpose(0, 1).matmul(p).matmul(a) - a.transpose(0, 1).matmul(p).matmul(b).matmul(b.transpose(0, 1).matmul(p).matmul(a)).div(s.flatten().items<double>()[0]) + q - p;
  out.writeln("P =", p.flatten().items<double>());
  out.writeln("largest entry of A'PA - A'PB (R + B'PB)^-1 B'PA + Q - P =", max_abs(residual));
  out.writeln("K =", sol.gain.flatten().items<double>());
}
```

Output

```text
P = [17.8349, 10.0125, 10.0125, 17.8566]
largest entry of A'PA - A'PB (R + B'PB)^-1 B'PA + Q - P = 0.0000
K = [0.9171, 1.6356]
```
### Example 3

A cross-weight and its effect on the gain.

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
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64), r = mat({1.0}, 1, 1), n = mat({0.05, 0.02}, 2, 1);
  const ctl::RiccatiResult plain = ctl::dare(a, b, q, r);
  const ctl::RiccatiResult coupled = ctl::dare(a, b, q, r, n);
  out.writeln("gain without a cross term =", plain.gain.flatten().items<double>());
  out.writeln("gain with the cross term  =", coupled.gain.flatten().items<double>());
}
```

Output

```text
gain without a cross term = [0.9171, 1.6356]
gain with the cross term  = [0.9192, 1.6121]
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
  const pluss::Series a = mat({1.0, 0.1, 0.0, 1.0}, 2, 2), b = mat({0.005, 0.1}, 2, 1);
  refused(out, "R that is not positive definite", [&] { ctl::dare(a, b, q, mat({-1.0}, 1, 1)); });
  refused(out, "a pair that cannot be stabilised", [&] { ctl::dare(mat({2.0, 0.0, 0.0, 2.0}, 2, 2), mat({1.0, 0.0}, 2, 1), q, mat({1.0}, 1, 1)); });
  refused(out, "B with the wrong number of rows", [&] { ctl::dare(a, mat({1.0, 0.0, 0.0}, 3, 1), q, mat({1.0}, 1, 1)); });
}
```

Output

```text
R that is not positive definite : eng::control::dare: R must be positive definite
a pair that cannot be stabilised : eng::control::dare: no stabilising solution; (A, B) must be stabilisable (every unstable mode reachable from the input) and the states weighted by Q must be detectable. Solver message: subsp::linearalg::dare: doubling iteration produced non-finite values
B with the wrong number of rows : eng::control::dare: B rows must equal the state count
```