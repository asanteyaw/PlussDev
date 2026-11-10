<!-- generated from analytica_exponential.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::analytica::exponential`

The integral of `exp(k x)` from `a` to `b`.

## Signature

```cpp
double integ::analytica::exponential(double k, double a, double b);
```

## What It Does

`∫ₐᵇ e^{kx} dx = (e^{kb} − e^{ka})/k`, and `b − a` when `k = 0` (the integrand is then 1). Exact, with no quadrature. The family is closed under growth and decay: a negative `k` is an exponential decay.

## Parameters

`double k`
: The rate.

`double a, double b`
: The limits; `b < a` gives the negative.

## Returns

The exact value of the integral, a `double`.

## Practical Examples

### Example 1

Growth and decay. `∫₀¹ e^{2x} dx = (e² − 1)/2 = 3.19452805`, and a decay with `k = −1` over `[0, 5]` is `1 − e^{−5} = 0.99326205`.

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
  out.writeln("k = 2  :", analytica::exponential(2.0, 0.0, 1.0));
  out.writeln("k = -1 :", analytica::exponential(-1.0, 0.0, 5.0));
}
```

Output

```text
k = 2  : 3.19452805
k = -1 : 0.99326205
```

### Example 2

The degenerate case. With `k = 0` the integrand is the constant 1, so the integral is the length.

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
  out.writeln(analytica::exponential(0.0, 2.0, 7.5));
}
```

Output

```text
5.5000
```

### Example 3

Checked against quadrature. The closed form and a numerical integral agree to the quadrature tolerance.

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

  out.writeln("closed form :", analytica::exponential(1.5, -1.0, 2.0));
  out.writeln("quadrature  :", numeric([](const Series &t) { return (t * 1.5).exp(); }, -1.0, 2.0));
}
```

Output

```text
closed form : 13.2416045087
quadrature  : 13.2416045087
```

## See Also

- [`quadra`](quadra.md): numerical quadrature of any integrand

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="analytica_cosine.html">
    <span class="label">Previous</span>
    <span class="title">cosine</span>
  </a>
  <a class="next-link" href="analytica_gaussian.html">
    <span class="label">Next</span>
    <span class="title">gaussian</span>
  </a>
</div>
<!-- apiref:nav:end -->
