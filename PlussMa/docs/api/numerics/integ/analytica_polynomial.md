<!-- generated from analytica_polynomial.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::analytica::polynomial`

The integral of a polynomial `c₀ + c₁x + c₂x² + …` from `a` to `b`.

## Signature

```cpp
double integ::analytica::polynomial(const std::vector<double> &coeffs, double a, double b);
```

## What It Does

`coeffs = [c₀, c₁, …, cₙ]` in increasing powers. Each term is integrated exactly by [`power`](analytica_power.md); the result is exact for any degree.

## Parameters

`const std::vector<double> &coeffs`
: Coefficients in increasing powers of `x`.

`double a, double b`
: The limits.

## Returns

The exact value of the integral, a `double`.

## Practical Examples

### Example 1

A quadratic. `∫₀¹ (1 + 2x + 3x²) dx = 1 + 1 + 1 = 3`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
namespace analytica = integ::analytica;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  out.writeln(analytica::polynomial({1.0, 2.0, 3.0}, 0.0, 1.0));
}
```

Output

```text
3.00000000
```

### Example 2

A cubic on a symmetric interval. Odd powers cancel: `∫₋₂² (x³ + x² + 1) dx = 16/3 + 4`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
namespace analytica = integ::analytica;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  out.writeln(analytica::polynomial({1.0, 0.0, 1.0, 1.0}, -2.0, 2.0), ", expected =", 16.0 / 3.0 + 4.0);
}
```

Output

```text
9.33333333 , expected = 9.33333333
```

### Example 3

An empty coefficient list is the zero polynomial.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
namespace analytica = integ::analytica;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln(analytica::polynomial({}, 0.0, 5.0));
}
```

Output

```text
0.0000
```

## See Also

- [`quadra`](quadra.md): numerical quadrature of any integrand

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="analytica_lorentzian.html">
    <span class="label">Previous</span>
    <span class="title">lorentzian</span>
  </a>
  <a class="next-link" href="analytica_power.html">
    <span class="label">Next</span>
    <span class="title">power</span>
  </a>
</div>
<!-- apiref:nav:end -->
