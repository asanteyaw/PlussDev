<!-- generated from coefficients.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::fourier::coefficients`

The full trigonometric Fourier coefficients of a function on a symmetric period `[−L, L]`.

## Signature

```cpp
operators::Operator operators::fourier::coefficients(std::string_view variable, double half_period, std::int64_t terms);
```

## What It Does

`coefficients("x", L, n)` applied to `f` returns the Fourier coefficients on the symmetric interval `[−L, L]`:

`aₖ = (1/L) ∫_{−L}^{L} f cos(kπx/L) dx`, `bₖ = (1/L) ∫_{−L}^{L} f sin(kπx/L) dx`,

packed into one vector `[a₀, a₁, …, a_{n−1}, b₁, …, b_{n−1}]` of length `2n − 1`. The matching series is `f ≈ a₀/2 + Σ (aₖ cos(kπx/L) + bₖ sin(kπx/L))`, rebuilt by [`series`](series.md). The full series is the cosine series of the *even part* of `f` plus the sine series of its *odd part*: an even function has all `bₖ = 0`, an odd function all `aₖ = 0`.

Use it for periodic functions and signals defined over one period. The half-period must be positive and `terms` between 1 and 4096.

## Parameters

`std::string_view variable`
: The variable to expand in.

`double half_period`
: `L`, half the period; the interval is `[−L, L]`. Positive.

`std::int64_t terms`
: How many cosine coefficients `a₀ … a_{terms−1}` (and `terms − 1` sine coefficients) to return; between 1 and 4096.

## Returns

An `Operator` whose application returns a rank-1 `Function` of shape `[2·terms − 1]`.

## Errors

- `half_period must be positive`.
- `terms must lie in [1, 4096]`.

## Practical Examples

### Example 1

An odd function: `f(x) = x` on `[−π, π]` has `aₖ = 0` and `bₖ = 2(−1)^{k+1}/k`. The vector holds `a₀ … a₃` (zeros) then `b₁ … b₃`.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const double pi = std::acos(-1.0);
  const math::Function f = math::function({"x"}, [](math::Function x) { return x; });

  const math::Function c = fourier::coefficients("x", pi, 4)(f);
  const auto v = c().items<double>();
  out.writeln("length:", c.shape()[0]);
  out.writeln("largest |a_k|, k = 0..3 (zero to rounding):", std::max({std::abs(v[0]), std::abs(v[1]), std::abs(v[2]), std::abs(v[3])}));
  out.writeln("b1..b3:", v[4], v[5], v[6], "  (2, -1, 2/3)");
}
```

Output

```text
length: 7
largest |a_k|, k = 0..3 (zero to rounding): 0.0000
b1..b3: 2.0000 -1.0000 0.6667   (2, -1, 2/3)
```

### Example 2

An even function: `f(x) = x²` on `[−π, π]` has `bₖ = 0` and `aₖ = 4(−1)^k/k²`, with `a₀ = 2π²/3`.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const double pi = std::acos(-1.0);

  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x; });
  const math::Function c = fourier::coefficients("x", pi, 4)(f);
  out.writeln("[a0..a3, b1..b3]:", c().items<double>());
  out.writeln("a0 = 2 pi^2/3 =", 2.0 * pi * pi / 3.0, "  a1, a2, a3 =", pluss::make_series({-4.0, 1.0, -4.0 / 9.0}).items<double>());
}
```

Output

```text
[a0..a3, b1..b3]: [6.5797, -4.0000, 1.0000, -0.4444, 0.0000, 0.0000, 0.0000]
a0 = 2 pi^2/3 = 6.5797   a1, a2, a3 = [-4.0000, 1.0000, -0.4444]
```

### Example 3

A period other than `2π`. On `[−1, 1]` (`L = 1`) the function `sin(πx)` has a single non-zero coefficient, `b₁ = 1`.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const double pi = std::acos(-1.0);

  const math::Function f = math::function({"x"}, [&](math::Function x) { return (pi * x).sin(); });
  const auto v = fourier::coefficients("x", 1.0, 4)(f)().items<double>();
  out.writeln("b1 =", v[4], "   largest of the other six |coefficients| =", std::max({std::abs(v[0]), std::abs(v[1]), std::abs(v[2]), std::abs(v[3]), std::abs(v[5]), std::abs(v[6])}));
}
```

Output

```text
b1 = 1.0000    largest of the other six |coefficients| = 0.0000
```

### Example 4

A half-period must be positive.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace fourier = pluss::operators::fourier;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  const math::Function f = math::function({"x"}, [](math::Function x) { return x; });
  try { (void)fourier::coefficients("x", 0.0, 3); } catch (const pluss::Error &e) { out.writeln(first_line(e)); }
}
```

Output

```text
operators::fourier::coefficients: half_period must be positive
```

## See Also

- [`series`](series.md): the reconstruction
- [`cosine_coefficients`](cosine_coefficients.md): half-range cosine coefficients

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="cosine_coefficients.html">
    <span class="label">Next</span>
    <span class="title">cosine_coefficients</span>
  </a>
</div>
<!-- apiref:nav:end -->
