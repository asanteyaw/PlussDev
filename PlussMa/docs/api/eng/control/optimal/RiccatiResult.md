<!-- generated from RiccatiResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::RiccatiResult`

The solution, gain and closed loop of a Riccati problem.

## Signature

```cpp
control::RiccatiResult
```

## What It Does

Returned by `care`, `dare` and `lqr`. It holds the Riccati solution `P` (the optimal cost-to-go matrix), the feedback gain `K` and the closed-loop matrix `A - B K`.

## Fields

| Field | Meaning | Shape |
|---|---|---|
| `value` | The stabilising Riccati solution `P`; the cost from state `x` is `x' P x`. | `[..., n, n]` |
| `gain` | The optimal feedback `K`, so that `u = -K x`. | `[..., m, n]` |
| `closed_loop` | `A - B K` (continuous) or the discrete equivalent. | `[..., n, n]` |

## Returns

Returned by `care`, `dare` and `lqr`.

## Errors

Reading a field does not run again.

## Interpretation and Use

`gain` equals `R^{-1} B' P` and `closed_loop` equals `A - B K` (the second example checks both to rounding). A larger input weight `R` makes control expensive and shrinks the gain: for the double integrator the largest gain entry falls from 4.04 to 1.73 to 0.86 as `R` goes from 0.1 to 1 to 10 (the third example).

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The three fields and their shapes.

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
  const ctl::RiccatiResult sol = ctl::lqr(system, q, r);
  out.writeln("value  (P)  shape", sol.value.size(0), "x", sol.value.size(1), "=", sol.value.flatten().items<double>());
  out.writeln("gain   (K)  shape", sol.gain.size(0), "x", sol.gain.size(1), "=", sol.gain.flatten().items<double>());
  out.writeln("closed (A - B K) shape", sol.closed_loop.size(0), "x", sol.closed_loop.size(1), "=", sol.closed_loop.flatten().items<double>());
}
```

Output

```text
value  (P)  shape 2 x 2 = [1.7321, 1.0000, 1.0000, 1.7321]
gain   (K)  shape 1 x 2 = [1.0000, 1.7321]
closed (A - B K) shape 2 x 2 = [0.0000, 1.0000, -1.0000, -1.7321]
```
### Example 2

The relations between the fields.

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
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64), r = mat({4.0}, 1, 1);
  const ctl::RiccatiResult sol = ctl::care(A2(), B2(), q, r);
  const pluss::Series k = B2().transpose(0, 1).matmul(sol.value).div(4.0);
  out.writeln("K - R^-1 B' P, largest entry =", max_abs(sol.gain - k));
  out.writeln("closed loop - (A - B K), largest entry =", max_abs(sol.closed_loop - (A2() - B2().matmul(sol.gain))));
}
```

Output

```text
K - R^-1 B' P, largest entry = 0.0000
closed loop - (A - B K), largest entry = 0.0000
```
### Example 3

How the input weight changes the gain.

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
  // A heavier input weight R makes control expensive: a smaller gain and a slower closed loop.
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64);
  for (const double r : {0.1, 1.0, 10.0}) {
    const ctl::RiccatiResult sol = ctl::care(A2(), B2(), q, mat({r}, 1, 1));
    const auto k = sol.gain.flatten().items<double>();
    out.writeln("R =", r, ": gain", k, " largest gain entry", std::max(std::abs(k[0]), std::abs(k[1])));
  }
}
```

Output

```text
R = 0.1000 : gain [3.1623, 4.0404]  largest gain entry 4.0404
R = 1.0000 : gain [1.0000, 1.7321]  largest gain entry 1.7321
R = 10.0000 : gain [0.3162, 0.8558]  largest gain entry 0.8558
```