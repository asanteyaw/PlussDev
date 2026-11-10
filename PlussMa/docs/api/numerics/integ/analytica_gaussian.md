<!-- generated from analytica_gaussian.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::analytica::gaussian`

The integral of the unnormalised Gaussian `exp(−(x − mean)² / (2 stddev²))`.

## Signature

```cpp
double integ::analytica::gaussian(double a, double b, double mean = 0.0, double stddev = 1.0);
```

## What It Does

Through the error function: `σ √(2π) · ½ (erf(z_b) − erf(z_a))` with `z = (x − mean)/(σ√2)`. There is **no** `1/(σ√(2π))` factor, so over the whole line it gives `σ√(2π)`; for a probability use [`gaussian_probability`](analytica_gaussian_probability.md).

## Parameters

`double a, double b`
: The limits.

`double mean`
: The centre.

`double stddev`
: The width; must be positive.

## Returns

The exact value of the integral, a `double`.

## Errors

- `stddev ≤ 0` throws.

## Practical Examples

### Example 1

Across the bell. The unnormalised kernel over `[−1, 1]` is `1.7112`; over a wide interval it is `√(2π) = 2.5066`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
namespace analytica = integ::analytica;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  out.writeln("[-1, 1]   :", analytica::gaussian(-1.0, 1.0));
  out.writeln("[-10, 10] :", analytica::gaussian(-10.0, 10.0), ", sqrt(2 pi) =", std::sqrt(2.0 * 3.14159265358979323846));
}
```

Output

```text
[-1, 1]   : 1.71124878
[-10, 10] : 2.50662827 , sqrt(2 pi) = 2.50662827
```

### Example 2

A shifted, wider kernel. With `mean = 2` and `stddev = 3` the whole-line value is `3√(2π) = 7.5198`.

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
  out.writeln(analytica::gaussian(-60.0, 60.0, 2.0, 3.0));
}
```

Output

```text
7.51988482
```

### Example 3

A non-positive width is rejected.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
namespace analytica = integ::analytica;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  try {
    (void)analytica::gaussian(0.0, 1.0, 0.0, 0.0);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::integ::analytica::gaussian: stddev must be positive
```

## See Also

- [`quadra`](quadra.md): numerical quadrature of any integrand

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="analytica_exponential.html">
    <span class="label">Previous</span>
    <span class="title">exponential</span>
  </a>
  <a class="next-link" href="analytica_gaussian_moment.html">
    <span class="label">Next</span>
    <span class="title">gaussian_moment</span>
  </a>
</div>
<!-- apiref:nav:end -->
