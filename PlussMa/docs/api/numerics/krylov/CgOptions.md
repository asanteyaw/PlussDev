<!-- generated from CgOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `krylov::CgOptions`

Iteration limit, tolerances and convergence-check cadence of conjugate gradients.

A lane has converged when its residual norm is below `absolute_tolerance + relative_tolerance·‖b‖`. `max_iterations` caps the work. `curvature_tolerance` is the smallest curvature `pᵀAp` accepted before a lane is flagged as a breakdown (a direction along which `A` is not positive). `check_interval` is how many iterations pass between convergence checks: 0 keeps the whole solve asynchronous and runs the full budget, which avoids a host round trip per check on an accelerator at the cost of extra iterations.

## Fields

### `max_iterations`

`std::int64_t`, default `128`.

The most iterations, 128 by default. Conjugate gradients on an `n`-dimensional system finishes in at most `n` iterations in exact arithmetic, so the default is generous for small systems and a real limit for large ones; raise it for a long, badly conditioned operator and read `converged` on the result rather than assuming the budget was enough.


### `relative_tolerance`

`double`, default `1e-10`.

A lane has converged when its residual norm is below `absolute_tolerance + relative_tolerance·‖b‖`. This is the relative part, `1e-10` by default: the residual must shrink by that factor from the size of the right-hand side. Loosen it (`1e-4`) when an approximate answer will do, since the cost grows with the logarithm of the accuracy; set it to 0 to rely on the absolute tolerance alone.


### `absolute_tolerance`

`double`, default `1e-12`.

The absolute floor of the stopping threshold, `1e-12` by default. It matters when `b` is very small or zero, where a purely relative test would demand an impossible residual; and it is the whole test when `relative_tolerance` is 0. Raise it to the accuracy you actually need in the units of the residual.


### `curvature_tolerance`

`double`, default `0`.

The smallest curvature `pᵀAp` a search direction may have before the lane is flagged as a breakdown and frozen, default 0 (only non-positive curvature is a breakdown). Raising it makes the solver give up on directions along which the operator is nearly singular rather than divide by a tiny number; the lane then reports `breakdown` and is not converged.


### `check_interval`

`std::int64_t`, default `8`.

How many iterations pass between convergence checks, 8 by default. Each check reads the residual back to the host, which on an accelerator stalls the pipeline, so a larger interval is faster per iteration but can overshoot the converged iterate by up to `check_interval − 1` extra iterations. Use 1 when the exact iteration count matters, and 0 to skip checking entirely and always run `max_iterations`.

## Guidance

Passed to `cg` and `pcg`.

## Practical Examples

### Example 1

The tolerance. A looser tolerance stops sooner, on the 400-point Laplacian with an oscillating right-hand side.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const std::int64_t n = 400;
  const auto laplacian = [n](const Series &x) {
    const Series zero = pluss::make_series({0.0});
    return x * 2.0 - zero.unite({x.slice(0, 0, n - 1, 1)}, 0) - x.slice(0, 1, n, 1).unite({zero}, 0);
  };
  const Series b = pluss::series::arange(0.0, static_cast<double>(n), 1.0, pluss::ScalarType::Float64).sin();
  for (const double tol : {1e-1, 1e-2, 1e-3}) {
    krylov::CgOptions options;
    options.relative_tolerance = tol;
    options.check_interval = 1;
    options.max_iterations = 400;
    out.writeln("tolerance", tol, ": iterations =", krylov::cg(laplacian, b, {-1}, options).iterations);
  }
}
```

Output

```text
tolerance 0.1000000000 : iterations = 1
tolerance 0.0100000000 : iterations = 10
tolerance 0.0010000000 : iterations = 92
```

### Example 2

The iteration limit. A budget too small to converge leaves the lane unconverged, and the result says so.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const std::int64_t n = 50;
  const auto laplacian = [n](const Series &x) {
    const Series zero = pluss::make_series({0.0});
    return x * 2.0 - zero.unite({x.slice(0, 0, n - 1, 1)}, 0) - x.slice(0, 1, n, 1).unite({zero}, 0);
  };
  krylov::CgOptions options;
  options.max_iterations = 5;
  const krylov::CgResult r = krylov::cg(laplacian, pluss::series::ones({n}, pluss::ScalarType::Float64), {-1}, options);
  out.writeln("iterations =", r.iterations, ", converged =", r.converged.all().item<bool>(), ", relative residual =", r.relative_residual.reshape({r.relative_residual.numel()}).items<double>());
}
```

Output

```text
iterations = 5 , converged = false , relative residual = [4.098780]
```

### Example 3

The check interval. Checking every iteration stops as soon as the tolerance is met; a larger interval overshoots to the next check.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Series A = pluss::make_series({4.0, 1.0, 1.0, 3.0}, {2, 2});
  const auto apply = [&](const Series &x) { return A.matmul(x.unsqueeze(-1)).squeeze(-1); };
  for (const std::int64_t interval : {1, 8}) {
    krylov::CgOptions options;
    options.check_interval = interval;
    out.writeln("check every", interval, ": iterations =", krylov::cg(apply, pluss::make_series({1.0, 2.0}), {-1}, options).iterations);
  }
}
```

Output

```text
check every 1 : iterations = 2
check every 8 : iterations = 8
```

### Example 4

The absolute tolerance is the floor under the relative one. With the relative tolerance switched off, the threshold is the absolute number alone, whatever the size of `b`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const std::int64_t n = 400;
  const auto laplacian = [n](const Series &x) {
    const Series zero = pluss::make_series({0.0});
    return x * 2.0 - zero.unite({x.slice(0, 0, n - 1, 1)}, 0) - x.slice(0, 1, n, 1).unite({zero}, 0);
  };
  const Series b = pluss::series::arange(0.0, static_cast<double>(n), 1.0, pluss::ScalarType::Float64).sin();

  for (const double tol : {1e-1, 1e-3}) {
    krylov::CgOptions options;
    options.relative_tolerance = 0.0;
    options.absolute_tolerance = tol;
    options.check_interval = 1;
    options.max_iterations = 400;
    out.writeln("absolute tolerance", tol, ": iterations =", krylov::cg(laplacian, b, {-1}, options).iterations);
  }
}
```

Output

```text
absolute tolerance 0.100000 : iterations = 13
absolute tolerance 0.001000 : iterations = 208
```

### Example 5

The curvature tolerance. `A = diag(1, 1e-8)` is positive definite, but along the second axis the curvature `pᵀAp` is tiny. By default that is accepted; a curvature tolerance of `1e-6` calls it a breakdown.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const Series d = pluss::make_series({1.0, 1e-8});
  const auto apply = [&](const Series &x) { return x * d; };
  const Series b = pluss::make_series({0.0, 1.0});
  krylov::CgOptions strict;
  strict.curvature_tolerance = 1e-6;
  const krylov::CgResult accepted = krylov::cg(apply, b, {-1});
  const krylov::CgResult flagged = krylov::cg(apply, b, {-1}, strict);
  out.writeln("default : breakdown =", accepted.breakdown.items<bool>(), ", converged =", accepted.converged.items<bool>());
  out.writeln("strict  : breakdown =", flagged.breakdown.items<bool>(), ", converged =", flagged.converged.items<bool>());
}
```

Output

```text
default : breakdown = [false] , converged = [true]
strict  : breakdown = [true] , converged = [false]
```

## See Also

- [`cg`](cg.md): uses it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="CgResult.html">
    <span class="label">Next</span>
    <span class="title">CgResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
