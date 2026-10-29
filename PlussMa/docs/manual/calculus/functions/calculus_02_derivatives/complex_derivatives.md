<!-- generated from complex_derivatives.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Complex and Wirtinger derivatives

[Back to Derivatives](../calculus_02_derivatives.md)

A `math::Function` may accept real or complex `Series` values and may return a complex `Series`. The graph remains a formula: applying an operator constructs another `Function`; no point is sampled until that result is called with values.

There are two distinct questions, and the API keeps them distinct.

1. **A real parameter with a complex-valued result.** Use `differential`. Characteristic functions are the central example: the argument `t` is real, while `phi(t)` is complex. The ordinary chain rule differentiates `conj`, `real`, `imag`, magnitude and phase along that real parameter.
2. **A genuinely complex variable.** Use the Wirtinger pair. `wirtinger("z")` constructs `partial f / partial z`; `conjugate_wirtinger("z")` constructs `partial f / partial conj(z)`. A function is holomorphic in `z` exactly where the second result is zero.

```cpp
math::Function math::Function::conj() const;
math::Function math::Function::real() const;
math::Function math::Function::imag() const;
math::Function math::Function::abs() const;
math::Function math::Function::angle() const;

operators::Operator operators::wirtinger(
    std::string_view variable,
    std::int64_t order = 1,
    operators::DiffOptions options = {});

operators::Operator operators::conjugate_wirtinger(
    std::string_view variable,
    std::int64_t order = 1,
    operators::DiffOptions options = {});
```

Both operators are structural, serializable and platform-neutral. They reject opaque callables: an autodiff fallback cannot infer whether a complex gradient means an ordinary directional derivative, a conjugate gradient or one member of the Wirtinger pair.

`differential("x")` and `wirtinger("x")` are therefore not aliases. The former asks how a formula changes along a real parameter named `x`; the latter asks for the complex derivative with respect to `x`. This distinction remains visible even when both happen to produce the same formula for a holomorphic expression.

## A characteristic function

For a normal random variable with mean `mu` and variance `sigma2`,

`phi(t) = exp(i mu t - sigma2 t^2 / 2)`.

The first derivative at zero is `i mu`; the second is `-(sigma2 + mu^2)`. These are analytic derivative functions built before zero is supplied.

```cpp
#include <pluss/ma.h>

#include <complex>
#include <iomanip>
#include <iostream>

namespace math = pluss::math;
namespace operators = pluss::operators;

int main() {
  using Complex = std::complex<double>;
  constexpr double mu = 1.5;
  constexpr double sigma2 = 0.7;
  const math::Function i = math::function(Complex{0.0, 1.0});
  const math::Function phi = math::function({"t"}, [i](math::Function t) {
    return (i * mu * t - 0.5 * sigma2 * t.pow(2.0)).exp();
  });
  const math::Function first = operators::differential("t")(phi);
  const math::Function second = operators::differential("t", 2)(phi);

  std::cout << std::fixed << std::setprecision(6);
  std::cout << "phi'(0)  = " << first(0.0).item<Complex>() << "\n";
  std::cout << "phi''(0) = " << second(0.0).item<Complex>() << "\n";
}
```

Output

```text
phi'(0)  = (0.000000,1.500000)
phi''(0) = (-2.950000,0.000000)
```

The same pattern recovers raw moments from any characteristic function: `E[X^n] = phi^(n)(0) / i^n`. Unlike a finite-difference estimate, the derivative can be saved, evaluated in batches and differentiated again.

## Holomorphic and non-holomorphic graphs

For `f(z) = z^3 + exp(z)`, `partial f / partial z = 3z^2 + exp(z)` and `partial f / partial conj(z) = 0`. Conjugation and magnitude expose why both operators are needed: `conj(z)` has derivatives `0` and `1`, while `abs(z)` has derivatives `conj(z)/(2 abs(z))` and `z/(2 abs(z))` away from zero.

```cpp
#include <pluss/ma.h>

#include <complex>
#include <iomanip>
#include <iostream>

namespace math = pluss::math;
namespace operators = pluss::operators;

int main() {
  using Complex = std::complex<double>;
  const math::Function z =
      math::function({"z"}, [](math::Function value) { return value; });
  const math::Function holomorphic = z.pow(3.0) + z.exp();
  const pluss::Series point = pluss::series::complex(
      pluss::make_series(3.0), pluss::make_series(4.0));

  const math::Function dz = operators::wirtinger("z")(holomorphic);
  const math::Function dbar =
      operators::conjugate_wirtinger("z")(holomorphic);
  const math::Function conjugate_z = operators::wirtinger("z")(z.conj());
  const math::Function conjugate_bar =
      operators::conjugate_wirtinger("z")(z.conj());
  const math::Function magnitude_z = operators::wirtinger("z")(z.abs());
  const math::Function magnitude_bar =
      operators::conjugate_wirtinger("z")(z.abs());

  std::cout << std::fixed << std::setprecision(6);
  std::cout << "holomorphic dz   = " << dz({point}).item<Complex>() << "\n";
  std::cout << "holomorphic dbar = " << dbar({point}).item<Complex>() << "\n\n";
  std::cout << "conj(z): dz = " << conjugate_z({point}).item<Complex>()
            << ", dbar = " << conjugate_bar({point}).item<Complex>() << "\n";
  std::cout << "abs(z):  dz = " << magnitude_z({point}).item<Complex>()
            << ", dbar = " << magnitude_bar({point}).item<Complex>() << "\n";
}
```

Output

```text
holomorphic dz   = (-34.128784,56.799225)
holomorphic dbar = (0.000000,0.000000)

conj(z): dz = (0.000000,-0.000000), dbar = (1.000000,-0.000000)
abs(z):  dz = (0.300000,-0.400000), dbar = (0.300000,0.400000)
```

At zero, magnitude and phase are non-smooth. `DiffOptions::nonsmooth` applies there exactly as it does for real `abs`: `Strict` rejects the evaluation, `Subgradient` and `Autodiff` choose zero, and `Piecewise` leaves the branch expression exposed.

## Branches and domains

Complex `log` and `sqrt` use the principal branches supplied by `Series`. Therefore `log(-1 + 0i) = i pi` and `sqrt(-1 + 0i) = i`. `log(0)` and `log1p(-1)` remain domain errors. A derivative formula is analytic only on a region that does not cross its branch cut or singularities; constructing the graph does not erase those mathematical restrictions.

Complex tensors use the same rules elementwise, preserve `Complex64` or `Complex128`, and execute on the input platform. CPU supports both complex precisions; MPS executes `Complex64` arithmetic, powers, transcendental functions, components and real/complex promotion natively, with no CPU bridge. Tensor contractions, matrix inverse, determinant and trace propagate each member of the Wirtinger pair structurally. No host container represents a complex tensor: the value is always a `Series`.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->
