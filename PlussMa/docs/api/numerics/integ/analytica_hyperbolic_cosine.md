<!-- generated from analytica_hyperbolic_cosine.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::analytica::hyperbolic_cosine`

The integral of `cosh(k x)` from `a` to `b`.

## Signature

```cpp
double integ::analytica::hyperbolic_cosine(double k, double a, double b);
```

## What It Does

`(sinh(kb) − sinh(ka))/k`, and `b − a` when `k = 0`.

## Parameters

`double k`
: The rate.

`double a, double b`
: The limits.

## Returns

The exact value of the integral, a `double`.

## Practical Examples

### Example 1

From zero. `∫₀¹ cosh x dx = sinh 1 = 1.17520119`.

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
  out.writeln(analytica::hyperbolic_cosine(1.0, 0.0, 1.0));
}
```

Output

```text
1.17520119
```

### Example 2

A symmetric interval doubles the one-sided value.

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
  out.writeln(analytica::hyperbolic_cosine(1.0, -1.0, 1.0), ", twice the one-sided =", 2.0 * analytica::hyperbolic_cosine(1.0, 0.0, 1.0));
}
```

Output

```text
2.35040239 , twice the one-sided = 2.35040239
```

### Example 3

The zero rate is the constant 1.

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
  out.writeln(analytica::hyperbolic_cosine(0.0, 0.0, 3.0));
}
```

Output

```text
3.0000
```

## See Also

- [`quadra`](quadra.md): numerical quadrature of any integrand

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="analytica_gaussian_probability.html">
    <span class="label">Previous</span>
    <span class="title">gaussian_probability</span>
  </a>
  <a class="next-link" href="analytica_hyperbolic_sine.html">
    <span class="label">Next</span>
    <span class="title">hyperbolic_sine</span>
  </a>
</div>
<!-- apiref:nav:end -->
