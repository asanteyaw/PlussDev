<!-- generated from PlaceResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::PlaceResult`

The gain, closed loop and diagnostics of a pole placement.

## Signature

```cpp
control::PlaceResult
```

## What It Does

Returned by `place`. Besides the gain it reports how the assignment was achieved and how trustworthy it is.

## Fields

| Field | Meaning |
|---|---|
| `gain` | The state-feedback gain `K`. |
| `closed_loop` | `A - B K`, the matrix whose spectrum was assigned. |
| `basis` | The eigenvector basis used to realise the poles. |
| `residual` | Agreement between the closed-loop spectrum and the request. |
| `condition` | Condition number of `basis`: a large value means a sensitive closed loop. |
| `sweeps` | Refinement sweeps actually completed. |

## Returns

Returned by `place`.

## Errors

Reading a field does not run again.

## Interpretation and Use

Check `condition` before trusting a gain: placing poles close together makes the eigenvector basis nearly dependent. The third example places two poles 2 apart, 0.01 apart and 0.0001 apart and the condition number rises from 7 to 1,004 to 100,004, even though the residual stays at rounding level. A tiny residual alone does not mean the closed loop is robust.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The fields and their shapes.

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
  const ctl::PlaceResult r = ctl::place(A2(), B2(), pluss::make_series({-2.0, -3.0}));
  out.writeln("gain shape", r.gain.size(0), "x", r.gain.size(1), " closed loop shape", r.closed_loop.size(0), "x", r.closed_loop.size(1), " basis shape", r.basis.size(0), "x", r.basis.size(1));
  out.writeln("residual", r.residual.abs().max().item<double>(), " condition", r.condition.max().item<double>(), " sweeps", r.sweeps);
}
```

Output

```text
gain shape 1 x 2  closed loop shape 2 x 2  basis shape 2 x 2
residual 0.0000  condition 14.0711  sweeps 0
```
### Example 2

`closed_loop` against `A - B K`.

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
  const ctl::PlaceResult r = ctl::place(A2(), B2(), pluss::make_series({-2.0, -3.0}));
  out.writeln("closed loop - (A - B K), largest entry =", max_abs(r.closed_loop - (A2() - B2().matmul(r.gain))));
}
```

Output

```text
closed loop - (A - B K), largest entry = 0.0000
```
### Example 3

The condition number for well separated and close poles.

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
  const struct { const char *label; double p1, p2; } cases[] = {{"well separated  ", -2.0, -5.0}, {"close together  ", -2.0, -2.01}, {"very close      ", -2.0, -2.0001}};
  for (const auto &c : cases) {
    const ctl::PlaceResult r = ctl::place(A2(), B2(), pluss::make_series({c.p1, c.p2}));
    out.writeln(c.label, "poles", c.p1, c.p2, ": condition of the basis", r.condition.max().item<double>(), " residual", r.residual.abs().max().item<double>());
  }
}
```

Output

```text
well separated   poles -2.0000 -5.0000 : condition of the basis 7.4673  residual 0.0000
close together   poles -2.0000 -2.0100 : condition of the basis 1004.0010  residual 0.0000
very close       poles -2.0000 -2.0001 : condition of the basis 100004.0000  residual 0.0000
```