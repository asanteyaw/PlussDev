<!-- generated from the_restart_length.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The restart length

[Back to General Systems and Exponentials](../krylov_02_general_systems_and_exponentials.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace krylov = pluss::subsp::math::krylov;
namespace lo = pluss::subsp::math::linear_operator;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
Series ones(std::vector<std::int64_t> shape) { return Series::zeros(shape, f64) + 1.0; }
// the 1-D second-difference operator (2 on the diagonal, -1 beside it) applied to the last axis, without forming a matrix
Series shift_in(const Series& x, bool from_left) {
  const std::int64_t n = x.size(-1);
  const Series zero = pluss::series::zeros_like(x.slice(-1, 0, 1, 1));
  return from_left ? zero.unite({x.slice(-1, 0, n - 1, 1)}, -1) : x.slice(-1, 1, n, 1).unite({zero}, -1);
}
Series laplacian(const Series& x) { return x * 2.0 - shift_in(x, true) - shift_in(x, false); }
int main() {
  const std::int64_t n = 150;
  auto convection = [&](const Series& x) {
    return laplacian(x) + (shift_in(x, false) - shift_in(x, true)) * 0.7;
  };
  const Series b = ones({n});
  std::cout << std::left << std::setw(10) << "restart" << std::setw(14) << "iterations" << std::setw(12) << "restarts" << std::setw(20) << "operator calls" << "relative residual\n";
  for (std::int64_t restart : {5, 15, 40}) {
    krylov::GmresOptions o;
    o.restart = restart;
    o.max_iterations = 5000;
    o.relative_tolerance = 1e-10;
    const auto r = krylov::gmres(convection, b, {-1}, o);
    std::cout << std::left << std::setw(10) << restart << std::setw(14) << r.iterations << std::setw(12) << r.restarts << std::setw(20) << r.operator_evaluations << std::scientific
              << std::setprecision(1) << r.relative_residual.squeeze().item<double>() << "\n";
  }
}
```

Output

```text
restart   iterations    restarts    operator calls      relative residual
5         210           42          253                 2.2e-11
15        315           21          337                 3.3e-11
40        480           12          493                 8.5e-11
```
A convection–diffusion operator is not symmetric, so conjugate gradients does not apply. The restart length trades memory and per-iteration work against the number of restarts, and there is no universal best: here the shortest basis needs the fewest operator calls (253 against 493 for the longest). Try a few lengths on your own operator, and count `operator_evaluations`, the cost that matters when applying the operator is expensive. A preconditioner for GMRES, applied inside your `apply`, helps in the same way as in Chapter 1.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="heat_flow.html">
    <span class="label">Next</span>
    <span class="title">Heat flow</span>
  </a>
</div>
<!-- apiref:nav:end -->
