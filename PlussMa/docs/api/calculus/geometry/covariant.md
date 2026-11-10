<!-- generated from covariant.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::geometry::covariant`

The covariant derivative of a tensor field in the direction of one coordinate: the ordinary derivative plus the connection terms for each index.

## Signature

```cpp
operators::Operator operators::geometry::covariant(std::string_view coordinate, math::Function connection, operators::geometry::TensorSignature signature, operators::DiffOptions options = {});
```

## What It Does

On a curved space the components of a tensor field change from point to point for two reasons: the field changes, and the coordinate frame turns. The *covariant derivative* removes the second effect. In the direction of coordinate `k`:

- for each contravariant (upper) index `i`: add `Γⁱ_mk Vᵐ`,
- for each covariant (lower) index `j`: subtract `Γᵐ_jk V_m`.

`covariant("phi", gamma, {1, 0})` is the operator `V ↦ ∇_φ V` for a vector field, `{0, 1}` for a covector, `{1, 1}` for a mixed tensor, `{0, 0}` for a scalar (where it is the ordinary derivative). The [`TensorSignature`](TensorSignature.md) states how many upper and lower indices the field has; the field's output rank must equal their sum. The coordinate must be one of the connection's variables, and the connection comes from [`levi_civita`](levi_civita.md) (or any `Function` of shape `[n, n, n]`).

Two properties are worth knowing. The Levi-Civita covariant derivative of the metric is zero (the connection is *metric-compatible*), and so is that of the Kronecker delta; both appear in the examples as checks. The operator holds a connection function and cannot be saved.

## Parameters

`std::string_view coordinate`
: The coordinate to differentiate along. It must be one of the connection's variables.

`math::Function connection`
: The connection `Γ[i][j][k]`, a `Function` of shape `[n, n, n]`, usually from `levi_civita`.

`operators::geometry::TensorSignature signature`
: The number of contravariant and covariant indices of the field.

`operators::DiffOptions options`
: Options for the derivative.

## Returns

An `Operator` whose application returns `∇_k T`, a `Function` with the same output shape as the field.

## Errors

- `the coordinate must be one of the connection's variables`.
- `the tensor's output rank must equal its signature`.

## Practical Examples

### Example 1

A vector field on the unit sphere. The unit `φ`-direction field `V = (0, 1)` has constant components, but it turns relative to the frame: `(∇_φ V)^θ = Γ^θ_φφ V^φ = −sin θ cos θ` and `(∇_φ V)^φ = 0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace geometry = pluss::operators::geometry;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const math::Function metric = math::function({"theta", "phi"}, [](math::Function theta, math::Function) {
    const math::Function one = theta * 0.0 + 1.0, zero = theta * 0.0, s = theta.sin() * theta.sin();
    return math::stack({math::stack({one, zero}), math::stack({zero, s})});   // diag(1, sin^2 theta)
  });
  const math::Function gamma = operators::geometry::levi_civita(metric);
  const auto point = {pluss::make_series(1.0), pluss::make_series(0.3)};   // theta = 1, phi = 0.3

  const math::Function V = math::function({"theta", "phi"}, [](math::Function theta, math::Function) { return math::stack({theta * 0.0, theta * 0.0 + 1.0}); });
  const math::Function nabla = operators::geometry::covariant("phi", gamma, {1, 0})(V);
  out.writeln("nabla_phi V:", nabla(point).items<double>(), "  (-sin cos, 0) = (", -std::sin(1.0) * std::cos(1.0), ", 0 )");
}
```

Output

```text
nabla_phi V: [-0.45465, 0.00000]   (-sin cos, 0) = ( -0.45465 , 0 )
```

### Example 2

A covector (lower index): the sign flips and the contraction is different. For `ω = (0, 1)`: `(∇_φ ω)_θ = −Γ^φ_θφ ω_φ = −cot θ`. A scalar field with signature `{0, 0}` has no index to correct: it is the ordinary derivative.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace geometry = pluss::operators::geometry;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const math::Function metric = math::function({"theta", "phi"}, [](math::Function theta, math::Function) {
    const math::Function one = theta * 0.0 + 1.0, zero = theta * 0.0, s = theta.sin() * theta.sin();
    return math::stack({math::stack({one, zero}), math::stack({zero, s})});   // diag(1, sin^2 theta)
  });
  const math::Function gamma = operators::geometry::levi_civita(metric);
  const auto point = {pluss::make_series(1.0), pluss::make_series(0.3)};   // theta = 1, phi = 0.3

  const math::Function omega = math::function({"theta", "phi"}, [](math::Function theta, math::Function) { return math::stack({theta * 0.0, theta * 0.0 + 1.0}); });
  out.writeln("nabla_phi omega:", operators::geometry::covariant("phi", gamma, {0, 1})(omega)(point).items<double>(), "  (-cot 1 =", -1.0 / std::tan(1.0), ", 0)");

  const math::Function scalar = math::function({"theta", "phi"}, [](math::Function theta, math::Function phi) { return theta * phi; });
  out.writeln("nabla_phi (theta phi) =", operators::geometry::covariant("phi", gamma, {0, 0})(scalar)(point).item<double>(), "  (= theta = 1)");
}
```

Output

```text
nabla_phi omega: [-0.64209, 0.00000]   (-cot 1 = -0.64209 , 0)
nabla_phi (theta phi) = 1.00000   (= theta = 1)
```

### Example 3

Checks that the Levi-Civita connection is metric-compatible. The covariant derivative of the metric itself (signature `{0, 2}`) is zero in every direction, and so is that of the identity (the Kronecker delta, signature `{1, 1}`).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace geometry = pluss::operators::geometry;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const math::Function metric = math::function({"theta", "phi"}, [](math::Function theta, math::Function) {
    const math::Function one = theta * 0.0 + 1.0, zero = theta * 0.0, s = theta.sin() * theta.sin();
    return math::stack({math::stack({one, zero}), math::stack({zero, s})});   // diag(1, sin^2 theta)
  });
  const math::Function gamma = operators::geometry::levi_civita(metric);
  const auto point = {pluss::make_series(1.0), pluss::make_series(0.3)};   // theta = 1, phi = 0.3

  const math::Function delta = math::function({"theta", "phi"}, [](math::Function theta, math::Function) {
    const math::Function one = theta * 0.0 + 1.0, zero = theta * 0.0;
    return math::stack({math::stack({one, zero}), math::stack({zero, one})});
  });
  for (const char *direction : {"theta", "phi"}) {
    const double on_metric = operators::geometry::covariant(direction, gamma, {0, 2})(metric)(point).reshape({4}).abs().max().item<double>();
    const double on_delta = operators::geometry::covariant(direction, gamma, {1, 1})(delta)(point).reshape({4}).abs().max().item<double>();
    out.writeln("direction", direction, ": max|nabla g| =", on_metric, "  max|nabla delta| =", on_delta);
  }
}
```

Output

```text
direction theta : max|nabla g| = 0.0000000000   max|nabla delta| = 0.0000000000
direction phi : max|nabla g| = 0.0000000000   max|nabla delta| = 0.0000000000
```

### Example 4

The checks: an unknown coordinate and a signature that does not match the field's rank are each refused with a message that names the problem.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace geometry = pluss::operators::geometry;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function metric = math::function({"theta", "phi"}, [](math::Function theta, math::Function) {
    const math::Function one = theta * 0.0 + 1.0, zero = theta * 0.0, s = theta.sin() * theta.sin();
    return math::stack({math::stack({one, zero}), math::stack({zero, s})});   // diag(1, sin^2 theta)
  });
  const math::Function gamma = operators::geometry::levi_civita(metric);
  const auto point = {pluss::make_series(1.0), pluss::make_series(0.3)};   // theta = 1, phi = 0.3
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  const math::Function V = math::function({"theta", "phi"}, [](math::Function theta, math::Function) { return math::stack({theta * 0.0, theta * 0.0 + 1.0}); });
  try { (void)operators::geometry::covariant("z", gamma, {1, 0})(V); } catch (const pluss::Error &e) { out.writeln("unknown coordinate:", first_line(e)); }
  try { (void)operators::geometry::covariant("phi", gamma, {2, 0})(V); } catch (const pluss::Error &e) { out.writeln("wrong signature:   ", first_line(e)); }
}
```

Output

```text
unknown coordinate: geometry::covariant: the coordinate must be one of the connection's variables
wrong signature:    geometry::covariant: the tensor's output rank must equal its signature
```

## See Also

- [`levi_civita`](levi_civita.md): builds the connection
- [`absolute_differential`](absolute_differential.md): the derivative along a curve
- [`operators::covariant`](../differential/covariant.md): the scalar-section form
- [`TensorSignature`](TensorSignature.md): index counts

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="absolute_differential.html">
    <span class="label">Previous</span>
    <span class="title">absolute_differential</span>
  </a>
  <a class="next-link" href="levi_civita.html">
    <span class="label">Next</span>
    <span class="title">levi_civita</span>
  </a>
</div>
<!-- apiref:nav:end -->
