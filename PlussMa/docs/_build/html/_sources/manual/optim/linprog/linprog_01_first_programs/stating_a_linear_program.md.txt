<!-- generated from stating_a_linear_program.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Stating a linear program

[Back to Linear Programming 1: First Programs](../linprog_01_first_programs.md)

A linear program asks for the cheapest (or most profitable) way to meet a set of linear requirements: minimise a sum of costs subject to limits on resources, balances that must hold exactly, and bounds on each quantity. Production, blending, transport and scheduling problems take this form, and for them the answer is exact and comes with a proof of its own correctness.

This page covers stating one. A workshop's product mix is solved by both methods (simplex and interior point), and they find the same plan. A requirement can be written in several equivalent ways (a row, a bound, an equality with a surplus variable), and the second program shows that the three formulations give the same optimum while a bound is cheaper than a row for the solver. The third is a trap: if `lower` is left out, the variables are *free*, not non-negative, and a program that looks bounded is unbounded.

## Program 1: A first linear program

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
    const auto x = r.x.host<double>();
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

## Program 2: The shapes a requirement can take

A requirement can be written in several equivalent ways, and which is most convenient depends on where it comes from. A "greater than or equal" is a `<=` row with both sides negated. A balance with surplus is an equality with an extra variable. A simple limit on one variable can be a bound or a row. The program minimizes `2 x0 + 3 x1 + 4 x2` subject to `x0 + x1 >= 2` and `x2 >= 1` in three formulations, and then shows a free variable and an upper-bounded one.

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
  std::cout << std::fixed << std::setprecision(4);
  const auto show = [](const char* label, const lp::Result& r, int count) {
    const auto x = r.x.host<double>();
    std::cout << std::left << std::setw(40) << label << "objective " << std::setw(9) << r.objective.item<double>() << "x =";
    for (int i = 0; i < count; ++i) std::cout << " " << x[static_cast<std::size_t>(i)];
    std::cout << "\n";
  };

  {  // (1) an inequality row, and a lower bound on x2
    lp::Problem p;
    p.c = vec({2, 3, 4});
    p.a_ub = mat({-1, -1, 0}, 1, 3);  // -x0 - x1 <= -2   (that is, x0 + x1 >= 2)
    p.b_ub = vec({-2});
    p.lower = vec({0, 0, 1});         // x2 >= 1
    show("row and bound", lp::solve(p), 3);
  }
  {  // (2) an equality with a surplus variable s (the fourth variable)
    lp::Problem p;
    p.c = vec({2, 3, 4, 0});
    p.a_eq = mat({1, 1, 0, -1}, 1, 4);  // x0 + x1 - s = 2
    p.b_eq = vec({2});
    p.lower = vec({0, 0, 1, 0});
    show("equality with a surplus variable", lp::solve(p), 3);
  }
  {  // (3) the bound on x2 written as a row
    lp::Problem p;
    p.c = vec({2, 3, 4});
    p.a_ub = mat({-1, -1, 0,
                  0, 0, -1}, 2, 3);  // -x0 - x1 <= -2 ; -x2 <= -1
    p.b_ub = vec({-2, -1});
    p.lower = vec({0, 0, -inf});
    show("both requirements as rows", lp::solve(p), 3);
  }
  std::cout << "\nfree and bounded variables: minimize x0 + 2 x1 with x0 + x1 >= -3, x0 free, 0 <= x1 <= 2\n";
  {
    lp::Problem p;
    p.c = vec({1, 2});
    p.a_ub = mat({-1, -1}, 1, 2);
    p.b_ub = vec({3});
    p.lower = vec({-inf, 0});
    p.upper = vec({inf, 2});
    show("a free variable and an upper bound", lp::solve(p), 2);
  }
  return 0;
}
```

Output

```text
row and bound                           objective 8.0000   x = 2.0000 0.0000 1.0000
equality with a surplus variable        objective 8.0000   x = 2.0000 0.0000 1.0000
both requirements as rows               objective 8.0000   x = 2.0000 0.0000 1.0000

free and bounded variables: minimize x0 + 2 x1 with x0 + x1 >= -3, x0 free, 0 <= x1 <= 2
a free variable and an upper bound      objective -3.0000  x = -3.0000 0.0000
```
All three formulations give the objective 8 at `x = (2, 0, 1)`: a row and a bound, an equality with a surplus variable, and both requirements as rows. Prefer the bound to the row wherever a requirement is about one variable: it is cheaper for the solver, and it stays out of the constraint matrix. The last case has a free variable (`lower = -inf`) and a variable with an upper limit: the optimum is `x0 = -3`, `x1 = 0`, objective -3.

## Program 5: The default bounds

If `lower` is left out, the variables are **free**. This surprises people who expect the usual `x >= 0`.

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
  p.c = vec({1, -1});
  p.a_ub = mat({-1, -1, 1, 1}, 2, 2);  // x0 + x1 >= 2 ; x0 + x1 <= 4
  p.b_ub = vec({-2, 4});

  const auto describe = [](const char* label, const lp::Problem& problem) {
    const auto r = lp::solve(problem);
    const auto status = static_cast<lp::Status>(static_cast<int>(r.status.item<double>()));
    std::cout << std::left << std::setw(30) << label << (status == lp::Status::Optimal ? "Optimal" : status == lp::Status::Unbounded ? "Unbounded" : "other");
    if (status == lp::Status::Optimal) {
      const auto x = r.x.host<double>();
      std::cout << std::fixed << std::setprecision(4) << "   x = (" << x[0] << ", " << x[1] << "), objective " << r.objective.item<double>();
    }
    std::cout << "\n";
  };
  describe("no bounds given (free)", p);
  lp::Problem nonnegative = p;
  nonnegative.lower = vec({0, 0});
  describe("lower = (0, 0)", nonnegative);
  return 0;
}
```

Output

```text
no bounds given (free)        Unbounded
lower = (0, 0)                Optimal   x = (0.0000, 4.0000), objective -4.0000
```
The same two rows and the same objective are unbounded with no bounds given and optimal, at `x = (0, 4)` with objective -4, when `lower = (0, 0)` is stated. Always set `lower` explicitly. The sparse solver of chapter 5 uses the other convention, `x >= 0` by default, so a model moved between the two needs its bounds checked.

## Decisions and pitfalls

- **Compare methods by what they return, not by iteration counts.** An iteration of the simplex and an iteration of the interior-point method are different amounts of work.
- **Put a limit on one variable in a bound, not a row.** It is cheaper for the solver and stays out of the constraint matrix.
- **Always set `lower` explicitly.** The dense solver's variables are free by default, which surprises people who expect `x >= 0`; the sparse solver of chapter 4 uses the other convention (non-negative by default), so a model moved between them needs its bounds checked.
- **A greater-than row is a less-than row with both sides negated.** Both forms are accepted; use whichever matches where the requirement comes from.

Where next: [Results and endings](results_and_endings.md) reads what a solve returns.
