<!-- generated from conjugate_gradients_with_an_operator.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Conjugate gradients with an operator

[Back to Symmetric Systems](../krylov_01_symmetric_systems.md)


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
  const std::int64_t n = 100;
  const Series b = ones({n});
  krylov::CgOptions o;
  o.max_iterations = 400;
  o.relative_tolerance = 1e-10;
  const auto r = krylov::cg(laplacian, b, {-1}, o);
  const auto x = r.solution.host<double>();
  double worst = 0.0;
  for (std::int64_t i = 1; i <= n; ++i) worst = std::max(worst, std::abs(x[i - 1] - 0.5 * i * (n + 1 - i)));
  std::cout << "iterations        : " << r.iterations << " (the system has " << n << " unknowns)\n";
  std::cout << std::scientific << std::setprecision(2) << "relative residual : " << r.relative_residual.squeeze().item<double>() << "\n";
  std::cout << "converged         : " << r.converged.squeeze().item<bool>() << ", breakdown " << r.breakdown.squeeze().item<bool>() << "\n";
  std::cout << "largest error     : " << worst << "\n";
}
```

Output

```text
iterations        : 56 (the system has 100 unknowns)
relative residual : 0.00e+00
converged         : 1, breakdown 0
largest error     : 0.00e+00
```
The operator here is the second-difference stencil, written with shifted copies of the vector; no matrix exists anywhere. In exact arithmetic, conjugate gradients finishes in at most as many iterations as the right-hand side has distinct eigen-components: here only the odd eigenvectors are excited, which is why it stops after 56 for 100 unknowns, and why the answer is exact to the last digit. In general expect far fewer iterations than unknowns when the eigenvalues cluster, and many more when they do not.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="preconditioning.html">
    <span class="label">Next</span>
    <span class="title">Preconditioning</span>
  </a>
</div>
<!-- apiref:nav:end -->
