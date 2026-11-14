<!-- generated from the_expression_vocabulary.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The expression vocabulary

[Back to Functions](../calculus_01_functions.md)

Inside a traced body a placeholder is a `Function`, and every operation on it records one node of the graph. The vocabulary is deliberately the same as the arithmetic and elementary functions of `Series`: each recorded node runs the corresponding `Series` operation at evaluation time, so a `Function` gives the values the same expression would give on arrays, on whatever platform and in whatever precision the arguments carry.

| Form | Operations |
|---|---|
| Binary operators | `+`, `-`, `*`, `/` between two `Function`s, or between a `Function` and a `double` on either side |
| Binary methods | `add`, `sub`, `mul`, `div`, `pow`, each taking a `Function` or a `double` |
| Unary methods | `exp`, `log`, `log1p`, `expm1`, `sqrt`, `sin`, `cos`, `tan`, `sinh`, `cosh`, `tanh`, `abs`, and unary minus |
| Constants | `function(double)`, `function(Series)` |

There is no other way to compute inside a body, and that is a deliberate boundary: everything a `Function` can do, it can also differentiate and integrate exactly (Chapters 2 to 4). [Functions from Series code](functions_from_series_code.md) describes the one escape hatch and what it costs.

## Every primitive, once

The program applies each unary method to the same four points. The values can be checked against any table of these functions.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <array>

namespace math = pluss::math;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series({0.2, 0.5, 1.25, 2.0});
  const math::Function id = math::function({"x"}, [](math::Function v) { return v; });

  out.writeln("x      ", x.items<double>());
  out.writeln("exp    ", id.exp()({x}).items<double>());
  out.writeln("log    ", id.log()({x}).items<double>());
  out.writeln("log1p  ", id.log1p()({x}).items<double>());
  out.writeln("expm1  ", id.expm1()({x}).items<double>());
  out.writeln("sqrt   ", id.sqrt()({x}).items<double>());
  out.writeln("sin    ", id.sin()({x}).items<double>());
  out.writeln("cos    ", id.cos()({x}).items<double>());
  out.writeln("tan    ", id.tan()({x}).items<double>());
  out.writeln("sinh   ", id.sinh()({x}).items<double>());
  out.writeln("cosh   ", id.cosh()({x}).items<double>());
  out.writeln("tanh   ", id.tanh()({x}).items<double>());
  out.writeln("abs(-x)", (-id).abs()({x}).items<double>());
  out.writeln("-x     ", (-id)({x}).items<double>());
}
```

Output

```text
x       [0.20000, 0.50000, 1.25000, 2.00000]
exp     [1.22140, 1.64872, 3.49034, 7.38906]
log     [-1.60944, -0.69315, 0.22314, 0.69315]
log1p   [0.18232, 0.40547, 0.81093, 1.09861]
expm1   [0.22140, 0.64872, 2.49034, 6.38906]
sqrt    [0.44721, 0.70711, 1.11803, 1.41421]
sin     [0.19867, 0.47943, 0.94898, 0.90930]
cos     [0.98007, 0.87758, 0.31532, -0.41615]
tan     [0.20271, 0.54630, 3.00957, -2.18504]
sinh    [0.20134, 0.52110, 1.60192, 3.62686]
cosh    [1.02007, 1.12763, 1.88842, 3.76220]
tanh    [0.19738, 0.46212, 0.84828, 0.96403]
abs(-x) [0.20000, 0.50000, 1.25000, 2.00000]
-x      [-0.20000, -0.50000, -1.25000, -2.00000]
```

Each line is the `Series` operation of the same name applied to `x`; nothing is approximated by the `Function` layer. `abs(-x)` returns `x`, and the unary minus negates. The functions have the domains they have for real numbers, and a violation is an error rather than a `NaN` ([Domains and errors](domains_and_errors.md)): `log` of the four positive points is fine, `log(0)` is not.

## Scalars and powers

A `double` is a constant that takes the shape of the other operand, and it may stand on either side of an operator. `pow` takes an exponent that is itself a `Function`, so a variable can appear in the base, the exponent or both. A constant base needs an explicit `function(2.0)`, because `2.0.pow(x)` is not C++.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto x = pluss::make_series({1.0, 2.0, 3.0});
  const auto y = pluss::make_series({1.0, 2.0, 0.5});

  const math::Function mixed = math::function({"x", "y"}, [](math::Function x, math::Function y) {
    return ((2.0 + x) - y).mul(x + 1.0).div(3.0).pow(y);
  });
  const math::Function reciprocal = math::function({"x"}, [](math::Function x) { return 6.0 / x - 1.0; });
  const math::Function variable_power = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x.pow(y); });
  const math::Function constant_base = math::function({"x"}, [](math::Function x) { return math::function(2.0).pow(x); });

  out.writeln("((2 + x) - y)(x + 1)/3 raised to y:", mixed({x, y}).items<double>());
  out.writeln("6/x - 1:", reciprocal({x}).items<double>());
  out.writeln("x^y:", variable_power({x, y}).items<double>());
  out.writeln("2^x:", constant_base({x}).items<double>());
}
```

Output

```text
((2 + x) - y)(x + 1)/3 raised to y: [1.3333, 4.0000, 2.4495]
6/x - 1: [5.0000, 2.0000, 1.0000]
x^y: [1.0000, 4.0000, 1.7321]
2^x: [2.0000, 4.0000, 8.0000]
```

The first line checks the arithmetic by hand at the first point: (2 + 1 − 1)(1 + 1)/3 = 4/3, raised to the power 1. At the third point the exponent is 0.5, giving the square root of 6, 2.4495. `6.0 / x - 1.0` shows scalars on both sides of a division. `x^y` and `2^x` are powers whose exponent is a variable, which later chapters differentiate with respect to either argument.

## Constants of any shape

`function(Series)` turns an array into a constant of the same rank and shape, and `function(double)` makes a scalar. A constant has no variables, so its `arity` is 0 and it is evaluated with `f()`. Constants may be combined with each other and with traced expressions by the usual broadcasting rule, and the shape of the result is computed when the expression is built. `rank()` and `shape()` report it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
using namespace pluss::tils;

static void describe(OutputEngine &out, const char *label, const math::Function &f) {
  out.writeln(label, "arity", f.arity(), " rank", f.rank(), " shape", f.shape());
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  const math::Function scalar = math::function(3.5);
  const math::Function vector = math::function(pluss::make_series({1.0, 2.0, 3.0}));
  const math::Function matrix = math::function(
      pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}));
  const math::Function tensor = math::function(pluss::make_series({
      {{1.0, 2.0}, {3.0, 4.0}},
      {{5.0, 6.0}, {7.0, 8.0}},
  }));

  describe(out, "scalar:", scalar);
  describe(out, "vector:", vector);
  describe(out, "matrix:", matrix);
  describe(out, "tensor:", tensor);

  const math::Function scaled = matrix * scalar;
  describe(out, "matrix * scalar:", scaled);
  out.writeln("values", scaled().host<double>().array().data);

  try {
    const math::Function bad = vector + math::function(pluss::make_series({1.0, 2.0}));
    (void)bad;
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln("[3] + [2]:", text.substr(0, text.find('\n')));
  }

  // A constant owns a copy: changing the source array, or a returned result, leaves the function unchanged.
  auto source = pluss::make_series({1.0, 2.0, 3.0});
  const math::Function owned = math::function(source);
  source.add_(10.0, 1.0);
  auto result = owned();
  result.mul_(5.0);
  out.writeln("after changing the source and a result:", owned().items<double>());

  // A function of no variables can also be traced: the callable takes no parameters.
  const math::Function traced_constant = math::function({}, [] { return 4; });
  out.writeln("traced constant:", traced_constant().item<double>());
}
```

Output

```text
scalar: arity 0  rank 0  shape []
vector: arity 0  rank 1  shape [3]
matrix: arity 0  rank 2  shape [2, 2]
tensor: arity 0  rank 3  shape [2, 2, 2]
matrix * scalar: arity 0  rank 2  shape [2, 2]
values [3.50, 7.00, 10.50, 14.00]
[3] + [2]: math::Function: output shapes are not broadcast-compatible
after changing the source and a result: [1.00, 2.00, 3.00]
traced constant: 4.00
```

The shape of a constant is the shape of its array, and `rank()` is the length of `shape()`. Multiplying a `[2, 2]` matrix by a scalar broadcasts and keeps `[2, 2]`, and the values are the matrix scaled by 3.5. Adding a `[3]` constant to a `[2]` constant cannot broadcast, and the error appears when the expression is built, not later when it is evaluated. This is the rule for all shape errors: they surface at construction. The last constant in the program shows the third way to make a zero-variable function, by tracing a callable with no parameters; a scalar return value becomes a constant. A constant owns a copy of its array: changing the source array afterwards, or changing a result returned by `owned()`, leaves the function unchanged, so a `Function` may be shared freely.

## Evaluating on arrays

The arguments to a `Function` are `Series` of any shape. Operations broadcast exactly as they do on arrays, the result keeps the arguments' floating dtype, and it is created on the arguments' platform (CPU, or MPS where available).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const math::Function sum = math::function({"left", "right"}, [](math::Function l, math::Function r) { return l + r; });

  const auto column = pluss::make_series({1.0F, 2.0F}, {2, 1});
  const auto row = pluss::make_series({10.0F, 20.0F, 30.0F}, {1, 3});
  const auto grid = sum({column, row});

  out.writeln("shape of the result:", grid.sizes());
  out.writeln("dtype is Float32:", grid.scalar_type() == pluss::ScalarType::Float32 ? "yes" : "no");
  out.writeln("row 0:", grid.select(0, 0).items<double>());
  out.writeln("row 1:", grid.select(0, 1).items<double>());
  out.writeln("on the CPU:", grid.platform() == pluss::pCPU ? "yes" : "no");
}
```

Output

```text
shape of the result: [2, 3]
dtype is Float32: yes
row 0: [11.000, 21.000, 31.000]
row 1: [12.000, 22.000, 32.000]
on the CPU: yes
```

Here the arguments have shapes `[2, 1]` and `[1, 3]`, and the result is the `[2, 3]` grid of all sums, just as `column + row` would be. The result is `Float32` because the arguments are, and it lives on the CPU because they do. The same function evaluated with `Float64` arguments gives a `Float64` result, so one definition serves every precision. Where the MPS backend is available, arguments placed there are evaluated there.

## Naming a coordinate

`f.arg("x")` returns the placeholder for variable `x` of `f` as a `Function` of the same variables. It is how an operator is handed the coordinate itself, for instance to multiply by it (Chapter 5). An unknown name is an error.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const math::Function square = math::function({"x"}, [](math::Function x) { return x.pow(2); });
  const math::Function x = square.arg("x");
  const auto values = pluss::make_series({1.0, 2.0, 3.0});

  out.writeln("arg(\"x\") evaluated:", x({values}).items<double>());
  out.writeln("x * square:", (x * square)({values}).items<double>());
  try {
    (void)square.arg("y");
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln("arg(\"y\"):", text.substr(0, text.find('\n')));
  }
}
```

Output

```text
arg("x") evaluated: [1.000, 2.000, 3.000]
x * square: [1.000, 8.000, 27.000]
arg("y"): math::Function: unknown variable 'y'
```

`arg("x")` is the variable itself, and multiplying it with `square` gives x · x² = x³. The error for an unknown name is raised immediately and lists the offending name.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="building_and_evaluating.html">
    <span class="label">Previous</span>
    <span class="title">Building and evaluating a function</span>
  </a>
  <a class="next-link" href="simplification_and_assumptions.html">
    <span class="label">Next</span>
    <span class="title">Simplification and assumptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
