<!-- generated from ctrb.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::ctrb`

Builds the controllability matrix.

## Signature

```cpp
pluss::Series lti::ctrb(const lti::StateSpace& system);
```

## What It Does

Returns `[B, AB, A^2 B, ..., A^{n-1} B]`, an `[n, n m]` matrix. Every state can be steered to any value by some input exactly when this matrix has full row rank `n`.

## Parameters

`system`
: A `StateSpace`. Continuous and discrete systems use the same matrix.

## Returns

A `Series` with matrix shape `[n, n m]` and the batch axes of the system.

## Errors

Construction errors are raised by `state`; `ctrb` itself does not fail for a valid system. Judging the rank is left to the caller.

## Interpretation and Use

For a single input the matrix is square and its determinant decides: the oscillator has determinant -1 and is controllable; the decoupled system, whose second state no input reaches, has determinant 0. For a numerical rank use a singular value decomposition rather than a determinant, which is sensitive to scaling.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The matrix and determinant of a controllable system and a system with an unreachable state.

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
  const lti::StateSpace controllable = oscillator();
  out.writeln("controllability matrix [B, AB] =", lti::ctrb(controllable).flatten().items<double>(), " determinant", det2(lti::ctrb(controllable)));
  // The second state is driven by nothing and does not feed the first, so no input can reach it.
  const lti::StateSpace hidden = lti::state(mat({-1.0, 0.0, 0.0, -2.0}, 2, 2), mat({1.0, 0.0}, 2, 1), mat({1.0, 1.0}, 1, 2), mat({0.0}, 1, 1));
  out.writeln("decoupled state: [B, AB] =", lti::ctrb(hidden).flatten().items<double>(), " determinant", det2(lti::ctrb(hidden)), "(zero: not controllable)");
}
```

Output

```text
controllability matrix [B, AB] = [0.0000, 1.0000, 1.0000, -0.4000]  determinant -1.0000
decoupled state: [B, AB] = [1.0000, -1.0000, 0.0000, 0.0000]  determinant 0.0000 (zero: not controllable)
```
### Example 2

The shape for one and for several inputs.

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
  const pluss::Series c1 = lti::ctrb(siso);
  out.writeln("two states, one input:", c1.size(0), "x", c1.size(1));
  const lti::StateSpace mimo = lti::state(mat({0.0, 1.0, 0.0, 0.0, 0.0, 1.0, -1.0, -2.0, -3.0}, 3, 3), mat({0.0, 1.0, 0.0, 0.0, 1.0, 1.0}, 3, 2), mat({1.0, 0.0, 0.0}, 1, 3), mat({0.0, 0.0}, 1, 2));
  const pluss::Series c2 = lti::ctrb(mimo);
  out.writeln("three states, two inputs:", c2.size(0), "x", c2.size(1), "(states by states times inputs)");
}
```

Output

```text
two states, one input: 2 x 2
three states, two inputs: 3 x 6 (states by states times inputs)
```
### Example 3

The matrix against `[B, AB]` built by hand.

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
  const lti::StateSpace s = oscillator();
  const pluss::Series c = lti::ctrb(s);
  const pluss::Series expected = pluss::series::unite({s.b(), s.a().matmul(s.b())}, 1);
  out.writeln("[B, AB] =", c.flatten().items<double>());
  out.writeln("built by hand =", expected.flatten().items<double>());
  out.writeln("largest difference =", (c - expected).abs().max().item<double>());
}
```

Output

```text
[B, AB] = [0.0000, 1.0000, 1.0000, -0.4000]
built by hand = [0.0000, 1.0000, 1.0000, -0.4000]
largest difference = 0.0000
```
## See Also

See the [State space](../state.md) family page for matrix shapes, batching, and analysis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="state.html">
    <span class="label">Previous</span>
    <span class="title">state</span>
  </a>
  <a class="next-link" href="obsv.html">
    <span class="label">Next</span>
    <span class="title">obsv</span>
  </a>
</div>
<!-- apiref:nav:end -->
