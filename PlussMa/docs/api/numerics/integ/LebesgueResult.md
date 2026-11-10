<!-- generated from LebesgueResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::LebesgueResult`

A Lebesgue integral with the bracket that encloses it.

`value` is the estimate, and `lower` and `upper` are the integrals of the lower and upper simple functions at the last refinement: the exact value lies between them. `error_estimate` is half the bracket plus the change from the previous refinement. `history` lists the value after each refinement for the first batch element, and `monotone` says whether that sequence was non-decreasing, the signature of an increasing sequence of simple approximations; it is a diagnostic of the numerical sequence, not a proof. `converged` says whether the estimate fell within the tolerance.

## Fields

### `value`

`pluss::Series`, default `undefined`.

The integral.

### `lower`

`pluss::Series`, default `undefined`.

Integral of the lower simple function.

### `upper`

`pluss::Series`, default `undefined`.

Integral of the upper simple function.

### `error_estimate`

`double`, default `+inf`.

Half the bracket plus the refinement change.

### `evaluations`

`std::int64_t`, default `0`.

Integrand evaluations.

### `refinements`

`std::int64_t`, default `0`.

Doublings made.

### `converged`

`bool`, default `false`.

Whether the estimate met the tolerance.

### `history`

`std::vector<double>`, default `empty`.

Value after each refinement (first batch element).

### `monotone`

`bool`, default `true`.

Whether the history was non-decreasing.

## Guidance

Returned by `lebesgue`.

## Practical Examples

### Example 1

The bracket.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(9));
  const integ::LebesgueResult r = integ::lebesgue([](const Series &t) { return t.cos(); }, 0.0, 1.0);
  out.writeln("lower =", r.lower.item<double>(), ", upper =", r.upper.item<double>(), ", exact =", std::sin(1.0));
}
```

Output

```text
lower = 0.841470986 , upper = 0.841470988 , exact = 0.841470985
```

### Example 2

The refinement history. The value after each doubling approaches the integral.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(9));
  integ::LebesgueOptions options;
  options.cells = 8;
  options.max_refinements = 5;
  options.tolerance = 1e-12;
  const integ::LebesgueResult r = integ::lebesgue([](const Series &t) { return t.square(); }, 0.0, 1.0, options);
  for (std::size_t i = 0; i < r.history.size(); ++i) out.writeln("level", i, ":", r.history[i]);
  out.writeln("monotone =", r.monotone);
}
```

Output

```text
level 0 : 0.332031250
level 1 : 0.333007812
level 2 : 0.333251953
level 3 : 0.333312988
level 4 : 0.333328247
level 5 : 0.333332062
monotone = true
```

### Example 3

A batch. Two integrands at once give one value, bracket and estimate each.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const Series k = pluss::series::make({1.0, 2.0}).reshape({2, 1});
  const integ::LebesgueResult r = integ::lebesgue([&](const Series &t) { return t.reshape({1, t.numel()}).pow(k); }, 0.0, 1.0);
  out.writeln("values =", r.value.items<double>(), ", exact = 1/2, 1/3");
}
```

Output

```text
values = [0.50000000, 0.33333333] , exact = 1/2, 1/3
```

## See Also

- [`lebesgue`](lebesgue.md): produces it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="LebesgueOptions.html">
    <span class="label">Previous</span>
    <span class="title">LebesgueOptions</span>
  </a>
  <a class="next-link" href="MeasureResult.html">
    <span class="label">Next</span>
    <span class="title">MeasureResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
