<!-- generated from magnitude_differential.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::magnitude_differential`

The absolute value of a derivative: `|f⁽ⁿ⁾|`.

## Signature

```cpp
operators::Operator operators::magnitude_differential(std::string_view variable, std::int64_t order = 1, operators::DiffOptions options = {});
```

## What It Does

`magnitude_differential("x", n)` is the operator `f ↦ |∂ⁿf/∂xⁿ|`: differentiate, then take the absolute value (the modulus, for a complex function). It exists because the *size* of a derivative, rather than its sign, is often the quantity of interest: a Lipschitz bound is `sup |f′|`, a roughness penalty is `∫ |f″|`, the speed of a particle is `|x′(t)|`.

It is the composition of `differential` and `abs`, packaged as one operator so that it has one key and is a single stage in a pipeline. Unlike the derivative it is *not* linear (`|a + b| ≠ |a| + |b|`), and it has no adjoint, which its [`capabilities`](../operators/Operator/capabilities.md) report. It follows the same options as `differential`, and the absolute value's corner at zero follows the `nonsmooth` option when the result is itself differentiated.

## Parameters

`std::string_view variable`
: The variable to differentiate with respect to.

`std::int64_t order`
: The derivative order. Positive; default 1.

`operators::DiffOptions options`
: Non-smooth policy, fallback and size limit.

## Returns

An `Operator` whose application returns `|∂ⁿf/∂xⁿ|` as a `Function`.

## Errors

- Same as `differential`: a non-positive order, an empty name, an unknown variable.

## Practical Examples

### Example 1

The size of the second derivative of `−x³`, `|−6x| = 6|x|`: the same on both sides of zero, where the second derivative itself changes sign.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return -(x * x * x); });
  const math::Function curvature = operators::magnitude_differential("x", 2)(f);
  const math::Function signed_curvature = operators::differential("x", 2)(f);
  const auto x = pluss::make_series({-2.0, -1.0, 1.0, 2.0});
  out.writeln("f'':    ", signed_curvature({x}).items<double>());
  out.writeln("|f''|:  ", curvature({x}).items<double>());
}
```

Output

```text
f'':     [12.0000, 6.0000, -6.0000, -12.0000]
|f''|:   [12.0000, 6.0000, 6.0000, 12.0000]
```

### Example 2

A roughness measure: the total variation of the slope of `sin` over `[0, π]` is `∫ |sin x| dx = 2`. The magnitude operator composes with the integral operator.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.sin(); });
  const operators::Operator roughness = operators::compose(operators::integral("x", 0.0, std::acos(-1.0)), operators::magnitude_differential("x", 2));
  out.writeln("integral of |f''| over [0, pi] =", roughness(f)().item<double>());
}
```

Output

```text
integral of |f''| over [0, pi] = 2.0000
```

### Example 3

The result is not linear and has no adjoint: its capabilities say so, unlike those of `differential`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const operators::Capabilities m = operators::magnitude_differential("x").capabilities();
  const operators::Capabilities d = operators::differential("x").capabilities();
  out.writeln("magnitude_differential: linear", m.linear, " adjoint_known", m.adjoint_known, " serializable", m.serializable);
  out.writeln("differential:           linear", d.linear, " adjoint_known", d.adjoint_known, " serializable", d.serializable);
}
```

Output

```text
magnitude_differential: linear false  adjoint_known false  serializable true
differential:           linear true  adjoint_known true  serializable true
```

## See Also

- [`differential`](differential.md): the signed derivative
- [`math::Function::abs`](../functions/Function/abs.md): the absolute value

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="jacobian.html">
    <span class="label">Previous</span>
    <span class="title">jacobian</span>
  </a>
  <a class="next-link" href="wirtinger.html">
    <span class="label">Next</span>
    <span class="title">wirtinger</span>
  </a>
</div>
<!-- apiref:nav:end -->
