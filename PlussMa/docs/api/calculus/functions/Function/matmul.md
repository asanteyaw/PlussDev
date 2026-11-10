<!-- generated from matmul.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fc9456788714 -->

# `math::Function::matmul`

[Back to `math::Function`](../Function.md)

The matrix product: contracts the last output axis of the receiver with the first output axis of the argument.

## Signature

```cpp
math::Function math::Function::matmul(const math::Function &rhs) const;
```

## What It Does

`a.matmul(b)` multiplies two tensor-valued functions in the usual way: the *last* output axis of `a` is summed against the *first* output axis of `b`, and the remaining axes are kept, `a`'s first. So a `[3, 2]` matrix times a `[2]` vector is a `[3]` vector; a `[3, 2]` matrix times a `[2, 4]` matrix is a `[3, 4]` matrix; two vectors of equal length give their dot product, a scalar.

It differs from the elementwise [`operator*`](../operator_mul.md): `*` multiplies matching components and needs shapes that broadcast, `matmul` sums over a shared axis. Use it for linear maps and for chaining them: a linear model applied to features, a Jacobian applied to a direction, a covariance transformed by a loading matrix.

A fixed matrix is brought in with `math::function(matrix_series)`, which makes a constant function. The axis lengths are checked when the product is built, so a mismatch is reported at construction, not at evaluation.

## Parameters

`const math::Function &rhs`
: The right factor. Its first output axis must have the same length as the receiver's last. Both operands must share a variable trace, or one must be a constant such as `math::function(matrix)`, which is how a fixed matrix enters a formula.

## Returns

A new `Function` whose output axes are the receiver's leading axes followed by the argument's trailing axes.

## Errors

- The contracted axes must have equal length.
- Both functions must be defined and share a trace (or one is constant).

## Practical Examples

### Example 1

A constant matrix times a vector function. `A` is `3 × 2` and the vector is `[x, x²]`; the product has shape `[3]`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto A = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({3, 2});  // 3 x 2

  const math::Function f = math::function({"x"}, [&](math::Function x) { return math::function(A).matmul(math::stack({x, x * x})); });
  out.writeln("shape:", f.shape());
  out.writeln("f(2) =", f(2.0).items<double>());
}
```

Output

```text
shape: [3]
f(2) = [10.0000, 22.0000, 34.0000]
```

`A · [2, 4] = [1·2 + 2·4, 3·2 + 4·4, 5·2 + 6·4] = [10, 22, 34]`.

### Example 2

A matrix times a matrix: `[3, 2]` times `[2, 4]` is `[3, 4]`. The constant `B` is the second factor.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto A = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({3, 2});  // 3 x 2

  const auto B = pluss::make_series({1.0, 0.0, 2.0, 1.0, 0.0, 3.0, 1.0, 1.0}).reshape({2, 4});
  const math::Function product = math::function({"x"}, [&](math::Function x) { return math::function(A).matmul(math::function(B)) * x; });
  out.writeln("shape:", product.shape());
  out.writeln("first row at x = 1:", product(1.0).select(0, 0).items<double>());
}
```

Output

```text
shape: [3, 4]
first row at x = 1: [1.0000, 6.0000, 4.0000, 3.0000]
```

The first row of `A B` is `[1, 2]·B = [1·1 + 2·0, 1·2 + 2·3, 1·0 + 2·1, ...]`, here `[1, 6, 4, 3]`.

### Example 3

Two vectors: the product is the dot product, a scalar function (rank 0).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function dot = math::function({"x"}, [](math::Function x) {
    const math::Function u = math::stack({x, x + 1.0, x + 2.0});
    const math::Function w = math::stack({x, x, x});
    return u.matmul(w);
  });
  out.writeln("rank:", dot.rank(), "  value at 1:", dot(1.0).item<double>());
}
```

Output

```text
rank: 0   value at 1: 6.0000
```

### Example 4

A Jacobian applied to a direction gives a directional derivative: `J · d` for `d = (1, 0)` is the partial derivative with respect to the first variable. The direction is a constant function, and the Jacobian shares the trace of the function it came from.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x", "y"}, [](math::Function x, math::Function y) { return math::stack({x * y, x + y * y}); });
  const math::Function jacobian = operators::jacobian()(f);
  const math::Function along_x = jacobian.matmul(math::function(pluss::make_series({1.0, 0.0})));
  const auto point = {pluss::make_series(2.0), pluss::make_series(3.0)};
  out.writeln("J . (1, 0) at (2, 3):", along_x(point).items<double>());
  out.writeln("(d/dx of x y, x + y^2) =", pluss::make_series({3.0, 1.0}).items<double>());
}
```

Output

```text
J . (1, 0) at (2, 3): [3.0000, 1.0000]
(d/dx of x y, x + y^2) = [3.0000, 1.0000]
```

## See Also

- [`contract`](contract.md): contract any pair of axes
- [`operator*`](../operator_mul.md): the elementwise product
- [`transpose`](transpose.md): swap axes

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="log1p.html">
    <span class="label">Previous</span>
    <span class="title">log1p</span>
  </a>
  <a class="next-link" href="mul.html">
    <span class="label">Next</span>
    <span class="title">mul</span>
  </a>
</div>
<!-- apiref:nav:end -->
