<!-- generated from Hold.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::Hold`

## Signature

```cpp
lti::Hold lti::Hold::Auto;
lti::Hold lti::Hold::Zero;
lti::Hold lti::Hold::Linear;
```

## What It Does

Selects how continuous simulation interprets inputs between timestamps.

## Fields

| Field | Meaning |
|---|---|
| `Auto` | Chooses the surface default: zero-order behavior unless the system contract requires otherwise. |
| `Zero` | Holds each input sample constant until the next timestamp. |
| `Linear` | Interpolates linearly between neighboring samples. |

## Returns

Stored in `SimOptions::hold` and consumed by `lti::sim`.

## Errors

Discrete simulation rejects `Linear` because the recurrence already advances on fixed samples.

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
  <a class="next-link" href="SimOptions.html">
    <span class="label">Next</span>
    <span class="title">SimOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
