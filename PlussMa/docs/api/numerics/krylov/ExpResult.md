<!-- generated from ExpResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `krylov::ExpResult`

The result of `exp_action` with its residual estimate and the work done.

`value` is `exp(tA) v`, shaped like the input; `residual_estimate` and `converged` are per lane (the estimate is the standard a-posteriori Krylov bound from the final Arnoldi subdiagonal); `iterations` is the Krylov dimension used and `operator_evaluations` the number of applications of `A`.

## Fields

### `value`

`pluss::Series`, default `undefined`.

The vector `exp(tA) v`, shaped like the input.


### `residual_estimate`

`pluss::Series`, default `undefined`.

The a-posteriori error estimate per lane: the Arnoldi subdiagonal entry that the projection ignores, times the size of the last component of the small solution. It is a reliable guide to the error, not a strict bound.


### `converged`

`pluss::Series`, default `undefined`.

Boolean per lane: the estimate is within `absolute_tolerance + relative_tolerance·‖value‖`.


### `iterations`

`std::int64_t`, default `0`.

The Krylov dimension used, `min(max_iterations, n)`; there is no early exit.


### `operator_evaluations`

`std::int64_t`, default `0`.

The applications of `A`, equal to the dimension.

## Guidance

Returned by `exp_action`.

## Practical Examples

### Example 1

Reading a result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const Series d = pluss::make_series({-1.0, -2.0});
  const auto apply = [&](const Series &x) { return x * d; };
  const krylov::ExpResult r = krylov::exp_action(apply, pluss::make_series({1.0, 1.0}), {-1}, 1.0);
  out.writeln("value =", r.value.items<double>(), ", estimate =", r.residual_estimate.items<double>());
  out.writeln("iterations =", r.iterations, ", operator evaluations =", r.operator_evaluations, ", converged =", r.converged.reshape({r.converged.numel()}).items<bool>());
}
```

Output

```text
value = [0.3678794412, 0.1353352832] , estimate = [0.0000000000]
iterations = 2 , operator evaluations = 2 , converged = [true]
```

### Example 2

A batch. Each row is its own vector, with its own estimate.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const Series d = pluss::make_series({-1.0, -2.0});
  const auto apply = [&](const Series &x) { return x * d; };
  const krylov::ExpResult r = krylov::exp_action(apply, pluss::make_series({1.0, 1.0, 2.0, 0.0}, {2, 2}), {-1}, 1.0);
  out.writeln("shape =", r.value.sizes(), ", values =", r.value.reshape({4}).items<double>());
}
```

Output

```text
shape = [2, 2] , values = [0.36787944, 0.13533528, 0.73575888, 0.00000000]
```

### Example 3

A zero operator leaves the vector unchanged.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto apply = [](const Series &x) { return x * 0.0; };
  const krylov::ExpResult r = krylov::exp_action(apply, pluss::make_series({3.0, -1.0}), {-1}, 5.0);
  out.writeln("value =", r.value.items<double>());
}
```

Output

```text
value = [3.000000, -1.000000]
```

## See Also

- [`exp_action`](exp_action.md): produces it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ExpOptions.html">
    <span class="label">Previous</span>
    <span class="title">ExpOptions</span>
  </a>
  <a class="next-link" href="GmresOptions.html">
    <span class="label">Next</span>
    <span class="title">GmresOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
