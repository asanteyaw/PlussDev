<!-- generated from scale.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::scale`

The operator that multiplies a function by a fixed real number.

## Signature

```cpp
operators::Operator operators::scale(double coefficient);
```

## What It Does

`scale(c)` is the operator `f ↦ c f`. It is the simplest linear operator and the building block of linear combinations (`linear_combination` is built from scaled operators). It commutes with every linear operator, `D (c f) = c D f`, and it is self-adjoint for a real coefficient. Scale by 0 gives the zero function, by −1 the negative.

Because the coefficient is a plain number, `scale` can be saved: it is one of the operators whose parameters travel with it. For a coefficient that depends on the variables, use [`multiply`](multiply.md).

## Parameters

`double coefficient`
: The factor. Any real number, including 0 and negatives.

## Returns

An `Operator` that multiplies its input by `coefficient`.

## Practical Examples

### Example 1

Scaling a function and chaining scales: the coefficients multiply.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x * x; });

  out.writeln("3 f at 2          =", operators::scale(3.0)(f)(2.0).item<double>());
  out.writeln("scale(3) then 2   =", operators::compose(operators::scale(2.0), operators::scale(3.0))(f)(2.0).item<double>(), " (6 f)");
  out.writeln("scale(-1) f at 2  =", operators::scale(-1.0)(f)(2.0).item<double>());
  out.writeln("scale(0) f at 2   =", operators::scale(0.0)(f)(2.0).item<double>());
}
```

Output

```text
3 f at 2          = 24.0000
scale(3) then 2   = 48.0000  (6 f)
scale(-1) f at 2  = -8.0000
scale(0) f at 2   = 0.0000
```

### Example 2

Scale commutes with a linear operator: differentiating the scaled function equals scaling the derivative.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x * x; });

  const operators::Operator D = operators::differential("x");
  const operators::Operator S = operators::scale(5.0);
  out.writeln("D(S f)(2) =", D(S(f))(2.0).item<double>());
  out.writeln("S(D f)(2) =", S(D(f))(2.0).item<double>());
  out.writeln("commutator [D, S] f (2) =", operators::commutator(D, S)(f)(2.0).item<double>());
}
```

Output

```text
D(S f)(2) = 60.0000
S(D f)(2) = 60.0000
commutator [D, S] f (2) = 0.0000
```

### Example 3

A scale operator can be saved and loaded: its coefficient is part of what is stored.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <sstream>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x * x; });

  std::stringstream bytes(std::ios::in | std::ios::out | std::ios::binary);
  math::io::save(operators::scale(2.5), bytes);
  const operators::Operator restored = math::io::load_operator(bytes);
  out.writeln("restored key:", restored.key(), "  restored(f)(2) =", restored(f)(2.0).item<double>(), "  (2.5 * 8 = 20)");
}
```

Output

```text
restored key: scale   restored(f)(2) = 20.0000   (2.5 * 8 = 20)
```

## See Also

- [`multiply`](multiply.md): multiply by a function instead of a number
- [`linear_combination`](linear_combination.md): sums scaled operators

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="multiply.html">
    <span class="label">Previous</span>
    <span class="title">multiply</span>
  </a>
  <a class="next-link" href="shift.html">
    <span class="label">Next</span>
    <span class="title">shift</span>
  </a>
</div>
<!-- apiref:nav:end -->
