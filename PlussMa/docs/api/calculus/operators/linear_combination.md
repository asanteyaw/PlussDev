<!-- generated from linear_combination.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::linear_combination`

The weighted sum of operators: `c₁A₁ + c₂A₂ + …`.

## Signature

```cpp
operators::Operator operators::linear_combination(std::initializer_list<operators::Term> terms);
operators::Operator operators::linear_combination(pluss::RangeView<operators::Term> terms);
operators::Operator operators::linear_combination(pluss::RangeView<operators::Operator> operators, pluss::RangeView<double> coefficients);
```

## What It Does

`linear_combination` builds the operator `f ↦ Σ cᵢ Aᵢ(f)`. This is how differential operators with several terms are written: `D² + 2D + I`, a Laplacian plus a potential, a finite-difference stencil made of shifts. Every operator in the sum is applied to the same input and the results are added.

There are three ways to give the terms, all equivalent. The brace list of `{coefficient, operator}` pairs reads best for a fixed expression. A `RangeView<Term>` takes the same pairs from a container built at run time (`Term` is `std::pair<double, Operator>`). The two-range form takes the operators and the coefficients as parallel lists; their lengths must match.

The combination is linear if all its operators are, has an adjoint if all do, and is savable only if all are (a combination that includes a `multiply` is not). The terms must be applicable to the same function: they must all find the variables they name.

## Parameters

`std::initializer_list<operators::Term> terms`
: `{coefficient, operator}` pairs. `Term` is `std::pair<double, operators::Operator>`.

`pluss::RangeView<operators::Term> terms`
: The same pairs as a range.

`pluss::RangeView<operators::Operator> operators`
: The operators, for the two-range form.

`pluss::RangeView<double> coefficients`
: The coefficients, one per operator, in the same order.

## Returns

An `Operator` equal to the weighted sum.

## Errors

- `operator and coefficient counts must match` for the two-range form.
- `at least one operator is required` for an empty list.
- Every operator must be defined.

## Practical Examples

### Example 1

The brace-list form: the operator `D − 3 I` annihilates `e^{3x}`, since `(e^{3x})′ = 3 e^{3x}`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function g = math::function({"x"}, [](math::Function x) { return (3.0 * x).exp(); });
  const operators::Operator L = operators::linear_combination({{1.0, operators::differential("x")}, {-3.0, operators::identity()}});
  out.writeln("(D - 3I) e^{3x} at 1 =", L(g)(1.0).item<double>());
  out.writeln("(D - 2I) e^{3x} at 1 =", operators::linear_combination({{1.0, operators::differential("x")}, {-2.0, operators::identity()}})(g)(1.0).item<double>(), "  (e^3 = 20.0855)");
}
```

Output

```text
(D - 3I) e^{3x} at 1 = 0.0000
(D - 2I) e^{3x} at 1 = 20.0855   (e^3 = 20.0855)
```

### Example 2

The `Term` range form, for a combination assembled at run time: a finite-difference approximation of the second derivative, `(f(x+h) − 2f(x) + f(x−h)) / h²`, with the weights built in a loop.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const double h = 0.01;
  std::vector<operators::Term> terms;
  terms.push_back({1.0 / (h * h), operators::shift("x", h)});
  terms.push_back({-2.0 / (h * h), operators::identity()});
  terms.push_back({1.0 / (h * h), operators::shift("x", -h)});
  const operators::Operator stencil = operators::linear_combination(pluss::RangeView<operators::Term>(terms));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.sin(); });
  out.writeln("stencil sin at 1  =", stencil(f)(1.0).item<double>());
  out.writeln("exact -sin(1)     =", -std::sin(1.0));
}
```

Output

```text
stencil sin at 1  = -0.84146
exact -sin(1)     = -0.84147
```

### Example 3

The two-range form: operators and coefficients as parallel lists. `1·I + 2·D + 3·D²` applied to `x²` at 2 is `4 + 8 + 6 = 18`.

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
  const std::vector<operators::Operator> ops = {operators::identity(), operators::differential("x"), operators::differential("x", 2)};
  const std::vector<double> coefficients = {1.0, 2.0, 3.0};
  const operators::Operator L = operators::linear_combination(pluss::RangeView<operators::Operator>(ops), pluss::RangeView<double>(coefficients));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x; });
  out.writeln("(I + 2D + 3D^2) x^2 at 2 =", L(f)(2.0).item<double>());
}
```

Output

```text
(I + 2D + 3D^2) x^2 at 2 = 18.0000
```

### Example 4

The counts must match, and a combination needs at least one term.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  const std::vector<operators::Operator> ops = {operators::identity(), operators::differential("x")};
  const std::vector<double> one = {1.0};
  try { (void)operators::linear_combination(pluss::RangeView<operators::Operator>(ops), pluss::RangeView<double>(one)); } catch (const pluss::Error &e) { out.writeln("count mismatch:", first_line(e)); }
  try { (void)operators::linear_combination(std::initializer_list<operators::Term>{}); } catch (const pluss::Error &e) { out.writeln("no terms:      ", first_line(e)); }
}
```

Output

```text
count mismatch: operators::linear_combination: operator and coefficient counts must match
no terms:       operators::linear_combination: at least one operator is required
```

## See Also

- [`compose`](compose.md): chains operators
- [`scale`](scale.md): a single scaled operator
- [`identity`](identity.md): the unit term

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="identity.html">
    <span class="label">Previous</span>
    <span class="title">identity</span>
  </a>
  <a class="next-link" href="multiply.html">
    <span class="label">Next</span>
    <span class="title">multiply</span>
  </a>
</div>
<!-- apiref:nav:end -->
