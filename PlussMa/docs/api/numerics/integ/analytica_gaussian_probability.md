<!-- generated from analytica_gaussian_probability.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::analytica::gaussian_probability`

The probability a normal variable falls between `a` and `b`.

## Signature

```cpp
double integ::analytica::gaussian_probability(double a, double b, double mean = 0.0, double stddev = 1.0);
```

## What It Does

The integral of the **normalised** Gaussian density, `Φ((b − mean)/stddev) − Φ((a − mean)/stddev)`: a probability in `[0, 1]`, equal to [`gaussian`](analytica_gaussian.md) divided by `σ√(2π)`.

## Parameters

`double a, double b`
: The limits.

`double mean`
: The mean.

`double stddev`
: The standard deviation; positive.

## Returns

The exact value of the integral, a `double`.

## Errors

- `stddev ≤ 0` throws.

## Practical Examples

### Example 1

The familiar probabilities. About 68% within one standard deviation, 95% within 1.96.

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
  out.writeln("within 1 sd    :", analytica::gaussian_probability(-1.0, 1.0));
  out.writeln("within 1.96 sd :", analytica::gaussian_probability(-1.96, 1.96));
}
```

Output

```text
within 1 sd    : 0.682689
within 1.96 sd : 0.950004
```

### Example 2

A general normal. For a mean of 100 and a standard deviation of 15, the probability of an IQ between 85 and 115 is again 68.3%.

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
  out.writeln(analytica::gaussian_probability(85.0, 115.0, 100.0, 15.0));
}
```

Output

```text
0.682689
```

### Example 3

The tails. The mass above 3 standard deviations, from 3 to a point far out.

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
  out.writeln(analytica::gaussian_probability(3.0, 40.0));
}
```

Output

```text
0.00134990
```

## See Also

- [`quadra`](quadra.md): numerical quadrature of any integrand

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="analytica_gaussian_moment.html">
    <span class="label">Previous</span>
    <span class="title">gaussian_moment</span>
  </a>
  <a class="next-link" href="analytica_hyperbolic_cosine.html">
    <span class="label">Next</span>
    <span class="title">hyperbolic_cosine</span>
  </a>
</div>
<!-- apiref:nav:end -->
