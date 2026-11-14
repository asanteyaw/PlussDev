<!-- generated from SimRun.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::SimRun`

## Signature

```cpp
lti::SimRun run = lti::sim(system, input, time);
```

## What It Does

Owns both externally visible outputs and internal state history from one simulation, allowing diagnostics and chained analysis without rerunning dynamics.

## Fields

| Field | Meaning |
|---|---|
| `values` | Output history with shape `[..., time, outputs]`. |
| `states` | State history with shape `[..., time, states]`, including the state associated with each output sample. |

## Returns

Returned by both `lti::sim` overloads.

## Errors

The result itself is valid by construction; errors occur during simulation before a `SimRun` is returned.

## Practical Examples

These verified programs cover the normal contract, a materially different mode or analysis, and validation behavior.

### Example 1

Simulate a continuous first-order step response on an irregular time grid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::subsp::eng::lti;
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

namespace lti = pluss::subsp::eng::lti;
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

namespace lti = pluss::subsp::eng::lti;
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
