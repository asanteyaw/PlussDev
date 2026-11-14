<!-- generated from loop.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Loop analysis

This branch evaluates the return ratio `L`, sensitivity `S = (I + L)^{-1}`, and complementary sensitivity `T = I - S`. It also extracts classical SISO gain, phase, and stability margins from a sampled angular-frequency grid.

## Sampled Margins

`margin` interpolates gain and phase crossings between adjacent samples rather than reporting only the nearest grid point. A sufficiently dense grid must still cover every crossing of interest. Missing gain or phase crossings are represented by an infinite margin and a `NaN` crossover frequency; stability margin is always the minimum sampled distance from the Nyquist response to `-1`.

## Practical Examples

The examples inspect all loop responses, calculate classical margins, and show the SISO and complex-response guards.

### Example 1

Evaluate open-loop, sensitivity, and complementary-sensitivity responses.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const lti::StateSpace plant = lti::state(
      pluss::make_series({-1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  const control::LoopResult result =
      control::loop(plant, pluss::make_series({0.0, 1.0}));

  out.writeln("response =",
              result.response.flatten().items<std::complex<double>>());
  out.writeln("sensitivity =",
              result.sens.flatten().items<std::complex<double>>());
  out.writeln("complementary =",
              result.comp.flatten().items<std::complex<double>>());
}
```

Output

```text
response = [(1,0), (0.5,-0.5)]
sensitivity = [(0.5,0), (0.6,0.2)]
complementary = [(0.5,0), (0.4,-0.2)]
```
### Example 2

Calculate gain, phase, and Nyquist-distance stability margins.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const lti::PoleZero plant = lti::zpk(
      pluss::series::empty({0}, pluss::ScalarType::Float64),
      pluss::make_series({0.0, -1.0, -2.0}), pluss::make_series(1.0));
  const pluss::Series omega = pluss::series::logspace(
      -3.0, 3.0, 20001, 10.0,
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const control::MarginResult result = control::margin(plant, omega);

  out.writeln("gain margin =", result.gain.item<double>());
  out.writeln("gain frequency =", result.gain_freq.item<double>());
  out.writeln("phase margin =", result.phase.item<double>());
  out.writeln("phase frequency =", result.phase_freq.item<double>());
  out.writeln("stability margin =", result.stability.item<double>());
}
```

Output

```text
gain margin = 6.000002
gain frequency = 1.414214
phase margin = 53.410787
phase frequency = 0.445748
stability margin = 0.637600
```
### Example 3

Reject classical margins for a MIMO model and a real-valued response array.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const lti::StateSpace mimo = lti::state(
      pluss::make_series({{-1.0, 0.0}, {0.0, -2.0}}),
      pluss::series::eye(2, pluss::ScalarType::Float64),
      pluss::series::eye(2, pluss::ScalarType::Float64),
      pluss::series::zeros({2, 2}, pluss::ScalarType::Float64));
  try {
    static_cast<void>(control::margin(mimo, pluss::make_series({0.1, 1.0})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("MIMO margin rejected =", message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(control::margin(
        pluss::make_series({1.0, 0.5}), pluss::make_series({0.1, 1.0})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("real response rejected =",
                message.substr(0, message.find('\n')));
  }
}
```

Output

```text
MIMO margin rejected = eng::control::margin: classical margins require a SISO system
real response rejected = eng::control::margin: response must have a complex dtype
```
```{toctree}
:maxdepth: 2
:hidden:

loop/LoopResult
loop/MarginResult
loop/loop
loop/margin
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

loop/LoopResult
loop/MarginResult
loop/loop
loop/margin
```
<!-- apiref:toctree:end -->
