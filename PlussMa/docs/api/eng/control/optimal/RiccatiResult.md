<!-- generated from RiccatiResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::RiccatiResult`

The complete output of a continuous or discrete algebraic Riccati solve.

## Signature

```cpp
control::RiccatiResult
```

## What It Does

This value object groups artifacts that belong to one control calculation so callers can preserve both the primary answer and the diagnostics needed to interpret it. Its `Series` members retain the computation's dtype, platform, and leading batch dimensions.

The three fields are deliberately returned together: a gain without its value matrix and resulting dynamics is difficult to audit.

## Fields

### `value`

The stabilizing Riccati solution, conventionally written `X` or `P`, with shape `[..., states, states]`.

### `gain`

The optimal state-feedback gain `K`, with shape `[..., inputs, states]`. The synthesized law is `u = -K x`.

### `closed_loop`

The closed-loop dynamics `A - B K`, with shape `[..., states, states]`.

## Returns

This type is returned by, or supplied to, the functions described in this branch. Reading its fields does not trigger another solve.

## Errors

The value object performs no deferred validation. Invalid dimensions, domains, or numerical conditions are rejected by the function that creates or consumes it.

## Interpretation and Use

Read `value` as the optimized cost-to-go curvature, `gain` as the implementable feedback law, and `closed_loop` as the immediate stability audit. Compare all three when changing costs: a numerically plausible gain is not sufficient if the resulting dynamics are not stabilizing.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Inspect all three result fields from a continuous synthesis.

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
### Example 2

See how a state-input cross weight changes the returned value, gain, and pole.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::ma::eng::control;
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

Compare the result contract across direct discrete solving and routed LQR.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::ma::eng::control;
namespace lti = pluss::ma::eng::lti;
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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="care.html">
    <span class="label">Next</span>
    <span class="title">care</span>
  </a>
</div>
<!-- apiref:nav:end -->
