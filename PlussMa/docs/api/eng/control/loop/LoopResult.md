<!-- generated from LoopResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::LoopResult`

The three canonical SISO feedback responses evaluated on one frequency grid.

## Signature

```cpp
control::LoopResult
```

## What It Does

This value object groups artifacts that belong to one control calculation so callers can preserve both the primary answer and the diagnostics needed to interpret it. Its `Series` members retain the computation's dtype, platform, and leading batch dimensions.



## Fields

### `response`

The open-loop complex frequency response `L(jw)` or `L(z)`.

### `sens`

Sensitivity `S = 1 / (1 + L)`, measuring disturbance and uncertainty transmission.

### `comp`

Complementary sensitivity `T = L / (1 + L)`, satisfying `S + T = 1` samplewise.

## Returns

This type is returned by, or supplied to, the functions described in this branch. Reading its fields does not trigger another solve.

## Errors

The value object performs no deferred validation. Invalid dimensions, domains, or numerical conditions are rejected by the function that creates or consumes it.

## Interpretation and Use

For unity negative feedback, `sens + comp` should equal one samplewise. Use that identity as a quick numerical check. Large sensitivity indicates disturbance/uncertainty amplification; complementary sensitivity describes reference tracking and high-frequency noise transmission.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Inspect all three responses from a state-space plant.

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
### Example 2

Verify `S + T = 1` through the pole-zero overload.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace control = pluss::ma::eng::control;
namespace lti = pluss::ma::eng::lti;
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

See why this result is restricted to SISO loops.

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
  <a class="next-link" href="MarginResult.html">
    <span class="label">Next</span>
    <span class="title">MarginResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
