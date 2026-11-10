<!-- generated from rank.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d12b38b28f6a -->

# `math::Function::rank`

[Back to `math::Function`](../Function.md)

The number of output axes of one evaluation: 0 for a scalar, 1 for a vector, 2 for a matrix, and so on.

## Signature

```cpp
std::int64_t math::Function::rank() const;
```

## What It Does

A function of scalar inputs produces a value, and that value need not be a single number. `rank()` is the number of axes of that value: a scalar formula has rank 0, a function that returns a vector of components has rank 1, a function that returns a matrix has rank 2. The rank is a property of the *function*, fixed when it is built; it does not change with the data you evaluate it on.

When you evaluate on arrays, batch axes are added in *front* of the output axes: a rank-1 function of three components evaluated on five points returns a `5 × 3` result. So `rank()` tells you how many trailing axes are the function's own, and the remaining leading axes are your batch.

You call `rank()` to find out what kind of object a function is, usually after an operator produced it: `gradient` of a scalar field is rank 1, `jacobian` of a vector field is rank 2, `hessian` of a scalar field is rank 2. It is also the guard for the tensor methods: `trace`, `det` and `inverse` need rank 2, `matmul` needs a trailing axis to contract. [`shape()`](shape.md) gives the length of each of those axes.

## Returns

The number of output axes, a non-negative integer.

## Errors

- The function must be defined.

## Practical Examples

### Example 1

Scalar formulas have rank 0. Stacking scalars gives a vector (rank 1); the Jacobian of a vector function and the Hessian of a scalar one are matrices (rank 2).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function field = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * x * y; });
  const math::Function vec = math::function({"x", "y"}, [](math::Function x, math::Function y) { return math::stack({x * y, x + y, x * x}); });

  out.writeln("scalar field:           ", field.rank());
  out.writeln("stack of three:         ", vec.rank());
  out.writeln("gradient of the field:  ", operators::gradient()(field).rank());
  out.writeln("jacobian of the stack:  ", operators::jacobian()(vec).rank());
  out.writeln("hessian of the field:   ", operators::hessian()(field).rank());
}
```

Output

```text
scalar field:            0
stack of three:          1
gradient of the field:   1
jacobian of the stack:   2
hessian of the field:    2
```

Each operator adds one output axis: the gradient turns a scalar into a vector, and the Jacobian and Hessian turn a vector (or the gradient of a scalar) into a matrix.

### Example 2

The rank counts only the function's own axes. Evaluated on five points, the rank-1 function below returns a `5 × 3` result: the batch axis comes first, and the rank is still 1.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function vec = math::function({"t"}, [](math::Function t) { return math::stack({t, t * t, t * t * t}); });
  const auto t = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
  const pluss::Series values = vec({t});
  out.writeln("function rank:", vec.rank());
  out.writeln("result dim:   ", values.dim(), " (one batch axis + the function's rank)");
  out.writeln("result shape: ", values.sizes());
}
```

Output

```text
function rank: 1
result dim:    2  (one batch axis + the function's rank)
result shape:  [5, 3]
```

The result has `1 + rank` axes: one batch axis of length 5, then the three components.

### Example 3

Branching on the rank before using a tensor method: `trace` needs a matrix, so apply it to a rank-2 function and say what to do otherwise.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function scalar = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * x + 3.0 * y * y; });
  const math::Function matrix = operators::hessian()(scalar);

  for (const math::Function &f : {scalar, matrix}) {
    if (f.rank() == 2) {
      out.writeln("rank 2: trace =", f.trace()({pluss::make_series(1.0), pluss::make_series(1.0)}).item<double>());
    } else {
      out.writeln("rank", f.rank(), ": not a matrix, no trace");
    }
  }
}
```

Output

```text
rank 0 : not a matrix, no trace
rank 2: trace = 8.0000
```

The Hessian of `x² + 3y²` is `diag(2, 6)`, whose trace is 8, the Laplacian.

## See Also

- [`shape`](shape.md): the length of each output axis
- [`trace`](trace.md): needs a rank-2 function
- [`operators::jacobian`](../../differential/jacobian.md): builds a rank-2 function from a vector field

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="provenance.html">
    <span class="label">Previous</span>
    <span class="title">provenance</span>
  </a>
  <a class="next-link" href="real.html">
    <span class="label">Next</span>
    <span class="title">real</span>
  </a>
</div>
<!-- apiref:nav:end -->
