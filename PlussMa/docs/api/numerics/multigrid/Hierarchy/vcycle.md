<!-- generated from vcycle.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c54c975602e0 -->

# `multigrid::Hierarchy::vcycle`

[Back to `multigrid::Hierarchy`](../Hierarchy.md)

Applies one symmetric V-cycle: an approximation to `A⁻¹ r`.

## Signature

```cpp
[[nodiscard]] pluss::Series multigrid::Hierarchy::vcycle(const pluss::Series &r) const;
```

## What It Does

One cycle smooths the equation on the fine grid, restricts the residual to the next level, recurses, prolongs the correction back and smooths again, with an exact (pseudo-inverse) or heavily smoothed solve on the coarsest level. Pre- and post-smoothing are identical Chebyshev polynomials and the restriction is the adjoint of the prolongation, so the cycle is itself a **symmetric positive** operator: the correct kind of preconditioner for conjugate gradients, which requires symmetry of `M`. Its cost is proportional to the number of cells. A single cycle reduces the error by a roughly grid-independent factor, so the preconditioned iteration count does not grow as the grid is refined.

## Parameters

`const pluss::Series &r`
: A residual field whose trailing dimensions are the axis sizes.

## Returns

The approximate solution `z ≈ A⁻¹ r`, shaped like `r`.

## Errors

- A field of the wrong shape throws.

## Practical Examples

### Example 1

The preconditioner effect. Two-dimensional Poisson problem on a 64×64 grid: conjugate gradients alone needs many iterations; with the V-cycle as preconditioner it needs a handful.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace mg = pluss::subsp::math::multigrid;
namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const std::int64_t n = 64;
  const std::vector<mg::Axis> axes = {{n, 1.0 / n, mg::Edge::Dirichlet, mg::Edge::Dirichlet}, {n, 1.0 / n, mg::Edge::Dirichlet, mg::Edge::Dirichlet}};
  const Series rhs = pluss::series::ones({n, n}, pluss::ScalarType::Float64);

  mg::Hierarchy h({.axes = axes}, rhs);
  krylov::CgOptions o;
  o.check_interval = 1;
  o.max_iterations = 2000;
  const auto apply = [&](const Series &u) { return h.apply(u); };
  const krylov::CgResult plain = krylov::cg(apply, rhs, {-2, -1}, o);
  const krylov::CgResult mgcg = krylov::pcg(apply, [&](const Series &z) { return h.vcycle(z); }, rhs, {-2, -1}, o);
  out.writeln("plain CG iterations =", plain.iterations, ", multigrid-preconditioned =", mgcg.iterations);
}
```

Output

```text
plain CG iterations = 132 , multigrid-preconditioned = 8
```

### Example 2

Mesh independence. As the grid is refined (each row doubles `n`), plain CG needs about twice as many iterations each time, but the preconditioned count stays constant.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace mg = pluss::subsp::math::multigrid;
namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  for (const std::int64_t n : {32, 64, 128}) {
    const std::vector<mg::Axis> axes = {{n, 1.0 / n, mg::Edge::Dirichlet, mg::Edge::Dirichlet}, {n, 1.0 / n, mg::Edge::Dirichlet, mg::Edge::Dirichlet}};
    const Series rhs = pluss::series::ones({n, n}, pluss::ScalarType::Float64);
    mg::Hierarchy h({.axes = axes}, rhs);
    krylov::CgOptions o;
    o.check_interval = 1;
    o.max_iterations = 2000;
    const auto apply = [&](const Series &u) { return h.apply(u); };
    const krylov::CgResult plain = krylov::cg(apply, rhs, {-2, -1}, o);
    const krylov::CgResult mgcg = krylov::pcg(apply, [&](const Series &z) { return h.vcycle(z); }, rhs, {-2, -1}, o);
    out.writeln("n =", n, ": plain", plain.iterations, ", multigrid-preconditioned", mgcg.iterations);
  }
}
```

Output

```text
n = 32 : plain 65 , multigrid-preconditioned 8
n = 64 : plain 132 , multigrid-preconditioned 8
n = 128 : plain 265 , multigrid-preconditioned 8
```

### Example 3

A single cycle as an approximate solve. One V-cycle on `A u = r` reduces the residual substantially; repeated cycles converge geometrically.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace mg = pluss::subsp::math::multigrid;
namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const std::int64_t n = 64;
  const std::vector<mg::Axis> axes = {{n, 1.0 / n, mg::Edge::Dirichlet, mg::Edge::Dirichlet}, {n, 1.0 / n, mg::Edge::Dirichlet, mg::Edge::Dirichlet}};
  const Series rhs = pluss::series::ones({n, n}, pluss::ScalarType::Float64);

  const mg::Hierarchy h({.axes = axes}, rhs);
  Series u = pluss::series::zeros({n, n}, pluss::ScalarType::Float64);
  for (int cycle = 0; cycle < 5; ++cycle) {
    u = u + h.vcycle(rhs - h.apply(u));
    out.writeln("cycle", cycle + 1, ": residual norm =", (rhs - h.apply(u)).square().sum().sqrt().item<double>());
  }
}
```

Output

```text
cycle 1 : residual norm = 6.0834255411
cycle 2 : residual norm = 0.8288121460
cycle 3 : residual norm = 0.1256594393
cycle 4 : residual norm = 0.0196114268
cycle 5 : residual norm = 0.0031134503
```

### Example 4

Symmetry of the cycle, the property that makes it a valid preconditioner: `⟨M r, s⟩ = ⟨r, M s⟩`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace mg = pluss::subsp::math::multigrid;
namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const std::int64_t n = 64;
  const Series r = (pluss::series::arange(0.0, 64.0, 1.0, pluss::ScalarType::Float64) * 0.4).sin();
  const Series s = (pluss::series::arange(0.0, 64.0, 1.0, pluss::ScalarType::Float64) * 0.9).cos();
  const mg::Hierarchy h({.axes = {{n, 1.0 / n, mg::Edge::Dirichlet, mg::Edge::Dirichlet}}}, r);
  out.writeln("<Mr,s> =", (h.vcycle(r) * s).sum().item<double>(), ", <r,Ms> =", (r * h.vcycle(s)).sum().item<double>());
}
```

Output

```text
<Mr,s> = -0.0000506279 , <r,Ms> = -0.0000506279
```

## See Also

- [`Hierarchy`](../Hierarchy.md): the class
- [`apply`](apply.md): the operator
- [`pcg`](../../krylov/pcg.md): uses it as a preconditioner

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="singular.html">
    <span class="label">Previous</span>
    <span class="title">singular</span>
  </a>
</div>
<!-- apiref:nav:end -->
