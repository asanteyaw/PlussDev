<!-- generated from taylor_and_power_series.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Taylor operators and power series

[Back to Operators](../calculus_05_operators.md)

A truncated Taylor expansion is an operator in its own right: it takes a `Function` and returns another `Function`, a polynomial in the expanded variable that you can evaluate, differentiate, integrate and compose like any other. Nothing is evaluated when you apply it. The coefficients are built from the derivatives of the function at the centre, so the result stays a formula waiting for values, and every variable you did not expand stays free.

| Operator | Result for `f` |
|---|---|
| `taylor(x, a, n)` | `Σ_{k ≤ n} f⁽ᵏ⁾(a)/k! · (x − a)ᵏ`, a polynomial `Function` of `x` |
| `taylor({x, y}, {a, b}, n)` | the multivariate polynomial `Σ_{|α| ≤ n} ∂ᵅf(a)/α! · Π (xᵢ − aᵢ)^αᵢ` |
| `taylor().coefficients(x, a, n)` | the coefficients `f⁽ᵏ⁾(a)/k!`, `k = 0..n`, as a vector-valued `Function` of shape `[n + 1]` |
| `taylor().remainder(x, n)` | the Lagrange factor `f⁽ⁿ⁺¹⁾(x)/(n + 1)!`, so that `|f(x) − Tₙ(x)| ≤ sup|factor| · |x − a|ⁿ⁺¹` |
| `power(x, a)` | acts on a coefficient vector `c` and returns the polynomial `Σ c_k (x − a)ᵏ` |
| `power().derivative()`, `power().integral()`, `power().product(g)` | termwise calculus on coefficient vectors |

```cpp
operators::Operator operators::expansion::taylor(std::string_view variable, double center, std::int64_t order, DiffOptions options = {}, Engine engine = Engine::Auto);
operators::Operator operators::expansion::taylor(StackArray<std::string_view> variables, RangeView<double> center, std::int64_t order, DiffOptions options = {});
operators::Operator operators::expansion::taylor().coefficients(std::string_view variable, double center, std::int64_t order, DiffOptions options = {}, Engine engine = Engine::Auto);
operators::Operator operators::expansion::taylor().remainder(std::string_view variable, std::int64_t order, DiffOptions options = {});
operators::Operator operators::expansion::power(std::string_view variable, double center);
operators::Operator operators::expansion::power().derivative();
operators::Operator operators::expansion::power().integral();
operators::Operator operators::expansion::power().product(math::Function other);
```

`Engine` chooses how the coefficients are found. `Structural` builds the derivative graph (exact, but the graph grows with the order). `Jet` carries truncated power series through the graph, so the cost grows with the square of the order instead of exponentially, for scalar elementwise functions. `Auto` uses `Structural` up to order 8 and `Jet` above it when the function allows. All three give an analytic result that evaluates, differentiates and saves.

## A polynomial that follows the function

The expansion of `exp` about 0 converges quickly, and the Lagrange factor bounds the error of every order. The program evaluates the order 2, 4 and 8 polynomials of `exp(x)` at `x = 0.5`, the error each leaves, and the bound the remainder gives.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace expansion = pluss::operators::expansion;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.exp(); });
  const auto x = pluss::make_series(0.5);

  // staged: keep the remainder operator, apply it once per order
  for (const std::int64_t order : {2, 4, 8}) {
    const math::Function poly = expansion::taylor("x", 0.0, order)(f);
    const math::Function factor = expansion::taylor().remainder("x", order)(f);
    const double bound = factor({x}).item<double>() * std::pow(0.5, order + 1);
    out.writeln("order ", order, "  error ", std::exp(0.5) - poly({x}).item<double>(), "  Lagrange bound ", bound);
  }
}
```

Output

```text
order  2   error  0.0237212707   Lagrange bound  0.0343483598
order  4   error  0.0002837707   Lagrange bound  0.0004293545
order  8   error  0.0000000057   Lagrange bound  0.0000000089
```

Each polynomial is itself a `Function`: you can differentiate it, and the derivative of the order-8 expansion is the order-7 expansion of the derivative.

## Several variables, and variables left free

The multivariate form expands in every variable you list; anything else stays an argument of the result. Here `f(x, y, a) = a · sin(x) · cos(y)` is expanded to order 3 about the origin in `x` and `y` and evaluated for several values of the untouched parameter `a`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace expansion = pluss::operators::expansion;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const math::Function f = math::function({"x", "y", "a"}, [](math::Function x, math::Function y, math::Function a) { return a * x.sin() * y.cos(); });

  // compact: apply and evaluate in one expression
  const math::Function poly = expansion::taylor({"x", "y"}, {0.0, 0.0}, 3)(f);
  out.writeln("free variables: ", poly.variables().size());
  for (const double a : {1.0, 2.0, 5.0})
    out.writeln("a = ", a, "  T3 = ", poly({pluss::make_series(0.2), pluss::make_series(0.3), pluss::make_series(a)}).item<double>(),
                "  f = ", a * std::sin(0.2) * std::cos(0.3));
}
```

Output

```text
free variables:  3
a =  1.00000000   T3 =  0.18966667   f =  0.18979606
a =  2.00000000   T3 =  0.37933333   f =  0.37959212
a =  5.00000000   T3 =  0.94833333   f =  0.94898030
```

## Coefficients as data, and series arithmetic

`taylor().coefficients` returns the coefficient vector as a `Function`, and the series operators act on such vectors, so a power series can be differentiated, integrated and multiplied termwise without ever building the polynomial. The program takes the order-6 coefficients of `1 / (1 − x)` (all ones), integrates them termwise to get the series of `−log(1 − x)`, rebuilds the polynomial with `power`, and compares at `x = 0.3`; it also multiplies the series of `exp` by itself and recovers the series of `exp(2x)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace expansion = pluss::operators::expansion;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const math::Function geometric = math::function({"x"}, [](math::Function x) { return 1.0 / (1.0 - x); });

  const math::Function coeffs = expansion::taylor().coefficients("x", 0.0, 6)(geometric);
  const math::Function integrated = expansion::power().integral()(coeffs);
  out.writeln("coefficients of 1/(1-x): ", coeffs().items<double>());
  out.writeln("termwise integral:       ", integrated().items<double>());
  out.writeln("series at 0.3 = ", expansion::power("x", 0.0)(integrated)({pluss::make_series(0.3)}).item<double>(), "   -log(0.7) = ", -std::log(0.7));

  const math::Function e = math::function({"x"}, [](math::Function x) { return x.exp(); });
  const math::Function e2 = math::function({"x"}, [](math::Function x) { return (x * 2.0).exp(); });
  const math::Function e_coeffs = expansion::taylor().coefficients("x", 0.0, 6)(e);
  out.writeln("exp * exp:   ", expansion::power().product(e_coeffs)(e_coeffs)().items<double>());
  out.writeln("exp(2x):     ", expansion::taylor().coefficients("x", 0.0, 6)(e2)().items<double>());
}
```

Output

```text
coefficients of 1/(1-x):  [1.00000000, 1.00000000, 1.00000000, 1.00000000, 1.00000000, 1.00000000, 1.00000000]
termwise integral:        [0.00000000, 1.00000000, 0.50000000, 0.33333333, 0.25000000, 0.20000000, 0.16666667, 0.14285714]
series at 0.3 =  0.35666374    -log(0.7) =  0.35667494
exp * exp:    [1.00000000, 2.00000000, 2.00000000, 1.33333333, 0.66666667, 0.26666667, 0.08888889]
exp(2x):      [1.00000000, 2.00000000, 2.00000000, 1.33333333, 0.66666667, 0.26666667, 0.08888889]
```

## What can be saved

All of these operators hold only names, numbers and options, so they save and load with `math::io::save` like the built-in operators (`power().product` holds a `Function` and does not). The functions they produce are ordinary analytic graphs and save too ([Saving and loading](saving_and_loading.md)).

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stochastic_operators.html">
    <span class="label">Previous</span>
    <span class="title">Stochastic operators</span>
  </a>
  <a class="next-link" href="fourier_series.html">
    <span class="label">Next</span>
    <span class="title">Fourier series operators</span>
  </a>
</div>
<!-- apiref:nav:end -->
