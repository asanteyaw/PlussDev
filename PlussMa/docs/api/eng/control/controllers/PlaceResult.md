<!-- generated from PlaceResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::PlaceResult`

State-feedback gain and pole-placement diagnostics.

## Signature

```cpp
control::PlaceResult
```

## What It Does

This value object groups artifacts that belong to one control calculation so callers can preserve both the primary answer and the diagnostics needed to interpret it. Its `Series` members retain the computation's dtype, platform, and leading batch dimensions.



## Fields

### `gain`

Feedback gain `K` for the law `u = -K x`.

### `closed_loop`

Closed-loop matrix `A - B K`.

### `basis`

Eigenvector basis used to realize the requested spectrum.

### `residual`

Assignment residual measuring agreement with the requested poles.

### `condition`

Condition estimate for the realized eigenvector basis.

### `sweeps`

Number of refinement sweeps actually completed.

## Returns

This type is returned by, or supplied to, the functions described in this branch. Reading its fields does not trigger another solve.

## Errors

The value object performs no deferred validation. Invalid dimensions, domains, or numerical conditions are rejected by the function that creates or consumes it.

## Interpretation and Use

Inspect `residual` and `condition` before accepting `gain`. `closed_loop` is the matrix whose spectrum was assigned, while `basis` explains numerical sensitivity. A tiny residual with extreme conditioning can still produce a fragile controller.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Inspect the gain, closed-loop matrix, residual, and condition.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace control = pluss::ma::eng::control;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const control::PlaceResult result = control::place(
      pluss::make_series({{0.0, 1.0}, {-2.0, -3.0}}),
      pluss::make_series({0.0, 1.0}, {2, 1}),
      pluss::make_series(
          {std::complex<double>{-4.0, 0.0}, std::complex<double>{-5.0, 0.0}}));

  out.writeln("gain =", result.gain.flatten().items<double>());
  out.writeln("closed loop =", result.closed_loop.flatten().items<double>());
  out.writeln("residual =", result.residual.item<double>());
  out.writeln("condition =", result.condition.item<double>());
}
```

Output

```text
gain = [18.000000, 6.000000]
closed loop = [0.000000, 1.000000, -20.000000, -9.000000]
residual = 0.000000
condition = 42.023796
```
### Example 2

Observe failure before an invalid result is returned.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>
#include <exception>
#include <string>

namespace control = pluss::ma::eng::control;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  try {
    static_cast<void>(control::place(
        pluss::make_series({{0.0, 1.0}, {-2.0, -3.0}}),
        pluss::series::zeros({2, 1}, pluss::ScalarType::Float64),
        pluss::make_series({std::complex<double>{-4.0, 0.0},
                            std::complex<double>{-5.0, 0.0}})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("uncontrollable plant rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(control::pid(
        pluss::make_series(1.0), pluss::make_series(0.0),
        pluss::make_series(0.0), pluss::make_series(-0.1)));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("negative filter rejected =",
                message.substr(0, message.find('\n')));
  }
}
```

Output

```text
uncontrollable plant rejected = eng::control::place: B must have non-zero numerical rank
negative filter rejected = eng::control::pid: filter must be non-negative
```
### Example 3

Compare placement output with an optimal-feedback result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::ma::eng::control;
namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const lti::StateSpace plant = lti::state(
      pluss::make_series({-2.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  const control::RiccatiResult result = control::lqr(
      plant, pluss::make_series({4.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}));

  out.writeln("value =", result.value.flatten().items<double>());
  out.writeln("gain =", result.gain.flatten().items<double>());
  out.writeln("closed loop =", result.closed_loop.flatten().items<double>());
}
```

Output

```text
value = [0.828427]
gain = [0.828427]
closed loop = [-2.828427]
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PlaceOptions.html">
    <span class="label">Previous</span>
    <span class="title">PlaceOptions</span>
  </a>
  <a class="next-link" href="PidRule.html">
    <span class="label">Next</span>
    <span class="title">PidRule</span>
  </a>
</div>
<!-- apiref:nav:end -->
