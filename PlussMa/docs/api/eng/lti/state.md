<!-- generated from state.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# State space

State-space models describe internal dynamics with four matrices:

```text
x' = A x + B u,   y = C x + D u
```

For discrete systems, `x'` means the next sample rather than a time derivative. `StateSpace` keeps this distinction explicit through `sample_time`: zero denotes continuous time and a positive value denotes discrete time.

## Shape Contract

The matrix tails are `A [states, states]`, `B [states, inputs]`, `C [outputs, states]`, and `D [outputs, inputs]`. Any leading dimensions are batch dimensions and follow normal broadcast rules. Construction promotes the four matrices to one dtype and requires one platform, avoiding mixed execution inside later analysis.

## Structural Analysis

`ctrb` and `obsv` expose the classical controllability and observability matrices. Their ranks answer whether every internal mode can be driven or reconstructed. `gram` solves the corresponding continuous or discrete Lyapunov equation; it therefore requires stable dynamics for the usual finite energy interpretation.

## Practical Examples

These examples construct and inspect a second-order system, compare continuous and discrete Gramians, and demonstrate shape and timebase validation.

### Example 1

Construct a continuous oscillator and print dimensions, timebase, controllability, and observability matrices.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const lti::StateSpace system = lti::state(
      pluss::make_series({{0.0, 1.0}, {-2.0, -3.0}}),
      pluss::make_series({0.0, 1.0}, {2, 1}),
      pluss::make_series({{1.0, 0.0}}),
      pluss::make_series({0.0}, {1, 1}));

  std::cout << "states = " << system.states()
            << ", inputs = " << system.inputs()
            << ", outputs = " << system.outputs() << "\n";
  out.writeln("continuous =", system.is_cont());
  out.writeln("controllability =", lti::ctrb(system).flatten().items<double>());
  out.writeln("observability =", lti::obsv(system).flatten().items<double>());
}
```

Output

```text
states = 2, inputs = 1, outputs = 1
continuous = true
controllability = [0.0000, 1.0000, 1.0000, -3.0000]
observability = [1.0000, 0.0000, 0.0000, 1.0000]
```
### Example 2

Compute reachability and observability Gramians for stable continuous and discrete scalar systems.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const lti::StateSpace continuous = lti::state(
      pluss::make_series({-1.0}, {1, 1}), pluss::make_series({2.0}, {1, 1}),
      pluss::make_series({3.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  const lti::StateSpace discrete = lti::state(
      pluss::make_series({0.5}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}), 0.25);

  out.writeln("continuous reach gram =",
              lti::gram(continuous, lti::Gram::Reach).flatten().items<double>());
  out.writeln("continuous observe gram =",
              lti::gram(continuous, lti::Gram::Observe).flatten().items<double>());
  out.writeln("discrete reach gram =",
              lti::gram(discrete, lti::Gram::Reach).flatten().items<double>());
}
```

Output

```text
continuous reach gram = [2.0000]
continuous observe gram = [4.5000]
discrete reach gram = [1.3333]
```
### Example 3

Show immediate rejection of an incompatible feedthrough matrix and an invalid negative sample time.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  try {
    static_cast<void>(lti::state(
        pluss::make_series({{1.0, 0.0}, {0.0, 1.0}}),
        pluss::make_series({1.0, 0.0}, {2, 1}),
        pluss::make_series({{1.0, 0.0}}),
        pluss::make_series({{0.0, 0.0}})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("bad feedthrough rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(lti::state(
        pluss::make_series({-1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
        pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}), -0.1));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("negative sample time rejected =",
                message.substr(0, message.find('\n')));
  }
}
```

Output

```text
bad feedthrough rejected = eng::lti::state: D must have shape [outputs, inputs]
negative sample time rejected = eng::lti::state: sample_time must be finite and non-negative
```
## Related Pages

Use [Pole-zero and response](response.md) for root-form models and frequency analysis, or [Simulation](simulation.md) to propagate a state-space system through an input history.

```{toctree}
:maxdepth: 1
:hidden:

state/StateSpace
state/Gram
state/state
state/ctrb
state/obsv
state/gramian
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

state/StateSpace
state/Gram
state/state
state/ctrb
state/obsv
state/gramian
```
<!-- apiref:toctree:end -->
