<!-- generated from the_constraint_forms.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The constraint forms

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
  std::cout << std::fixed << std::setprecision(4);
  {
    const auto r = qp::solve(mat({{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}), vec({0, 0, 0}), mat({{1, 1, 1}}), vec({1.0}), vec({1.0}));
    const auto x = r.x.to_vec<double>();
    std::cout << "equality    : " << x[0] << ", " << x[1] << ", " << x[2] << "   converged " << r.converged << "\n";
  }
  {
    // 0.5|x - c|^2 = 0.5 x'x - c'x + const, so q = -c
    const auto r = qp::solve(mat({{1, 0}, {0, 1}}), vec({-1.0, -1.0}), mat({{1, 1}}), vec({4.0}), vec({inf}));
    const auto x = r.x.to_vec<double>();
    std::cout << "one-sided   : " << x[0] << ", " << x[1] << "   converged " << r.converged << "\n";
  }
  {
    const auto p = mat({{4.0, 1.0}, {1.0, 2.0}});
    const auto r = qp::solve(p, vec({1.0, 1.0}), mat({{1, 0}, {0, 1}}), vec({-inf, -inf}), vec({inf, inf}));
    const auto x = r.x.to_vec<double>();
    std::cout << "unconstrained: " << x[0] << ", " << x[1] << "   (exact " << -1.0 / 7.0 << ", " << -3.0 / 7.0 << ")\n";
  }
}
```

Output

```text
equality    : 0.3333, 0.3333, 0.3333   converged 1
one-sided   : 2.0000, 2.0000   converged 1
unconstrained: -0.1429, -0.4286   (exact -0.1429, -0.4286)
```
An equality, a one-sided bound and an unconstrained problem all use the same call; only the rows and the bounds differ.
