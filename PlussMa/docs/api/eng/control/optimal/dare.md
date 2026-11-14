<!-- generated from dare.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::dare`

Solves a discrete algebraic Riccati equation.

## Signature

```cpp
control::RiccatiResult control::dare(
    const pluss::Series& a,
    const pluss::Series& b,
    const pluss::Series& q,
    const pluss::Series& r,
    pluss::ma::linearalg::MatrixEqOptions options = {});

control::RiccatiResult control::dare(
    const pluss::Series& a,
    const pluss::Series& b,
    const pluss::Series& q,
    const pluss::Series& r,
    const pluss::Series& cross,
    pluss::ma::linearalg::MatrixEqOptions options = {});
```

## What It Does

`dare` computes the infinite-horizon discrete regulator. It uses the same cost convention and batched shape contract as `care`, but stability is interpreted in the discrete unit disk.

## Parameters

`a`
: The one-step state transition matrix `A`.

`b`
: The one-step input matrix `B`.

`q`
: Hermitian positive-semidefinite state cost `Q`.

`r`
: Hermitian positive-definite input cost `R`.

`cross`
: Optional state-input cross cost `S`.

`options`
: Numerical matrix-equation controls.

## Returns

A `control::RiccatiResult` for the stabilizing discrete solution.

## Errors

Throws for incompatible dimensions, invalid cost matrices, or failure to obtain a stabilizing solution.

## Interpretation and Use

Use `dare` for sampled dynamics expressed directly as matrices. Its stability condition is discrete: closed-loop eigenvalues must lie inside the unit disk. The returned gain is a full-state law, not a controller with an observer or actuator limits.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Solve `dare` directly and verify parity with discrete `lqr`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const pluss::Series a = pluss::make_series({0.5}, {1, 1});
  const pluss::Series b = pluss::make_series({1.0}, {1, 1});
  const pluss::Series q = pluss::make_series({1.0}, {1, 1});
  const pluss::Series r = pluss::make_series({1.0}, {1, 1});
  const control::RiccatiResult direct = control::dare(a, b, q, r);
  const lti::StateSpace plant = lti::state(
      a, b, pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({0.0}, {1, 1}), 1.0);
  const control::RiccatiResult routed = control::lqr(plant, q, r);

  out.writeln("direct value =", direct.value.flatten().items<double>());
  out.writeln("routed value =", routed.value.flatten().items<double>());
  out.writeln("discrete gain =", routed.gain.flatten().items<double>());
}
```

Output

```text
direct value = [1.132782]
routed value = [1.132782]
discrete gain = [0.265564]
```
### Example 2

Contrast the continuous cross-cost convention shared by the Riccati family.

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
### Example 3

Review cost-matrix validation that also governs discrete synthesis.

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
  <a class="prev-link" href="care.html">
    <span class="label">Previous</span>
    <span class="title">care</span>
  </a>
  <a class="next-link" href="lqr.html">
    <span class="label">Next</span>
    <span class="title">lqr</span>
  </a>
</div>
<!-- apiref:nav:end -->
