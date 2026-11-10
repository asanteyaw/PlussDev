<!-- generated from place.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::place`

Assigns state-feedback poles.

## Signature

```cpp
control::PlaceResult control::place(
    const pluss::Series& a,
    const pluss::Series& b,
    const pluss::Series& poles,
    control::PlaceOptions options = {});

control::PlaceResult control::place(
    const lti::StateSpace& system,
    const pluss::Series& poles,
    control::PlaceOptions options = {});
```

## What It Does

`place` assigns the spectrum of `A - B K`. The state-space overload is a convenience coordinator over the same matrix kernel; it does not duplicate the placement algorithm.

## Parameters

`a`
: Square state matrix `A`.

`b`
: Input matrix `B`.

`system`
: State-space model whose `A` and `B` matrices are used.

`poles`
: Desired closed-loop poles. Real systems require conjugate-paired complex poles.

`options`
: Refinement, residual, and conditioning controls.

## Returns

A `PlaceResult` containing the gain, closed-loop matrix, and assignment diagnostics.

## Errors

Throws for uncontrollable systems, invalid pole counts or conjugacy, incompatible shapes, ill-conditioned bases, or residuals above tolerance.

## Interpretation and Use

Use matrix overloads in synthesis code that already owns `A` and `B`; use the state-space overload to preserve model provenance and avoid extraction boilerplate. Desired poles are shared across leading batches and real matrices require conjugate pairs.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Assign two real poles through the matrix overload.

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

Reject an uncontrollable input matrix.

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

Contrast prescribed poles with LQR-selected poles.

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
  <a class="prev-link" href="PidController.html">
    <span class="label">Previous</span>
    <span class="title">Pid</span>
  </a>
  <a class="next-link" href="pid.html">
    <span class="label">Next</span>
    <span class="title">pid</span>
  </a>
</div>
<!-- apiref:nav:end -->
