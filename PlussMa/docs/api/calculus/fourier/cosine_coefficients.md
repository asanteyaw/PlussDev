<!-- generated from cosine_coefficients.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::fourier::cosine_coefficients`

The cosine-series coefficients `A₀ … A_{terms−1}` of a function on an interval, as a vector function.

## Signature

```cpp
operators::Operator operators::fourier::cosine_coefficients(std::string_view variable, double lower, double upper, std::int64_t terms);
operators::Operator operators::fourier::cosine_coefficients(std::string_view variable, double lower, double upper, std::int64_t terms, double support_lower, double support_upper);
```

## What It Does

`cosine_coefficients("x", a, b, n)` is the operator that takes `f` and returns the vector `[A₀, A₁, …, A_{n−1}]` of its half-range cosine coefficients on `[a, b]`. The result is a rank-1 `Function` of shape `[n]`; the coefficients are definite integrals of `f` against cosines, so they depend on any variables of `f` other than the expanded one. The coefficients are what you store, transform or manipulate; to reconstruct the function from them use [`cosine_series`](cosine_series.md).

Half-range expansions on `[lower, upper]` use `uₖ = kπ / (upper − lower)`:

- cosine: `f ≈ A₀/2 + Σₖ₌₁ Aₖ cos(uₖ (x − lower))`, with `Aₖ = 2/(b − a) ∫ f cos(uₖ (x − lower)) dx`,
- sine: `f ≈ Σₖ₌₁ Bₖ sin(uₖ (x − lower))`, with `Bₖ = 2/(b − a) ∫ f sin(uₖ (x − lower)) dx`.

The cosine series extends `f` to an even function, so it converges well (like `1/k²`) even when `f` does not vanish at the ends, which is why it is preferred for smooth functions on an interval. The terms count must lie in `[1, 4096]` and the interval must be finite.

The second overload integrates only over a sub-interval `[support_lower, support_upper]` of `[lower, upper]`, while keeping the cosines anchored at `lower` with the full width. This is the form needed for the COS method of option pricing, where the payoff is non-zero on part of the truncation interval: the coefficients of a payoff such as `max(S − K, 0)` are integrals over `[K, upper]`.

## Parameters

`std::string_view variable`
: The variable to expand in.

`double lower`
: The left end of the interval.

`double upper`
: The right end; must be greater than `lower` and finite.

`std::int64_t terms`
: How many coefficients to return, `A₀ … A_{terms−1}`; between 1 and 4096.

`double support_lower`
: The left end of the sub-interval of integration (second overload); must lie in `[lower, upper]`.

`double support_upper`
: The right end of the sub-interval; must exceed `support_lower`.

## Returns

An `Operator` whose application returns a rank-1 `Function` of shape `[terms]`.

## Errors

- `terms must lie in [1, 4096]`.
- `the interval must be finite with lower < upper`.
- `the support must be a non-empty sub-interval`.

## Practical Examples

### Example 1

The coefficients of `f(x) = x` on `[0, π]`: `A₀ = π`, `Aₖ = 2((−1)ᵏ − 1)/(π k²)`, which is `−4/(π k²)` for odd `k` and `0` for even `k`. Computed here and compared with the formula.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace fourier = pluss::operators::fourier;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const double pi = std::acos(-1.0);
  const math::Function f = math::function({"x"}, [](math::Function x) { return x; });

  const math::Function A = fourier::cosine_coefficients("x", 0.0, pi, 5)(f);
  const auto a = A().items<double>();
  out.writeln("shape:", A.shape());
  out.writeln("A0, A1, A3:", a[0], a[1], a[3]);
  out.writeln("formula:   ", pi, -4.0 / pi, -4.0 / (9.0 * pi));
  out.writeln("A2, A4 (zero to rounding):", std::abs(a[2]), std::abs(a[4]));
}
```

Output

```text
shape: [5]
A0, A1, A3: 3.14159 -1.27324 -0.14147
formula:    3.14159 -1.27324 -0.14147
A2, A4 (zero to rounding): 0.00000 0.00000
```

### Example 2

The sub-interval overload. Integrating `f(x) = x` over only `[0, π/2]` of the full interval `[0, π]` gives `A₀ = π/4` and `A₁ = (2/π)(π/2 − 1)`; the cosines are still those of the full width `π`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace fourier = pluss::operators::fourier;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const double pi = std::acos(-1.0);
  const math::Function f = math::function({"x"}, [](math::Function x) { return x; });

  const math::Function A = fourier::cosine_coefficients("x", 0.0, pi, 4, 0.0, pi / 2)(f);
  out.writeln("support [0, pi/2]:", A().items<double>());
  out.writeln("A0 = pi/4 =", pi / 4.0, "   A1 = (2/pi)(pi/2 - 1) =", 2.0 / pi * (pi / 2.0 - 1.0));
}
```

Output

```text
support [0, pi/2]: [0.78540, 0.36338, -0.31831, -0.40407]
A0 = pi/4 = 0.78540    A1 = (2/pi)(pi/2 - 1) = 0.36338
```

### Example 3

Coefficients depend on the function's other variables. For `f(x, a) = exp(a x)` the vector is a function of `a`: evaluated at three values of `a` it gives a `[3, terms]` array.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace fourier = pluss::operators::fourier;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const double pi = std::acos(-1.0);

  const math::Function f = math::function({"x", "a"}, [](math::Function x, math::Function a) { return (a * x).exp(); });
  const math::Function A = fourier::cosine_coefficients("x", 0.0, 1.0, 3)(f);
  out.writeln("variables left:", A.arity(), "  shape", A.shape());
  const pluss::Series coefficients = A({pluss::make_series({0.0, 1.0, 2.0})});
  out.writeln("shape at three a:", coefficients.sizes());
  out.writeln("A0 at a = 0, 1, 2:", coefficients.select(1, 0).items<double>(), " (2 (e^a - 1) / a: 0 -> 2, 1 -> 3.4366, 2 -> 6.3891)");
}
```

Output

```text
variables left: 1   shape [3]
shape at three a: [3, 3]
A0 at a = 0, 1, 2: [2.0000, 3.4366, 6.3891]  (2 (e^a - 1) / a: 0 -> 2, 1 -> 3.4366, 2 -> 6.3891)
```

### Example 4

What is refused: a term count outside `[1, 4096]`, an interval that is not finite with `lower < upper`, and a support that is not a non-empty sub-interval.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace fourier = pluss::operators::fourier;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const double pi = std::acos(-1.0);
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  try { (void)fourier::cosine_coefficients("x", 0.0, pi, 0); } catch (const pluss::Error &e) { out.writeln("terms = 0:        ", first_line(e)); }
  try { (void)fourier::cosine_coefficients("x", 1.0, 1.0, 3); } catch (const pluss::Error &e) { out.writeln("lower = upper:    ", first_line(e)); }
  try { (void)fourier::cosine_coefficients("x", 0.0, 1.0, 3, -1.0, 0.5); } catch (const pluss::Error &e) { out.writeln("support outside:  ", first_line(e)); }
}
```

Output

```text
terms = 0:         operators::fourier::cosine_coefficients: terms must lie in [1, 4096]
lower = upper:     operators::fourier::cosine_coefficients: the interval must be finite with lower < upper
support outside:   operators::fourier::cosine_coefficients: the support must be a non-empty sub-interval
```

## See Also

- [`cosine_series`](cosine_series.md): the reconstruction
- [`sine_coefficients`](sine_coefficients.md): the sine counterpart
- [`integral`](../integral/integral.md): the integrals the coefficients are built from

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="coefficients.html">
    <span class="label">Previous</span>
    <span class="title">coefficients</span>
  </a>
  <a class="next-link" href="cosine_series.html">
    <span class="label">Next</span>
    <span class="title">cosine_series</span>
  </a>
</div>
<!-- apiref:nav:end -->
