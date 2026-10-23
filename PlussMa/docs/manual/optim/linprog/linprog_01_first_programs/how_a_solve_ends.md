<!-- generated from how_a_solve_ends.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# How a solve ends

[Back to First Programs](../linprog_01_first_programs.md)


Each of the four outcomes, produced on purpose.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <limits>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

const char* name_of(lp::Status status) {
  switch (status) {
    case lp::Status::Optimal: return "Optimal";
    case lp::Status::PrimalInfeasible: return "PrimalInfeasible";
    case lp::Status::Unbounded: return "Unbounded";
    case lp::Status::IterationLimit: return "IterationLimit";
    case lp::Status::NearOptimal: return "NearOptimal";
  }
  return "?";
}

int main() {
  const double inf = std::numeric_limits<double>::infinity();
  const auto run = [&](const char* what, const lp::Problem& p, const lp::Options& o = {}) {
    const auto r = lp::solve(p, o);
    const auto status = static_cast<lp::Status>(static_cast<int>(r.status.item<double>()));
    std::cout << std::left << std::setw(44) << what << std::setw(18) << name_of(status) << "success: " << std::boolalpha << lp::success(status) << "\n";
    return r;
  };

  lp::Problem optimal;  // minimize x0 + x1 with x0 + x1 >= 1
  optimal.c = vec({1, 1});
  optimal.a_ub = mat({-1, -1}, 1, 2);
  optimal.b_ub = vec({-1});
  optimal.lower = vec({0, 0});
  run("x0 + x1 >= 1", optimal);

  lp::Problem infeasible;  // x0 + x1 <= 1 and x0 + x1 >= 3
  infeasible.c = vec({1, 1});
  infeasible.a_ub = mat({1, 1, -1, -1}, 2, 2);
  infeasible.b_ub = vec({1, -3});
  infeasible.lower = vec({0, 0});
  const auto r_infeasible = run("x0 + x1 <= 1 and x0 + x1 >= 3", infeasible);
  const auto farkas = r_infeasible.farkas.to_vec<double>();
  std::cout << "    Farkas multipliers (y for each row): " << std::fixed << std::setprecision(3) << farkas[0] << ", " << farkas[1] << "\n";

  lp::Problem unbounded;  // minimize -x0 with x0 free to grow
  unbounded.c = vec({-1, 0});
  unbounded.a_ub = mat({0, 1}, 1, 2);
  unbounded.b_ub = vec({1});
  unbounded.lower = vec({0, 0});
  unbounded.upper = vec({inf, inf});
  const auto r_unbounded = run("minimize -x0 with x0 unrestricted above", unbounded);
  const auto ray = r_unbounded.ray.to_vec<double>();
  std::cout << "    a ray along which the objective falls: (" << std::fixed << std::setprecision(3) << ray[0] << ", " << ray[1] << ")\n";

  lp::Options tiny;
  tiny.max_iterations = 1;
  run("an iteration budget of 1", optimal, tiny);
  return 0;
}
```

Output

```text
x0 + x1 >= 1                                Optimal           success: true
x0 + x1 <= 1 and x0 + x1 >= 3               PrimalInfeasible  success: false
    Farkas multipliers (y for each row): -1.648, -0.852
minimize -x0 with x0 unrestricted above     Unbounded         success: false
    a ray along which the objective falls: (1.583, 0.000)
an iteration budget of 1                    Optimal           success: true
```
| Status | Code | Meaning | What the result carries |
|---|---|---|---|
| `Optimal` | 0 | the optimum was found | `x`, `objective`, the duals |
| `PrimalInfeasible` | 1 | no point satisfies the constraints | `farkas`: multipliers that prove it |
| `Unbounded` | 2 | the objective falls without limit | `ray`: a direction of unbounded descent |
| `IterationLimit` | 3 | the budget ran out first | the last iterate |
| `NearOptimal` | 4 | the iteration stalled with small residuals | the best iterate; `lp::success` accepts it |

Read the status before reading `x`: a result for an infeasible program still has an `x`, and it means nothing. `lp::success(status)` is true for `Optimal` and `NearOptimal`. The Farkas multipliers and the ray are certificates, and chapter 2 shows how to check them.
