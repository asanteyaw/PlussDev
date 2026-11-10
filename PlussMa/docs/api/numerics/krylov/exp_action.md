<!-- generated from exp_action.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `krylov::exp_action`

Applies the matrix exponential `exp(t A)` to a vector without forming the matrix.

## Signature

```cpp
template <typename Apply> krylov::ExpResult krylov::exp_action(Apply &&apply, const pluss::Series &input, const std::vector<std::int64_t> &solve_dims, double time, const krylov::ExpOptions &options = {});
```

## What It Does

The solution of the linear system `y' = A y` at time `t` from `y(0) = v` is `exp(tA) v`. Forming `exp(tA)` for a large `A` is out of the question; the Arnoldi process instead builds a small orthonormal basis of the Krylov subspace of `A` and `v`, takes the exponential of the small projected matrix, and maps it back. `apply(x)` returns `A x`; `input` is the vector `v` (real or complex, leading axes are independent lanes), `solve_dims` the trailing state axes, and `time` the (finite) multiplier `t`, which may be negative.

The result holds the value, an a-posteriori error estimate per lane (the standard Krylov bound), the Krylov dimension used and the operator applications. The Krylov dimension is fixed in advance at `min(max_iterations, n)`, where `n` is the size of one state; the solver does not stop early. The tolerances only decide the per-lane `converged` flag: a lane has converged when its estimate is within `absolute_tolerance + relative_tolerance·‖result‖`. If it has not, raise `max_iterations` or shorten `time` (apply the action in several shorter steps).

**The method.** To compute `exp(tA) v`, the Arnoldi process builds an orthonormal basis `V_m` of the Krylov subspace `span{v, Av, …, A^{m−1} v}` and the Hessenberg matrix `H_m = V_mᵀ A V_m`. The approximation is `exp(tA) v ≈ ‖v‖ V_m exp(t H_m) e_1`, which needs the exponential of only an `m × m` matrix (computed with a matrix-exponential routine on the small matrix). The error decays superlinearly once `m` exceeds roughly `‖tA‖`, so for a modest `m` the approximation is very accurate even for operators of enormous size.

**When to use it.** It is the right tool for evolving a linear system `y' = A y` over a time `t` exactly (not by time stepping), for diffusion and Schrödinger-type problems, and for the exponential integrators that appear in stiff problems. For a very long time, split the interval: `exp(tA) = exp((t/k)A)^k`, applying the action `k` times with a small dimension each, which is cheaper than one huge Krylov space.

## Parameters

`Apply &&apply`
: `Series -> Series`: the product `A x`.

`const pluss::Series &input`
: The vector `v`; leading axes are lanes.

`const std::vector<std::int64_t> &solve_dims`
: The trailing state axes.

`double time`
: The finite multiplier `t`.

`const krylov::ExpOptions &options`
: Iteration limit and tolerances.

## Returns

An [`ExpResult`](ExpResult.md).

## Errors

- A non-finite `time`, or an empty, non-finite or non-floating `input`, or invalid options throws (`std::invalid_argument`).

## Practical Examples

### Example 1

A diagonal operator. `A = diag(−1, −2)`, so `exp(tA)[1, 1] = [e^{−t}, e^{−2t}]`; at `t = 1` that is `[0.36788, 0.13534]`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const Series d = pluss::make_series({-1.0, -2.0});
  const auto apply = [&](const Series &x) { return x * d; };
  const krylov::ExpResult r = krylov::exp_action(apply, pluss::make_series({1.0, 1.0}), {-1}, 1.0);
  out.writeln("exp(A) v =", r.value.items<double>(), ", exact =", std::exp(-1.0), std::exp(-2.0));
  out.writeln("Krylov dimension =", r.iterations, ", converged =", r.converged.all().item<bool>());
}
```

Output

```text
exp(A) v = [0.36787944, 0.13533528] , exact = 0.36787944 0.13533528
Krylov dimension = 2 , converged = true
```

### Example 2

A rotation. `A = [[0, 1], [−1, 0]]` generates rotations: `exp(tA)[1, 0] = [cos t, −sin t]`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const Series A = pluss::make_series({0.0, 1.0, -1.0, 0.0}, {2, 2});
  const auto apply = [&](const Series &x) { return A.matmul(x.unsqueeze(-1)).squeeze(-1); };
  const krylov::ExpResult r = krylov::exp_action(apply, pluss::make_series({1.0, 0.0}), {-1}, 0.7);
  out.writeln("result =", r.value.items<double>(), ", exact =", std::cos(0.7), -std::sin(0.7));
}
```

Output

```text
result = [0.76484219, -0.64421769] , exact = 0.76484219 -0.64421769
```

### Example 3

The heat equation. The matrix-free Laplacian of size 50 (scaled by `−1`) applied to a spike diffuses it; the total stays near 1 (it can only leak through the ends).

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
  const auto apply = [n](const Series &x) {   // the discrete Laplacian, a negative semi-definite operator
    const Series zero = pluss::make_series({0.0});
    return zero.unite({x.slice(0, 0, n - 1, 1)}, 0) + x.slice(0, 1, n, 1).unite({zero}, 0) - x * 2.0;
  };
  const Series v = pluss::series::zeros({n}, pluss::ScalarType::Float64) + (pluss::series::arange(0.0, static_cast<double>(n), 1.0, pluss::ScalarType::Float64) == 25.0).to(pluss::ScalarType::Float64);
  const krylov::ExpResult r = krylov::exp_action(apply, v, {-1}, 2.0);
  out.writeln("peak =", r.value.select(0, 25).item<double>(), ", neighbour =", r.value.select(0, 26).item<double>(), ", total =", r.value.sum().item<double>());
}
```

Output

```text
peak = 0.207002 , neighbour = 0.178751 , total = 1.000000
```

### Example 4

Backward in time. A negative `time` applies `exp(−|t| A)`, undoing the forward evolution.

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
  const krylov::ExpResult forward = krylov::exp_action(apply, pluss::make_series({1.0, 1.0}), {-1}, 0.5);
  const krylov::ExpResult back = krylov::exp_action(apply, forward.value, {-1}, -0.5);
  out.writeln("forward =", forward.value.items<double>(), ", back =", back.value.items<double>());
}
```

Output

```text
forward = [0.60653066, 0.36787944] , back = [1.00000000, 1.00000000]
```

## See Also

- [`ExpOptions`](ExpOptions.md): the settings
- [`gmres`](gmres.md): solving instead of exponentiating

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cg.html">
    <span class="label">Previous</span>
    <span class="title">cg</span>
  </a>
  <a class="next-link" href="gmres.html">
    <span class="label">Next</span>
    <span class="title">gmres</span>
  </a>
</div>
<!-- apiref:nav:end -->
