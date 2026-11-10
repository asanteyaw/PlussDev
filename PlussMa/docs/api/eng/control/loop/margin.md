<!-- generated from margin.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::margin`

Interpolates classical SISO stability margins.

## Signature

```cpp
control::MarginResult control::margin(
    const pluss::Series& response,
    const pluss::Series& omega);

control::MarginResult control::margin(
    const lti::StateSpace& system,
    const pluss::Series& omega);

control::MarginResult control::margin(
    const lti::PoleZero& system,
    const pluss::Series& omega);
```

## What It Does

Crossings are interpolated between adjacent samples rather than snapped to bins. Supply a sufficiently resolved grid around expected crossovers; no sampled-data method can recover a crossing absent from the covered band.

## Parameters

`response`
: A one-dimensional complex open-loop response sampled at `omega`.

`system`
: A SISO LTI model evaluated internally before margin extraction.

`omega`
: Strictly ordered real angular frequencies.

## Returns

A `MarginResult` containing gain, phase, and stability measures plus their angular frequencies.

## Errors

Throws for real-only response data, MIMO models, incompatible response/frequency lengths, or invalid frequency ordering.

## Interpretation and Use

The response overload is best when frequency data already exists; model overloads avoid separately calling `freqresp`. Crossing interpolation improves on selecting the nearest sample but cannot recover behavior outside the supplied band, so widen and refine `omega` before trusting absent crossovers.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Compute margins directly from a model over a dense grid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::ma::eng::control;
namespace lti = pluss::ma::eng::lti;
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

Produce open-loop response data suitable for the direct overload.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace control = pluss::ma::eng::control;
namespace lti = pluss::ma::eng::lti;
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

See response dtype and SISO validation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace control = pluss::ma::eng::control;
namespace lti = pluss::ma::eng::lti;
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
  <a class="prev-link" href="loop.html">
    <span class="label">Previous</span>
    <span class="title">loop</span>
  </a>
</div>
<!-- apiref:nav:end -->
