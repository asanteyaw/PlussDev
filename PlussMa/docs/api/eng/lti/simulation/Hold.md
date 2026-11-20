<!-- generated from Hold.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::Hold`

How the input is interpolated between time points.

## Signature

```cpp
lti::Hold lti::Hold::Auto;
lti::Hold lti::Hold::Zero;
lti::Hold lti::Hold::Linear;
```

## What It Does

Between two samples the input is either held constant (`Zero`), varies linearly (`Linear`) or is chosen for you (`Auto`). The choice does not change the integrator, only the assumption about what the input does between samples.

## Values

| Value | Between samples |
|---|---|
| `Zero` | The input keeps the value of the earlier sample (zero-order hold). |
| `Linear` | The input varies linearly to the next sample (first-order hold). |
| `Auto` | Chosen by the library; for the continuous system in the examples it gave the linear result. |

## Returns

Field `hold` of `SimOptions`.

## Errors

Time vectors that do not increase are rejected whatever the hold.

## Interpretation and Use

Use `Zero` for a signal that really is piecewise constant, such as a digital-to-analog output, and `Linear` for a sampled smooth signal. The first example sends a ramp through a first-order system: the linear hold reproduces the exact response, the zero hold lags it by a visible error. The second shows a piecewise-constant input, where the zero hold is exact.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A ramp input under each hold, against the exact response.

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
  const pluss::Series t = pluss::make_series({0.0, 0.5, 1.0, 1.5, 2.0});
  const pluss::Series ramp = pluss::make_series({0.0, 0.5, 1.0, 1.5, 2.0}, {5, 1});
  // For u = t the exact response from rest is t - 1 + exp(-t).
  for (const auto [label, hold] : {std::pair{"Zero  ", lti::Hold::Zero}, {"Linear", lti::Hold::Linear}, {"Auto  ", lti::Hold::Auto}}) {
    const auto y = lti::sim(s, ramp, t, {.hold = hold}).values.flatten().items<double>();
    double worst = 0.0;
    for (std::size_t i = 0; i < y.size(); ++i) worst = std::max(worst, std::abs(y[i] - (t.items<double>()[i] - 1.0 + std::exp(-t.items<double>()[i]))));
    out.writeln(label, "y(2) =", y.back(), " largest error against the exact ramp response =", worst);
  }
}
```

Output

```text
Zero   y(2) = 0.9012  largest error against the exact ramp response = 0.2341
Linear y(2) = 1.1353  largest error against the exact ramp response = 0.0000
Auto   y(2) = 1.1353  largest error against the exact ramp response = 0.0000
```
### Example 2

A piecewise-constant input under `Zero` and `Linear`.

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
  // For a piecewise-constant input the zero-order hold is exact.
  const lti::StateSpace s = lti::state(mat({-1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  const pluss::Series t = pluss::make_series({0.0, 0.5, 1.0, 1.5, 2.0});
  const pluss::Series steps = pluss::make_series({1.0, 1.0, 0.0, 0.0, 0.0}, {5, 1});
  for (const auto [label, hold] : {std::pair{"Zero  ", lti::Hold::Zero}, {"Linear", lti::Hold::Linear}}) {
    const auto y = lti::sim(s, steps, t, {.hold = hold}).values.flatten().items<double>();
    out.writeln(label, "y at 0.5, 1.0, 1.5 =", y[1], y[2], y[3]);
  }
}
```

Output

```text
Zero   y at 0.5, 1.0, 1.5 = 0.3935 0.6321 0.3834
Linear y at 0.5, 1.0, 1.5 = 0.3935 0.4191 0.2542
```
### Example 3

Time vectors that every hold refuses.

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
  refused(out, "a time vector with a repeated sample under Linear hold", [&] { lti::sim(s, pluss::series::ones({3, 1}, pluss::ScalarType::Float64), pluss::make_series({0.0, 0.5, 0.5}), {.hold = lti::Hold::Linear}); });
  refused(out, "a time vector that does not increase", [&] { lti::sim(s, pluss::series::ones({3, 1}, pluss::ScalarType::Float64), pluss::make_series({0.0, 0.0, 1.0}), {.hold = lti::Hold::Zero}); });
}
```

Output

```text
a time vector with a repeated sample under Linear hold : eng::lti::sim: time values must be strictly increasing
a time vector that does not increase : eng::lti::sim: time values must be strictly increasing
```
## See Also

See [Simulation](../simulation.md) for shapes, batching, hold policy, and continuous/discrete behavior.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="SimOptions.html">
    <span class="label">Next</span>
    <span class="title">SimOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
