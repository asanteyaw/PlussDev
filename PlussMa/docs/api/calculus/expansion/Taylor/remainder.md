<!-- generated from remainder.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 738766789a16 -->

# `operators::expansion::Taylor::remainder`

[Back to `operators::expansion::Taylor`](../Taylor.md)

The Lagrange remainder factor `f⁽ⁿ⁺¹⁾(x)/(n+1)!`, which bounds the error of the Taylor polynomial.

## Signature

```cpp
operators::Operator operators::expansion::Taylor::remainder(std::string_view variable, std::int64_t order, operators::DiffOptions options = {}) const;
```

## What It Does

Truncating the Taylor series at order `n` leaves an error. Lagrange's form says: between the centre `a` and `x`, `f(x) − Tₙ(x) = f⁽ⁿ⁺¹⁾(ξ)/(n+1)! (x − a)ⁿ⁺¹` for some `ξ`, so

`|f(x) − Tₙ(x)| ≤ sup |f⁽ⁿ⁺¹⁾|/(n+1)! · |x − a|ⁿ⁺¹`,

the supremum taken over the interval between `a` and `x`. `taylor().remainder("x", n)` is the operator that returns the factor `f⁽ⁿ⁺¹⁾(x)/(n+1)!` as a function of `x` (and of the other variables); its largest absolute value over the interval of interest is the constant in the bound. Use it to choose the order for a required accuracy, or to certify an approximation.

The remainder factor depends only on `f` and `n`, not on the centre, so it takes no centre argument. The order must lie in `[0, 64]`.

## Parameters

`std::string_view variable`
: The variable of the expansion.

`std::int64_t order`
: The truncation order `n`, in `[0, 64]`; the factor is `f⁽ⁿ⁺¹⁾/(n+1)!`.

`operators::DiffOptions options`
: Options for the derivatives.

## Returns

An `Operator` whose application returns the remainder factor as a `Function`.

## Errors

- `order must lie in [0, 64]`.
- An unknown variable raises `unknown variable '<name>'`.

## Practical Examples

### Example 1

For `exp` the factor is `eˣ/(n+1)!`; for `n = 3` at `x = 0.5` it is `e^{0.5}/24 = 0.06870`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace expansion = pluss::operators::expansion;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.exp(); });

  const math::Function factor = operators::expansion::taylor().remainder("x", 3)(f);
  out.writeln("factor(0.5) =", factor(0.5).item<double>(), "  e^0.5 / 24 =", std::exp(0.5) / 24.0);
}
```

Output

```text
factor(0.5) = 0.068697   e^0.5 / 24 = 0.068697
```

### Example 2

The bound in action. For `exp` about 0 at `x = 0.5`, the true error of each polynomial lies below `sup|factor| · 0.5ⁿ⁺¹`, where the supremum over `[0, 0.5]` is the factor at the right end (it is increasing).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace expansion = pluss::operators::expansion;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.exp(); });

  for (const std::int64_t order : {2, 4, 6, 8}) {
    const math::Function T = operators::expansion::taylor("x", 0.0, order)(f);
    const double bound = operators::expansion::taylor().remainder("x", order)(f)(0.5).item<double>() * std::pow(0.5, order + 1);
    out.writeln("order", order, ": error", std::exp(0.5) - T(0.5).item<double>(), "  bound", bound);
  }
}
```

Output

```text
order 2 : error 0.023721270700   bound 0.034348359806
order 4 : error 0.000283770700   bound 0.000429354498
order 6 : error 0.000001652645   bound 0.000002555682
order 8 : error 0.000000005664   bound 0.000000008874
```

### Example 3

For `sin` every derivative is bounded by 1, so the factor never exceeds `1/(n+1)!`: for `n = 5` the factor is at most `1/720`. Evaluated along a grid it stays below that.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace expansion = pluss::operators::expansion;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const math::Function s = math::function({"x"}, [](math::Function x) { return x.sin(); });
  const math::Function factor = operators::expansion::taylor().remainder("x", 5)(s);
  const auto x = pluss::series::linspace(-3.0, 3.0, 7, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  out.writeln("factor on a grid:", factor({x}).items<double>());
  out.writeln("largest |factor| =", factor({x}).abs().max().item<double>(), "  1/720 =", 1.0 / 720.0);
}
```

Output

```text
factor on a grid: [0.00019600, 0.00126291, 0.00116871, 0.00000000, -0.00116871, -0.00126291, -0.00019600]
largest |factor| = 0.00126291   1/720 = 0.00138889
```

### Example 4

The order range is checked.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace expansion = pluss::operators::expansion;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  try { (void)operators::expansion::taylor().remainder("x", -1); } catch (const pluss::Error &e) { out.writeln(first_line(e)); }
}
```

Output

```text
operators::expansion::Taylor::remainder: order must lie in [0, 64]
```

## See Also

- [`taylor`](../taylor_operator.md): the polynomial whose error it bounds
- [`coefficients`](coefficients.md): the coefficients

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="coefficients.html">
    <span class="label">Previous</span>
    <span class="title">coefficients</span>
  </a>
</div>
<!-- apiref:nav:end -->
