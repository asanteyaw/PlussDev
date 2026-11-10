<!-- generated from product.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6bf28bf1a2a6 -->

# `operators::expansion::Power::product`

[Back to `operators::expansion::Power`](../Power.md)

Multiplies two power series (the Cauchy product), truncated to the shorter length.

## Signature

```cpp
operators::Operator operators::expansion::Power::product(math::Function other) const;
```

## What It Does

`power().product(g)` acts on a coefficient vector `c` and multiplies the series it represents by the series with coefficients `g`: the result has coefficients `dₖ = Σ_{i+j=k} cᵢ gⱼ` (the Cauchy product, a convolution of the coefficient vectors), truncated to the shorter of the two lengths. Both series must be about the same centre. It is how the series of a product `f g` is obtained from the series of `f` and of `g`, such as `exp(x) · sin(x)`, without differentiating the product.

`g` is a rank-1 coefficient `Function`. The operator holds that function, so it cannot be saved (unlike the other termwise operators, which hold only numbers).

## Parameters

`math::Function other`
: The coefficient vector of the second series, a rank-1 function.

## Returns

An `Operator` whose application returns the product's coefficients, truncated to the shorter length.

## Errors

- `the second factor must be a coefficient vector`.

## Practical Examples

### Example 1

`exp(x)` times itself is `exp(2x)`, whose coefficients are `2ᵏ/k!`: the product of the order-6 coefficient vector with itself, truncated to 7 entries, gives `1, 2, 2, 4/3, 2/3, 4/15, 4/45`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace expansion = pluss::operators::expansion;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.exp(); });

  const math::Function c = operators::expansion::taylor().coefficients("x", 0.0, 6)(f);
  out.writeln("exp * exp:      ", operators::expansion::power().product(c)(c)().items<double>());
  const math::Function doubled = math::function({"x"}, [](math::Function x) { return (2.0 * x).exp(); });
  out.writeln("coefficients of exp(2x):", operators::expansion::taylor().coefficients("x", 0.0, 6)(doubled)().items<double>());
}
```

Output

```text
exp * exp:       [1.000000, 2.000000, 2.000000, 1.333333, 0.666667, 0.266667, 0.088889]
coefficients of exp(2x): [1.000000, 2.000000, 2.000000, 1.333333, 0.666667, 0.266667, 0.088889]
```

### Example 2

Two different series. `exp(x) · sin(x) = x + x² + x³/3 + 0·x⁴ − …`: the product of the order-4 coefficients is `0, 1, 1, 1/3, 0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace expansion = pluss::operators::expansion;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.exp(); });

  const math::Function s = math::function({"x"}, [](math::Function x) { return x.sin(); });
  const math::Function ce = operators::expansion::taylor().coefficients("x", 0.0, 4)(f);
  const math::Function cs = operators::expansion::taylor().coefficients("x", 0.0, 4)(s);
  out.writeln("exp * sin:", operators::expansion::power().product(cs)(ce)().items<double>());
}
```

Output

```text
exp * sin: [0.000000, 1.000000, 1.000000, 0.333333, 0.000000]
```

### Example 3

Truncation to the shorter length. Multiplying a 5-entry series by a 3-entry one gives 3 entries (anything beyond the shorter length is not determined by the shorter series).

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
  const math::Function long_series = math::function(pluss::make_series({1.0, 1.0, 1.0, 1.0, 1.0}));
  const math::Function short_series = math::function(pluss::make_series({1.0, 2.0, 3.0}));
  const math::Function product = operators::expansion::power().product(short_series)(long_series);
  out.writeln("shape:", product.shape(), "  coefficients:", product().items<double>());
}
```

Output

```text
shape: [3]   coefficients: [1.0000, 3.0000, 6.0000]
```

### Example 4

The second factor must be a coefficient vector (rank 1).

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

  const math::Function c = math::function(pluss::make_series({1.0, 2.0, 3.0}));
  const math::Function scalar = math::function({"x"}, [](math::Function x) { return x; });
  try { (void)operators::expansion::power().product(scalar)(c); } catch (const pluss::Error &e) { out.writeln(first_line(e)); }
}
```

Output

```text
operators::expansion::Power::product: the second factor must be a coefficient vector
```

## See Also

- [`derivative`](derivative.md): the termwise derivative
- [`integral`](integral.md): the termwise integral
- [`power`](../power_operator.md): rebuilds the polynomial

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="integral.html">
    <span class="label">Previous</span>
    <span class="title">integral</span>
  </a>
</div>
<!-- apiref:nav:end -->
