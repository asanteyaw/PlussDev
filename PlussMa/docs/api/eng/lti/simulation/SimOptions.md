<!-- generated from SimOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::SimOptions`

The options of `sim`.

## Signature

```cpp
lti::SimOptions options{};
```

## What It Does

Currently one field: the hold used for the input between samples.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `hold` | A `Hold`. | `Auto` |

## Returns

Argument of `sim`.

## Errors

Invalid inputs are reported by `sim`.

## Interpretation and Use

Construct it with a designated initializer: `{.hold = lti::Hold::Zero}`. The default `Auto` matched `Linear` exactly in the first example and differed from `Zero` by 0.2341 on the ramp.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The default and what it equals on a ramp.

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
  const lti::SimOptions defaults;
  out.writeln("the default hold is Auto:", defaults.hold == lti::Hold::Auto);
  const lti::StateSpace s = lti::state(mat({-1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  const pluss::Series t = pluss::make_series({0.0, 0.5, 1.0, 1.5, 2.0});
  const pluss::Series ramp = pluss::make_series({0.0, 0.5, 1.0, 1.5, 2.0}, {5, 1});
  const auto automatic = lti::sim(s, ramp, t).values.flatten().items<double>();
  const auto linear = lti::sim(s, ramp, t, {.hold = lti::Hold::Linear}).values.flatten().items<double>();
  const auto zero = lti::sim(s, ramp, t, {.hold = lti::Hold::Zero}).values.flatten().items<double>();
  double to_linear = 0.0, to_zero = 0.0;
  for (std::size_t i = 0; i < automatic.size(); ++i) { to_linear = std::max(to_linear, std::abs(automatic[i] - linear[i])); to_zero = std::max(to_zero, std::abs(automatic[i] - zero[i])); }
  out.writeln("Auto against Linear, largest difference", to_linear, "; Auto against Zero", to_zero);
}
```

Output

```text
the default hold is Auto: true
Auto against Linear, largest difference 0.0000 ; Auto against Zero 0.2341
```
### Example 2

`Zero` and `Linear` on the same ramp.

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
  const pluss::Series t = pluss::make_series({0.0, 1.0, 2.0});
  const pluss::Series ramp = pluss::make_series({0.0, 1.0, 2.0}, {3, 1});
  for (const auto [label, hold] : {std::pair{"Zero  ", lti::Hold::Zero}, {"Linear", lti::Hold::Linear}}) out.writeln(label, "y =", lti::sim(s, ramp, t, {.hold = hold}).values.flatten().items<double>());
}
```

Output

```text
Zero   y = [0.0000, 0.0000, 0.6321]
Linear y = [0.0000, 0.3679, 1.1353]
```
### Example 3

Arguments `sim` refuses under these options.

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
  refused(out, "a time vector that does not increase under Linear hold", [&] { lti::sim(s, pluss::series::ones({3, 1}, pluss::ScalarType::Float64), pluss::make_series({0.0, 1.0, 0.5}), {.hold = lti::Hold::Linear}); });
  refused(out, "an input of the wrong width", [&] { lti::sim(s, pluss::series::ones({2, 3}, pluss::ScalarType::Float64), pluss::make_series({0.0, 1.0}), {.hold = lti::Hold::Zero}); });
}
```

Output

```text
a time vector that does not increase under Linear hold : eng::lti::sim: time values must be strictly increasing
an input of the wrong width : eng::lti::sim: input width must equal the system input count
```
## See Also

See [Simulation](../simulation.md) for shapes, batching, hold policy, and continuous/discrete behavior.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Hold.html">
    <span class="label">Previous</span>
    <span class="title">Hold</span>
  </a>
  <a class="next-link" href="SimRun.html">
    <span class="label">Next</span>
    <span class="title">SimRun</span>
  </a>
</div>
<!-- apiref:nav:end -->
