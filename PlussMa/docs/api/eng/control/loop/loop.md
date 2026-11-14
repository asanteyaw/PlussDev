<!-- generated from loop.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::loop`

Evaluates open- and closed-loop SISO frequency responses.

## Signature

```cpp
control::LoopResult control::loop(
    const lti::StateSpace& system,
    const pluss::Series& omega);

control::LoopResult control::loop(
    const lti::PoleZero& system,
    const pluss::Series& omega);
```

## What It Does

The model-specific overload delegates frequency evaluation to the LTI layer, then computes sensitivity functions with vectorized complex `Series` arithmetic.

## Parameters

`system`
: A SISO state-space or pole-zero model.

`omega`
: Real angular frequencies in radians per unit time.

## Returns

A `LoopResult` with `L`, `S`, and `T` over the requested grid.

## Errors

Throws when the model is not SISO or the frequency grid is invalid.

## Interpretation and Use

`loop` is the response coordinator; it delegates plant evaluation to LTI and then computes `S` and `T` vectorially. Choose the overload matching the model already in hand. Both require a SISO loop because scalar unity-feedback formulas are used.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Evaluate a state-space loop.

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

Evaluate the same feedback quantities from poles, zeros, and gain.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const lti::PoleZero plant = lti::zpk(
      pluss::make_series(std::initializer_list<std::complex<double>>{}),
      pluss::make_series({std::complex<double>{-1.0, 0.0}}),
      pluss::make_series(2.0));
  const control::LoopResult result =
      control::loop(plant, pluss::make_series({0.0, 1.0}));

  out.writeln("pole-zero loop =",
              result.response.items<std::complex<double>>());
  out.writeln("S plus T =",
              (result.sens + result.comp).items<std::complex<double>>());
}
```

Output

```text
pole-zero loop = [(2,0), (1,-1)]
S plus T = [(1,0), (1,0)]
```
### Example 3

Observe the SISO contract at failure boundaries.

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
  <a class="prev-link" href="MarginResult.html">
    <span class="label">Previous</span>
    <span class="title">MarginResult</span>
  </a>
  <a class="next-link" href="margin.html">
    <span class="label">Next</span>
    <span class="title">margin</span>
  </a>
</div>
<!-- apiref:nav:end -->
