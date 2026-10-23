<!-- generated from a_first_linear_program.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A first linear program

[Back to First Programs](../linprog_01_first_programs.md)


A workshop earns 3 per unit of product 1 and 5 per unit of product 2. Product 1 needs 1 hour on machine A; product 2 needs 2 hours on machine B; and each unit of the two needs 3 and 2 hours on machine C. The machines offer 4, 12 and 18 hours. How much of each should be made?

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  lp::Problem problem;
  problem.c = vec({-3.0, -5.0});                         // minimize c . x, so a profit is entered negated
  problem.a_ub = mat({1, 0,                              // x          <= 4
                      0, 2,                              //     2 y    <= 12
                      3, 2}, 3, 2);                      // 3 x + 2 y  <= 18
  problem.b_ub = vec({4.0, 12.0, 18.0});
  problem.lower = vec({0.0, 0.0});                       // x, y >= 0

  std::cout << std::left << std::setw(16) << "method" << std::setw(12) << "status" << std::setw(8) << "iters" << std::setw(14) << "x, y" << "profit\n";
  for (const auto method : {lp::Method::InteriorPoint, lp::Method::Simplex}) {
    lp::Options options;
    options.method = method;
    const auto r = lp::solve(problem, options);
    const auto x = r.x.to_vec<double>();
    std::ostringstream point;
    point << std::fixed << std::setprecision(3) << x[0] << ", " << x[1];
    std::cout << std::left << std::setw(16) << (method == lp::Method::InteriorPoint ? "interior point" : "simplex") << std::setw(12)
              << (lp::success(static_cast<lp::Status>(static_cast<int>(r.status.item<double>()))) ? "optimal" : "other") << std::setw(8) << r.iterations
              << std::setw(14) << point.str() << std::fixed << std::setprecision(4) << -r.objective.item<double>() << "\n";
  }
  return 0;
}
```

Output

```text
method          status      iters   x, y          profit
interior point  optimal     5       2.000, 6.000  36.0000
simplex         optimal     4       2.000, 6.000  36.0000
```
Both methods find the same plan: 2 units of product 1 and 6 of product 2, for a profit of 36. The simplex took 4 iterations and the interior point method 5; an iteration of either is a different amount of work, so compare the methods by what they return and not by that count. The profit is printed as `-r.objective`, because the problem minimizes `-3x - 5y`.
