<!-- generated from integral.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d8c5831baa53 -->

# `operators::expansion::Power::integral`

[Back to `operators::expansion::Power`](../Power.md)

Integrates a power series termwise from the centre: `c′₀ = 0`, `c′_{k+1} = cₖ/(k + 1)`.

## Signature

```cpp
operators::Operator operators::expansion::Power::integral() const;
```

## What It Does

`power().integral()` acts on a coefficient vector `[c₀, …, c_n]` and returns the coefficients of the series integrated from the centre, `[0, c₀, c₁/2, c₂/3, …, c_n/(n+1)]`: one entry *more*, with a leading zero (the integral vanishes at the centre). It is exact, and it is how a power series for `log`, `arctan` or an error function is obtained from the series of its derivative. The input must be a rank-1 coefficient vector.

## Returns

An `Operator` whose application returns the integrated coefficients, one entry more, with a leading zero.

## Errors

- The input must be a rank-1 coefficient vector.

## Practical Examples

### Example 1

From the geometric series to a logarithm. `1/(1 − x) = 1 + x + x² + …` integrates to `x + x²/2 + x³/3 + …`, which is `−log(1 − x)`. At `x = 0.3` the series truncated at order 6 gives `0.356664`; the exact value is `−log 0.7 = 0.356675`.

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
  const math::Function geometric = math::function({"x"}, [](math::Function x) { return 1.0 / (1.0 - x); });
  const math::Function c = operators::expansion::taylor().coefficients("x", 0.0, 6)(geometric);
  const math::Function integrated = operators::expansion::power().integral()(c);
  out.writeln("coefficients:", integrated().items<double>());
  out.writeln("series at 0.3:", operators::expansion::power("x", 0.0)(integrated)(0.3).item<double>(), "  -log(0.7) =", -std::log(0.7));
}
```

Output

```text
coefficients: [0.000000, 1.000000, 0.500000, 0.333333, 0.250000, 0.200000, 0.166667, 0.142857]
series at 0.3: 0.356664   -log(0.7) = 0.356675
```

### Example 2

Integration and differentiation are inverses on coefficients, apart from the constant: integrate `[1, 2, 3]` to `[0, 1, 1, 1]`, then differentiate back to `[1, 2, 3]`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace expansion = pluss::operators::expansion;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function c = math::function(pluss::make_series({1.0, 2.0, 3.0}));
  const math::Function up = operators::expansion::power().integral()(c);
  out.writeln("integrated:   ", up().items<double>());
  out.writeln("then derived: ", operators::expansion::power().derivative()(up)().items<double>());
}
```

Output

```text
integrated:    [0.0000, 1.0000, 1.0000, 1.0000]
then derived:  [1.0000, 2.0000, 3.0000]
```

### Example 3

The arctangent from its derivative. `1/(1 + x²) = 1 − x² + x⁴ − …` integrates to `arctan x = x − x³/3 + x⁵/5 − …`; the order-9 series at `x = 0.5` is `0.463684`, against `atan 0.5 = 0.463648`.

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return 1.0 / (1.0 + x * x); });
  const math::Function arctan_series = operators::expansion::power("x", 0.0)(operators::expansion::power().integral()(operators::expansion::taylor().coefficients("x", 0.0, 9)(f)));
  out.writeln("series at 0.5:", arctan_series(0.5).item<double>(), "  atan(0.5) =", std::atan(0.5));
}
```

Output

```text
series at 0.5: 0.463684   atan(0.5) = 0.463648
```

### Example 4

The input must be a coefficient vector (rank 1).

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

  const math::Function scalar = math::function({"x"}, [](math::Function x) { return x; });
  try { (void)operators::expansion::power().integral()(scalar); } catch (const pluss::Error &e) { out.writeln(first_line(e)); }
}
```

Output

```text
operators::expansion::Power::integral: needs a coefficient vector
```

## See Also

- [`derivative`](derivative.md): the termwise derivative
- [`product`](product.md): the Cauchy product
- [`power`](../power_operator.md): rebuilds the polynomial

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="derivative.html">
    <span class="label">Previous</span>
    <span class="title">derivative</span>
  </a>
  <a class="next-link" href="product.html">
    <span class="label">Next</span>
    <span class="title">product</span>
  </a>
</div>
<!-- apiref:nav:end -->
