<!-- generated from exercise_boundary.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d7fde80059de -->

# `module::Field::exercise_boundary`

[Back to `module::Field`](../Field.md)

The early-exercise boundary of an American option under Black-Scholes: the asset price at which exercise becomes optimal, at given times to maturity.

## Signature

```cpp
pluss::Series module::Field::exercise_boundary(const pluss::Series &times_to_maturity, pluss::Config cfg = {}) const;
```

## What It Does

For an American put the holder exercises when the asset falls below the boundary; for a call, when it rises above it. The boundary is a curve in the time to maturity, found by the same integral-equation solution as the `Spectral` engine (the controls are `controls.spectral`), and this method returns it at the times you give. At expiry the put boundary is {math}`K\min(1,r/q)`: the strike when the dividend yield does not exceed the rate, lower when it does.

Only Black-Scholes has a boundary curve; under Heston the exercise region is a surface in asset and variance, so another model throws. The instrument must be American with one strike, maturity, rate and dividend yield, and every time must lie between 0 and the maturity.

## Parameters

`const pluss::Series &times_to_maturity`
: Times to maturity in years, each in [0, maturity].

`pluss::Config cfg`
: Device of the result.

## Returns

The boundary, one asset price per time.

## Errors

- A non-American instrument, a model other than Black-Scholes, a strip, or a time outside [0, maturity] throws.

## Practical Examples

### Example 1

The boundary of a one-year put at five times to maturity: at expiry it is the strike, and it moves away as the time to maturity grows.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  mod::Field field{.instrument = fin::instrument::american(fin::pricing::Put, 100.0, 1.0), .market = fin::market::make(100.0, 0.05, 0.0),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.25}), .controls = {.engine = mod::Engine::Spectral}};
  out.writeln("time to maturity   0.00 0.10 0.25 0.50 1.00");
  out.writeln("put boundary      ", field.exercise_boundary(pluss::make_series({0.0, 0.1, 0.25, 0.5, 1.0})).items<double>());
}
```

Output

```text
time to maturity   0.00 0.10 0.25 0.50 1.00
put boundary       [100.000, 87.176, 82.766, 78.946, 74.890]
```

### Example 2

With a dividend yield above the rate the put boundary at expiry is below the strike, K r / q.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  mod::Field field{.instrument = fin::instrument::american(fin::pricing::Put, 100.0, 1.0), .market = fin::market::make(100.0, 0.02, 0.06),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.25}), .controls = {.engine = mod::Engine::Spectral}};
  out.writeln("at expiry:", field.exercise_boundary(pluss::make_series({0.0})).items<double>(), " K r/q =", 100.0 * 0.02 / 0.06);
}
```

Output

```text
at expiry: [33.333]  K r/q = 33.333
```

### Example 3

A call is exercised above its boundary: with a 7% yield the call boundary lies above the strike.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  mod::Field field{.instrument = fin::instrument::american(fin::pricing::Call, 100.0, 1.0), .market = fin::market::make(100.0, 0.03, 0.07),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.25}), .controls = {.engine = mod::Engine::Spectral}};
  out.writeln("call boundary:", field.exercise_boundary(pluss::make_series({0.0, 0.5, 1.0})).items<double>());
}
```

Output

```text
call boundary: [100.000, 127.764, 134.796]
```

### Example 4

Refused: a European instrument, another model, and a time beyond the maturity.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Field field{.instrument = fin::instrument::american(fin::pricing::Put, 100.0, 1.0), .market = fin::market::make(100.0, 0.05, 0.0),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.25}), .controls = {.engine = mod::Engine::Spectral}};
  try {
    (void)(field.exercise_boundary(pluss::make_series({2.0})));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
  field.model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04});
  try {
    (void)(field.exercise_boundary(pluss::make_series({0.5})));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
  field.instrument = fin::instrument::make(fin::pricing::Put, 100.0, 1.0);
  try {
    (void)(field.exercise_boundary(pluss::make_series({0.5})));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::module::Field::exercise_boundary: times_to_maturity must lie in [0, maturity]
subsp::math::finance::module::Field::exercise_boundary: only Black-Scholes has an exercise boundary curve (other models' exercise regions are surfaces); the model is Heston
subsp::math::finance::module::Field::exercise_boundary: the instrument must be American
```

## See Also

- [`Field`](../Field.md): the class
- [`Spectral`](../Controls.md): the engine whose controls it uses

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="engine_name.html">
    <span class="label">Previous</span>
    <span class="title">engine_name</span>
  </a>
  <a class="next-link" href="implied_vol.html">
    <span class="label">Next</span>
    <span class="title">implied_vol</span>
  </a>
</div>
<!-- apiref:nav:end -->
