<!-- generated from trace.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 93f8ae6d2105 -->

# `math::Function::trace`

[Back to `math::Function`](../Function.md)

The trace of a square matrix function: the sum of its diagonal, as a scalar function.

## Signature

```cpp
math::Function math::Function::trace() const;
```

## What It Does

`f.trace()` is the function `x ↦ tr f(x) = Σ f(x)ᵢᵢ` for a function whose output is a square matrix. The result is scalar (rank 0), one value per matrix on a batch.

The trace is linear and cyclic, `tr(AB) = tr(BA)`. Its most familiar use here is the Laplacian: the trace of the Hessian of a scalar field is `Σ ∂²f/∂xᵢ²`. It is also the sum of eigenvalues, the total variance of a covariance matrix, and the term in Jacobi's formula for the derivative of a determinant.

A non-square output is refused when the trace is built.

## Returns

A new scalar `Function` (rank 0).

## Errors

- A non-square output raises `this operation requires a square matrix output`.

## Practical Examples

### Example 1

The trace of a `2 × 2` matrix function: `2 + 3 = 5` for `[[2, 1], [1, 3]]`, scaled by `x`.

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
  out.writeln("trace at x = 1:", m.trace()(1.0).item<double>());
  out.writeln("trace at x = 4:", m.trace()(4.0).item<double>());
}
```

Output

```text
trace at x = 1: 5.0000
trace at x = 4: 20.0000
```

### Example 2

The Laplacian as the trace of the Hessian. For `f = x² + 3y² + z`, `∇²f = 2 + 6 + 0 = 8`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x", "y", "z"}, [](math::Function x, math::Function y, math::Function z) { return x * x + 3.0 * y * y + z; });
  const math::Function laplacian = operators::hessian()(f).trace();
  out.writeln("Laplacian:", laplacian({pluss::make_series(1.0), pluss::make_series(2.0), pluss::make_series(3.0)}).item<double>());
}
```

Output

```text
Laplacian: 8.0000
```

### Example 3

Cyclicity, `tr(AB) = tr(BA)`, on two non-commuting matrices that depend on `x`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto P = pluss::make_series({1.0, 2.0, 0.0, 3.0}).reshape({2, 2});
  const auto Q = pluss::make_series({0.0, 1.0, 4.0, 1.0}).reshape({2, 2});
  const math::Function ab = math::function({"x"}, [&](math::Function x) { return (math::function(P) * x).matmul(math::function(Q)).trace(); });
  const math::Function ba = math::function({"x"}, [&](math::Function x) { return math::function(Q).matmul(math::function(P) * x).trace(); });
  out.writeln("tr(AB):", ab(2.0).item<double>(), "  tr(BA):", ba(2.0).item<double>());
}
```

Output

```text
tr(AB): 22.0000   tr(BA): 22.0000
```

### Example 4

A vector-valued function has no trace: the output must be a square matrix.

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
  const math::Function v = math::function({"x"}, [](math::Function x) { return math::stack({x, x}); });
  try {
    (void)v.trace();
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
math::Function: this operation requires a square matrix output
```

## See Also

- [`det`](det.md): the determinant
- [`inverse`](inverse.md): the inverse
- [`operators::hessian`](../../differential/hessian.md): its trace is the Laplacian

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tanh.html">
    <span class="label">Previous</span>
    <span class="title">tanh</span>
  </a>
  <a class="next-link" href="transpose.html">
    <span class="label">Next</span>
    <span class="title">transpose</span>
  </a>
</div>
<!-- apiref:nav:end -->
