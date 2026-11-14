<!-- generated from geometry_operators.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Geometry operators

[Back to Operators](../calculus_05_operators.md)

On a curved space the derivative of a vector field needs a correction for the way the coordinate frame turns. The geometry operators build that correction as ordinary `Function`s: the Levi-Civita connection of a metric, the covariant derivative of a tensor field along a coordinate, and the absolute (intrinsic) differential of a tensor along a curve. Indices follow one convention everywhere: contravariant axes first, then covariant axes, described by a `TensorSignature{contravariant, covariant}`.

| Entry point | Result |
|---|---|
| `levi_civita(metric, coordinates)` | the connection `Γ[i][j][k] = Γⁱ_jk` of the metric `g_ij(x)`, a rank-3 `Function` |
| `covariant(coordinate, connection, signature)` | the operator `∇_k V = ∂_k V + Γ-terms` for the chosen coordinate |
| `absolute_differential(curve, connection, parameter, signature)` | the operator `DV/dt = dV/dt + Γ-terms · dxᵏ/dt` along a curve |

```cpp
math::Function operators::geometry::levi_civita(const math::Function &metric, StackArray<std::string_view> coordinates = {}, DiffOptions options = {});
operators::Operator operators::geometry::covariant(std::string_view coordinate, math::Function connection, TensorSignature signature, DiffOptions options = {});
operators::Operator operators::geometry::absolute_differential(math::Function curve, math::Function connection, std::string_view parameter, TensorSignature signature = {1, 0}, DiffOptions options = {});
```

## The sphere

For the unit sphere `g = diag(1, sin²θ)` the nonzero symbols are `Γ^θ_φφ = −sinθ cosθ` and `Γ^φ_θφ = Γ^φ_φθ = cotθ`. The covariant derivative along `φ` of the field `V = (0, 1)` (unit `φ` component) is then `(∇_φ V)^θ = Γ^θ_φφ`, `(∇_φ V)^φ = 0`. Along the great circle `θ = π/2` the absolute differential of a vector with constant components vanishes, because the equator is a geodesic.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace geometry = pluss::operators::geometry;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function metric = math::function({"theta", "phi"}, [](math::Function th, math::Function) {
    const math::Function s = th.sin() * th.sin(), one = th * 0.0 + 1.0, zero = th * 0.0;
    return math::stack({math::stack({one, zero}), math::stack({zero, s})});
  });
  const math::Function gamma = geometry::levi_civita(metric);   // staged: keep the connection
  const double theta = 1.0;
  out.writeln("Gamma[i][j][k], i,j,k in {theta, phi}: ", gamma({pluss::make_series(theta), pluss::make_series(0.3)}).reshape({8}).items<double>());
  out.writeln("-sin cos = ", -std::sin(theta) * std::cos(theta), "   cot = ", 1.0 / std::tan(theta));

  const math::Function field = math::function({"theta", "phi"}, [](math::Function th, math::Function) { return math::stack({th * 0.0, th * 0.0 + 1.0}); });
  const math::Function nabla_phi = geometry::covariant("phi", gamma, {1, 0})(field);   // compact: apply at once
  out.writeln("nabla_phi V = ", nabla_phi({pluss::make_series(theta), pluss::make_series(0.3)}).items<double>());

  const math::Function curve = math::function({"t"}, [](math::Function t) { return math::stack({t * 0.0 + std::acos(-1.0) / 2, t}); });
  const math::Function transported = math::function({"t"}, [](math::Function t) { return math::stack({t * 0.0, t * 0.0 + 1.0}); });
  const math::Function D = geometry::absolute_differential(curve, gamma, "t")(transported);
  out.writeln("DV/dt on the equator = ", D({pluss::make_series(0.7)}).items<double>());
}
```

Output

```text
Gamma[i][j][k], i,j,k in {theta, phi}:  [0.000000, 0.000000, 0.000000, -0.454649, 0.000000, 0.642093, 0.642093, 0.000000]
-sin cos =  -0.454649    cot =  0.642093
nabla_phi V =  [-0.454649, 0.000000]
DV/dt on the equator =  [-0.000000, 0.000000]
```

## What can be saved

`covariant` and `absolute_differential` hold a connection and cannot be saved; the connection and the functions they produce save like any other `Function`.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fourier_series.html">
    <span class="label">Previous</span>
    <span class="title">Fourier series operators</span>
  </a>
  <a class="next-link" href="integral_transforms.html">
    <span class="label">Next</span>
    <span class="title">Fourier and Laplace transforms</span>
  </a>
</div>
<!-- apiref:nav:end -->
