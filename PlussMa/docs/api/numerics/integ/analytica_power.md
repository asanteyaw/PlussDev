<!-- generated from analytica_power.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::analytica::power`

The integral of `x^n` from `a` to `b`.

## Signature

```cpp
double integ::analytica::power(double n, double a, double b);
```

## What It Does

`∫ₐᵇ xⁿ dx = (b^{n+1} − a^{n+1})/(n + 1)`, and `ln|b| − ln|a|` for `n = −1`, which needs `a` and `b` nonzero and on the same side of zero (an interval across 0 has no finite integral). Mind the domain of non-integer powers: `a` and `b` must be nonnegative for those.

## Parameters

`double n`
: The exponent.

`double a, double b`
: The limits.

## Returns

The exact value of the integral, a `double`.

## Errors

- `n = −1` with `a` or `b` zero, or on opposite sides of zero, throws.

## Practical Examples

### Example 1

Powers. `∫₀³ x² dx = 9` and `∫₁⁴ x^{1/2} dx = 14/3`.

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
  out.writeln("x^2     on [0, 3] :", analytica::power(2.0, 0.0, 3.0));
  out.writeln("x^0.5   on [1, 4] :", analytica::power(0.5, 1.0, 4.0));
}
```

Output

```text
x^2     on [0, 3] : 9.00000000
x^0.5   on [1, 4] : 4.66666667
```

### Example 2

The logarithmic case. `n = −1` gives `ln(b/a)`: `∫₁^e dx/x = 1`.

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
  out.writeln(analytica::power(-1.0, 1.0, std::exp(1.0)));
}
```

Output

```text
1.00000000
```

### Example 3

An interval across zero for `n = −1` has no finite integral, and the call says so.

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
    (void)analytica::power(-1.0, -1.0, 1.0);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::integ::analytica::power: n == -1 requires a and b nonzero and on the same side of 0
```

## See Also

- [`quadra`](quadra.md): numerical quadrature of any integrand

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="analytica_polynomial.html">
    <span class="label">Previous</span>
    <span class="title">polynomial</span>
  </a>
  <a class="next-link" href="analytica_sigmoid.html">
    <span class="label">Next</span>
    <span class="title">sigmoid</span>
  </a>
</div>
<!-- apiref:nav:end -->
