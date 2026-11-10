<!-- generated from analytica_hyperbolic_sine.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::analytica::hyperbolic_sine`

The integral of `sinh(k x)` from `a` to `b`.

## Signature

```cpp
double integ::analytica::hyperbolic_sine(double k, double a, double b);
```

## What It Does

`(cosh(kb) − cosh(ka))/k`, and 0 when `k = 0`.

## Parameters

`double k`
: The rate.

`double a, double b`
: The limits.

## Returns

The exact value of the integral, a `double`.

## Practical Examples

### Example 1

From zero. `∫₀¹ sinh x dx = cosh 1 − 1 = 0.54308063`.

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
  out.writeln(analytica::hyperbolic_sine(1.0, 0.0, 1.0));
}
```

Output

```text
0.54308063
```

### Example 2

An odd function over a symmetric interval integrates to zero.

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
  out.writeln(analytica::hyperbolic_sine(2.0, -3.0, 3.0));
}
```

Output

```text
0.00000000
```

### Example 3

Checked against quadrature.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
namespace analytica = integ::analytica;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const auto numeric = [&](const std::function<Series(const Series &)> &f, double a, double b) { return integ::quadra(f, a, b).value.item<double>(); };

  out.writeln("closed form :", analytica::hyperbolic_sine(1.5, 0.0, 2.0));
  out.writeln("quadrature  :", numeric([](const Series &t) { return (t * 1.5).sinh(); }, 0.0, 2.0));
}
```

Output

```text
closed form : 6.0451079972
quadrature  : 6.0451079972
```

## See Also

- [`quadra`](quadra.md): numerical quadrature of any integrand

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="analytica_hyperbolic_cosine.html">
    <span class="label">Previous</span>
    <span class="title">hyperbolic_cosine</span>
  </a>
  <a class="next-link" href="analytica_hyperbolic_tangent.html">
    <span class="label">Next</span>
    <span class="title">hyperbolic_tangent</span>
  </a>
</div>
<!-- apiref:nav:end -->
