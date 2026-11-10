<!-- generated from inverse.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 74c75e5e3f15 -->

# `math::Function::inverse`

[Back to `math::Function`](../Function.md)

The matrix inverse of a square matrix function.

## Signature

```cpp
math::Function math::Function::inverse() const;
```

## What It Does

`f.inverse()` is the function `x ↦ f(x)⁻¹` for a function whose output is a square matrix (rank 2, equal axes). When you evaluate on a batch, each matrix of the batch is inverted. The inverse of a function is again a function, so it can be differentiated (`(A⁻¹)′ = −A⁻¹ A′ A⁻¹`), multiplied, or traced.

Use it for linear solves written as functions of parameters (`A(θ)⁻¹ b`), for covariance and precision matrices, and for the Newton step `H⁻¹ ∇f`. A singular matrix has no inverse: the check happens when values arrive, and a singular input raises `inverse domain excludes singular matrices`. A non-square output is refused when the inverse is built.

## Returns

A new `Function` whose value is the inverse matrix, with the same output shape.

## Errors

- A non-square output raises `this operation requires a square matrix output`.
- A singular matrix at evaluation raises `inverse domain excludes singular matrices`.

## Practical Examples

### Example 1

The inverse of a `2 × 2` matrix function. For `[[2, 1], [1, 3]]` scaled by `x = 2`, the inverse is `(1/2)·[[3, −1], [−1, 2]]/5 = [[0.3, −0.1], [−0.1, 0.2]]`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto M = pluss::make_series({2.0, 1.0, 1.0, 3.0}).reshape({2, 2});  // [[2, 1], [1, 3]]

  const math::Function m = math::function({"x"}, [&](math::Function x) { return math::function(M) * x; });
  out.writeln("m(2)^-1:", m.inverse()(2.0).reshape({4}).items<double>());
}
```

Output

```text
m(2)^-1: [0.3000, -0.1000, -0.1000, 0.2000]
```

### Example 2

A linear solve written as a function of a parameter: `x = A⁻¹ b` with `b = (1, 2)`. The check `A x = b` is the product with the original matrix.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto M = pluss::make_series({2.0, 1.0, 1.0, 3.0}).reshape({2, 2});  // [[2, 1], [1, 3]]

  const math::Function solution = math::function({"s"}, [&](math::Function s) {
    return (math::function(M) * s).inverse().matmul(math::function(pluss::make_series({1.0, 2.0})));
  });
  out.writeln("x at s = 1:", solution(1.0).items<double>());
  const auto x = solution(1.0);
  out.writeln("A x:       ", math::function(M).matmul(math::function(x))().items<double>());
}
```

Output

```text
x at s = 1: [0.2000, 0.6000]
A x:        [1.0000, 2.0000]
```

### Example 3

Newton's step from a Hessian: the minimiser of the quadratic `x² + x y + y²` is found in one step as `−H⁻¹ ∇f`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * x + x * y + y * y - 3.0 * x; });
  const math::Function step = -operators::hessian()(f).inverse().matmul(operators::gradient()(f));
  const auto start = {pluss::make_series(5.0), pluss::make_series(-4.0)};
  out.writeln("Newton step from (5, -4):", step(start).items<double>());
  out.writeln("new point:               ", pluss::make_series({5.0 + step(start).select(0, 0).item<double>(), -4.0 + step(start).select(0, 1).item<double>()}).items<double>());
}
```

Output

```text
Newton step from (5, -4): [-3.0000, 3.0000]
new point:                [2.0000, -1.0000]
```

The gradient of `x² + xy + y² − 3x` vanishes at `(2, −1)`; one Newton step lands exactly there because the function is quadratic.

### Example 4

A singular matrix is refused at evaluation, and a non-square one when the inverse is built.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <functional>
#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function singular = math::function({"x"}, [](math::Function x) { return math::function(pluss::make_series({1.0, 2.0, 2.0, 4.0}).reshape({2, 2})) * x; });
  const math::Function wide = math::function({"x"}, [&](math::Function x) { return math::function(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({2, 3})) * x; });
  const auto report = [&](const char *label, const std::function<void()> &call) {
    try { call(); } catch (const pluss::Error &e) { const std::string text(e.what()); out.writeln(label, text.substr(0, text.find('\n'))); }
  };
  report("singular:  ", [&] { (void)singular.inverse()(1.0); });
  report("not square:", [&] { (void)wide.inverse(); });
}
```

Output

```text
singular:   math::Function: inverse domain excludes singular matrices
not square: math::Function: this operation requires a square matrix output
```

## See Also

- [`det`](det.md): the determinant
- [`trace`](trace.md): the trace
- [`matmul`](matmul.md): apply the inverse to a vector

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="imag.html">
    <span class="label">Previous</span>
    <span class="title">imag</span>
  </a>
  <a class="next-link" href="log.html">
    <span class="label">Next</span>
    <span class="title">log</span>
  </a>
</div>
<!-- apiref:nav:end -->
