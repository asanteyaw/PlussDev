<!-- generated from analytica_gaussian_moment.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::analytica::gaussian_moment`

The integral of `(x − mean)` times the unnormalised Gaussian, from `a` to `b`.

## Signature

```cpp
double integ::analytica::gaussian_moment(double a, double b, double mean = 0.0, double stddev = 1.0);
```

## What It Does

`σ² (exp(−(a−mean)²/(2σ²)) − exp(−(b−mean)²/(2σ²)))`: the first-moment companion of [`gaussian`](analytica_gaussian.md), useful for the mean of a truncated normal variable. It is zero on a symmetric interval about `mean`.

## Parameters

`double a, double b`
: The limits.

`double mean`
: The centre.

`double stddev`
: The width; positive.

## Returns

The exact value of the integral, a `double`.

## Errors

- `stddev ≤ 0` throws.

## Practical Examples

### Example 1

One side of the bell. `∫₀^∞ x e^{−x²/2} dx = 1`.

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
  out.writeln(analytica::gaussian_moment(0.0, 40.0));
}
```

Output

```text
1.00000000
```

### Example 2

A symmetric interval is zero: the two sides cancel.

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
  out.writeln(analytica::gaussian_moment(-2.0, 2.0));
}
```

Output

```text
0.00000000
```

### Example 3

The mean of a truncated normal. Dividing the moment by the mass gives `E[X | 0 < X < 1] = 0.45986`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
namespace analytica = integ::analytica;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  out.writeln(analytica::gaussian_moment(0.0, 1.0) / analytica::gaussian(0.0, 1.0));
}
```

Output

```text
0.459862
```

## See Also

- [`quadra`](quadra.md): numerical quadrature of any integrand

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="analytica_gaussian.html">
    <span class="label">Previous</span>
    <span class="title">gaussian</span>
  </a>
  <a class="next-link" href="analytica_gaussian_probability.html">
    <span class="label">Next</span>
    <span class="title">gaussian_probability</span>
  </a>
</div>
<!-- apiref:nav:end -->
