<!-- generated from jacobians_and_hessians.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Jacobians and Hessians

[Back to Vector Calculus](../calculus_03_vector_calculus.md)

Two operators extend the gradient to the other shapes of derivative. Both take the same optional variable list and `DiffOptions` as `gradient`.

| Operator | Input | Output |
|---|---|---|
| `jacobian(vars)` | vector-valued, of output length `m` | the `m × n` matrix of first derivatives, rows for outputs, columns for the `n` selected variables |
| `jacobian(vars)` | scalar-valued | the gradient, of length `n` |
| `hessian(vars)` | scalar-valued | the `n × n` matrix of second derivatives |

In both cases the derivative axes come **last**, after the batch axes of the arguments. The entry at row `i` and column `j` of the Jacobian is ∂fᵢ/∂xⱼ, and of the Hessian ∂²f/∂xᵢ∂xⱼ with `i` and `j` indexing the selected variables in the order selected.

## The Hessian

For P(x, y) = x³ + xy + y² the Hessian is [[6x, 1], [1, 2]]. The program evaluates it at three points, with the variables in the declared and in the reversed order:

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  const math::Function potential = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x.pow(3) + x * y + y.pow(2); });
  const math::Function hessian = operators::hessian()(potential);
  const math::Function reversed = operators::hessian({"y", "x"})(potential);
  out.writeln("rank", hessian.rank(), " shape", hessian.shape());

  const auto x = pluss::make_series({0.5, 1.0, 2.0});
  const auto y = pluss::make_series({1.5, 2.0, 3.0});
  const auto h = hessian({x, y});
  const auto r = reversed({x, y});
  out.writeln("result shape", h.sizes());
  for (std::int64_t point = 0; point < 3; ++point) {
    const auto m = h.select(0, point);
    const auto q = r.select(0, point);
    out.writeln("x =", x.select(0, point).item<double>(), " Hessian", m.select(0, 0).pull<double>(), m.select(0, 1).pull<double>(),
                "  in (y, x) order", q.select(0, 0).pull<double>(), q.select(0, 1).pull<double>());
  }
}
```

Output

```text
rank 2  shape [2, 2]
result shape [3, 2, 2]
x = 0.50  Hessian [3.00, 1.00] [1.00, 2.00]   in (y, x) order [2.00, 1.00] [1.00, 3.00]
x = 1.00  Hessian [6.00, 1.00] [1.00, 2.00]   in (y, x) order [2.00, 1.00] [1.00, 6.00]
x = 2.00  Hessian [12.00, 1.00] [1.00, 2.00]   in (y, x) order [2.00, 1.00] [1.00, 12.00]
```

The Hessian is `[2, 2]` for one point and `[3, 2, 2]` for three: the two derivative axes come last. At x = 0.5 the matrix is [[3, 1], [1, 2]], because ∂²P/∂x² = 6x = 3, the cross derivative is 1 and ∂²P/∂y² = 2. The matrix is symmetric, as the Hessian of a smooth function is. With the selector `{"y", "x"}` the rows and columns are exchanged, so the entries 2 and 6x swap places: [[2, 1], [1, 3]] at x = 0.5.

## The Jacobian

The Jacobian of a vector-valued function has one row per output. Vector-valued functions come from the operators themselves (the gradient is one), or from combining scalar formulas with constant vectors. The program builds the map

v(x, y) = (xy, x + y, x²)

from three constant basis vectors, so that the output has length 3. The Jacobian is [[y, x], [1, 1], [2x, 0]], and it is evaluated at a single point, `(2, 3)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(1));
  const auto basis = [](std::vector<double> e) { return math::function(pluss::make_series(e)); };
  const math::Function e1 = basis({1, 0, 0}), e2 = basis({0, 1, 0}), e3 = basis({0, 0, 1});
  const math::Function v = math::function({"x", "y"}, [&](math::Function x, math::Function y) { return x * y * e1 + (x + y) * e2 + x.pow(2) * e3; });

  const auto point_x = pluss::make_series(2.0, as_f64());
  const auto point_y = pluss::make_series(3.0, as_f64());
  out.writeln("v rank", v.rank(), " shape", v.shape(), " v(2, 3) =", v({point_x, point_y}).pull<double>());

  const math::Function jacobian = operators::jacobian()(v);
  out.writeln("Jacobian rank", jacobian.rank(), " shape", jacobian.shape());
  const auto j = jacobian({point_x, point_y});
  for (std::int64_t row = 0; row < 3; ++row) out.writeln("  row", row, j.select(0, row).pull<double>());

  const auto jyx = operators::jacobian({"y", "x"})(v)({point_x, point_y});
  out.writeln("columns in (y, x) order:");
  for (std::int64_t row = 0; row < 3; ++row) out.writeln("  row", row, jyx.select(0, row).pull<double>());

  const auto jy = operators::jacobian({"y"})(v)({point_x, point_y});
  out.writeln("only y: shape", jy.sizes());
}
```

Output

```text
v rank 1  shape [3]  v(2, 3) = [6.0, 5.0, 4.0]
Jacobian rank 2  shape [3, 2]
  row 0 [3.0, 2.0]
  row 1 [1.0, 1.0]
  row 2 [4.0, 0.0]
columns in (y, x) order:
  row 0 [2.0, 3.0]
  row 1 [1.0, 1.0]
  row 2 [0.0, 4.0]
only y: shape [3, 1]
```

The function `v` has rank 1 and shape `[3]`, and the Jacobian has shape `[3, 2]`: row `i` holds the derivatives of the i-th output with respect to `x` and `y`. At (2, 3): the first output xy has derivatives (y, x) = (3, 2); the second, x + y, has (1, 1); the third, x², has (2x, 0) = (4, 0). With the selector `{"y", "x"}` the columns are exchanged, and selecting `y` alone leaves a single column of shape `[3, 1]`.

The Jacobian of the gradient is the Hessian. This is not a coincidence but a definition, and the identity gives a check on a formula:

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function potential = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x.pow(3) + x * y + y.pow(2); });
  const math::Function gradient = operators::gradient()(potential);
  const math::Function jacobian_of_gradient = operators::jacobian()(gradient);
  const math::Function hessian = operators::hessian()(potential);

  const auto x = pluss::make_series({0.5, 1.0, 2.0});
  const auto y = pluss::make_series({1.5, 2.0, 3.0});
  out.writeln("gradient rank", gradient.rank(), " Jacobian rank", jacobian_of_gradient.rank(), " Hessian rank", hessian.rank());
  out.writeln("equal:", jacobian_of_gradient({x, y}).allclose(hessian({x, y})) ? "yes" : "no");

  // The Jacobian of a scalar function is its gradient.
  out.writeln("Jacobian of a scalar function equals the gradient:",
              operators::jacobian()(potential)({x, y}).allclose(gradient({x, y})) ? "yes" : "no");
}
```

Output

```text
gradient rank 1  Jacobian rank 2  Hessian rank 2
equal: yes
Jacobian of a scalar function equals the gradient: yes
```

The Jacobian of the gradient equals the Hessian to the tolerance of `allclose` (the derivative formulas are the same, built in two orders), and the Jacobian of a scalar function is its gradient.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->
