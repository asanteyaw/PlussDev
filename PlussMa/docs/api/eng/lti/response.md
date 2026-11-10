<!-- generated from response.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Pole-zero and response

`PoleZero` stores transfer behavior as zeros, poles, gain, and a timebase. This form makes stability and modal structure explicit, while `freqresp` evaluates either pole-zero or state-space models without requiring users to convert representation first.

## Timebase And Frequency

Continuous systems use `sample_time == 0` and evaluate at `s = j omega`. Discrete systems use positive sample time and evaluate on the unit circle at `z = exp(j omega T)`. In both cases `omega` is angular frequency per time unit, not ordinary cycles per unit.

## Stability And Realization

A continuous pole-zero system is stable when every pole has negative real part. A discrete system is stable when every pole lies strictly inside the unit circle. `to_sos` converts a discrete, proper root representation into `[b0,b1,b2,a0,a1,a2]` rows and pairs roots to reduce numerical sensitivity.

## Practical Examples

These examples inspect a discrete pole-zero system, compare equivalent state-space and root responses, and show frequency-grid and realization errors.

### Example 1

Construct a stable discrete system, evaluate two frequencies, and convert it to one second-order section.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>
#include <iostream>

namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const lti::PoleZero system = lti::zpk(
      pluss::make_series({0.0}), pluss::make_series({0.5}),
      pluss::make_series(0.5), 1.0);
  const pluss::Series response =
      lti::freqresp(system, pluss::make_series({0.0, 3.141592653589793}));

  std::cout << std::boolalpha << "discrete = " << system.is_disc()
            << ", stable = " << system.is_stable() << "\n";
  out.writeln("zeros =", system.zeros().items<std::complex<double>>());
  out.writeln("poles =", system.poles().items<std::complex<double>>());
  out.writeln("response =", response.items<std::complex<double>>());
  out.writeln("sos =", lti::to_sos(system).flatten().items<double>());
}
```

Output

```text
discrete = true, stable = true
zeros = [(0,0)]
poles = [(0.5,0)]
response = [(1,0), (0.333333,-1.36072e-17)]
sos = [0.5000, 0.0000, 0.0000, 1.0000, -0.5000, 0.0000]
```
### Example 2

Evaluate equivalent continuous state-space and pole-zero systems on the same angular-frequency grid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;

  const lti::StateSpace state_system = lti::state(
      pluss::make_series({-1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  const lti::PoleZero root_system = lti::zpk(
      pluss::series::empty({0}, pluss::ScalarType::Float64),
      pluss::make_series({-1.0}),
      pluss::make_series(1.0));
  const pluss::Series omega = pluss::make_series({0.0, 1.0, 2.0});

  out.writeln("state response =",
              lti::freqresp(state_system, omega).flatten().items<std::complex<double>>());
  out.writeln("root response =",
              lti::freqresp(root_system, omega).items<std::complex<double>>());
}
```

Output

```text
state response = [(1,0), (0.5,-0.5), (0.2,-0.4)]
root response = [(1,0), (0.5,-0.5), (0.2,-0.4)]
```
### Example 3

Demonstrate rejection of an integer frequency grid and an attempt to realize a continuous system as delay-form SOS coefficients.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const lti::PoleZero system = lti::zpk(
      pluss::series::empty({0}, pluss::ScalarType::Float64),
      pluss::make_series({-1.0}),
      pluss::make_series(1.0));
  try {
    static_cast<void>(lti::freqresp(system, pluss::make_series({0, 1})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("integer grid rejected =", message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(lti::to_sos(system));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("continuous SOS rejected =", message.substr(0, message.find('\n')));
  }
}
```

Output

```text
integer grid rejected = eng::lti::freqresp: omega must have a real floating dtype
continuous SOS rejected = eng::lti::to_sos: only discrete systems can be realized by the delay-form SOS filter
```
## Related Pages

Use [State space](state.md) for internal-state analysis and [Simulation](simulation.md) for time-domain trajectories.

```{toctree}
:maxdepth: 1
:hidden:

response/PoleZero
response/zpk
response/to_sos
response/freqresp
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

response/PoleZero
response/zpk
response/to_sos
response/freqresp
```
<!-- apiref:toctree:end -->
