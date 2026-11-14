<!-- generated from care.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::care`

Solves a continuous algebraic Riccati equation.

## Signature

```cpp
control::RiccatiResult control::care(
    const pluss::Series& a,
    const pluss::Series& b,
    const pluss::Series& q,
    const pluss::Series& r,
    pluss::ma::linearalg::MatrixEqOptions options = {});

control::RiccatiResult control::care(
    const pluss::Series& a,
    const pluss::Series& b,
    const pluss::Series& q,
    const pluss::Series& r,
    const pluss::Series& cross,
    pluss::ma::linearalg::MatrixEqOptions options = {});
```

## What It Does

`care` solves the continuous-time equation and synthesizes the stabilizing feedback law. Leading dimensions are batch axes and broadcast under normal `Series` rules.

## Parameters

`a`
: The continuous state matrix `A`, shaped `[..., states, states]`.

`b`
: The input matrix `B`, shaped `[..., states, inputs]`.

`q`
: Hermitian positive-semidefinite state cost `Q`.

`r`
: Hermitian positive-definite input cost `R`.

`cross`
: Optional state-input cross cost `S`; the joint cost block must remain positive semidefinite.

`options`
: Numerical matrix-equation tolerances and iteration controls.

## Returns

A `control::RiccatiResult` containing `X`, `K`, and `A - B K`.

## Errors

Throws when shapes are incompatible, costs violate their Hermitian definiteness contracts, or no stabilizing solution can be established.

## Interpretation and Use

Use `care` when matrices are already available and the dynamics are continuous. The cross-weight overload is not a numerical hint: it changes the quadratic objective. Start without it unless the model genuinely contains state-input coupling, and always inspect `closed_loop` after synthesis.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Solve the cross-weight overload and inspect its stabilizing solution.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::subsp::eng::control;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const control::RiccatiResult result = control::care(
      pluss::make_series({-2.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({4.0}, {1, 1}),
      pluss::make_series({2.0}, {1, 1}),
      pluss::make_series({0.5}, {1, 1}));

  out.writeln("cross-weight value =", result.value.flatten().items<double>());
  out.writeln("cross-weight gain =", result.gain.flatten().items<double>());
  out.writeln("closed-loop pole =", result.closed_loop.flatten().items<double>());
}
```

Output

```text
cross-weight value = [0.791503]
cross-weight gain = [0.645751]
closed-loop pole = [-2.645751]
```
### Example 2

Use the equivalent state-space route for a standard continuous regulator.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
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
### Example 3

See the exact failures for singular `R` and non-Hermitian `Q`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace control = pluss::subsp::eng::control;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const pluss::Series scalar = pluss::make_series({1.0}, {1, 1});
  try {
    static_cast<void>(control::care(
        pluss::make_series({-2.0}, {1, 1}), scalar,
        pluss::make_series({4.0}, {1, 1}),
        pluss::make_series({0.0}, {1, 1})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("singular R rejected =", message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(control::care(
        pluss::make_series({{-1.0, 0.0}, {0.0, -2.0}}),
        pluss::make_series({1.0, 0.0}, {2, 1}),
        pluss::make_series({{1.0, 2.0}, {0.0, 1.0}}), scalar));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("non-Hermitian Q rejected =",
                message.substr(0, message.find('\n')));
  }
}
```

Output

```text
singular R rejected = eng::control::care: R must be positive definite
non-Hermitian Q rejected = eng::control::care: Q must be Hermitian
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="RiccatiResult.html">
    <span class="label">Previous</span>
    <span class="title">RiccatiResult</span>
  </a>
  <a class="next-link" href="dare.html">
    <span class="label">Next</span>
    <span class="title">dare</span>
  </a>
</div>
<!-- apiref:nav:end -->
