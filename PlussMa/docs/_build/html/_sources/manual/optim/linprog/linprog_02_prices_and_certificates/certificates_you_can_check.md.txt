<!-- generated from certificates_you_can_check.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Certificates you can check

[Back to Prices and Certificates](../linprog_02_prices_and_certificates.md)


A verdict of infeasible or unbounded is only as good as its proof, and the proof is in the result.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <limits>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  std::cout << std::fixed << std::setprecision(4) << std::boolalpha;
  {
    // x0 + x1 <= 1 and x0 + x1 >= 3 with x >= 0
    lp::Problem p;
    p.c = vec({1, 1});
    p.a_ub = mat({1, 1, -1, -1}, 2, 2);
    p.b_ub = vec({1, -3});
    p.lower = vec({0, 0});
    const auto r = lp::solve(p);
    const auto y = r.farkas.to_vec<double>();  // one multiplier per inequality row
    const double g0 = y[0] * 1 + y[1] * -1, g1 = y[0] * 1 + y[1] * -1;  // g = A' y
    const double b_dot_y = y[0] * 1 + y[1] * -3;
    // with x >= 0 and no upper bound, g . x is largest at x = 0 if every component of g is <= 0
    const double largest = (g0 <= 0 && g1 <= 0) ? 0.0 : std::numeric_limits<double>::infinity();
    std::cout << "infeasible program: g = A'y = (" << g0 << ", " << g1 << "), largest g . x on x >= 0 is " << largest << ", b . y = " << b_dot_y << "\n";
    std::cout << "  every feasible point would need g . x >= b . y, but g . x <= " << largest << " < " << b_dot_y << ", so there is none: " << (largest < b_dot_y) << "\n\n";
  }
  {
    // minimize -x0 with x1 <= 1 and x >= 0: x0 can grow without limit
    lp::Problem p;
    p.c = vec({-1, 0});
    p.a_ub = mat({0, 1}, 1, 2);
    p.b_ub = vec({1});
    p.lower = vec({0, 0});
    const auto r = lp::solve(p);
    const auto d = r.ray.to_vec<double>();
    const double along_row = 0.0 * d[0] + 1.0 * d[1];  // A d must be <= 0
    const double cost_change = -1.0 * d[0] + 0.0 * d[1];  // c . d must be < 0
    std::cout << "unbounded program: ray d = (" << d[0] << ", " << d[1] << ")\n";
    std::cout << "  A d = " << along_row << " (must be <= 0), c . d = " << cost_change << " (must be < 0), d >= 0 so the bounds are respected\n";
    std::cout << "  the ray is a valid direction of unbounded descent: " << (along_row <= 1e-6 && cost_change < 0 && d[0] >= -1e-6 && d[1] >= -1e-6) << "\n";
  }
  return 0;
}
```

Output

```text
infeasible program: g = A'y = (-0.7956, -0.7956), largest g . x on x >= 0 is 0.0000, b . y = 0.9088
  every feasible point would need g . x >= b . y, but g . x <= 0.0000 < 0.9088, so there is none: true

unbounded program: ray d = (1.5830, 0.0000)
  A d = 0.0000 (must be <= 0), c . d = -1.5830 (must be < 0), d >= 0 so the bounds are respected
  the ray is a valid direction of unbounded descent: true
```
**Infeasible.** The program `x0 + x1 <= 1`, `x0 + x1 >= 3`, `x >= 0` cannot be satisfied. The Farkas multipliers `y` are weights on the rows, each zero or negative for a `<=` row. Multiply each row by its weight and add them. Because the weights are negative, each inequality flips, and the sum says that every point satisfying the rows has `g . x >= b . y`, where `g = A'y`. Here `g = (-0.7956, -0.7956)` and `b . y = 0.9088`. On `x >= 0` the largest `g . x` can be is 0, which is below 0.9088. So any feasible point would need `g . x >= 0.9088` while the bounds keep `g . x <= 0`: there is none. The program checks the inequality with arithmetic that does not call the solver, and it holds.

**Unbounded.** The program "minimize `-x0` with `x1 <= 1`, `x >= 0`" can lower its objective forever by growing `x0`. The ray `d = (1.583, 0)` is a direction along which every row stays satisfied (`A d = 0 <= 0`), the bounds are respected (`d >= 0`) and the cost falls (`c . d = -1.583 < 0`). Any feasible point plus any multiple of `d` is feasible with a lower cost. The scale of a ray or of a set of multipliers is arbitrary; only the direction and the signs matter.

These checks are worth running in your own code when a verdict surprises you, and they turn "the solver says infeasible" into a statement you can show someone.
