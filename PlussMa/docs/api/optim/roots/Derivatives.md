<!-- generated from Derivatives.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `roots::Derivatives`

The value of a function and its first three derivatives at the same points: what the `householder` callback returns.

`Derivatives` bundles four `Series` of the same shape as the iterates: `value` is `f(x)`, and `first`, `second`, `third` are `f′(x)`, `f″(x)` and `f‴(x)`. They are returned together so that the callback can share intermediate quantities, for example computing `exp(x)` once for a function whose derivatives all contain it.

## Fields

### `value`

`pluss::Series`, default `undefined`.

`f(x)`.

### `first`

`pluss::Series`, default `undefined`.

`f′(x)`.

### `second`

`pluss::Series`, default `undefined`.

`f″(x)`.

### `third`

`pluss::Series`, default `undefined`.

`f‴(x)`.

## Guidance

All four members must have the same shape as the iterates the callback receives.

## Practical Examples

### Example 1

Building one by hand for `f(x) = x³`: `f = x³`, `f′ = 3x²`, `f″ = 6x`, `f‴ = 6`, at three points.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Series x = pluss::make_series({1.0, 2.0, 3.0});
  const roots::Derivatives d{x * x * x, x * x * 3.0, x * 6.0, pluss::series::full_like(x, 6.0)};

  out.writeln("value  =", d.value.items<double>());
  out.writeln("first  =", d.first.items<double>());
  out.writeln("second =", d.second.items<double>());
  out.writeln("third  =", d.third.items<double>());
}
```

Output

```text
value  = [1.0000, 8.0000, 27.0000]
first  = [3.0000, 12.0000, 27.0000]
second = [6.0000, 12.0000, 18.0000]
third  = [6.0000, 6.0000, 6.0000]
```

### Example 2

Sharing work between the four members. For `f(x) = eˣ − 3` all four share one evaluation of `exp`; the root is `ln 3 = 1.0986…`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const roots::DerivativeFn evaluate = [](const Series &x) {
    const Series e = x.exp();
    return roots::Derivatives{e - 3.0, e, e, e};
  };
  roots::Options options;
  options.x_tolerance = 1e-13;
  options.check_every = 1;
  out.writeln("root =", roots::householder(evaluate, pluss::make_series(0.0), options).root.item<double>(), "  ln 3 =", std::log(3.0));
}
```

Output

```text
root = 1.098612288668   ln 3 = 1.098612288668
```

### Example 3

Members that are not needed can be zero. For a linear function only `value` and `first` matter; `second` and `third` are zero and the step reduces to Newton's.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const roots::DerivativeFn evaluate = [](const Series &x) {
    return roots::Derivatives{x * 4.0 - 10.0, pluss::series::full_like(x, 4.0), pluss::series::zeros_like(x), pluss::series::zeros_like(x)};
  };
  roots::Options options;
  options.x_tolerance = 1e-12;
  const roots::Result result = roots::householder(evaluate, pluss::make_series(0.0), options);
  out.writeln("root =", result.root.item<double>(), ", iterations =", result.iterations, "  (10 / 4 = 2.5)");
}
```

Output

```text
root = 2.500000 , iterations = 8   (10 / 4 = 2.5)
```

## See Also

- [`householder`](householder.md): the solver that consumes it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Options.html">
    <span class="label">Next</span>
    <span class="title">Options</span>
  </a>
</div>
<!-- apiref:nav:end -->
