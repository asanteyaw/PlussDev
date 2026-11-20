<!-- generated from place.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::place`

Assigns the closed-loop poles of a state-feedback loop.

## Signature

```cpp
control::PlaceResult control::place(
    const pluss::Series& a,
    const pluss::Series& b,
    const pluss::Series& poles,
    control::PlaceOptions options = {});

control::PlaceResult control::place(
    const lti::StateSpace& system,
    const pluss::Series& poles,
    control::PlaceOptions options = {});
```

## What It Does

Finds a gain `K` such that the eigenvalues of `A - B K` are the requested `poles`. For a single input the gain is unique; with several inputs the freedom is used to make the closed-loop eigenvectors well conditioned, refining them over a few sweeps. There is an overload for `(A, B)` and one for a `StateSpace`.

## Parameters

`a`, `b`
: The dynamics and input matrix, or `system` for the `StateSpace` overload.

`poles`
: One desired pole per state. For a real system, complex poles must come in conjugate pairs.

`options`
: `PlaceOptions`: refinement sweeps, tolerance and condition limit.

## Returns

A `PlaceResult`: `gain`, `closed_loop`, `basis`, `residual`, `condition` and `sweeps`.

## Errors

A pair that is not controllable, an unpaired complex pole for a real system, the wrong number of poles, a non-finite pole and an ill-conditioned eigenvector basis throw `pluss::Error`.

## Interpretation and Use

For the double integrator `x'' = u` and poles at -2 and -3 the characteristic polynomial is `s^2 + 5 s + 6`, the gain `[6, 5]`, and the closed-loop trace and determinant are -5 and 6 (the first example). A conjugate pair at `-1 +- j` gives `[2, 2]` (the second). The `StateSpace` overload returns the same gain (the third).

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Two real poles for a double integrator.

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
  // x'' = u: place the closed-loop poles at -2 and -3, so det(sI - A + BK) = s^2 + 5 s + 6.
  const pluss::Series poles = pluss::make_series({-2.0, -3.0});
  const ctl::PlaceResult r = ctl::place(A2(), B2(), poles);
  out.writeln("gain K =", r.gain.flatten().items<double>(), " expected [6, 5]");
  const auto m = r.closed_loop.flatten().items<double>();
  out.writeln("closed loop trace =", m[0] + m[3], " (sum of the poles -5)  determinant =", m[0] * m[3] - m[1] * m[2], " (product of the poles 6)");
}
```

Output

```text
gain K = [6.0000, 5.0000]  expected [6, 5]
closed loop trace = -5.0000  (sum of the poles -5)  determinant = 6.0000  (product of the poles 6)
```
### Example 2

A conjugate pole pair.

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
  // A conjugate pair at -1 +- j: s^2 + 2 s + 2.
  const pluss::Series poles = pluss::make_series({std::complex<double>(-1.0, 1.0), std::complex<double>(-1.0, -1.0)});
  const ctl::PlaceResult r = ctl::place(A2(), B2(), poles);
  const auto m = r.closed_loop.flatten().items<double>();
  out.writeln("gain K =", r.gain.flatten().items<double>(), " expected [2, 2]");
  out.writeln("closed loop trace =", m[0] + m[3], " determinant =", m[0] * m[3] - m[1] * m[2], " (expected -2 and 2)");
}
```

Output

```text
gain K = [2.0000, 2.0000]  expected [2, 2]
closed loop trace = -2.0000  determinant = 2.0000  (expected -2 and 2)
```
### Example 3

The `StateSpace` overload against the matrix overload.

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
  const pluss::Series poles = pluss::make_series({-2.0, -3.0});
  const ctl::PlaceResult a = ctl::place(system, poles);
  const ctl::PlaceResult b = ctl::place(A2(), B2(), poles);
  out.writeln("StateSpace overload gain =", a.gain.flatten().items<double>(), " matrix overload gain =", b.gain.flatten().items<double>());
}
```

Output

```text
StateSpace overload gain = [6.0000, 5.0000]  matrix overload gain = [6.0000, 5.0000]
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
  refused(out, "a pair that is not controllable", [&] { ctl::place(mat({1.0, 0.0, 0.0, 2.0}, 2, 2), mat({1.0, 0.0}, 2, 1), pluss::make_series({-1.0, -2.0})); });
  refused(out, "one complex pole without its conjugate", [&] { ctl::place(A2(), B2(), pluss::make_series({std::complex<double>(-1.0, 1.0), std::complex<double>(-2.0, 0.0)})); });
  refused(out, "the wrong number of poles", [&] { ctl::place(A2(), B2(), pluss::make_series({-1.0, -2.0, -3.0})); });
  refused(out, "a non-finite pole", [&] { ctl::place(A2(), B2(), pluss::make_series({-1.0, std::nan("")})); });
}
```

Output

```text
a pair that is not controllable : eng::control::place: (A, B) must be controllable
one complex pole without its conjugate : eng::control::place: complex poles for a real system must appear in conjugate pairs
the wrong number of poles : eng::control::place: poles must be a vector with one value per state
a non-finite pole : eng::control::place: inputs must contain only finite values
```