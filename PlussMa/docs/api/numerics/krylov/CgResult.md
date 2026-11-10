<!-- generated from CgResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `krylov::CgResult`

The solution of a conjugate-gradient solve with per-lane residuals and flags.

`solution` has the shape of the right-hand side. `residual_norm` and `relative_residual` hold the final residual per lane (shape of the leading axes), `converged` and `breakdown` are boolean per lane, and `iterations` is the number of iterations run for the whole batch.

## Fields

### `solution`

`pluss::Series`, default `undefined`.

The solution `x`, shaped like the right-hand side; lanes that did not converge hold the last iterate.


### `residual_norm`

`pluss::Series`, default `undefined`.

The final `‖b − A x‖` per lane, with the shape of the leading axes. It is recomputed rather than updated, so it is not subject to the drift of a recurrence.


### `relative_residual`

`pluss::Series`, default `undefined`.

The residual divided by `‖b‖` per lane: the number to compare with `relative_tolerance`, comparable across lanes with different scales.


### `converged`

`pluss::Series`, default `undefined`.

Boolean per lane: the residual met `absolute_tolerance + relative_tolerance·‖b‖`.


### `breakdown`

`pluss::Series`, default `undefined`.

Boolean per lane: a search direction had curvature at or below `curvature_tolerance`, which means the operator is not positive definite along that direction. The lane stops and is not converged; switch to GMRES.


### `iterations`

`std::int64_t`, default `0`.

The iterations run for the whole batch: the loop continues until every lane is done or the limit is reached, and is a multiple of `check_interval` unless the limit stopped it.

## Guidance

Returned by `cg` and `pcg`.

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
  out.print_defaults(PrintOptions{}.fixed(12));
  const Series A = pluss::make_series({4.0, 1.0, 1.0, 3.0}, {2, 2});
  const auto apply = [&](const Series &x) { return A.matmul(x.unsqueeze(-1)).squeeze(-1); };
  const krylov::CgResult r = krylov::cg(apply, pluss::make_series({1.0, 2.0}), {-1});
  out.writeln("solution =", r.solution.items<double>(), ", residual =", r.residual_norm.reshape({r.residual_norm.numel()}).items<double>(), ", relative =", r.relative_residual.reshape({r.relative_residual.numel()}).items<double>());
  out.writeln("converged =", r.converged.reshape({r.converged.numel()}).items<bool>(), ", breakdown =", r.breakdown.items<bool>(), ", iterations =", r.iterations);
}
```

Output

```text
solution = [0.090909090909, 0.636363636364] , residual = [0.000000000000] , relative = [0.000000000000]
converged = [true] , breakdown = [false] , iterations = 8
```

### Example 2

Per-lane flags. In a batch, one lane can converge while another has not.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const Series A = pluss::make_series({4.0, 1.0, 1.0, 3.0}, {2, 2});
  const auto apply = [&](const Series &x) { return A.matmul(x.unsqueeze(-1)).squeeze(-1); };
  krylov::CgOptions options;
  options.max_iterations = 1;
  options.check_interval = 1;
  const krylov::CgResult r = krylov::cg(apply, pluss::make_series({1.0, 2.0, 0.0, 0.0}, {2, 2}), {-1}, options);
  out.writeln("converged per lane =", r.converged.reshape({r.converged.numel()}).items<bool>(), ", residual per lane =", r.residual_norm.reshape({r.residual_norm.numel()}).items<double>());
}
```

Output

```text
converged per lane = [false, true] , residual per lane = [0.55901699, 0.00000000]
```

### Example 3

The residual is the distance from solving the system: `‖b − A x‖` recomputed from the solution.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const Series A = pluss::make_series({4.0, 1.0, 1.0, 3.0}, {2, 2});
  const auto apply = [&](const Series &x) { return A.matmul(x.unsqueeze(-1)).squeeze(-1); };
  const Series b = pluss::make_series({1.0, 2.0});
  const krylov::CgResult r = krylov::cg(apply, b, {-1});
  out.writeln("reported =", r.residual_norm.reshape({r.residual_norm.numel()}).items<double>(), ", recomputed =", (b - apply(r.solution)).square().sum().sqrt().item<double>());
}
```

Output

```text
reported = [0.000000000000] , recomputed = 0.000000000000
```

## See Also

- [`cg`](cg.md): produces it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="CgOptions.html">
    <span class="label">Previous</span>
    <span class="title">CgOptions</span>
  </a>
  <a class="next-link" href="ExpOptions.html">
    <span class="label">Next</span>
    <span class="title">ExpOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
