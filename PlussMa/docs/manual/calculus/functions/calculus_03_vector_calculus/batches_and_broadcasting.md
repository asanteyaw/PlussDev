<!-- generated from batches_and_broadcasting.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Batches and broadcasting

[Back to Vector Calculus](../calculus_03_vector_calculus.md)

Everything in this chapter works on batches. The arguments of a derivative function are `Series` of any shape, the leading axes are batch axes, and the new derivative axes are appended after them. A thousand points are therefore differentiated by one call, and a grid of points by one call on a matrix.

## The rule

If the arguments broadcast to the batch shape `B`, then for a scalar function

| Operator | Result shape |
|---|---|
| `gradient` | `B + [n]` |
| `hessian` | `B + [n, n]` |
| `jacobian` of a vector-valued function with output length `m` | `B + [m, n]` |

where `n` is the number of selected variables. The arguments need not have equal shapes; they broadcast against each other exactly as they do in ordinary array arithmetic. The values for one point do not depend on the other points.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(1));
  const math::Function potential = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x.pow(3) + x * y + y.pow(2); });
  const math::Function gradient = operators::gradient()(potential);
  const math::Function hessian = operators::hessian()(potential);

  // A 2 x 2 grid of points.
  const auto x = pluss::make_series({{0.5, 1.0}, {2.0, 3.0}});
  const auto y = pluss::make_series({{1.5, 2.0}, {3.0, 4.0}});
  const auto g = gradient({x, y});
  out.writeln("grid of points {2, 2}  -> gradient", g.sizes(), " Hessian", hessian({x, y}).sizes());
  out.writeln("point (1, 0) is x = 2, y = 3; its gradient:", g.select(0, 1).select(0, 0).items<double>());

  // A vector of x against one fixed y.
  const auto line = pluss::make_series({1.0, 2.0, 3.0});
  const auto fixed = pluss::make_series(1.0, as_f64());
  const auto h = gradient({line, fixed});
  out.writeln("three x against y = 1 -> gradient", h.sizes());
  out.writeln("  dP/dx", h.select(1, 0).items<double>(), " dP/dy", h.select(1, 1).items<double>());

  // A column against a row: a 2 x 3 grid of points.
  const auto column = pluss::make_series({1.0, 2.0}, {2, 1});
  const auto row = pluss::make_series({10.0, 20.0, 30.0}, {1, 3});
  out.writeln("column [2,1] against row [1,3] -> gradient", gradient({column, row}).sizes());
}
```

Output

```text
grid of points {2, 2}  -> gradient [2, 2, 2]  Hessian [2, 2, 2, 2]
point (1, 0) is x = 2, y = 3; its gradient: [15.0, 8.0]
three x against y = 1 -> gradient [3, 2]
  dP/dx [4.0, 13.0, 28.0]  dP/dy [3.0, 4.0, 5.0]
column [2,1] against row [1,3] -> gradient [2, 3, 2]
```

A `2 × 2` grid of points gives a gradient of shape `[2, 2, 2]` and a Hessian of shape `[2, 2, 2, 2]`: the grid first, then the derivative axes. The point at row 1, column 0 has x = 2 and y = 3, and its gradient (3x² + y, x + 2y) = (15, 8) is the entry found at that position. Three values of `x` against one fixed `y` broadcast to three points and a `[3, 2]` gradient. A column of shape `[2, 1]` against a row of shape `[1, 3]` is the grid of all six pairs, and the gradient is `[2, 3, 2]`.

## Batches and vector-valued functions

The derivative axes are appended after the batch axes, so the gradient of a scalar function of batched arguments is unambiguous. A vector-valued function you assemble yourself from constants is different, because its own output axis is also an axis of the arrays it is combined with. The batch axes of the arguments and the output axis of a constant are aligned like any two arrays, from the trailing end, so a batch of three points combined with a constant of length three is a pairing of elements, not a grid of vectors. Two ways avoid the ambiguity: build vector-valued functions with the operators (`gradient` and `jacobian` always place their axes after the batch), and evaluate hand-assembled ones at one point at a time.

## Mixing precision and platform

The precision and platform of the arguments carry through the derivative functions as they do through any function: `Float32` arguments give a `Float32` gradient, and arguments on the MPS platform are evaluated there.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function potential = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x.pow(3) + x * y + y.pow(2); });
  const math::Function gradient = operators::gradient()(potential);
  const auto x32 = pluss::make_series({0.5F, 1.0F, 2.0F});
  const auto y32 = pluss::make_series({1.5F, 2.0F, 3.0F});
  const auto g32 = gradient({x32, y32});
  out.writeln("Float32 arguments -> Float32 gradient:", g32.scalar_type() == pluss::ScalarType::Float32 ? "yes" : "no");
  out.writeln("on the CPU:", g32.platform() == pluss::pCPU ? "yes" : "no");
  out.writeln("dP/dx", g32.select(1, 0).items<float>(), " dP/dy", g32.select(1, 1).items<float>());
}
```

Output

```text
Float32 arguments -> Float32 gradient: yes
on the CPU: yes
dP/dx [2.2500, 5.0000, 15.0000]  dP/dy [3.5000, 5.0000, 8.0000]
```

The gradient inherits the precision of the arguments, and the values match the double-precision ones of the previous pages to the digits a single-precision number carries.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="jacobians_and_hessians.html">
    <span class="label">Previous</span>
    <span class="title">Jacobians and Hessians</span>
  </a>
  <a class="next-link" href="a_newton_step_from_the_hessian.html">
    <span class="label">Next</span>
    <span class="title">Worked problem: Newton's method on a batch of starts</span>
  </a>
</div>
<!-- apiref:nav:end -->
