<!-- generated from a_first_qp.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A first QP

[Back to Quadratic Programming](../qp_01_quadratic_programs.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
const double inf = std::numeric_limits<double>::infinity();
Series mat(std::vector<std::vector<double>> rows) {
  std::vector<double> flat;
  for (const auto& r : rows) flat.insert(flat.end(), r.begin(), r.end());
  return pluss::make_series(flat, {static_cast<std::int64_t>(rows.size()), static_cast<std::int64_t>(rows[0].size())}).to(f64);
}
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const auto p = mat({{4.0, 1.0}, {1.0, 2.0}});
  const auto q = vec({1.0, 1.0});
  const auto a = mat({{1.0, 1.0}, {1.0, 0.0}, {0.0, 1.0}});
  const auto lower = vec({1.0, 0.0, 0.0});
  const auto upper = vec({1.0, 0.7, 0.7});
  const auto r = qp::solve(p, q, a, lower, upper);
  const auto x = r.x.to_vec<double>();
  std::cout << std::fixed << std::setprecision(4) << "x          : " << x[0] << ", " << x[1] << "\n";
  std::cout << "objective  : " << r.objective.item<double>() << "\n";
  std::cout << "converged  : " << (r.converged ? "yes" : "no") << " after " << r.iterations << " iterations\n";
  std::cout << std::scientific << std::setprecision(1) << "residuals  : primal " << r.primal_residual << ", dual " << r.dual_residual << "\n";
  const auto y = r.y.to_vec<double>();
  std::cout << std::fixed << std::setprecision(4) << "multipliers: " << y[0] << ", " << y[1] << ", " << y[2] << "  (one per constraint row)\n";
}
```

Output

```text
x          : 0.3000, 0.7000
objective  : 1.8799
converged  : yes after 80 iterations
residuals  : primal 1.7e-05, dual 6.6e-06
multipliers: -2.8999, -0.0000, 0.1999  (one per constraint row)
```
The sum constraint is active (its multiplier is large), `x1` sits at its upper bound (positive multiplier) and `x0`'s bound is slack (multiplier 0). The accuracy is about 1e-4, which is what the default tolerance of 1e-5 on the residuals buys; tighten it for more (Program 4).
