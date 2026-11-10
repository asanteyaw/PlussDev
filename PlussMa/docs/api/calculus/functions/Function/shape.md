<!-- generated from shape.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2ad82cc43278 -->

# `math::Function::shape`

[Back to `math::Function`](../Function.md)

The length of each output axis of one evaluation, as a list of integers.

## Signature

```cpp
pluss::RangeView<std::int64_t> math::Function::shape() const;
```

## What It Does

`shape()` lists the lengths of a function's output axes, in order: an empty list for a scalar, `[3]` for a vector of three components, `[3, 2]` for a three-by-two matrix. Its length is [`rank()`](rank.md). Like the rank it describes one evaluation at scalar inputs, so it never includes the batch axes you add by evaluating on arrays: those lead, and the shape is what trails them.

Use it to know what a derived function looks like before evaluating it. The shape of an operator's output is part of the operator's contract: the Jacobian of a map from `n` inputs to `m` outputs is `[m, n]` (one row per output, one column per input), the Hessian of a scalar field is `[n, n]`, and the connection of a metric on `n` coordinates is `[n, n, n]`. It is also what you check before a tensor method: `matmul` contracts the last axis of the left function with the first of the right one, so those two lengths must agree.

The shape is computed when the function is built, not when it is evaluated, so reading it is free and never fails because of the data.

## Returns

A `RangeView<std::int64_t>` with one entry per output axis; empty for a scalar function.

## Errors

- The function must be defined.

## Practical Examples

### Example 1

The shape of a scalar, a vector, and the matrix that stacking two vectors produces. Stacking adds a trailing axis, so two vectors of three components stack to `[3, 2]`: the vectors are the columns.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

static std::string shape_of(const math::Function &f) {
  std::string text = "[";
  for (const std::int64_t n : f.shape()) text += (text.size() > 1 ? ", " : "") + std::to_string(n);
  return text + "]";
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x", "y"}, [](math::Function x, math::Function y) {
    const math::Function a = math::stack({x, y, x * y});
    const math::Function b = math::stack({x + y, x - y, x / (y + 2.0)});
    return math::stack({a, b});
  });
  const math::Function scalar = math::function({"x"}, [](math::Function x) { return x.exp(); });
  const math::Function vector = math::function({"x", "y"}, [](math::Function x, math::Function y) { return math::stack({x, y, x * y}); });

  out.writeln("scalar: ", shape_of(scalar), " rank", scalar.rank());
  out.writeln("vector: ", shape_of(vector), " rank", vector.rank());
  out.writeln("stacked:", shape_of(f), " rank", f.rank());
}
```

Output

```text
scalar:  []  rank 0
vector:  [3]  rank 1
stacked: [3, 2]  rank 2
```

The scalar's shape is the empty list. Stacking the two vectors of three components gives `[3, 2]`.

### Example 2

What the differential operators produce: the gradient of a scalar field of two variables is `[2]`, the Hessian is `[2, 2]`, and the Jacobian of a map from two variables to three outputs is `[3, 2]` (rows are outputs, columns are inputs).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

static std::string shape_of(const math::Function &f) {
  std::string text = "[";
  for (const std::int64_t n : f.shape()) text += (text.size() > 1 ? ", " : "") + std::to_string(n);
  return text + "]";
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function field = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * x * y; });
  const math::Function map = math::function({"x", "y"}, [](math::Function x, math::Function y) { return math::stack({x * y, x + y, x * x}); });

  out.writeln("gradient of field:", shape_of(operators::gradient()(field)));
  out.writeln("hessian of field: ", shape_of(operators::hessian()(field)));
  out.writeln("jacobian of map:  ", shape_of(operators::jacobian()(map)));
}
```

Output

```text
gradient of field: [2]
hessian of field:  [2, 2]
jacobian of map:   [3, 2]
```

The Jacobian has one row per output and one column per input, so its shape is `[outputs, inputs]`.

### Example 3

A rank-3 function: the Levi-Civita connection of the polar-coordinate metric `diag(1, r²)` has shape `[2, 2, 2]`, indexed `Γ[i][j][k]`. Reading the shape tells you the number of coordinates without evaluating anything.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

static std::string shape_of(const math::Function &f) {
  std::string text = "[";
  for (const std::int64_t n : f.shape()) text += (text.size() > 1 ? ", " : "") + std::to_string(n);
  return text + "]";
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function metric = math::function({"r", "theta"}, [](math::Function r, math::Function) {
    const math::Function one = r * 0.0 + 1.0, zero = r * 0.0;
    return math::stack({math::stack({one, zero}), math::stack({zero, r * r})});
  });
  const math::Function connection = operators::geometry::levi_civita(metric);
  out.writeln("metric:    ", shape_of(metric), " rank", metric.rank());
  out.writeln("connection:", shape_of(connection), " rank", connection.rank());
  out.writeln("coordinates:", connection.shape()[0]);
}
```

Output

```text
metric:     [2, 2]  rank 2
connection: [2, 2, 2]  rank 3
coordinates: 2
```

The first axis of the connection has one entry per coordinate, so `connection.shape()[0]` is the dimension of the space.

## See Also

- [`rank`](rank.md): the number of output axes, which is the length of the shape
- [`matmul`](matmul.md): contracts the last axis of one function with the first of another
- [`operators::jacobian`](../../differential/jacobian.md): produces a `[outputs, inputs]` function

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="select.html">
    <span class="label">Previous</span>
    <span class="title">select</span>
  </a>
  <a class="next-link" href="sin.html">
    <span class="label">Next</span>
    <span class="title">sin</span>
  </a>
</div>
<!-- apiref:nav:end -->
