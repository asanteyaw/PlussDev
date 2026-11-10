<!-- generated from series.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::fourier::series`

The truncated full Fourier series of a function on a symmetric period `[−L, L]`, as a function of the original variable.

## Signature

```cpp
operators::Operator operators::fourier::series(std::string_view variable, double half_period, std::int64_t terms);
```

## What It Does

`series("x", L, n)` applied to `f` returns `x ↦ a₀/2 + Σₖ₌₁^{n−1} (aₖ cos(kπx/L) + bₖ sin(kπx/L))`, with the coefficients of [`coefficients`](coefficients.md). It is an ordinary function: evaluate it, differentiate it, integrate it. It approximates `f` on `[−L, L]` and repeats with period `2L` outside.

At a point where `f` is smooth the series converges to `f`; at a jump (including the junction of the periodic extension, where `f(L) ≠ f(−L)`) it converges to the average of the two sides, with Gibbs overshoot nearby.

## Parameters

`std::string_view variable`
: The variable to expand in.

`double half_period`
: `L`, half the period. Positive.

`std::int64_t terms`
: The number of cosine terms (and one fewer sine terms); between 1 and 4096.

## Returns

An `Operator` whose application returns the series as a `Function`.

## Errors

- `half_period must be positive`.
- `terms must lie in [1, 4096]`.

## Practical Examples

### Example 1

The series of `f(x) = x` on `[−π, π]` converges slowly because the periodic extension has a jump at `±π`. With 3, 10 and 40 terms the error at `x = 1` is large and changes sign from one truncation to the next, decaying only like `1/n`.

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

  for (const std::int64_t terms : {3, 10, 40}) {
    const math::Function approx = fourier::series("x", pi, terms)(f);
    out.writeln("terms", terms, ": series(1) =", approx(1.0).item<double>(), "  error", approx(1.0).item<double>() - 1.0);
  }
}
```

Output

```text
terms 3 : series(1) = 0.77364   error -0.22636
terms 10 : series(1) = 0.98765   error -0.01235
terms 40 : series(1) = 1.02804   error 0.02804
```

### Example 2

The periodic extension. The value at `x = 4`, outside `[−π, π]`, is the value at `4 − 2π ≈ −2.283`; and at the jump `x = π` the series returns the average of the two sides, 0.

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return x; });

  const math::Function approx = fourier::series("x", pi, 60)(f);
  out.writeln("series(4)  =", approx(4.0).item<double>(), "   4 - 2 pi =", 4.0 - 2.0 * pi);
  out.writeln("series(pi) =", approx(pi).item<double>(), "   (average of the left limit pi and the right limit -pi)");
}
```

Output

```text
series(4)  = -2.2548    4 - 2 pi = -2.2832
series(pi) = 0.0000    (average of the left limit pi and the right limit -pi)
```

### Example 3

A smooth periodic function is represented almost exactly. The 8-term series of `exp(cos x)` on `[−π, π]` matches it to about six digits.

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
  out.print_defaults(PrintOptions{}.fixed(9));
  const double pi = std::acos(-1.0);

  const math::Function f = math::function({"x"}, [](math::Function x) { return x.cos().exp(); });
  const math::Function approx = fourier::series("x", pi, 8)(f);
  const auto x = pluss::series::linspace(-pi, pi, 7, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  out.writeln("max |series - f| on a grid:", (approx({x}) - f({x})).abs().max().item<double>());
}
```

Output

```text
max |series - f| on a grid: 0.000000211
```

## See Also

- [`coefficients`](coefficients.md): the coefficients
- [`cosine_series`](cosine_series.md): half-range cosine series

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="inverse_transform.html">
    <span class="label">Previous</span>
    <span class="title">inverse_transform</span>
  </a>
  <a class="next-link" href="sine_coefficients.html">
    <span class="label">Next</span>
    <span class="title">sine_coefficients</span>
  </a>
</div>
<!-- apiref:nav:end -->
