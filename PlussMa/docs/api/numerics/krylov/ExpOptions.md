<!-- generated from ExpOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `krylov::ExpOptions`

Krylov dimension and convergence tolerances of `exp_action`.

`max_iterations` is the Krylov dimension used (capped by the size of one state); it is always used in full. The tolerances do not shorten the work: they decide whether each lane's error estimate counts as `converged`, namely when it is within `absolute_tolerance + relative_tolerance·‖result‖`. A large `‖tA‖` needs a larger dimension.

## Fields

### `max_iterations`

`std::int64_t`, default `32`.

The Krylov dimension, 32 by default, capped by the size of one state. It is always used in full: there is no early exit. The error of the approximation falls very fast once the dimension exceeds roughly `‖tA‖` (the spectral width of the operator times the time), so the right value grows with `t` and with the operator's spread; raise it when `converged` is false, or take the action in several shorter time steps instead.


### `relative_tolerance`

`double`, default `1e-9`.

The relative part of the verdict threshold `absolute_tolerance + relative_tolerance·‖result‖`, `1e-9` by default. It does not change how much work is done, only whether the error estimate counts as small enough to set `converged`.


### `absolute_tolerance`

`double`, default `1e-12`.

The absolute part of the verdict threshold, `1e-12` by default. It matters when the result is tiny (a strongly decaying evolution), where a relative test is hard to meet, and it is the whole threshold when `relative_tolerance` is 0.

## Guidance

Passed to `exp_action`.

## Practical Examples

### Example 1

The dimension against the error estimate. The matrix-free Laplacian of size 50 over `t = 2`: the estimate falls steeply as the dimension grows, and the lane is flagged converged once it is within tolerance.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const std::int64_t n = 50;
  const auto apply = [n](const Series &x) {
    const Series zero = pluss::make_series({0.0});
    return zero.unite({x.slice(0, 0, n - 1, 1)}, 0) + x.slice(0, 1, n, 1).unite({zero}, 0) - x * 2.0;
  };
  const Series v = pluss::series::zeros({n}, pluss::ScalarType::Float64) + (pluss::series::arange(0.0, static_cast<double>(n), 1.0, pluss::ScalarType::Float64) == 25.0).to(pluss::ScalarType::Float64);
  for (const std::int64_t dimension : {4, 8, 16, 24}) {
    krylov::ExpOptions options;
    options.max_iterations = dimension;
    const krylov::ExpResult r = krylov::exp_action(apply, v, {-1}, 2.0, options);
    out.writeln("dimension", dimension, ": estimate =", r.residual_estimate.reshape({1}).items<double>(), ", converged =", r.converged.all().item<bool>());
  }
}
```

Output

```text
dimension 4 : estimate = [0.073367582040] , converged = false
dimension 8 : estimate = [0.001016395806] , converged = false
dimension 16 : estimate = [0.000000000820] , converged = false
dimension 24 : estimate = [0.000000000000] , converged = true
```

### Example 2

The tolerance does not change the work, only the verdict. The same dimension of 8 is used; a tighter tolerance flips the flag.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const std::int64_t n = 50;
  const auto apply = [n](const Series &x) {
    const Series zero = pluss::make_series({0.0});
    return zero.unite({x.slice(0, 0, n - 1, 1)}, 0) + x.slice(0, 1, n, 1).unite({zero}, 0) - x * 2.0;
  };
  const Series v = pluss::series::zeros({n}, pluss::ScalarType::Float64) + (pluss::series::arange(0.0, static_cast<double>(n), 1.0, pluss::ScalarType::Float64) == 25.0).to(pluss::ScalarType::Float64);
  for (const double tol : {1e-2, 1e-6}) {
    krylov::ExpOptions options;
    options.max_iterations = 8;
    options.relative_tolerance = tol;
    options.absolute_tolerance = 0.0;
    const krylov::ExpResult r = krylov::exp_action(apply, v, {-1}, 2.0, options);
    out.writeln("tolerance", tol, ": iterations =", r.iterations, ", converged =", r.converged.all().item<bool>());
  }
}
```

Output

```text
tolerance 0.01000000 : iterations = 8 , converged = true
tolerance 0.00000100 : iterations = 8 , converged = false
```

### Example 3

A longer time needs a larger dimension. At a fixed dimension of 6, the estimate grows with `t`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const std::int64_t n = 50;
  const auto apply = [n](const Series &x) {
    const Series zero = pluss::make_series({0.0});
    return zero.unite({x.slice(0, 0, n - 1, 1)}, 0) + x.slice(0, 1, n, 1).unite({zero}, 0) - x * 2.0;
  };
  const Series v = pluss::series::zeros({n}, pluss::ScalarType::Float64) + (pluss::series::arange(0.0, static_cast<double>(n), 1.0, pluss::ScalarType::Float64) == 25.0).to(pluss::ScalarType::Float64);
  krylov::ExpOptions options;
  options.max_iterations = 6;
  for (const double t : {0.5, 2.0, 8.0})
    out.writeln("t =", t, ": estimate =", krylov::exp_action(apply, v, {-1}, t, options).residual_estimate.reshape({1}).items<double>());
}
```

Output

```text
t = 0.5000000000 : estimate = [0.0001403994]
t = 2.0000000000 : estimate = [0.0120029460]
t = 8.0000000000 : estimate = [0.0338310049]
```

### Example 4

The absolute tolerance on its own. With the relative tolerance switched off, the verdict compares the error estimate with the absolute number alone. The dimension is the same in both runs; only the flag changes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const std::int64_t n = 50;
  const auto apply = [n](const Series &x) {
    const Series zero = pluss::make_series({0.0});
    return zero.unite({x.slice(0, 0, n - 1, 1)}, 0) + x.slice(0, 1, n, 1).unite({zero}, 0) - x * 2.0;
  };
  const Series v = pluss::series::zeros({n}, pluss::ScalarType::Float64) + (pluss::series::arange(0.0, static_cast<double>(n), 1.0, pluss::ScalarType::Float64) == 25.0).to(pluss::ScalarType::Float64);

  for (const double tol : {1e-1, 1e-6}) {
    krylov::ExpOptions options;
    options.max_iterations = 8;
    options.relative_tolerance = 0.0;
    options.absolute_tolerance = tol;
    const krylov::ExpResult r = krylov::exp_action(apply, v, {-1}, 2.0, options);
    out.writeln("absolute tolerance", tol, ": estimate =", r.residual_estimate.reshape({1}).items<double>(), ", converged =", r.converged.all().item<bool>());
  }
}
```

Output

```text
absolute tolerance 0.10000000 : estimate = [0.00101640] , converged = true
absolute tolerance 0.00000100 : estimate = [0.00101640] , converged = false
```

## See Also

- [`exp_action`](exp_action.md): uses it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="CgResult.html">
    <span class="label">Previous</span>
    <span class="title">CgResult</span>
  </a>
  <a class="next-link" href="ExpResult.html">
    <span class="label">Next</span>
    <span class="title">ExpResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
