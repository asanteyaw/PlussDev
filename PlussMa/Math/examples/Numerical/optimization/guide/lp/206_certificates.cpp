// A verdict you can check. An infeasible program comes with Farkas multipliers y, one per row, each zero or negative for a `<=`
// row. Multiplying each row by its (nonpositive) weight and adding gives, for every point that satisfies the rows,
// g . x >= b . y with g = A' y. If the largest value that g . x can reach inside the bounds is smaller than b . y, no point can
// satisfy both, so the rows cannot hold. An unbounded program comes with a ray d: moving along d keeps every constraint
// satisfied and lowers the objective forever. The program verifies both certificates by arithmetic that does not use the solver again.
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
    const auto y = r.farkas.host<double>();  // one multiplier per inequality row
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
    const auto d = r.ray.host<double>();
    const double along_row = 0.0 * d[0] + 1.0 * d[1];  // A d must be <= 0
    const double cost_change = -1.0 * d[0] + 0.0 * d[1];  // c . d must be < 0
    std::cout << "unbounded program: ray d = (" << d[0] << ", " << d[1] << ")\n";
    std::cout << "  A d = " << along_row << " (must be <= 0), c . d = " << cost_change << " (must be < 0), d >= 0 so the bounds are respected\n";
    std::cout << "  the ray is a valid direction of unbounded descent: " << (along_row <= 1e-6 && cost_change < 0 && d[0] >= -1e-6 && d[1] >= -1e-6) << "\n";
  }
  return 0;
}
