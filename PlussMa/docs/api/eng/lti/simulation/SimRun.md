<!-- generated from SimRun.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::SimRun`

The result of `sim`.

## Signature

```cpp
lti::SimRun run = lti::sim(system, input, time);
```

## What It Does

The output samples and the state trajectory. `values` has one row per time point and one column per output; `states` has one row per time point and one column per state. The first row of `states` is the initial state.

## Fields

| Field | Meaning | Shape |
|---|---|---|
| `values` | `y = C x + D u` at every time point. | `[time, outputs]` |
| `states` | The state at every time point. | `[time, states]` |

## Returns

Returned by `sim`.

## Errors

Reading a field does not run again.

## Interpretation and Use

Because both are returned, the outputs can be recomputed from the states and the input: the second example reconstructs `C x + D u` and finds no difference at rounding. Pass the last row of `states` as `initial` to continue a simulation.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The shapes and the initial state in the first row.

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
  const lti::StateSpace s = lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1));
  const pluss::Series t = pluss::make_series({0.0, 0.5, 1.0, 1.5, 2.0});
  const lti::SimRun run = lti::sim(s, pluss::series::ones({5, 1}, pluss::ScalarType::Float64), t);
  out.writeln("values", run.values.size(0), "x", run.values.size(1), "(time by outputs)   states", run.states.size(0), "x", run.states.size(1), "(time by states)");
  out.writeln("the first row of states is the initial state:", run.states.select(0, 0).items<double>());
}
```

Output

```text
values 5 x 1 (time by outputs)   states 5 x 2 (time by states)
the first row of states is the initial state: [0.0000, 0.0000]
```
### Example 2

The outputs recomputed from the states and the input.

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
  // y = C x + D u at every sample: recompute the output from the returned states.
  const lti::StateSpace s = lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.5}, 1, 1));
  const pluss::Series t = pluss::make_series({0.0, 0.5, 1.0, 1.5});
  const pluss::Series u = pluss::make_series({1.0, 1.0, 1.0, 1.0}, {4, 1});
  const lti::SimRun run = lti::sim(s, u, t);
  const pluss::Series recomputed = run.states.matmul(s.c().transpose(0, 1)) + u.matmul(s.d().transpose(0, 1));
  out.writeln("largest difference between values and C x + D u =", (run.values - recomputed).abs().max().item<double>());
}
```

Output

```text
largest difference between values and C x + D u = 0.0000
```
### Example 3

An initial state of the wrong size.

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
  const lti::SimRun run = lti::sim(s, pluss::series::ones({3, 1}, pluss::ScalarType::Float64), t);
  out.writeln("a run is a plain pair of Series: values", run.values.numel(), "samples, states", run.states.numel(), "samples");
  refused(out, "continuing from a state with two entries for a one-state system", [&] { lti::sim(s, pluss::series::ones({3, 1}, pluss::ScalarType::Float64), t, pluss::make_series({1.0, 2.0})); });
}
```

Output

```text
a run is a plain pair of Series: values 3 samples, states 3 samples
continuing from a state with two entries for a one-state system : eng::lti::sim: initial state must have shape [..., states]
```
## See Also

See [Simulation](../simulation.md) for shapes, batching, hold policy, and continuous/discrete behavior.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SimOptions.html">
    <span class="label">Previous</span>
    <span class="title">SimOptions</span>
  </a>
  <a class="next-link" href="sim.html">
    <span class="label">Next</span>
    <span class="title">sim</span>
  </a>
</div>
<!-- apiref:nav:end -->
