<!-- generated from antiderivatives.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Antiderivatives

[Back to Integrals](../calculus_04_integrals.md)

`operators::antiderivative(variable)` returns a function whose derivative with respect to `variable` is the original. It is the inverse of `differential`, and it is the first of three integral operators (the others, `integral(variable, lower)` and `integral(variable, lower, upper)`, are on the next page).

```cpp
operators::Operator operators::antiderivative(std::string_view variable);
```

The antiderivative of a formula is exact only if the formula is built from patterns whose primitive is known. Pluss applies a table of exact rules, and for anything outside the table it keeps the integral as an unevaluated integral, which is computed numerically when the function is evaluated. `provenance()` tells the two apart: `Structural` for an exact result, `Quadrature` for a numerical one.

## The exact rules

| Integrand (`u` free of `x`) | Primitive |
|---|---|
| `u` (anything not using `x`) | `u · x` |
| `x^n` for a constant exponent `n ≠ -1` | `x^(n+1) / (n+1)` |
| `1/x`, `x^-1` | `log|x|` |
| `exp(x)` | `exp(x)` |
| `expm1(x)` | `exp(x) - x` |
| `sin(x)`, `cos(x)` | `-cos(x)`, `sin(x)` |
| `sinh(x)`, `cosh(x)` | `cosh(x)`, `sinh(x)` |
| `f + g`, `f - g`, `-f` | the sums and differences of the primitives |
| `u * f`, `f * u`, `f / u` | `u` times, or divided into, the primitive of `f` |

A formula built only from these, such as `x³ + sin x` or `3 y x²`, has an exact primitive. The rules are applied term by term, and a term with no rule makes the whole integral numerical, because the result is a single function. Notably, a rule that needs a substitution is **not** in the table: `exp(2x)`, `sin(3x)`, `(x+1)²`, `x sin x`, `log x` and `1/(1+x)` are all integrated numerically, although their primitives are elementary. The program makes the list explicit, so the boundary is never a surprise.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <functional>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  using Body = std::function<math::Function(math::Function, math::Function)>;
  const std::pair<const char *, Body> cases[] = {
      {"x                 ", [](auto x, auto) { return x; }},
      {"x^2               ", [](auto x, auto) { return x.pow(2); }},
      {"x^0.5             ", [](auto x, auto) { return x.pow(0.5); }},
      {"x^-2              ", [](auto x, auto) { return x.pow(-2.0); }},
      {"1/x               ", [](auto x, auto) { return 1.0 / x; }},
      {"exp(x)            ", [](auto x, auto) { return x.exp(); }},
      {"expm1(x)          ", [](auto x, auto) { return x.expm1(); }},
      {"sin(x)            ", [](auto x, auto) { return x.sin(); }},
      {"cos(x)            ", [](auto x, auto) { return x.cos(); }},
      {"sinh(x), cosh(x)  ", [](auto x, auto) { return x.sinh() + x.cosh(); }},
      {"sin(x) + x^2      ", [](auto x, auto) { return x.sin() + x.pow(2); }},
      {"y sin(x)          ", [](auto x, auto y) { return y * x.sin(); }},
      {"sin(x) / y        ", [](auto x, auto y) { return x.sin() / y; }},
      {"y x               ", [](auto x, auto y) { return y * x; }},
      {"exp(2x)           ", [](auto x, auto) { return (x * 2.0).exp(); }},
      {"sin(3x)           ", [](auto x, auto) { return (x * 3.0).sin(); }},
      {"(x + 1)^2         ", [](auto x, auto) { return (x + 1.0).pow(2); }},
      {"x sin(x)          ", [](auto x, auto) { return x * x.sin(); }},
      {"log(x)            ", [](auto x, auto) { return x.log(); }},
      {"sqrt(x)           ", [](auto x, auto) { return x.sqrt(); }},
      {"tanh(x)           ", [](auto x, auto) { return x.tanh(); }},
      {"exp(-x^2)         ", [](auto x, auto) { return (-x.pow(2)).exp(); }},
      {"1/(1 + x)         ", [](auto x, auto) { return 1.0 / (x + 1.0); }},
      {"x^y               ", [](auto x, auto y) { return x.pow(y); }},
  };
  for (const auto &[label, body] : cases) {
    const math::Function f = math::function({"x", "y"}, body);
    const math::Function primitive = operators::antiderivative("x")(f);
    out.writeln(label, primitive.provenance() == math::Provenance::Structural ? "exact" : "numerical (quadrature)");
  }
}
```

Output

```text
x                  exact
x^2                exact
x^0.5              exact
x^-2               exact
1/x                exact
exp(x)             exact
expm1(x)           exact
sin(x)             exact
cos(x)             exact
sinh(x), cosh(x)   exact
sin(x) + x^2       exact
y sin(x)           exact
sin(x) / y         exact
y x                exact
exp(2x)            numerical (quadrature)
sin(3x)            numerical (quadrature)
(x + 1)^2          numerical (quadrature)
x sin(x)           numerical (quadrature)
log(x)             numerical (quadrature)
sqrt(x)            numerical (quadrature)
tanh(x)            numerical (quadrature)
exp(-x^2)          numerical (quadrature)
1/(1 + x)          numerical (quadrature)
x^y                numerical (quadrature)
```

Every integrand in the first group has an exact primitive, including powers with any constant exponent, sums and differences of the elementary functions, and a factor or divisor that does not involve `x` (`y sin(x)`, `sin(x)/y`, `y x`). Everything in the second group is numerical, including several whose primitives are easy to write down by hand, such as `exp(2x)`, `sin(3x)` and `(x+1)²`: the rule table has no chain-rule substitution and no integration by parts, so a formula that needs either is integrated numerically. A numerical result is not wrong, only different in kind, as the third program shows. When an exact primitive is wanted and the formula is in the second group, expand it into rule terms first (`(x+1)²` as `x² + 2x + 1`, which is in the table).

## Using an exact primitive

An exact antiderivative is a `Structural` function like any other. It can be evaluated at arrays, differentiated (which returns the original, up to the simplification of the result), combined and saved. The primitive here is of `x³ + sin x`, namely `x⁴/4 − cos x` (the constant of integration is zero):

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.pow(3) + x.sin(); });
  const math::Function primitive = operators::antiderivative("x")(f);
  const auto x = pluss::make_series({0.5, 1.0, 2.0});

  out.writeln("F(x)            ", primitive({x}).pull<double>());
  out.writeln("x^4/4 - cos(x)  ", x.pow(4).div(4.0).sub(x.cos()).pull<double>());
  out.writeln("F'(x)           ", operators::differential("x")(primitive)({x}).pull<double>());
  out.writeln("f(x)            ", f({x}).pull<double>());
  out.writeln("provenance exact:", primitive.provenance() == math::Provenance::Structural ? "yes" : "no");
}
```

Output

```text
F(x)             [-0.861958, -0.290302, 4.416147]
x^4/4 - cos(x)   [-0.861958, -0.290302, 4.416147]
F'(x)            [0.604426, 1.841471, 8.909297]
f(x)             [0.604426, 1.841471, 8.909297]
provenance exact: yes
```

`F` equals x⁴/4 − cos x at the three points, and differentiating it returns the original integrand: the last two value rows coincide. The result is `Structural`, and carries no constant of integration; the constant is zero because the rules are written without one. Use the anchored integral of the next page when the value at a particular point must be zero.

## When the primitive is numerical

An integrand outside the table still has an antiderivative function, but it is defined by an integral that is evaluated numerically each time. The antiderivative of `exp(−x²)` is anchored at 0 (the error function up to a constant factor), so its value at `x` is ∫₀ˣ exp(−t²) dt. Two things differ from an exact primitive: the argument must be a **single value** (a scalar `Series`), and the evaluation costs a quadrature. The next two pages explain both.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const math::Function gaussian = math::function({"x"}, [](math::Function x) { return (-x.pow(2)).exp(); });
  const math::Function primitive = operators::antiderivative("x")(gaussian);
  out.writeln("provenance numerical:", primitive.provenance() == math::Provenance::Quadrature ? "yes" : "no");
  for (const double value : {0.5, 1.0, 2.0}) {
    // sqrt(pi)/2 * erf(x) is the same integral.
    out.writeln("F(", value, ") =", primitive({pluss::make_series(value, as_f64())}).item<double>(), "  sqrt(pi)/2 erf:", std::sqrt(M_PI) / 2.0 * std::erf(value));
  }
}
```

Output

```text
provenance numerical: yes
F( 0.5000000000 ) = 0.4612810064   sqrt(pi)/2 erf: 0.4612810064
F( 1.0000000000 ) = 0.7468241328   sqrt(pi)/2 erf: 0.7468241328
F( 2.0000000000 ) = 0.8820813908   sqrt(pi)/2 erf: 0.8820813908
```

The numerical primitive is anchored at 0 and agrees with the error function to all ten printed digits at the three points. It is a `Quadrature` function: each evaluation integrates from 0 to the argument. Its argument must be a single value, and the next page explains why.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->
