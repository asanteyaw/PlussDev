<!-- generated from batches.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Batches

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
  const std::int64_t n = 60;
  const Series scales = vec({1.0, 10.0, 100.0, 1000.0}).reshape({4, 1});
  const Series b = ones({4, n}) * scales;
  krylov::CgOptions o;
  o.max_iterations = 200;
  o.relative_tolerance = 1e-10;
  const auto r = krylov::cg(laplacian, b, {-1}, o);
  const auto mid = r.solution.select(-1, n / 2).host<double>();
  const auto conv = r.converged.host<bool>();
  const auto rel = r.relative_residual.host<double>();
  std::cout << "iterations: " << r.iterations << " (all lanes together)\n";
  std::cout << std::left << std::setw(8) << "lane" << std::setw(12) << "converged" << std::setw(20) << "relative residual" << "x at the midpoint\n";
  for (int i = 0; i < 4; ++i)
    std::cout << std::left << std::setw(8) << i << std::setw(12) << conv[i] << std::scientific << std::setprecision(1) << std::setw(20) << rel[i] << std::fixed << std::setprecision(2) << mid[i] << "\n";
}
```

Output

```text
iterations: 32 (all lanes together)
lane    converged   relative residual   x at the midpoint
0       1           0.0e+00             465.00
1       1           0.0e+00             4650.00
2       1           0.0e+00             46500.00
3       1           0.0e+00             465000.00
```
Four right-hand sides are solved together with the same operator and the same array operations; `converged` and `relative_residual` come back per lane, and the loop ends when every lane has converged. Lanes finish at different iteration counts in general, and the reported count is the batch's.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="preconditioning.html">
    <span class="label">Previous</span>
    <span class="title">Preconditioning</span>
  </a>
  <a class="next-link" href="a_sparse_matrix_as_an_operator.html">
    <span class="label">Next</span>
    <span class="title">A sparse matrix as an operator</span>
  </a>
</div>
<!-- apiref:nav:end -->
