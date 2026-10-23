<!-- generated from linear_constraints.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Linear constraints

[Back to The Pattern Search Minimiser](../dfo_07_pattern_search.md)


Bounds and the two linear forms are enforced on every trial point exactly. No penalty is involved, so the returned point satisfies them to rounding.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}

int main() {
  // By hand: on the plane x0 - x1 = 0.5 with x0 + x1 + x2 = 3 we have x1 = x0 - 0.5 and x2 = 3.5 - 2 x0, so
  // f = (x0-2)^2 + (x0-2.5)^2 + (1.5-2 x0)^2, f' = 12 x0 - 15 = 0, giving x0 = 1.25, x1 = 0.75, x2 = 1, f = 3.125.
  dfo::PSConfig c;
  c.lower_bounds = vec({0.0, 0.0, 0.0});
  c.upper_bounds = vec({5.0, 5.0, 5.0});
  c.linear_ineq_matrix = pluss::make_series(std::vector<double>{1.0, 1.0, 1.0}, {1, 3}).to(f64);
  c.linear_ineq_rhs = vec({3.0});
  c.linear_eq_matrix = pluss::make_series(std::vector<double>{1.0, -1.0, 0.0}, {1, 3}).to(f64);
  c.linear_eq_rhs = vec({0.5});
  auto f = [](const pluss::Series& x) { return (x - 2.0).square().sum(); };
  const auto o = dfo::pattern_search_minimizer(f, vec({0.5, 0.0, 0.5}), c);
  const auto x = o.minimizer.to_vec<double>();
  std::cout << std::fixed << std::setprecision(4) << "minimizer : " << x[0] << ", " << x[1] << ", " << x[2] << "  (expected 1.2500, 0.7500, 1.0000)\n";
  std::cout << "minimum   : " << o.minimum << "  (expected 3.1250)\n";
  std::cout << "x0 - x1   : " << x[0] - x[1] << "  (must be 0.5)\n";
  std::cout << "sum       : " << x[0] + x[1] + x[2] << "  (must be <= 3)\n";
  std::cout << "status    : " << dfo::to_string(o.status) << ", evaluations " << o.diagnostics.evaluations << "\n";
}
```

Output

```text
minimizer : 1.2500, 0.7500, 1.0000  (expected 1.2500, 0.7500, 1.0000)
minimum   : 3.1250  (expected 3.1250)
x0 - x1   : 0.5000  (must be 0.5)
sum       : 3.0000  (must be <= 3)
status    : Converged, evaluations 177
```
The start is a vertex of the feasible region, where two inequalities and the equality meet. Moving off it means going along an edge of the feasible cone, which the method builds from the active constraints.
