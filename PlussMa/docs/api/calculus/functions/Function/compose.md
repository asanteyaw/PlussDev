<!-- generated from compose.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d25fe1a747f7 -->

# `math::Function::compose`

[Back to `math::Function`](../Function.md)

Substitutes functions for the variables: `f.compose({g, h})` is `f(g, h)`.

## Signature

```cpp
math::Function math::Function::compose(pluss::RangeView<math::Function> arguments) const;
math::Function math::Function::compose(std::initializer_list<math::Function> arguments) const;
```

## What It Does

Composition is how one function is plugged into another. Each variable of `f`, in declaration order, is replaced by the corresponding function from the list, so `sq.compose({sine})` is `x ↦ x²` with `x` replaced by `sin t`, that is `t ↦ sin² t`. The result's variables are those of the arguments, not of `f`.

Compose is also the way to combine functions that were built separately, which cannot be added or multiplied directly because they have different variables: substitute both into a combining function. The arguments must all come from *one* variable trace (several functions of the same `t`), or be constants; this is what lets the substituted pieces share the variables of the result.

Composition is exact and differentiates by the chain rule, so `operators::differential` of a composition is the right product of derivatives. The list must have one function per variable of `f`.

## Parameters

`std::initializer_list<math::Function> arguments`
: One function per variable of the receiver, in declaration order. They must come from one variable trace, or be constants. The `RangeView` overload takes the list as a range.

## Returns

A new `Function` of the arguments' variables.

## Errors

- `math::Function::compose: expected one argument per variable` when the count is wrong.
- Arguments from different traces are refused.

## Practical Examples

### Example 1

A single-variable function composed with another: `x²` composed with `sin t` is `sin² t`. The new function's variable is `t`.

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
  const math::Function square = math::function({"x"}, [](math::Function x) { return x * x; });
  const math::Function sine = math::function({"t"}, [](math::Function t) { return t.sin(); });
  const math::Function composed = square.compose({sine});
  out.writeln("variable:", composed.variables()[0], "  arity", composed.arity());
  out.writeln("sin^2(1) =", composed(1.0).item<double>(), "  by hand", std::sin(1.0) * std::sin(1.0));
}
```

Output

```text
variable: t   arity 1
sin^2(1) = 0.7081   by hand 0.7081
```

### Example 2

Several variables: `outer(u, v) = u v + u` with `u = t²` and `v = t + 1` becomes a function of `t`. Both substituted functions are built in one callable, so they share the variable `t`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function outer = math::function({"u", "v"}, [](math::Function u, math::Function v) { return u * v + u; });
  const math::Function inner = math::function({"t"}, [&](math::Function t) { return outer.compose({t * t, t + 1.0}); });
  out.writeln("arity:", inner.arity(), "  inner(2) =", inner(2.0).item<double>(), "  (4 * 3 + 4 = 16)");

  // The RangeView overload takes the same list as a range:
  const math::Function twice = math::function({"t"}, [&](math::Function t) {
    const std::vector<math::Function> arguments = {t * t, t * t};
    return outer.compose(pluss::RangeView<math::Function>(arguments));
  });
  out.writeln("outer(t^2, t^2) at 2 =", twice(2.0).item<double>(), "  (16 + 4 = 20)");
}
```

Output

```text
arity: 1   inner(2) = 16.0000   (4 * 3 + 4 = 16)
outer(t^2, t^2) at 2 = 20.0000   (16 + 4 = 20)
```

### Example 3

The chain rule is automatic: the derivative of the composition `sin²(t)` is `2 sin t cos t`.

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
  const math::Function square = math::function({"x"}, [](math::Function x) { return x * x; });
  const math::Function sine = math::function({"t"}, [](math::Function t) { return t.sin(); });
  const math::Function d = operators::differential("t")(square.compose({sine}));
  out.writeln("d/dt sin^2 t at 1 =", d(1.0).item<double>());
  out.writeln("2 sin 1 cos 1     =", 2.0 * std::sin(1.0) * std::cos(1.0));
}
```

Output

```text
d/dt sin^2 t at 1 = 0.9093
2 sin 1 cos 1     = 0.9093
```

### Example 4

The count must match: one function per variable of `f`. A function of two variables given one argument is refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function outer = math::function({"u", "v"}, [](math::Function u, math::Function v) { return u * v; });
  const math::Function sine = math::function({"t"}, [](math::Function t) { return t.sin(); });
  try {
    (void)outer.compose({sine});
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
math::Function::compose: expected one argument per variable
```

## See Also

- [`arity`](arity.md): how many arguments are needed
- [`add`](add.md): combine functions from one trace
- [`operators::compose`](../../operators/compose.md): composing operators, a different thing

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="call.html">
    <span class="label">Previous</span>
    <span class="title">call</span>
  </a>
  <a class="next-link" href="conj.html">
    <span class="label">Next</span>
    <span class="title">conj</span>
  </a>
</div>
<!-- apiref:nav:end -->
