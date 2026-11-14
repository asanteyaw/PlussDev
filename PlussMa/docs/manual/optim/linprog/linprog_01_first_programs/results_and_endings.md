<!-- generated from results_and_endings.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Reading a result and how a solve ends

[Back to Linear Programming 1: First Programs](../linprog_01_first_programs.md)

A linear-program result carries the answer *and its proof*. The solution `x` and the objective come with the **dual values** (one per row, the price of that constraint), the bound duals and `dual_objective`, which for a correct optimum equals the objective: a feasible point with a matching dual value cannot be improved. That is the certificate of optimality, and checking it takes arithmetic rather than trust.

There are five ways a solve can end, and the status must be read before the point. `Optimal` and `NearOptimal` give a plan; `PrimalInfeasible` carries `farkas` multipliers that prove no plan exists; `Unbounded` carries a `ray` along which the objective falls forever; `IterationLimit` means the budget ran out. A result for an infeasible program still has an `x`, and it means nothing.

## Program 3: Reading the result

A small program with an equality row, an inequality row and bounds: minimize `2 x0 + 3 x1 + x2` subject to `x0 + x1 + x2 = 10`, `x0 - x1 <= 2` and `0 <= x <= 6`. The program prints every field of the result.

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
  p.c = vec({2, 3, 1});
  p.a_eq = mat({1, 1, 1}, 1, 3);
  p.b_eq = vec({10});
  p.a_ub = mat({1, -1, 0}, 1, 3);
  p.b_ub = vec({2});
  p.lower = vec({0, 0, 0});
  p.upper = vec({6, 6, 6});
  const auto r = lp::solve(p);

  std::cout << std::fixed << std::setprecision(4);
  const auto x = r.x.host<double>();
  std::cout << "status          " << static_cast<int>(r.status.item<double>()) << "   (0 = Optimal)\n";
  std::cout << "x               " << x[0] << ", " << x[1] << ", " << x[2] << "\n";
  std::cout << "objective       " << r.objective.item<double>() << "   (the optimum of c . x)\n";
  std::cout << "dual_objective  " << r.dual_objective.item<double>() << "   (equals the objective at an optimum: the certificate)\n";
  std::cout << "dual_eq         " << r.dual_eq.host<double>()[0] << "   (change of the optimum per unit of the equality's right-hand side)\n";
  std::cout << "dual_ub         " << r.dual_ub.host<double>()[0] << "   (the same for the inequality; zero or negative for a minimization)\n";
  std::cout << "iterations      " << r.iterations << "\n";
  std::cout << "the ray and the Farkas multipliers are meaningful only for an unbounded or an infeasible program (next program)\n";
  return 0;
}
```

Output

```text
status          0   (0 = Optimal)
x               3.0000, 1.0000, 6.0000
objective       15.0000   (the optimum of c . x)
dual_objective  15.0000   (equals the objective at an optimum: the certificate)
dual_eq         2.5000   (change of the optimum per unit of the equality's right-hand side)
dual_ub         -0.5000   (the same for the inequality; zero or negative for a minimization)
iterations      6
the ray and the Farkas multipliers are meaningful only for an unbounded or an infeasible program (next program)
```
The optimum is `x = (3, 1, 6)` with an objective of 15 (the cheap variable `x2` is at its upper bound of 6). `dual_objective` equals the objective, which is the certificate that the answer is optimal: a feasible point with a matching dual value cannot be improved. `dual_eq` and `dual_ub` are the prices of the rows (chapter 2). `status` is a number held in a `Series`; the status codes are in the next program.

## Program 4: How a solve ends

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

int main() {
  const double inf = std::numeric_limits<double>::infinity();
  const auto run = [&](const char* what, const lp::Problem& p, const lp::Options& o = {}) {
    const auto r = lp::solve(p, o);
    const auto status = static_cast<lp::Status>(static_cast<int>(r.status.item<double>()));
    std::cout << std::left << std::setw(44) << what << std::setw(18) << lp::to_string(status) << "success: " << std::boolalpha << lp::success(status) << "\n";
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
  const auto farkas = r_infeasible.farkas.host<double>();
  std::cout << "    Farkas multipliers (y for each row): " << std::fixed << std::setprecision(3) << farkas[0] << ", " << farkas[1] << "\n";

  lp::Problem unbounded;  // minimize -x0 with x0 free to grow
  unbounded.c = vec({-1, 0});
  unbounded.a_ub = mat({0, 1}, 1, 2);
  unbounded.b_ub = vec({1});
  unbounded.lower = vec({0, 0});
  unbounded.upper = vec({inf, inf});
  const auto r_unbounded = run("minimize -x0 with x0 unrestricted above", unbounded);
  const auto ray = r_unbounded.ray.host<double>();
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

## Decisions and pitfalls

- **Read the status before reading `x`.** `lp::success(status)` is true for `Optimal` and `NearOptimal` only.
- **`dual_objective` equal to `objective` is the proof of optimality.** A gap means the answer is not yet optimal.
- **The Farkas multipliers and the ray are certificates.** Chapter 2 shows how to check them by arithmetic.
- **`NearOptimal` is accepted by `lp::success`.** It means the iteration stalled with small residuals; check the residuals if the plan is critical.

Where next: [Chapter 2](../linprog_02_prices_and_certificates.md) uses the duals as prices and checks the certificates.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stating_a_linear_program.html">
    <span class="label">Previous</span>
    <span class="title">Stating a linear program</span>
  </a>
</div>
<!-- apiref:nav:end -->
