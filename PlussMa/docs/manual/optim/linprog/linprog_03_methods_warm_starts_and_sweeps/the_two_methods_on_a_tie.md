<!-- generated from the_two_methods_on_a_tie.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The two methods on a tie

[Back to Methods, Warm Starts and Sweeps](../linprog_03_methods_warm_starts_and_sweeps.md)


Minimize `x0 + x1` subject to `x0 + x1 >= 1` and `0 <= x <= 1`. Every point on the segment from `(1, 0)` to `(0, 1)` is optimal.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  lp::Problem p;
  p.c = vec({1, 1});
  p.a_ub = mat({-1, -1}, 1, 2);
  p.b_ub = vec({-1});
  p.lower = vec({0, 0});
  p.upper = vec({1, 1});

  std::cout << std::fixed << std::setprecision(4);
  for (const auto method : {lp::Method::InteriorPoint, lp::Method::Simplex}) {
    lp::Options options;
    options.method = method;
    const auto r = lp::solve(p, options);
    const auto x = r.x.to_vec<double>();
    std::cout << std::left << std::setw(16) << (method == lp::Method::InteriorPoint ? "interior point" : "simplex") << "objective " << r.objective.item<double>()
              << "   x = (" << x[0] << ", " << x[1] << ")\n";
  }
  return 0;
}
```

Output

```text
interior point  objective 1.0000   x = (0.5000, 0.5000)
simplex         objective 1.0000   x = (1.0000, 0.0000)
```
Both methods report the objective 1. The simplex method stops at a corner, `(1, 0)`. The interior point method approaches the optimal segment from inside and converges to a point in its middle, `(0.5, 0.5)`. Neither is wrong. A corner is a plan that uses as few variables as possible, which is usually what you want to act on; a central point spreads the weight evenly, which is what you want when the optimum is a statement about a mixture. If the answer will be used as a plan, ask for the simplex method; if only the optimal value matters, use the default.
