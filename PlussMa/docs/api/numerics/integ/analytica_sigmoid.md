<!-- generated from analytica_sigmoid.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::analytica::sigmoid`

The integral of the logistic sigmoid `1 / (1 + e^{−x})` from `a` to `b`.

## Signature

```cpp
double integ::analytica::sigmoid(double a, double b);
```

## What It Does

The antiderivative of the sigmoid is the softplus `ln(1 + eˣ)`, so the integral is `softplus(b) − softplus(a)`, computed in a form that does not overflow for large `|x|`.

## Parameters

`double a, double b`
: The limits.

## Returns

The exact value of the integral, a `double`.

## Practical Examples

### Example 1

From zero. `∫₀¹ 1/(1 + e^{−x}) dx = ln(1 + e) − ln 2 = 0.62011451`.

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
  out.writeln(analytica::sigmoid(0.0, 1.0));
}
```

Output

```text
0.62011451
```

### Example 2

Large limits do not overflow. For `b` very large the integral grows like `b`.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  out.writeln(analytica::sigmoid(0.0, 800.0), ", 800 - ln 2 =", 800.0 - std::log(2.0));
}
```

Output

```text
799.306853 , 800 - ln 2 = 799.306853
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

  out.writeln("closed form :", analytica::sigmoid(-2.0, 3.0));
  out.writeln("quadrature  :", numeric([](const Series &t) { return (t * -1.0).exp().add(1.0).reciprocal(); }, -2.0, 3.0));
}
```

Output

```text
closed form : 2.9216593405
quadrature  : 2.9216593405
```

## See Also

- [`quadra`](quadra.md): numerical quadrature of any integrand

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="analytica_power.html">
    <span class="label">Previous</span>
    <span class="title">power</span>
  </a>
  <a class="next-link" href="analytica_sine.html">
    <span class="label">Next</span>
    <span class="title">sine</span>
  </a>
</div>
<!-- apiref:nav:end -->
