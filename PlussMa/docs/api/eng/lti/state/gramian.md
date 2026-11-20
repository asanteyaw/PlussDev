<!-- generated from gramian.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::gramian`

Solves the Lyapunov equation of a stable system for its Gramian.

## Signature

```cpp
pluss::Series lti::gram(
    const lti::StateSpace& system,
    lti::Gram side,
    linearalg::MatrixEqOptions options = {});
```

## What It Does

Returns the reachability Gramian `W` with `A W + W A' + B B' = 0` or the observability Gramian with `A' W + W A + C' C = 0` for a continuous system, and the discrete equations `W = A W A' + B B'` and `W = A' W A + C' C` for a discrete one. The Gramian measures how much energy an input needs to reach each state direction, or how much output energy a state produces.

## Parameters

`system`
: A `StateSpace`. It must be stable.

`side`
: `Gram::Reach` or `Gram::Observe`.

`options`
: Solver settings of the Lyapunov solver.

## Returns

A `Series` of shape `[n, n]` (with batch axes): symmetric and positive semidefinite.

## Errors

An unstable or marginal system throws `pluss::Error`: the message names the stability condition that failed and appends the solver's own message.

## Interpretation and Use

For a scalar system `x' = a x + b u` the reachability Gramian is `b^2 / (2 |a|)` and the observability Gramian `c^2 / (2 |a|)`, which the first example checks. The second shows the residual of the Lyapunov equation at rounding level for the oscillator. Use the Gramians for balanced truncation, for the energy needed to reach a state and for the H2 norm.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A scalar system against the closed form.

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
  // dx/dt = -x + 2u, y = 3x. The Lyapunov equation A W + W A' + B B' = 0 gives W = b^2 / (2 |a|) = 2.
  const lti::StateSpace s = lti::state(mat({-1.0}, 1, 1), mat({2.0}, 1, 1), mat({3.0}, 1, 1), mat({0.0}, 1, 1));
  out.writeln("reachability Gramian =", lti::gram(s, lti::Gram::Reach).flatten().items<double>(), " expected b^2 / (2|a|) = 2");
  out.writeln("observability Gramian =", lti::gram(s, lti::Gram::Observe).flatten().items<double>(), " expected c^2 / (2|a|) = 4.5");
}
```

Output

```text
reachability Gramian = [2.0000]  expected b^2 / (2|a|) = 2
observability Gramian = [4.5000]  expected c^2 / (2|a|) = 4.5
```
### Example 2

The residual of both Lyapunov equations for the oscillator.

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
  const pluss::Series w = lti::gram(s, lti::Gram::Reach);
  const pluss::Series residual = s.a().matmul(w) + w.matmul(s.a().transpose(0, 1)) + s.b().matmul(s.b().transpose(0, 1));
  out.writeln("reachability Gramian =", w.flatten().items<double>());
  out.writeln("largest entry of A W + W A' + B B' =", residual.abs().max().item<double>());
  const pluss::Series o = lti::gram(s, lti::Gram::Observe);
  const pluss::Series residual_o = s.a().transpose(0, 1).matmul(o) + o.matmul(s.a()) + s.c().transpose(0, 1).matmul(s.c());
  out.writeln("largest entry of A' W + W A + C' C for the observability Gramian =", residual_o.abs().max().item<double>());
}
```

Output

```text
reachability Gramian = [0.3125, 0.0000, 0.0000, 1.2500]
largest entry of A W + W A' + B B' = 0.0000
largest entry of A' W + W A + C' C for the observability Gramian = 0.0000
```
### Example 3

Systems for which no Gramian exists.

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
  const lti::StateSpace unstable = lti::state(mat({0.5}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  refused(out, "an unstable continuous system", [&] { lti::gram(unstable, lti::Gram::Reach); });
  const lti::StateSpace marginal = lti::state(mat({0.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  refused(out, "a marginally stable system", [&] { lti::gram(marginal, lti::Gram::Reach); });
  const lti::StateSpace outside = lti::state(mat({1.5}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 0.1);
  refused(out, "a discrete system with a pole outside the unit circle", [&] { lti::gram(outside, lti::Gram::Reach); });
}
```

Output

```text
an unstable continuous system : eng::lti::gram: the Gramian could not be computed; it exists only for a stable system (every eigenvalue of A in the left half-plane). Solver message: subsp::linearalg::dlyap: iteration produced non-finite values
a marginally stable system : eng::lti::gram: the Gramian could not be computed; it exists only for a stable system (every eigenvalue of A in the left half-plane). Solver message: subsp::linearalg::lyap: continuous residual exceeds tolerance; A may not be Hurwitz stable
a discrete system with a pole outside the unit circle : eng::lti::gram: the Gramian could not be computed; it exists only for a stable system (every eigenvalue of A inside the unit circle). Solver message: subsp::linearalg::dlyap: iteration produced non-finite values
```
## See Also

See the [State space](../state.md) family page for matrix shapes, batching, and analysis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="obsv.html">
    <span class="label">Previous</span>
    <span class="title">obsv</span>
  </a>
</div>
<!-- apiref:nav:end -->
