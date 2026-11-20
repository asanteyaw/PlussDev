<!-- generated from sim.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::sim`

Simulates a state-space system over a time grid.

## Signature

```cpp
lti::SimRun lti::sim(
    const lti::StateSpace& system,
    const pluss::Series& input,
    const pluss::Series& time,
    const lti::SimOptions& options = {});

lti::SimRun lti::sim(
    const lti::StateSpace& system,
    const pluss::Series& input,
    const pluss::Series& time,
    const pluss::Series& initial,
    const lti::SimOptions& options = {});
```

## What It Does

Advances `x` through the supplied time points with the input held between them (the `Hold` of `SimOptions`) using exact discretisation, so there is no integration step error: the only error is how the input behaves between samples. The initial state defaults to zero. The result is a `SimRun` with the outputs and the states at every time point.

## Parameters

`system`
: A `StateSpace`.

`input`
: `[time, inputs]`, one row per time point.

`time`
: A strictly increasing rank-one `Series` of time points, s.

`initial`
: Optional initial state, shape `[..., states]`; omitted means zero.

`options`
: A `SimOptions`: the input hold.

## Returns

A `SimRun`: `values` of shape `[time, outputs]` and `states` of shape `[time, states]`.

## Errors

A mismatched input length or width, a time vector that does not increase and an initial state of the wrong size throw `pluss::Error`.

## Interpretation and Use

For `x' = -x + u` and a unit step the response is `1 - exp(-t)`, and the first example reproduces it at eleven samples to rounding. With no input the state decays from the initial value as `x0 exp(-t)` (the second example). Choose the time grid by what you need to observe, not by accuracy: the solution at each grid point is exact for the held input.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A step response against `1 - exp(-t)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <optional>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace sig = pluss::subsp::eng::signal;
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
  // dx/dt = -x + u: the step response is 1 - exp(-t).
  const lti::StateSpace s = lti::state(mat({-1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  std::vector<double> t, u;
  for (int i = 0; i <= 10; ++i) { t.push_back(0.5 * i); u.push_back(1.0); }
  const lti::SimRun run = lti::sim(s, pluss::make_series(u, {11, 1}), pluss::make_series(t));
  const auto y = run.values.flatten().items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < y.size(); ++i) worst = std::max(worst, std::abs(y[i] - (1.0 - std::exp(-t[i]))));
  out.writeln("y(0) =", y.front(), " y(5) =", y.back(), " exact", 1.0 - std::exp(-5.0), " largest difference over all 11 samples =", worst);
}
```

Output

```text
y(0) = 0.0000  y(5) = 0.9933  exact 0.9933  largest difference over all 11 samples = 0.0000
```
### Example 2

A free response from an initial state.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <optional>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace sig = pluss::subsp::eng::signal;
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
  // With no input the output decays as x0 exp(-t).
  const lti::StateSpace s = lti::state(mat({-1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  const pluss::Series t = pluss::make_series({0.0, 1.0, 2.0, 3.0});
  const lti::SimRun run = lti::sim(s, pluss::series::zeros({4, 1}, pluss::ScalarType::Float64), t, pluss::make_series({2.0}));
  out.writeln("y =", run.values.flatten().items<double>(), " exact 2 exp(-t) =", 2.0, 2.0 * std::exp(-1.0), 2.0 * std::exp(-2.0), 2.0 * std::exp(-3.0));
}
```

Output

```text
y = [2.0000, 0.7358, 0.2707, 0.0996]  exact 2 exp(-t) = 2.0000 0.7358 0.2707 0.0996
```
### Example 3

Two inputs and two outputs: the shapes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <optional>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace sig = pluss::subsp::eng::signal;
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
  const lti::StateSpace s = lti::state(pluss::series::eye(3, pluss::ScalarType::Float64).mul(-1.0), pluss::series::ones({3, 2}, pluss::ScalarType::Float64), pluss::series::ones({2, 3}, pluss::ScalarType::Float64), pluss::series::zeros({2, 2}, pluss::ScalarType::Float64));
  const pluss::Series t = pluss::make_series({0.0, 0.5, 1.0, 1.5});
  const lti::SimRun run = lti::sim(s, pluss::series::ones({4, 2}, pluss::ScalarType::Float64), t);
  out.writeln("values shape", run.values.size(0), "x", run.values.size(1), " states shape", run.states.size(0), "x", run.states.size(1));
}
```

Output

```text
values shape 4 x 2  states shape 4 x 3
```
### Example 4

Every refused argument.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <optional>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace sig = pluss::subsp::eng::signal;
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
  const lti::StateSpace s = lti::state(mat({-1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  const pluss::Series t = pluss::make_series({0.0, 0.5, 1.0});
  refused(out, "an input with the wrong number of samples", [&] { lti::sim(s, pluss::series::ones({2, 1}, pluss::ScalarType::Float64), t); });
  refused(out, "an input with the wrong number of channels", [&] { lti::sim(s, pluss::series::ones({3, 2}, pluss::ScalarType::Float64), t); });
  refused(out, "a time vector that does not increase", [&] { lti::sim(s, pluss::series::ones({3, 1}, pluss::ScalarType::Float64), pluss::make_series({0.0, 1.0, 0.5})); });
  refused(out, "an initial state of the wrong size", [&] { lti::sim(s, pluss::series::ones({3, 1}, pluss::ScalarType::Float64), t, pluss::make_series({1.0, 2.0})); });
}
```

Output

```text
an input with the wrong number of samples : eng::lti::sim: input and time lengths must match
an input with the wrong number of channels : eng::lti::sim: input width must equal the system input count
a time vector that does not increase : eng::lti::sim: time values must be strictly increasing
an initial state of the wrong size : eng::lti::sim: initial state must have shape [..., states]
```
## See Also

See [Simulation](../simulation.md) for shapes, batching, hold policy, and continuous/discrete behavior.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SimRun.html">
    <span class="label">Previous</span>
    <span class="title">SimRun</span>
  </a>
</div>
<!-- apiref:nav:end -->
