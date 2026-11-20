<!-- generated from PlaceOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::PlaceOptions`

Numerical controls of pole placement.

## Signature

```cpp
control::PlaceOptions
```

## What It Does

Three settings: how many refinement sweeps to run when there are several inputs, the tolerance for rank decisions, and the largest acceptable condition number of the eigenvector basis.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `sweeps` | Refinement sweeps over the eigenvectors; only used with more than one input. | `5` |
| `tolerance` | Relative tolerance for rank and conjugate-pair decisions. | `1e-10` |
| `condition_limit` | Largest accepted condition number of the assigned basis. | `1e12` |

## Returns

Argument of `place`.

## Errors

A negative `sweeps`, a non-positive `tolerance` and a basis whose condition number exceeds `condition_limit` throw `pluss::Error`.

## Interpretation and Use

With a single input the basis is unique, so the sweeps do nothing (the first example reports zero). With two inputs each sweep reduces the condition number of the basis: from 11.15 with no sweeps to 9.68 after one and 9.60 after five (the second example). Lower `condition_limit` to refuse poor placements early (the third example), or raise it only when a sensitive closed loop is acceptable.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The defaults, and what a single-input problem does with the sweeps.

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
  const ctl::PlaceOptions defaults;
  out.writeln("default sweeps", defaults.sweeps, " tolerance is 1e-10:", defaults.tolerance == 1e-10, " condition limit is 1e12:", defaults.condition_limit == 1e12);
  const ctl::PlaceResult r = ctl::place(A2(), B2(), pluss::make_series({-2.0, -3.0}));
  out.writeln("a single-input problem uses", r.sweeps, "sweeps (the basis is unique); residual below 1e-12:", r.residual.abs().max().item<double>() < 1e-12);
}
```

Output

```text
default sweeps 5  tolerance is 1e-10: true  condition limit is 1e12: true
a single-input problem uses 0 sweeps (the basis is unique); residual below 1e-12: true
```
### Example 2

Refinement sweeps on a two-input problem.

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
  // Four states driven through two inputs: with more than one input the eigenvector basis can be refined, sweep by sweep.
  const pluss::Series a = mat({0, 1, 0, 0, 0, 0, 0.2, 0, 0, 0, 0, 1, 0.1, 0, 0, 0}, 4, 4);
  const pluss::Series b = mat({0, 0, 1, 0, 0, 0, 0, 1}, 4, 2);
  const pluss::Series poles = pluss::make_series({-1.0, -1.5, -2.0, -2.5});
  for (const std::int64_t sweeps : {0, 1, 3, 5}) {
    const ctl::PlaceResult r = ctl::place(a, b, poles, {.sweeps = sweeps});
    out.writeln("sweeps allowed", sweeps, ": sweeps used", r.sweeps, " condition of the basis", r.condition.max().item<double>());
  }
}
```

Output

```text
sweeps allowed 0 : sweeps used 0  condition of the basis 11.1483
sweeps allowed 1 : sweeps used 1  condition of the basis 9.6753
sweeps allowed 3 : sweeps used 3  condition of the basis 9.6052
sweeps allowed 5 : sweeps used 5  condition of the basis 9.6041
```
### Example 3

A condition limit that rejects a close pole pair.

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
  // A very small condition limit refuses a basis that a looser one accepts.
  const pluss::Series poles = pluss::make_series({-1.0, -1.001});
  const ctl::PlaceResult loose = ctl::place(A2(), B2(), poles);
  out.writeln("condition of the eigenvector basis with the default limit:", loose.condition.max().item<double>());
  refused(out, "a condition limit of 10", [&] { ctl::place(A2(), B2(), poles, {.condition_limit = 10.0}); });
}
```

Output

```text
condition of the eigenvector basis with the default limit: 4002.0002
a condition limit of 10 : eng::control::place: the assigned eigenvector basis is ill-conditioned
```