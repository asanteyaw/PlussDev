<!-- generated from MarginResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::MarginResult`

Classical loop margins and the frequencies where they occur.

## Signature

```cpp
control::MarginResult
```

## What It Does

This value object groups artifacts that belong to one control calculation so callers can preserve both the primary answer and the diagnostics needed to interpret it. Its `Series` members retain the computation's dtype, platform, and leading batch dimensions.



## Fields

### `gain`

Gain margin as a multiplicative factor, not decibels.

### `phase`

Phase margin in degrees.

### `stability`

Minimum sampled/interpolated Nyquist distance to `-1`.

### `gain_freq`

Angular frequency of the phase crossover used for gain margin.

### `phase_freq`

Angular frequency of the gain crossover used for phase margin.

### `stability_freq`

Angular frequency of the minimum Nyquist distance.

## Returns

This type is returned by, or supplied to, the functions described in this branch. Reading its fields does not trigger another solve.

## Errors

The value object performs no deferred validation. Invalid dimensions, domains, or numerical conditions are rejected by the function that creates or consumes it.

## Interpretation and Use

`gain` is multiplicative rather than decibels, `phase` is degrees, and every frequency is angular. Infinite margins are meaningful when no relevant crossover exists. Stability margin complements classical margins by measuring the closest Nyquist approach to `-1`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Print every classical margin and its crossover frequency.

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
### Example 2

Inspect the response from which margins are derived.

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
### Example 3

Reject unsuitable MIMO and real-only response inputs.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="LoopResult.html">
    <span class="label">Previous</span>
    <span class="title">LoopResult</span>
  </a>
  <a class="next-link" href="loop.html">
    <span class="label">Next</span>
    <span class="title">loop</span>
  </a>
</div>
<!-- apiref:nav:end -->
