<!-- generated from call.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c6c6feacc03d -->

# `math::Function::operator()`

[Back to `math::Function`](../Function.md)

The call operator, `f(...)`: evaluates the function with a scalar, with a list of `Series` in declaration order, or with named bindings in any order.

## Signature

```cpp
pluss::Series math::Function::operator()() const;
pluss::Series math::Function::operator()(double argument) const;
pluss::Series math::Function::operator()(float argument) const;
pluss::Series math::Function::operator()(int argument) const;
pluss::Series math::Function::operator()(std::int64_t argument) const;
pluss::Series math::Function::operator()(std::complex<double> argument) const;
pluss::Series math::Function::operator()(std::complex<float> argument) const;
pluss::Series math::Function::operator()(std::initializer_list<pluss::Series> arguments) const;
pluss::Series math::Function::operator()(pluss::RangeView<pluss::Series> arguments) const;
pluss::Series math::Function::operator()(std::initializer_list<math::Binding> bindings) const;
pluss::Series math::Function::operator()(pluss::RangeView<math::Binding> bindings) const;
```

## What It Does

Calling a function replays its recorded formula on the data you supply and returns the result as a `Series`. There are four ways to supply the data, and which to use depends on how many variables the function has and how the data is held.

| Call | Use it when |
|---|---|
| `f()` | the function has no variables (a constant, or a fully integrated function) |
| `f(2.0)`, `f(2.0F)`, `f(2)`, `f(std::int64_t{2})` | the function has one variable and the value is a single native number |
| `f(1.0 + 2.0i)`, `f(std::complex<float>{...})` | one variable, and the value is a complex number |
| `f({a, b, ...})`, `f(RangeView<Series>)` | any number of variables, given by position in the order of [`variables()`](variables.md) |
| `f({{"y", b}, {"x", a}})`, `f(RangeView<Binding>)` | any number of variables, given by name in any order |

The scalar forms are shorthand for one-variable functions: `f(2.0)` is `f({pluss::make_series(2.0)})`, and the argument keeps the numeric type you gave it. A function of several variables has no scalar shorthand; give a `Series` for each variable.

**Batches.** Each argument may be a `Series` of any shape, and the arguments broadcast against each other exactly as `Series` arithmetic does. The function's output axes ([`shape()`](shape.md)) come last; whatever shape the broadcast arguments have comes in front. So one call can evaluate a function at a thousand points, on a grid, or over a stack of grids, with no loop.

The call checks everything before any arithmetic happens: the number of arguments, the names, and that each `Series` is defined. Domain errors (a `log` of a negative number, a division by zero) are raised while evaluating, on the values that arrive.

## Parameters

`double argument`
: A native scalar for a function of one variable. The `float`, `int`, `std::int64_t`, `std::complex<double>` and `std::complex<float>` overloads do the same for their types.

`std::initializer_list<pluss::Series> arguments`
: One `Series` per variable, in the order of `variables()`. Each may have any shape; they broadcast together.

`pluss::RangeView<pluss::Series> arguments`
: The same list as a range, for arguments assembled at run time.

`std::initializer_list<math::Binding> bindings`
: `{name, Series}` pairs, one per variable, in any order. A `Binding` is `std::pair<std::string_view, pluss::Series>`.

`pluss::RangeView<math::Binding> bindings`
: The same pairs as a range.

## Returns

A `pluss::Series`: the function's value, with the broadcast shape of the arguments in front of the function's own output axes. A scalar call on a scalar function returns a rank-0 `Series`.

## Errors

- The number of positional arguments must equal `arity()`: `ordered argument count N does not match function arity M`.
- By name, every variable must appear exactly once; unknown, repeated and missing names are each refused.
- Every `Series` must be defined.
- Domain errors (for example `log` of a non-positive number or a zero denominator) are raised during evaluation, naming the operation.

## Practical Examples

### Example 1

One variable, native scalar. The four numeric overloads pass the number as a scalar of its own type.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x - 2.0 * x; });
  out.writeln("f(3.0)             =", f(3.0).item<double>());
  out.writeln("f(3.0F)            =", f(3.0F).item<double>());
  out.writeln("f(3)               =", f(3).item<double>());
  out.writeln("f(std::int64_t{3}) =", f(std::int64_t{3}).item<double>());
}
```

Output

```text
f(3.0)             = 3.0000
f(3.0F)            = 3.0000
f(3)               = 3.0000
f(std::int64_t{3}) = 3.0000
```

All four give `3² − 6 = 3`. The result is a scalar `Series`; `item<double>()` reads it as a number.

### Example 2

Complex arguments. The function below is the polynomial `z² + 1`; at `z = 1 + 2i` it is `−2 + 4i`. The two overloads differ only in precision.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function p = math::function({"z"}, [](math::Function z) { return z * z + 1.0; });
  const std::complex<double> w = p(std::complex<double>{1.0, 2.0}).item<std::complex<double>>();
  const std::complex<float> v = p(std::complex<float>{1.0F, 2.0F}).item<std::complex<float>>();
  out.writeln("complex<double>:", w.real(), w.imag());
  out.writeln("complex<float>: ", v.real(), v.imag());
}
```

Output

```text
complex<double>: -2.0000 4.0000
complex<float>:  -2.0000 4.0000
```

A complex scalar is passed as a rank-0 complex `Series`, so complex-valued results come back as complex `Series`.

### Example 3

No variables: a constant is called with an empty argument list, and so is a function whose variables have all been integrated out.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function constant = math::function(2.5);
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x; });
  const math::Function area = operators::integral("x", 0.0, 3.0)(f);
  out.writeln("constant: arity", constant.arity(), " value", constant().item<double>());
  out.writeln("area:     arity", area.arity(), " value", area().item<double>());
}
```

Output

```text
constant: arity 0  value 2.5000
area:     arity 0  value 9.0000
```

`∫₀³ x² dx = 9`.

### Example 4

By position. The list is matched to the variables in declaration order; both spellings take any `Series`, here vectors of three points. `RangeView` is the form for argument lists built at run time.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x + 10.0 * y; });
  const auto x = pluss::make_series({1.0, 2.0, 3.0});
  const auto y = pluss::make_series({0.1, 0.2, 0.3});

  out.writeln("f({x, y}) =", f({x, y}).items<double>());

  std::vector<pluss::Series> args = {x, y};
  out.writeln("RangeView =", f(pluss::RangeView<pluss::Series>(args)).items<double>());
}
```

Output

```text
f({x, y}) = [2.0000, 4.0000, 6.0000]
RangeView = [2.0000, 4.0000, 6.0000]
```

The two calls are the same evaluation.

### Example 5

By name. A binding is a `{name, Series}` pair; the pairs may come in any order, and each variable must appear exactly once.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x + 10.0 * y; });
  const auto x = pluss::make_series({1.0, 2.0, 3.0});
  const auto y = pluss::make_series({0.1, 0.2, 0.3});

  out.writeln("y first: ", f({{"y", y}, {"x", x}}).items<double>());

  std::vector<math::Binding> bindings = {{"x", x}, {"y", y}};
  out.writeln("RangeView:", f(pluss::RangeView<math::Binding>(bindings)).items<double>());
}
```

Output

```text
y first:  [2.0000, 4.0000, 6.0000]
RangeView: [2.0000, 4.0000, 6.0000]
```

Naming the arguments removes any doubt about the order, which is worth the extra braces for functions with more than two variables.

### Example 6

Batches and broadcasting. A column of three `x` values against a row of two `y` values gives a `3 × 2` grid, and a vector-valued function appends its own axis after the batch axes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function g = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * y; });
  const math::Function pair = math::function({"x", "y"}, [](math::Function x, math::Function y) { return math::stack({x + y, x * y}); });
  const auto column = pluss::make_series({1.0, 2.0, 3.0}).reshape({3, 1});
  const auto row = pluss::make_series({10.0, 20.0}).reshape({1, 2});

  out.writeln("scalar function on a grid, shape:", g({column, row}).sizes());
  out.writeln("vector function on a grid, shape:", pair({column, row}).sizes());
  out.writeln("the grid:", g({column, row}).reshape({6}).items<double>());
}
```

Output

```text
scalar function on a grid, shape: [3, 2]
vector function on a grid, shape: [3, 2, 2]
the grid: [10.0000, 20.0000, 20.0000, 40.0000, 30.0000, 60.0000]
```

Batch axes first (`3 × 2`), then the function's own axis (the pair `x + y`, `x · y`).

### Example 7

The checks that run before evaluation: the wrong number of arguments, an unknown or repeated name, a missing name and an undefined `Series` are each refused with a message that names the problem.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <functional>
#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * y; });
  const auto a = pluss::make_series({1.0, 2.0});
  const auto attempt = [&](const char *label, const std::function<void()> &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      const std::string text(e.what());
      out.writeln(label, text.substr(0, text.find('\n')));
    }
  };
  attempt("one of two, by position:", [&] { (void)f({a}); });
  attempt("missing name:           ", [&] { (void)f({{"x", a}}); });
  attempt("unknown name:           ", [&] { (void)f({{"x", a}, {"z", a}}); });
  attempt("repeated name:          ", [&] { (void)f({{"x", a}, {"x", a}, {"y", a}}); });
  attempt("undefined Series:       ", [&] { (void)f({a, pluss::Series{}}); });
}
```

Output

```text
one of two, by position: math::Function: ordered argument count 1 does not match function arity 2
missing name:            math::Function: missing variable binding 'y'
unknown name:            math::Function: unknown variable binding 'z'
repeated name:           math::Function: duplicate variable binding 'x'
undefined Series:        math::Function: arguments must be defined Series values
```

All are `pluss::Error`, raised before any arithmetic.

## See Also

- [`variables`](variables.md): the names and the order of positional arguments
- [`arity`](arity.md): how many arguments a call needs
- [`shape`](shape.md): the output axes that follow the batch axes

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="assumption.html">
    <span class="label">Previous</span>
    <span class="title">assumption</span>
  </a>
  <a class="next-link" href="compose.html">
    <span class="label">Next</span>
    <span class="title">compose</span>
  </a>
</div>
<!-- apiref:nav:end -->
