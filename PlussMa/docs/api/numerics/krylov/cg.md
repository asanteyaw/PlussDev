<!-- generated from cg.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `krylov::cg`

Solves `A x = b` for a symmetric positive-definite operator by conjugate gradients.

## Signature

```cpp
template <typename Apply> krylov::CgResult krylov::cg(Apply &&apply, const pluss::Series &rhs, const std::vector<std::int64_t> &solve_dims, const krylov::CgOptions &options = {}, const pluss::Series &initial = pluss::series::undefined());
```

## What It Does

`apply(x)` must return `A x` with the shape of `x`. `rhs` is the right-hand side, a real floating `Series`; `solve_dims` names the **trailing** axes that form one system (typically `{-1}`), and every leading axis is an independent lane solved at the same time. Conjugate gradients minimises the energy `½ xᵀAx − bᵀx` over the Krylov subspace, which for a symmetric positive-definite `A` gives the solution in at most `n` iterations in exact arithmetic and, for a well-conditioned operator, to high accuracy in far fewer.

Each lane stops when its residual falls below `absolute_tolerance + relative_tolerance·‖b‖`, or at `max_iterations`; the result reports per-lane residual norms and convergence flags, and a per-lane `breakdown` flag for a direction of non-positive curvature (an operator that is not positive definite). The solver checks convergence every `check_interval` iterations (8 by default) so that it need not wait for the host each step, so the reported `iterations` is a multiple of `check_interval` unless the limit is hit first; `initial` is an optional starting guess. For an operator with a good approximate inverse use [`pcg`](pcg.md), and for a non-symmetric operator use [`gmres`](gmres.md).

**The method.** Conjugate gradients builds the solution as `x_k = x_0 + Σ α_j p_j` along search directions that are mutually `A`-conjugate (`p_iᵀ A p_j = 0`), each step choosing the step length that minimises the energy `½ xᵀAx − bᵀx` along the current direction. Conjugacy means the minimisation in one direction is never undone by later ones, which is why the method finishes in at most `n` steps in exact arithmetic and why the error after `k` steps is as good as any polynomial of degree `k` in `A` can make it: the error falls like `((√κ − 1)/(√κ + 1))^k` where `κ` is the condition number. Rounding breaks exact termination, so treat the `n`-step bound as a guide and rely on the residual.

**What to supply.** `apply` should be a pure function of its argument, symmetric positive definite as an operator (`xᵀ A y = yᵀ A x`, `xᵀ A x > 0`); the solver does not check it, but breakdown flags and a stalling residual are the symptoms of a violation. Because the operator is a function, the same code solves a dense matrix, a stencil, a sparse product or a composition of factors, and a batch of right-hand sides costs one pass per iteration whatever its size.

**Cost and accuracy.** The work per iteration is one application of `A` plus a few vector operations. The iteration count depends on the conditioning of `A`, not its size: a Laplacian's condition number grows with the grid, so the count grows with the grid too, and [`pcg`](pcg.md) is the remedy.

## Parameters

`Apply &&apply`
: `Series (Series)`: the product `A x`, same shape as `x`.

`const pluss::Series &rhs`
: The right-hand side; leading axes are lanes.

`const std::vector<std::int64_t> &solve_dims`
: The trailing axes of one system, normally `{-1}`.

`const krylov::CgOptions &options`
: Iteration limit, tolerances.

`const pluss::Series &initial`
: A starting guess of the shape of `rhs`; undefined starts from zero.

## Returns

A [`CgResult`](CgResult.md).

## Errors

- An undefined, empty, non-floating or non-finite `rhs`, a starting guess of the wrong shape, or invalid options throws (`std::invalid_argument`).

## Practical Examples

### Example 1

A small system with a known answer. `[[4, 1], [1, 3]] x = [1, 2]` has `x = [1/11, 7/11] = [0.0909, 0.6364]`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const Series A = pluss::make_series({4.0, 1.0, 1.0, 3.0}, {2, 2});   // symmetric positive definite
  const auto apply = [&](const Series &x) { return A.matmul(x.unsqueeze(-1)).squeeze(-1); };

  krylov::CgOptions options;
  options.check_interval = 1;
  const krylov::CgResult r = krylov::cg(apply, pluss::make_series({1.0, 2.0}), {-1}, options);
  out.writeln("x =", r.solution.items<double>(), ", exact =", 1.0 / 11.0, 7.0 / 11.0);
  out.writeln("iterations =", r.iterations, ", converged =", r.converged.all().item<bool>(), ", relative residual =", r.relative_residual.reshape({r.relative_residual.numel()}).items<double>());
}
```

Output

```text
x = [0.09090909, 0.63636364] , exact = 0.09090909 0.63636364
iterations = 2 , converged = true , relative residual = [0.00000000]
```

### Example 2

A large operator that is never formed. The one-dimensional Laplacian of size 50, applied by shifting; the solution of `A x = 1` is the parabola `x_i = i(n + 1 − i)/2`.

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
  const auto laplacian = [n](const Series &x) {   // tridiagonal [-1, 2, -1], applied without forming the matrix
    const Series zero = pluss::make_series({0.0});
    const Series left = zero.unite({x.slice(0, 0, n - 1, 1)}, 0);
    const Series right = x.slice(0, 1, n, 1).unite({zero}, 0);
    return x * 2.0 - left - right;
  };

  const Series b = pluss::series::ones({n}, pluss::ScalarType::Float64);
  krylov::CgOptions options;
  options.max_iterations = 200;
  options.check_interval = 1;
  const krylov::CgResult r = krylov::cg(laplacian, b, {-1}, options);
  const double exact_mid = 25.0 * 26.0 / 2.0;
  out.writeln("x[24] =", r.solution.select(0, 24).item<double>(), ", exact =", exact_mid);
  out.writeln("iterations =", r.iterations, ", converged =", r.converged.all().item<bool>());
}
```

Output

```text
x[24] = 325.000000 , exact = 325.000000
iterations = 25 , converged = true
```

### Example 3

A batch. Three right-hand sides are three independent systems solved together; each lane has its own residual.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const Series A = pluss::make_series({4.0, 1.0, 1.0, 3.0}, {2, 2});   // symmetric positive definite
  const auto apply = [&](const Series &x) { return A.matmul(x.unsqueeze(-1)).squeeze(-1); };

  const Series rhs = pluss::make_series({1.0, 2.0, 0.0, 1.0, 5.0, -2.0}, {3, 2});
  const krylov::CgResult r = krylov::cg(apply, rhs, {-1});
  out.writeln("solution shape =", r.solution.sizes(), ", lane residuals =", r.relative_residual.reshape({r.relative_residual.numel()}).items<double>());
  out.writeln("all converged =", r.converged.all().item<bool>());
}
```

Output

```text
solution shape = [3, 2] , lane residuals = [0.0000000000, 0.0000000000, 0.0000000000]
all converged = true
```

### Example 4

A starting guess. Starting at the exact solution needs no further progress.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const Series A = pluss::make_series({4.0, 1.0, 1.0, 3.0}, {2, 2});   // symmetric positive definite
  const auto apply = [&](const Series &x) { return A.matmul(x.unsqueeze(-1)).squeeze(-1); };

  const Series exact = pluss::make_series({1.0 / 11.0, 7.0 / 11.0});
  const krylov::CgResult r = krylov::cg(apply, pluss::make_series({1.0, 2.0}), {-1}, krylov::CgOptions{}, exact);
  out.writeln("residual norm =", r.residual_norm.reshape({r.residual_norm.numel()}).items<double>(), ", converged =", r.converged.all().item<bool>());
}
```

Output

```text
residual norm = [0.000000000000] , converged = true
```

### Example 5

An operator that is not positive definite is detected. A matrix with a negative eigenvalue gives a breakdown flag.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Series A = pluss::make_series({1.0, 0.0, 0.0, -1.0}, {2, 2});
  const auto apply = [&](const Series &x) { return A.matmul(x.unsqueeze(-1)).squeeze(-1); };
  const krylov::CgResult r = krylov::cg(apply, pluss::make_series({1.0, 1.0}), {-1});
  out.writeln("breakdown =", r.breakdown.items<bool>(), ", converged =", r.converged.reshape({r.converged.numel()}).items<bool>());
}
```

Output

```text
breakdown = [true] , converged = [false]
```

## See Also

- [`pcg`](pcg.md): with a preconditioner
- [`gmres`](gmres.md): for non-symmetric operators
- [`CgOptions`](CgOptions.md): the settings

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="GmresResult.html">
    <span class="label">Previous</span>
    <span class="title">GmresResult</span>
  </a>
  <a class="next-link" href="exp_action.html">
    <span class="label">Next</span>
    <span class="title">exp_action</span>
  </a>
</div>
<!-- apiref:nav:end -->
