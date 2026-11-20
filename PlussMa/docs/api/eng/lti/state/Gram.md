<!-- generated from Gram.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::Gram`

Chooses which Gramian `gram` computes.

## Signature

```cpp
lti::Gram lti::Gram::Reach;
lti::Gram lti::Gram::Observe;
```

## What It Does

`Gram::Reach` solves the Lyapunov equation driven by `B B'` and measures how much input energy each state direction costs; `Gram::Observe` solves the equation driven by `C' C` and measures how much output energy each state direction produces.

## Values

| Value | Equation (continuous) | Driven by |
|---|---|---|
| `Reach` | `A W + W A' + B B' = 0` | The input matrix. |
| `Observe` | `A' W + W A + C' C = 0` | The output matrix. |

## Returns

Argument of `gram`.

## Errors

The enum has no invalid values; `gram` reports unstable systems.

## Interpretation and Use

The two are dual: the observability Gramian of a system equals the reachability Gramian of its dual `(A', C', B', D')` (the first example). In the discrete case the Gramian is the sum of the energy of the impulse response (the second example sums a geometric series to `b^2 / (1 - a^2)`).

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The duality of the two sides.

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
  const lti::StateSpace dual = lti::state(s.a().transpose(0, 1), s.c().transpose(0, 1), s.b().transpose(0, 1), s.d().transpose(0, 1));
  out.writeln("Observe Gramian of the system  =", lti::gram(s, lti::Gram::Observe).flatten().items<double>());
  out.writeln("Reach Gramian of the dual      =", lti::gram(dual, lti::Gram::Reach).flatten().items<double>());
}
```

Output

```text
Observe Gramian of the system  = [1.3000, 0.1250, 0.1250, 0.3125]
Reach Gramian of the dual      = [1.3000, 0.1250, 0.1250, 0.3125]
```
### Example 2

The discrete equations against their geometric-series closed forms.

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
  const lti::StateSpace s = lti::state(mat({0.5}, 1, 1), mat({1.0}, 1, 1), mat({2.0}, 1, 1), mat({0.0}, 1, 1), 0.25);
  out.writeln("reach: sum of a^(2k) b^2 = b^2 / (1 - a^2) =", lti::gram(s, lti::Gram::Reach).flatten().items<double>(), " expected", 1.0 / (1.0 - 0.25));
  out.writeln("observe: c^2 / (1 - a^2) =", lti::gram(s, lti::Gram::Observe).flatten().items<double>(), " expected", 4.0 / (1.0 - 0.25));
}
```

Output

```text
reach: sum of a^(2k) b^2 = b^2 / (1 - a^2) = [1.3333]  expected 1.3333
observe: c^2 / (1 - a^2) = [5.3333]  expected 5.3333
```
### Example 3

How the two sides scale with `b` and `c`.

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
  out.writeln("Gram::Reach has value", static_cast<int>(lti::Gram::Reach), "and Gram::Observe has value", static_cast<int>(lti::Gram::Observe));
  const lti::StateSpace s = lti::state(mat({-2.0}, 1, 1), mat({1.0}, 1, 1), mat({4.0}, 1, 1), mat({0.0}, 1, 1));
  out.writeln("with b = 1 and c = 4 the two differ by c^2 / b^2 = 16: reach", lti::gram(s, lti::Gram::Reach).flatten().items<double>()[0], " observe", lti::gram(s, lti::Gram::Observe).flatten().items<double>()[0]);
}
```

Output

```text
Gram::Reach has value 0 and Gram::Observe has value 1
with b = 1 and c = 4 the two differ by c^2 / b^2 = 16: reach 0.2500  observe 4.0000
```
## See Also

See the [State space](../state.md) family page for matrix shapes, batching, and analysis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="StateSpace.html">
    <span class="label">Previous</span>
    <span class="title">StateSpace</span>
  </a>
  <a class="next-link" href="state.html">
    <span class="label">Next</span>
    <span class="title">state</span>
  </a>
</div>
<!-- apiref:nav:end -->
