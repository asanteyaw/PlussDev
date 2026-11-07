<!-- generated from partial_derivatives_and_higher_orders.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Partial derivatives and higher orders

[Back to Derivatives](../calculus_02_derivatives.md)

`operators::differential(variable, order)` is an `Operator`: a value that turns one `Function` into another. Applied to a function it returns the exact derivative with respect to the named variable, as a new `Function` that has the same variables and is evaluated the same way. Nothing is approximated: the library differentiates the recorded graph rule by rule.

```cpp
operators::Operator operators::differential(std::string_view variable, std::int64_t order = 1, DiffOptions options = {});
```

An `Operator` is called like a function, `d(f)`, and returns a `Function`. Chapter 5 describes operators as values; here one is created and used in the same line.

## A partial derivative

The program differentiates a function of two variables with respect to each, evaluates the results on a grid of points, and compares them with the derivatives worked out by hand:

f(x, y) = x² · sin(xy) · exp(cos x)

- ∂f/∂x = exp(cos x) · [2x sin(xy) + x²y cos(xy) − x² sin x sin(xy)]
- ∂f/∂y = x³ cos(xy) · exp(cos x)

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function f = math::function({"x", "y"}, [](math::Function x, math::Function y) {
    return x.pow(2) * (x * y).sin() * x.cos().exp();
  });
  const math::Function dfdx = operators::differential("x")(f);
  const math::Function dfdy = operators::differential("y")(f);

  const auto x = pluss::make_series({0.25, 0.5, 1.0, 1.5});
  const auto y = pluss::make_series({0.4, 0.7, 1.1, 1.4});

  const auto xy = x.mul(y);
  const auto envelope = x.cos().exp();
  const auto by_hand_x = envelope.mul(x.mul(2.0).mul(xy.sin()).add(x.pow(2).mul(y).mul(xy.cos())).sub(x.pow(2).mul(x.sin()).mul(xy.sin())));
  const auto by_hand_y = x.pow(3).mul(xy.cos()).mul(envelope);

  out.writeln("df/dx  ", dfdx({x, y}).pull<double>());
  out.writeln("by hand", by_hand_x.pull<double>());
  out.writeln("df/dy  ", dfdy({x, y}).pull<double>());
  out.writeln("by hand", by_hand_y.pull<double>());
  out.writeln("variables of df/dx:", dfdx.variables()[0], dfdx.variables()[1], " arity", dfdx.arity());
}
```

Output

```text
df/dx   [0.193014, 1.121222, 2.628765, -1.006747]
by hand [0.193014, 1.121222, 2.628765, -1.006747]
df/dy   [0.040967, 0.282408, 0.778609, -1.828747]
by hand [0.040967, 0.282408, 0.778609, -1.828747]
variables of df/dx: x y  arity 2
```

Both derivatives agree with the hand formulas to every printed digit. The derivative functions keep the variables of the original, `x` then `y`, so `dfdx` is still a function of two variables and is called with both, even though a derivative may not depend on all of them (the next example shows the extreme case). `d` does not mutate `f`: it returns a new function, and `f` is unchanged.

## Mixed partials and repeated application

Because the result is a `Function` with the same variables, a derivative can be differentiated again, in any variable. `differential("y")(differential("x")(f))` is the mixed partial ∂²f/∂y∂x, and the order in which the two are taken does not change the value for a smooth function.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // g(x, y) = x^2 y^3, so g_xy = 6 x y^2 and g_xx = 2 y^3.
  const math::Function g = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x.pow(2) * y.pow(3); });
  const auto x = pluss::make_series({1.0, 2.0, 3.0});
  const auto y = pluss::make_series({3.0, 3.0, 2.0});

  const math::Function gxy = operators::differential("y")(operators::differential("x")(g));
  const math::Function gyx = operators::differential("x")(operators::differential("y")(g));
  const math::Function gxx = operators::differential("x", 2)(g);

  out.writeln("g_xy        ", gxy({x, y}).pull<double>());
  out.writeln("g_yx        ", gyx({x, y}).pull<double>());
  out.writeln("6 x y^2     ", x.mul(y.pow(2)).mul(6.0).pull<double>());
  out.writeln("g_xx        ", gxx({x, y}).pull<double>());
  out.writeln("2 y^3       ", y.pow(3).mul(2.0).pull<double>());

  // A variable the formula does not use has derivative zero, with the shape of the evaluation.
  const math::Function unused = operators::differential("y")(math::function({"x", "y"}, [](math::Function x, math::Function) { return x * x; }));
  out.writeln("d/dy of x*x ", unused({x, y}).pull<double>());
}
```

Output

```text
g_xy         [54.000000, 108.000000, 72.000000]
g_yx         [54.000000, 108.000000, 72.000000]
6 x y^2      [54.000000, 108.000000, 72.000000]
g_xx         [54.000000, 54.000000, 16.000000]
2 y^3        [54.000000, 54.000000, 16.000000]
d/dy of x*x  [0.000000, 0.000000, 0.000000]
```

The two mixed partials are identical and equal to 6xy² at every point, and `g_xx` equals 2y³. The last line is the derivative with respect to a variable the formula does not use: it is zero, and it has the shape of the arguments (three values), not a bare scalar zero, so it can be added to other results without reshaping.

## Direct higher orders

`differential("x", n)` differentiates `n` times in one step. The recorded graph is shared between the steps (a sub-expression used twice is differentiated once), so a high order does not repeat work. The program takes the third and fifth derivatives of p(x) = x⁵ − 2x³ + x, for which p‴ = 60x² − 12 and p⁽⁵⁾ = 120.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function p = math::function({"x"}, [](math::Function x) { return x.pow(5) - 2.0 * x.pow(3) + x; });
  const auto x = pluss::make_series({-2.0, -0.5, 0.0, 1.5});

  const math::Function third = operators::differential("x", 3)(p);
  const math::Function fifth = operators::differential("x", 5)(p);
  const math::Function sixth = operators::differential("x", 6)(p);

  out.writeln("x         ", x.pull<double>());
  out.writeln("p'''      ", third({x}).pull<double>());
  out.writeln("60x^2 - 12", x.pow(2).mul(60.0).sub(12.0).pull<double>());
  out.writeln("p(5)      ", fifth({x}).pull<double>());
  out.writeln("p(6)      ", sixth({x}).pull<double>());

  // Three steps of order one give the same function as one step of order three.
  const math::Function stepwise = operators::differential("x")(operators::differential("x")(operators::differential("x")(p)));
  out.writeln("three first derivatives", stepwise({x}).pull<double>());
}
```

Output

```text
x          [-2.0000, -0.5000, 0.0000, 1.5000]
p'''       [228.0000, 3.0000, -12.0000, 123.0000]
60x^2 - 12 [228.0000, 3.0000, -12.0000, 123.0000]
p(5)       [120.0000, 120.0000, 120.0000, 120.0000]
p(6)       [0.0000, 0.0000, 0.0000, 0.0000]
three first derivatives [228.0000, 3.0000, -12.0000, 123.0000]
```

The third derivative matches 60x² − 12 at all four points, the fifth derivative of a quintic with leading coefficient 1 is the constant 5! = 120 (still returned for every point), and the sixth is zero. Three one-step derivatives and one three-step derivative are the same function.

## Exact derivatives and `Series` calculus agree

`Series` has its own derivative machinery, `x.calc.diff(fn, order)`, which differentiates a callable by automatic differentiation at the points of `x`. The two routes use different mechanisms and must agree, and checking them against each other is a good test of a formula you have written. The `Function` route gives a function you can evaluate anywhere afterwards; the `Series` route gives numbers at the points you pass.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.pow(2) * x.sin() * x.cos().exp(); });
  const math::Function first = operators::differential("x")(f);
  const math::Function second = operators::differential("x", 2)(f);
  const auto callback = [](const pluss::Series &x) { return x.pow(2).mul(x.sin()).mul(x.cos().exp()); };

  out.writeln("point   exact f'   Series f'   exact f''   Series f''");
  for (const double value : {0.3, 0.8, 1.4}) {
    const auto point = pluss::make_series(value, as_f64());
    out.writeln(value, first({point}).item<double>(), point.calc.diff(callback, 1).item<double>(), second({point}).item<double>(),
                point.calc.diff(callback, 2).item<double>());
  }
}
```

Output

```text
point   exact f'   Series f'   exact f''   Series f''
0.30000000 0.66400835 0.66400835 3.98287953 3.98287953
0.80000000 2.53765318 2.53765318 1.67598044 1.67598044
1.40000000 1.40930125 1.40930125 -4.21500764 -4.21500764
```

The two columns agree to all eight digits at every point, for both orders. When they do not agree, the formula passed to `Function` and the one passed to the callback are different formulas.

## Errors

The order must be positive, the variable name non-empty, and the variable must belong to the function it is applied to. The first two are rejected when the operator is created and the last when it is applied.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <functional>
#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

static void attempt(OutputEngine &out, const char *label, const std::function<void()> &call) {
  try {
    call();
    out.writeln(label, "-> no error");
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln(label, "->", text.substr(0, text.find('\n')));
  }
}

int main() {
  OutputEngine out;
  const math::Function f = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * y; });
  attempt(out, "empty variable name ", [] { (void)operators::differential("", 1); });
  attempt(out, "order 0             ", [] { (void)operators::differential("x", 0); });
  attempt(out, "negative order      ", [] { (void)operators::differential("x", -1); });
  attempt(out, "variable not in f   ", [&] { (void)operators::differential("z")(f); });
  attempt(out, "undefined function  ", [] { (void)operators::differential("x")(math::Function{}); });
  attempt(out, "valid               ", [&] { (void)operators::differential("x", 2)(f); });
}
```

Output

```text
empty variable name  -> operators::differential: variable must not be empty
order 0              -> operators::differential: order must be positive
negative order       -> operators::differential: order must be positive
variable not in f    -> operators::differential: unknown variable 'z'
undefined function   -> operators::Operator: input Function must be defined
valid                -> no error
```

The order and the name are checked when the operator is made, with no function involved. A missing variable is reported when the operator meets the function, because only then is there a variable list to check. Applying an operator to an undefined `Function` is an error in every operator.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->
