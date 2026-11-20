<!-- generated from obsv.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::obsv`

Builds the observability matrix.

## Signature

```cpp
pluss::Series lti::obsv(const lti::StateSpace& system);
```

## What It Does

Returns the stack `[C; C A; C A^2; ...; C A^{n-1}]`, an `[n p, n]` matrix. The initial state can be recovered from the outputs exactly when this matrix has full column rank `n`.

## Parameters

`system`
: A `StateSpace`.

## Returns

A `Series` with matrix shape `[n p, n]` and the batch axes of the system.

## Errors

Construction errors are raised by `state`; judging the rank is left to the caller.

## Interpretation and Use

Observability is the dual of controllability: the observability matrix of `(A, C)` is the transpose of the controllability matrix of `(A', C')` (the third example). A state that the output does not depend on, directly or through the dynamics, makes the determinant zero (the first example).

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A system whose states are all visible and one with an invisible state.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
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

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto det2 = [](const pluss::Series &m) { const auto v = m.flatten().items<double>(); return v[0] * v[3] - v[1] * v[2]; };
  const lti::StateSpace observable = oscillator();
  out.writeln("observability matrix [C; CA] =", lti::obsv(observable).flatten().items<double>(), " determinant", det2(lti::obsv(observable)));
  // Only the first state is measured and it does not depend on the second: the second is invisible.
  const lti::StateSpace hidden = lti::state(mat({-1.0, 0.0, 0.0, -2.0}, 2, 2), mat({1.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1));
  out.writeln("invisible state: [C; CA] =", lti::obsv(hidden).flatten().items<double>(), " determinant", det2(lti::obsv(hidden)), "(zero: not observable)");
}
```

Output

```text
observability matrix [C; CA] = [1.0000, 0.0000, 0.0000, 1.0000]  determinant 1.0000
invisible state: [C; CA] = [1.0000, 0.0000, -1.0000, 0.0000]  determinant 0.0000 (zero: not observable)
```
### Example 2

The shape for one and for several outputs.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
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

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const lti::StateSpace siso = oscillator();
  const pluss::Series o1 = lti::obsv(siso);
  out.writeln("two states, one output:", o1.size(0), "x", o1.size(1));
  const lti::StateSpace two_outputs = lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0, 0.0, 1.0}, 2, 2), mat({0.0, 0.0}, 2, 1));
  const pluss::Series o2 = lti::obsv(two_outputs);
  out.writeln("two states, two outputs:", o2.size(0), "x", o2.size(1), "(states times outputs by states)");
}
```

Output

```text
two states, one output: 2 x 2
two states, two outputs: 4 x 2 (states times outputs by states)
```
### Example 3

The duality with `ctrb`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
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

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // Observability of (A, C) is controllability of (A', C'): the observability matrix is the transpose of the controllability matrix of the dual.
  const lti::StateSpace s = oscillator();
  const lti::StateSpace dual = lti::state(s.a().transpose(0, 1), s.c().transpose(0, 1), s.b().transpose(0, 1), s.d().transpose(0, 1));
  out.writeln("obsv(system)       =", lti::obsv(s).flatten().items<double>());
  out.writeln("ctrb(dual)^T       =", lti::ctrb(dual).transpose(0, 1).flatten().items<double>());
}
```

Output

```text
obsv(system)       = [1.0000, 0.0000, 0.0000, 1.0000]
ctrb(dual)^T       = [1.0000, 0.0000, 0.0000, 1.0000]
```
## See Also

See the [State space](../state.md) family page for matrix shapes, batching, and analysis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ctrb.html">
    <span class="label">Previous</span>
    <span class="title">ctrb</span>
  </a>
  <a class="next-link" href="gramian.html">
    <span class="label">Next</span>
    <span class="title">gram</span>
  </a>
</div>
<!-- apiref:nav:end -->
