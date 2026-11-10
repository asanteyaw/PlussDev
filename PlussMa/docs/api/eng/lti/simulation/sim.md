<!-- generated from sim.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::sim`

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

Propagates continuous or discrete state-space dynamics over an explicit time grid. The first overload starts from zero state; the second uses a supplied initial state. Continuous propagation supports zero-order and linear input holds.

## Parameters

`system`
: Valid state-space model.

`input`
: Samples shaped `[..., time, inputs]`.

`time`
: Finite, strictly increasing real floating vector. Discrete spacing must match `sample_time`.

`initial`
: Optional state shaped `[..., states]`; omitted means zero.

`options`
: Input interpolation policy.

## Returns

`SimRun` with values `[..., time, outputs]` and states `[..., time, states]`, preserving promoted precision and platform.

## Errors

Rejects malformed or incompatible shapes, mismatched lengths or platforms, nonfinite/nonincreasing time, wrong discrete spacing, incompatible initial state, and linear hold for discrete dynamics.

## Practical Examples

These verified programs cover the normal contract, a materially different mode or analysis, and validation behavior.

### Example 1

Simulate a continuous first-order step response on an irregular time grid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const lti::StateSpace system = lti::state(
      pluss::make_series({-1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  const pluss::Series time = pluss::make_series({0.0, 0.2, 0.7, 1.5});
  const pluss::Series input = pluss::make_series({1.0, 1.0, 1.0, 1.0}, {4, 1});
  const lti::SimRun run = lti::sim(
      system, input, time, {.hold = lti::Hold::Zero});

  out.writeln("values =", run.values.flatten().items<double>());
  out.writeln("states =", run.states.flatten().items<double>());
}
```

Output

```text
values = [0.0000, 0.1813, 0.5034, 0.7769]
states = [0.0000, 0.1813, 0.5034, 0.7769]
```
### Example 2

Compare hold policies with an explicit initial state.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const lti::StateSpace system = lti::state(
      pluss::make_series({-1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  const pluss::Series time = pluss::make_series({0.0, 0.5, 1.0});
  const pluss::Series ramp = pluss::make_series({0.0, 0.5, 1.0}, {3, 1});
  const pluss::Series initial = pluss::make_series({2.0});
  const lti::SimRun zero = lti::sim(
      system, ramp, time, initial, {.hold = lti::Hold::Zero});
  const lti::SimRun linear = lti::sim(
      system, ramp, time, initial, {.hold = lti::Hold::Linear});

  out.writeln("zero hold =", zero.values.flatten().items<double>());
  out.writeln("linear hold =", linear.values.flatten().items<double>());
}
```

Output

```text
zero hold = [2.0000, 1.2131, 0.9325]
linear hold = [2.0000, 1.3196, 1.1036]
```
### Example 3

Exercise timestamp and input-shape validation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const lti::StateSpace system = lti::state(
      pluss::make_series({-1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  try {
    static_cast<void>(lti::sim(
        system, pluss::make_series({1.0, 1.0, 1.0}, {3, 1}),
        pluss::make_series({0.0, 0.5, 0.5})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("repeated time rejected =", message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(lti::sim(
        system, pluss::make_series({1.0, 1.0, 1.0}),
        pluss::make_series({0.0, 0.5, 1.0})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("rank-one input rejected =", message.substr(0, message.find('\n')));
  }
}
```

Output

```text
repeated time rejected = eng::lti::sim: time values must be strictly increasing
rank-one input rejected = eng::lti::sim: input must have shape [..., time, inputs]
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
