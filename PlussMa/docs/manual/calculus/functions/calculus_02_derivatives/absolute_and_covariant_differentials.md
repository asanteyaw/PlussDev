<!-- generated from absolute_and_covariant_differentials.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Magnitude, absolute and covariant differentials

[Back to Derivatives](../calculus_02_derivatives.md)

`differential` is the ordinary derivative. Its relatives are separate operators, because they are different operators with different properties and should not be confused with it or with each other.

| Operator | Result for `f` | Linear in `f` |
|---|---|---|
| `differential(x)` | `f'` | yes |
| `magnitude_differential(x, n)` | `abs(f⁽ⁿ⁾)`, the magnitude of the n-th derivative | no |
| `covariant(x, Γ)` | `f' + Γ f`, the derivative corrected by a connection (scalar fields) | yes |
| `geometry::absolute_differential(curve, Γ, t, signature)` | the intrinsic derivative `DV/dt` of a tensor along a curve | yes |
| `geometry::covariant(x, Γ, signature)` | the covariant derivative `∇ₖV` of a tensor field | yes |

All of them accept `DiffOptions` and obey the same policies, budgets and fallback as `differential`. The first three act on scalar fields of one coordinate and are described first; the geometric pair, for tensors on a curved space, is at the end of the page.

## The magnitude differential

`magnitude_differential` returns the magnitude of the derivative. It is used where only the size of the rate of change matters, as for a step-size bound or a Lipschitz estimate, and it is the right tool whenever you would otherwise write `abs` around a derivative by hand. Because `abs` is not linear, neither is the operator, and its capabilities say so.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.pow(3) - x; });
  const auto x = pluss::make_series({-2.0, -0.5, 0.5, 2.0});

  out.writeln("x         ", x.items<double>());
  out.writeln("f'        ", operators::differential("x")(f)({x}).items<double>());
  out.writeln("|f'|      ", operators::magnitude_differential("x")(f)({x}).items<double>());
  out.writeln("f''       ", operators::differential("x", 2)(f)({x}).items<double>());
  out.writeln("|f''|     ", operators::magnitude_differential("x", 2)(f)({x}).items<double>());
  out.writeln("linear?  differential:", operators::differential("x").capabilities().linear,
              " magnitude_differential:", operators::magnitude_differential("x").capabilities().linear);
}
```

Output

```text
x          [-2.0000, -0.5000, 0.5000, 2.0000]
f'         [11.0000, -0.2500, -0.2500, 11.0000]
|f'|       [11.0000, 0.2500, 0.2500, 11.0000]
f''        [-12.0000, -3.0000, 3.0000, 12.0000]
|f''|      [12.0000, 3.0000, 3.0000, 12.0000]
linear?  differential: true  magnitude_differential: false
```

The function f(x) = x³ − x has f′ = 3x² − 1, negative on (−0.58, 0.58) and positive outside it, and `|f'|` is the same list with the signs removed (the middle two entries change from −0.25 to 0.25). The same holds for the second derivative, 6x, and its magnitude. The last line reports the capability: the ordinary differential is linear, the magnitude one is not.

## The covariant differential

The covariant differential adds a connection term to the derivative. For a field `f` of one coordinate `x` and a connection `Γ(x)` it is

∇f = f′ + Γ f.

The term `Γ f` corrects the derivative for the way the coordinate system itself changes from point to point; with `Γ = 0` it reduces to the ordinary derivative. The connection is itself a `Function`, so it can depend on the coordinate (and on other variables), and it is matched to the function's variables by name. The operator is linear in `f`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.pow(3) - x; });
  const auto x = pluss::make_series({-2.0, -0.5, 0.5, 2.0});

  // A constant connection, and a connection that depends on the coordinate: Gamma(x) = x.
  const math::Function constant_connection = math::function(3.0);
  const math::Function varying = math::function({"x"}, [](math::Function v) { return v; });

  const auto plain = operators::differential("x")(f)({x});
  out.writeln("f'                   ", plain.items<double>());
  out.writeln("f' + 3 f             ", operators::covariant("x", constant_connection)(f)({x}).items<double>());
  out.writeln("by hand              ", plain.add(x.pow(3).sub(x).mul(3.0)).items<double>());
  out.writeln("f' + x f             ", operators::covariant("x", varying)(f)({x}).items<double>());
  out.writeln("by hand              ", plain.add(x.mul(x.pow(3).sub(x))).items<double>());

  // Applying the operator twice composes: (d + 3)(d + 3) f = f'' + 6 f' + 9 f.
  const operators::Operator nabla = operators::covariant("x", constant_connection);
  const auto twice = operators::compose(nabla, nabla)(f)({x});
  const auto second = operators::differential("x", 2)(f)({x});
  out.writeln("nabla nabla f        ", twice.items<double>());
  out.writeln("f'' + 6 f' + 9 f     ", second.add(plain.mul(6.0)).add(x.pow(3).sub(x).mul(9.0)).items<double>());
}
```

Output

```text
f'                    [11.0000, -0.2500, -0.2500, 11.0000]
f' + 3 f              [-7.0000, 0.8750, -1.3750, 29.0000]
by hand               [-7.0000, 0.8750, -1.3750, 29.0000]
f' + x f              [23.0000, -0.4375, -0.4375, 23.0000]
by hand               [23.0000, -0.4375, -0.4375, 23.0000]
nabla nabla f         [0.0000, -1.1250, -1.8750, 132.0000]
f'' + 6 f' + 9 f      [0.0000, -1.1250, -1.8750, 132.0000]
```

The covariant differential with a constant connection 3 is f′ + 3f, and with Γ(x) = x it is f′ + x f; both agree with the by-hand lines. Applying the operator twice composes in the expected way: with constant Γ the operator is d + Γ, and (d + 3)² f = f″ + 6f′ + 9f, which the last two lines show to be equal. A connection that depends on the coordinate is differentiated correctly, since the derivative acts on `Γ f` only through `f`, as the defining formula says.

### A connection on a function of several variables

When the function has more than one variable, the covariant differential is taken with respect to one of them, and a connection written in terms of any of the function's variables is accepted: the connection is rebuilt over the function's variables by name when the operator is applied.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function field = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * y; });
  // The connection is a function of y only; the derivative is taken in x.
  const math::Function gamma = math::function({"y"}, [](math::Function y) { return y; });

  const auto x = pluss::make_series(2.0, as_f64());
  const auto y = pluss::make_series(3.0, as_f64());
  const math::Function nabla_x = operators::covariant("x", gamma)(field);
  out.writeln("variables of the result:", nabla_x.variables()[0], nabla_x.variables()[1]);
  out.writeln("d/dx(xy) + y * (xy) at (2, 3):", nabla_x({x, y}).item<double>(), " (3 + 3 * 6 = 21)");
}
```

Output

```text
variables of the result: x y
d/dx(xy) + y * (xy) at (2, 3): 21.0000  (3 + 3 * 6 = 21)
```

The result is a function of both of the field's variables, `x` and `y`, and its value at (2, 3) is the derivative in `x` of `xy`, which is `y = 3`, plus the connection `y = 3` times the field `xy = 6`, which is 18: in all 21. The connection mentions only `y`; it is attached to the field's `y`, by name. A connection naming a variable the field does not have is rejected when the operator is applied.

The scalar `covariant` operator can be saved only if it is built from the library's own operators ([Saving and loading](../calculus_05_operators/saving_and_loading.md) lists which); an operator that holds an arbitrary `Function` as its connection is not serializable, and says so in `capabilities()`.


## Tensors on a curved space: the absolute and covariant differentials

On a curved space a vector or tensor has components that depend on the coordinates, so differentiating the components alone does not give the rate of change of the tensor itself. The correction is the connection, and for a space with a metric `g` the connection is the Levi-Civita one,

Γⁱⱼₖ = ½ gⁱˡ (∂ⱼ g_{lk} + ∂ₖ g_{lj} − ∂ₗ g_{jk}).

`geometry::levi_civita(metric)` builds it as a `Function` of output shape `[n, n, n]`, indexed `[i][j][k]`, from a metric `Function` of output shape `[n, n]`; the metric's variables are the coordinates.

A tensor has a signature `{contravariant, covariant}`: the number of upper and lower indices. Its `Function` output axes are those indices, upper ones first. Two operators then apply the connection:

| Operator | Meaning |
|---|---|
| `geometry::absolute_differential(curve, Γ, t, signature)` | the absolute (intrinsic) differential along the curve `x(t)`: `DVⁱ/dt = dVⁱ/dt + Γⁱⱼₖ Vʲ dxᵏ/dt` for an upper index, with the opposite sign for each lower index. The input is the tensor along the curve, a `Function` of `t`. |
| `geometry::covariant(x, Γ, signature)` | the covariant derivative `∇ₖV` in the direction of one coordinate: `∂ₖVⁱ + Γⁱⱼₖ Vʲ` for an upper index, minus the matching term for a lower one. The input is a tensor field, a `Function` of the coordinates. |

A tensor is parallel-transported along a curve when `DV/dt = 0`. On the unit sphere, with coordinates `(θ, φ)`, the metric is `diag(1, sin²θ)`; the circle of latitude `θ = θ₀` has velocity `(0, 1)`, and a vector carried around it turns, relative to the local frame, at the rate `cos θ₀`. The program checks all of this against closed forms.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
namespace geometry = pluss::operators::geometry;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function metric = math::function({"theta", "phi"}, [](math::Function theta, math::Function phi) {
    const math::Function zero = theta * 0.0 + phi * 0.0;
    return math::stack({math::stack({zero + 1.0, zero}), math::stack({zero, theta.sin() * theta.sin()})});
  });
  const math::Function gamma = geometry::levi_civita(metric);

  const double theta0 = 1.0;
  const auto point = std::vector<pluss::Series>{pluss::make_series(theta0, as_f64()), pluss::make_series(0.3, as_f64())};
  const auto christoffel = gamma(pluss::RangeView<pluss::Series>(point)).reshape({8}).items<double>();
  out.writeln("Gamma^theta_{phi phi}   ", christoffel[3], "   closed form ", -std::sin(theta0) * std::cos(theta0));
  out.writeln("Gamma^phi_{theta phi}   ", christoffel[5], "   closed form ", std::cos(theta0) / std::sin(theta0));

  // The circle of latitude theta = theta0, parametrised by t = phi.
  const math::Function curve = math::function({"t"}, [&](math::Function t) { return math::stack({t * 0.0 + theta0, t}); });
  const operators::Operator D = geometry::absolute_differential(curve, gamma, "t", {1, 0});

  // Its own velocity is not parallel: D(velocity)/dt = (-sin cos, 0).
  const math::Function acceleration = D(operators::differential("t")(curve));
  const auto t0 = pluss::make_series(0.7, as_f64());
  out.writeln("D(velocity)/dt          ", acceleration({t0}).items<double>());

  // The transported vector V = (cos(w t), -sin(w t) / sin(theta0)), w = cos(theta0), has DV/dt = 0.
  const double w = std::cos(theta0), s0 = std::sin(theta0);
  const math::Function transported = math::function({"t"}, [&](math::Function t) {
    return math::stack({(t * w).cos(), (t * w).sin() * (-1.0 / s0)});
  });
  out.writeln("DV/dt for transport     ", D(transported)({t0}).items<double>());
  const math::Function constant = math::function({"t"}, [](math::Function t) { return math::stack({t * 0.0 + 1.0, t * 0.0}); });
  out.writeln("DV/dt for V = (1, 0)    ", D(constant)({t0}).items<double>(), "   cot(theta0) = ", w / s0);
}
```

Output

```text
Gamma^theta_{phi phi}    -0.454649    closed form  -0.454649
Gamma^phi_{theta phi}    0.642093    closed form  0.642093
D(velocity)/dt           [-0.454649, 0.000000]
DV/dt for transport      [0.000000, 0.000000]
DV/dt for V = (1, 0)     [0.000000, 0.642093]    cot(theta0) =  0.642093
```

`Γ^θ_{φφ} = −sin θ cos θ` and `Γ^φ_{θφ} = cot θ` are the textbook values, so the connection built from the metric is right. The velocity of a latitude circle other than the equator is not parallel, and its absolute differential is `(−sin θ₀ cos θ₀, 0)`. The vector with components `(cos ωt, −sin ωt / sin θ₀)` is transported along the circle: its absolute differential is zero, whereas the constant component vector `(1, 0)` is not, and its derivative is `(0, cot θ₀)`. After one circuit the transported vector has turned through `2π cos θ₀` relative to the local frame, so it falls short of a full turn by `2π(1 − cos θ₀)`, the area enclosed by the circle.

### Covariant derivatives of fields

For a field on the space itself, the covariant derivative is taken in one coordinate direction. The metric is compatible with its own connection, so its covariant derivative vanishes in both directions, and the covariant Hessian of a scalar is symmetric.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
namespace geometry = pluss::operators::geometry;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const math::Function metric = math::function({"theta", "phi"}, [](math::Function theta, math::Function phi) {
    const math::Function zero = theta * 0.0 + phi * 0.0;
    return math::stack({math::stack({zero + 1.0, zero}), math::stack({zero, theta.sin() * theta.sin()})});
  });
  const math::Function gamma = geometry::levi_civita(metric);
  const std::vector<pluss::Series> point{pluss::make_series(0.8, as_f64()), pluss::make_series(0.4, as_f64())};
  const pluss::RangeView<pluss::Series> at(point);

  // nabla_k g_ij = 0 (signature {0, 2}) and nabla_k g^ij = 0 (signature {2, 0}).
  out.writeln("nabla_theta g      ", geometry::covariant("theta", gamma, {0, 2})(metric)(at).reshape({4}).items<double>());
  out.writeln("nabla_phi g^-1     ", geometry::covariant("phi", gamma, {2, 0})(metric.inverse())(at).reshape({4}).items<double>());

  // The covariant Hessian of f: H_{ji} = nabla_j (d_i f) is symmetric.
  const math::Function f = math::function({"theta", "phi"}, [](math::Function theta, math::Function phi) {
    return (theta * theta).sin() * phi.cos() + theta * phi;
  });
  const math::Function df = operators::gradient({"theta", "phi"})(f);
  const auto row_theta = geometry::covariant("theta", gamma, {0, 1})(df)(at).items<double>();
  const auto row_phi = geometry::covariant("phi", gamma, {0, 1})(df)(at).items<double>();
  out.writeln("H_{theta phi}      ", row_theta[1]);
  out.writeln("H_{phi theta}      ", row_phi[0]);
}
```

Output

```text
nabla_theta g       [0.00000000, 0.00000000, 0.00000000, 0.00000000]
nabla_phi g^-1      [0.00000000, 0.00000000, 0.00000000, 0.00000000]
H_{theta phi}       -0.05086840
H_{phi theta}       -0.05086840
```

Both metric derivatives are zero to rounding, and the two off-diagonal entries of the covariant Hessian agree. Neither fact holds for the plain derivative of the components, which is why the connection terms are needed.

Both geometric operators hold an arbitrary connection `Function`, so they cannot be saved ([Saving and loading](../calculus_05_operators/saving_and_loading.md)). The scalar `magnitude_differential` and `differential` can.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="opaque_functions_and_the_autodiff_fallback.html">
    <span class="label">Previous</span>
    <span class="title">Opaque functions and the autodiff fallback</span>
  </a>
</div>
<!-- apiref:nav:end -->
