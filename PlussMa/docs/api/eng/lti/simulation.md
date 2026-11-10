<!-- generated from simulation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Simulation

Simulation propagates a validated `StateSpace` system over an explicit time vector and returns both observable outputs and complete state history. The API supports leading batch dimensions, continuous and discrete systems, real or complex dynamics, and reverse sensitivity through the underlying `Series` operations.

## Input And Output Shapes

Input has shape `[..., time, inputs]`. Output values have shape `[..., time, outputs]`, and states have shape `[..., time, states]`. Optional initial state has shape `[..., states]`; omitted initial state is zero. System and signal batch axes broadcast when compatible.

## Hold Policy

Continuous simulation must specify how input changes between timestamps. Zero-order hold keeps each sample constant until the next time. Linear hold interpolates between neighboring samples. `Auto` selects the appropriate default. A discrete system already defines sample-to-sample behavior, so its time spacing must equal `sample_time` and linear interpolation is invalid.

## Practical Examples

These examples simulate an irregular continuous-time step response, compare hold policies with a nonzero initial state, and demonstrate time and shape validation.

### Example 1

Run a stable first-order system under constant input on an irregular time grid.

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

Compare zero-order and linearly interpolated ramp inputs while starting from the same nonzero state.

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

Show rejection of repeated timestamps and an input missing its required trailing input axis.

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
## Related Pages

Use [State space](state.md) to construct and analyze the system, and [Pole-zero and response](response.md) to inspect frequency-domain behavior.

```{toctree}
:maxdepth: 1
:hidden:

simulation/Hold
simulation/SimOptions
simulation/SimRun
simulation/sim
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

simulation/Hold
simulation/SimOptions
simulation/SimRun
simulation/sim
```
<!-- apiref:toctree:end -->
