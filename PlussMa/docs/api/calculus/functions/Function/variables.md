<!-- generated from variables.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fe2b8a05c067 -->

# `math::Function::variables`

[Back to `math::Function`](../Function.md)

The names of the function's variables, in the order a by-position call binds them.

## Signature

```cpp
pluss::RangeView<std::string_view> math::Function::variables() const;
```

## What It Does

`variables()` returns the variable names as a read-only view, in the order they were declared in `math::function({"x", "y"}, ...)`. That order is the one a by-position call uses: `f({a, b})` binds `a` to the first name and `b` to the second. By-name calls (`f({{"y", b}, {"x", a}})`) accept the names in any order.

Use it when code has to discover what a function expects: to print a signature, to build a by-name binding list from a table of values, to check that a function has the variable an operator is going to act on (`differential("t")` needs a variable `t`), or to pair the variables of a function with the columns of a data set.

The view refers to names stored inside the function. Read it while the function is alive; copy the names (`std::string(name)`) if you need them afterwards. After an operator the list reflects the new function, not the old: integrating over `x` removes `x` from the result's variables.

## Returns

A `RangeView<std::string_view>` of the names, valid while the function is alive.

## Errors

- The function must be defined.

## Practical Examples

### Example 1

The variables in declaration order. The order is the declaration order, not alphabetical.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"time", "rate", "level"}, [](math::Function t, math::Function r, math::Function l) { return t * r + l; });
  out.write("variables:");
  for (const std::string_view name : f.variables()) out.write(" ", name);
  out.writeln();
  out.writeln("arity:", f.arity());
}
```

Output

```text
variables:  time  rate  level
arity: 3
```

`time` comes first although it sorts last; a by-position call `f({a, b, c})` binds `a` to `time`.

### Example 2

Build a by-name binding list from the names the function reports, here a table of values keyed by name. The function is evaluated with arguments the code did not have to know the order of.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <map>
#include <string>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"b", "a"}, [](math::Function b, math::Function a) { return 10.0 * b + a; });
  const std::map<std::string, double> table = {{"a", 1.0}, {"b", 2.0}};

  std::vector<math::Binding> bindings;
  for (const std::string_view name : f.variables()) {
    bindings.emplace_back(name, pluss::make_series(table.at(std::string(name))));
  }
  out.writeln("f(a = 1, b = 2) =", f(pluss::RangeView<math::Binding>(bindings)).item<double>());
}
```

Output

```text
f(a = 1, b = 2) = 21.0000
```

`math::Binding` is a `std::pair<std::string_view, pluss::Series>`. The bindings borrow the names from the function, which outlives them here. `10 b + a = 21`.

### Example 3

The list follows the function through operators. Integrating over `x` binds it, so it leaves the list; differentiating keeps every name.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto names = [](const math::Function &f) {
    std::string joined;
    for (const std::string_view name : f.variables()) joined += std::string(name) + " ";
    return joined;
  };
  const math::Function f = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * x * y; });
  out.writeln("f:                 ", names(f));
  out.writeln("df/dx:             ", names(operators::differential("x")(f)));
  out.writeln("integral over x:   ", names(operators::integral("x", 0.0, 1.0)(f)));
}
```

Output

```text
f:                  x y
df/dx:              x y
integral over x:    y
```

After the integral the only name left is `y`: the result is a function of `y` alone.

## See Also

- [`arity`](arity.md): the number of variables
- [`arg`](arg.md): turns a variable into an expression
- [`operator()`](call.md): supplying arguments by position or by name

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transpose.html">
    <span class="label">Previous</span>
    <span class="title">transpose</span>
  </a>
</div>
<!-- apiref:nav:end -->
