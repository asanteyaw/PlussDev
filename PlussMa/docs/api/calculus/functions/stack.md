<!-- generated from stack.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `math::stack`

Assembles a vector- or matrix-valued function from several functions, adding a trailing output axis.

## Signature

```cpp
math::Function math::stack(pluss::RangeView<math::Function> components);
math::Function math::stack(std::initializer_list<math::Function> components);
```

## What It Does

`math::stack({a, b, c})` builds the function whose value is the vector `[a, b, c]`: the components become the entries along a new trailing output axis. Stacking scalar functions gives a rank-1 function (a vector); stacking vector functions of equal length gives a rank-2 function whose *columns* are those vectors, which is how the [`jacobian`](../differential/jacobian.md) is laid out; stacking matrices adds a third axis, and so on. All components must have the same output shape.

Stacking is how a system of equations, a vector field, a point on a curve or a parameter vector is written as one function. The components may be of different kinds (a constant among variables is fine) as long as they belong to one variable trace, so a stack must be assembled inside a single `math::function(...)` callable, or from functions derived from one another. Once built, operators act on the whole vector: `jacobian` and `divergence` read it as a vector field, and [`matmul`](Function/matmul.md) multiplies it by a matrix.

The `RangeView` overload takes the components as a range, for a number of components that is only known at run time.

## Parameters

`std::initializer_list<math::Function> components`
: The functions to stack, all with the same output shape and from one variable trace (or constants). The `RangeView` overload takes them as a range.

## Returns

A new `Function` with one more output axis than the components, of length equal to their number.

## Errors

- `stacked outputs must have equal shapes` when the components differ in shape.
- Components from different traces are refused.
- `math::stack: at least one component is required` for an empty list.

## Practical Examples

### Example 1

Scalars stacked into a vector: `(x, x², sin x)` is a rank-1 function with shape `[3]`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function v = math::function({"x"}, [](math::Function x) { return math::stack({x, x * x, x.sin()}); });
  out.writeln("rank", v.rank(), "shape", v.shape());
  out.writeln("v(2) =", v(2.0).items<double>());
}
```

Output

```text
rank 1 shape [3]
v(2) = [2.0000, 4.0000, 0.9093]
```

### Example 2

Vectors stacked into a matrix: two vectors of three components give a `[3, 2]` function whose columns are the vectors. A constant may sit among the components.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function m = math::function({"x"}, [](math::Function x) {
    const math::Function first = math::stack({x, x + 1.0, x + 2.0});
    const math::Function second = math::stack({math::function(10.0), x * 0.0 + 20.0, x * 0.0 + 30.0});
    return math::stack({first, second});
  });
  out.writeln("rank", m.rank(), "shape", m.shape());
  out.writeln("m(1) =", m(1.0).reshape({6}).items<double>(), " (column 0 = [1, 2, 3], column 1 = [10, 20, 30])");
}
```

Output

```text
rank 2 shape [3, 2]
m(1) = [1.0000, 10.0000, 2.0000, 20.0000, 3.0000, 30.0000]  (column 0 = [1, 2, 3], column 1 = [10, 20, 30])
```

### Example 3

A run-time number of components. The `RangeView` overload takes them from a vector: the monomials `1, x, x², x³, …` up to a degree chosen at run time.

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
  const int degree = 4;
  const math::Function monomials = math::function({"x"}, [&](math::Function x) {
    std::vector<math::Function> terms;
    math::Function power = x * 0.0 + 1.0;
    for (int k = 0; k <= degree; ++k) {
      terms.push_back(power);
      power = power * x;
    }
    return math::stack(pluss::RangeView<math::Function>(terms));
  });
  out.writeln("shape", monomials.shape());
  out.writeln("monomials(2) =", monomials(2.0).items<double>());
}
```

Output

```text
shape [5]
monomials(2) = [1.0000, 2.0000, 4.0000, 8.0000, 16.0000]
```

### Example 4

The components must agree in shape and in trace; an empty list is refused too.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <functional>
#include <string>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto report = [&](const char *label, const std::function<void()> &call) {
    try { call(); } catch (const pluss::Error &e) { const std::string text(e.what()); out.writeln(label, text.substr(0, text.find('\n'))); }
  };

  report("different shapes: ", [&] { (void)math::function({"x"}, [](math::Function x) { return math::stack({x, math::stack({x, x})}); }); });
  report("different traces: ", [&] {
    const math::Function a = math::function({"x"}, [](math::Function x) { return x; });
    const math::Function b = math::function({"x"}, [](math::Function x) { return x * 2.0; });
    (void)math::stack({a, b});
  });
  report("no components:    ", [&] { std::vector<math::Function> none; (void)math::stack(pluss::RangeView<math::Function>(none)); });
}
```

Output

```text
different shapes:  math::Function: stacked outputs must have equal shapes
different traces:  math::Function: expressions from different variable traces cannot be combined directly
no components:     math::stack: at least one component is required
```

## See Also

- [`function`](function_factory.md): builds the components
- [`select`](Function/select.md): takes a component back out
- [`shape`](Function/shape.md): the stacked axis shows up here

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="operator_sub.html">
    <span class="label">Previous</span>
    <span class="title">operator-</span>
  </a>
</div>
<!-- apiref:nav:end -->
