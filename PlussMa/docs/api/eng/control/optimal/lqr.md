<!-- generated from lqr.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::lqr`

Synthesizes optimal feedback from a state-space model.

## Signature

```cpp
control::RiccatiResult control::lqr(
    const lti::StateSpace& system,
    const pluss::Series& q,
    const pluss::Series& r,
    pluss::ma::linearalg::MatrixEqOptions options = {});

control::RiccatiResult control::lqr(
    const lti::StateSpace& system,
    const pluss::Series& q,
    const pluss::Series& r,
    const pluss::Series& cross,
    pluss::ma::linearalg::MatrixEqOptions options = {});
```

## What It Does

`lqr` is the timebase-aware router: a continuous model delegates to `care`, while a discrete model delegates to `dare`. It avoids duplicating the model matrices at the call site.

## Parameters

`system`
: A validated continuous or discrete `lti::StateSpace` model.

`q`
: State cost `Q`.

`r`
: Input cost `R`.

`cross`
: Optional state-input cross cost `S`.

`options`
: Numerical matrix-equation controls passed to the selected solver.

## Returns

The same `control::RiccatiResult` as direct `care` or `dare`.

## Errors

Throws under the corresponding Riccati solver's shape, cost, or convergence failures.

## Interpretation and Use

Prefer `lqr` when the plant already exists as `StateSpace`: it reads the timebase and delegates to the correct Riccati kernel. This prevents an accidental continuous solve on discrete matrices. Use direct `care` or `dare` only when matrix-level control is intentional.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Synthesize and inspect a continuous state-space regulator.

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
### Example 2

Route a discrete model and compare it with direct `dare`.

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
### Example 3

Observe the cost validation inherited from the delegated solver.

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
  <a class="prev-link" href="dare.html">
    <span class="label">Previous</span>
    <span class="title">dare</span>
  </a>
</div>
<!-- apiref:nav:end -->
