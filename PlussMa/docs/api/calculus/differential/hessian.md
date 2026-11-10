<!-- generated from hessian.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::hessian`

The matrix of second partial derivatives of a scalar function.

## Signature

```cpp
operators::Operator operators::hessian(pluss::StackArray<std::string_view> variables = {}, operators::DiffOptions options = {});
```

## What It Does

`hessian()` is the operator `f ↦ H` with `H[i][j] = ∂²f/∂xᵢ∂xⱼ`, a symmetric matrix of shape `[n, n]` for `n` variables. With no arguments all variables are used, in declaration order; naming variables restricts both axes to those variables.

The Hessian describes curvature. At a critical point (`∇f = 0`) its definiteness decides the character of the point: positive definite for a minimum, negative definite for a maximum, indefinite for a saddle. It is the matrix in Newton's method for minimisation, `x ← x − H⁻¹∇f`, in the second-order Taylor expansion `f(x + h) ≈ f(x) + ∇f·h + ½ hᵀHh`, and in the Laplace approximation of an integral. Its trace is the Laplacian.

The input must be scalar-valued.

## Parameters

`pluss::StackArray<std::string_view> variables`
: The variables for both axes, in order. Empty (the default) means all variables in declaration order.

`operators::DiffOptions options`
: Non-smooth policy, fallback and size limit.

## Returns

An `Operator` whose application returns the Hessian: a rank-2 `Function` of shape `[n, n]`.

## Errors

- The input must be scalar-valued: `input Function must be scalar-valued`.
- An unknown variable raises `unknown variable '<name>'`.

## Practical Examples

### Example 1

The Hessian of `f = xy + yz²` at `(1, 2, 3)`. The matrix is symmetric: `f_xy = 1`, `f_yz = 2z = 6`, `f_zz = 2y = 4`, and the rest are zero.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x", "y", "z"}, [](math::Function x, math::Function y, math::Function z) { return x * y + y * z * z; });
  const auto point = {pluss::make_series(1.0), pluss::make_series(2.0), pluss::make_series(3.0)};
  const math::Function H = operators::hessian()(f);
  out.writeln("shape:", H.shape());
  out.writeln("H, row by row:", H(point).reshape({9}).items<double>());
}
```

Output

```text
shape: [3, 3]
H, row by row: [0.0000, 1.0000, 0.0000, 1.0000, 0.0000, 6.0000, 0.0000, 6.0000, 4.0000]
```

### Example 2

A subset of the variables: the Hessian with respect to `x` and `z` only is the `[2, 2]` block of those rows and columns.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x", "y", "z"}, [](math::Function x, math::Function y, math::Function z) { return x * y + y * z * z; });
  const auto point = {pluss::make_series(1.0), pluss::make_series(2.0), pluss::make_series(3.0)};
  const math::Function H = operators::hessian({"x", "z"})(f);
  out.writeln("shape:", H.shape(), " H =", H(point).reshape({4}).items<double>(), " (f_xx, f_xz, f_zx, f_zz)");
}
```

Output

```text
shape: [2, 2]  H = [0.0000, 0.0000, 0.0000, 4.0000]  (f_xx, f_xz, f_zx, f_zz)
```

### Example 3

Classifying a critical point by the determinant and trace of the Hessian. `x² − y²` has a saddle at the origin (negative determinant); `x² + y²` a minimum (positive determinant, positive trace).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto classify = [&](const char *label, const math::Function &f) {
    const math::Function H = operators::hessian()(f);
    const auto origin = {pluss::make_series(0.0), pluss::make_series(0.0)};
    const double det = H.det()(origin).item<double>();
    const double trace = H.trace()(origin).item<double>();
    out.writeln(label, "det H =", det, " trace H =", trace, det < 0 ? "  saddle" : (trace > 0 ? "  minimum" : "  maximum"));
  };
  classify("x^2 - y^2:        ", math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * x - y * y; }));
  classify("x^2 + y^2:        ", math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * x + y * y; }));
  classify("-(x^2 + 2 y^2):   ", math::function({"x", "y"}, [](math::Function x, math::Function y) { return -(x * x + 2.0 * y * y); }));
}
```

Output

```text
x^2 - y^2:         det H = -4.0000  trace H = 0.0000   saddle
x^2 + y^2:         det H = 4.0000  trace H = 4.0000   minimum
-(x^2 + 2 y^2):    det H = 8.0000  trace H = -6.0000   maximum
```

### Example 4

A vector-valued function has no Hessian: the input must be scalar-valued.

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
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  const math::Function vector = math::function({"x", "y"}, [](math::Function x, math::Function y) { return math::stack({x, y}); });
  try { (void)operators::hessian()(vector); } catch (const pluss::Error &e) { out.writeln(first_line(e)); }
}
```

Output

```text
operators::hessian: input Function must be scalar-valued
```

## See Also

- [`gradient`](gradient.md): first derivatives
- [`jacobian`](jacobian.md): vector-valued functions
- [`math::Function::trace`](../functions/Function/trace.md): the trace is the Laplacian

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gradient.html">
    <span class="label">Previous</span>
    <span class="title">gradient</span>
  </a>
  <a class="next-link" href="jacobian.html">
    <span class="label">Next</span>
    <span class="title">jacobian</span>
  </a>
</div>
<!-- apiref:nav:end -->
