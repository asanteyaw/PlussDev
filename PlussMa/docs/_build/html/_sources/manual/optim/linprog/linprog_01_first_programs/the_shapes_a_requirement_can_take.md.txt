<!-- generated from the_shapes_a_requirement_can_take.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The shapes a requirement can take

[Back to First Programs](../linprog_01_first_programs.md)


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
    const auto x = r.x.to_vec<double>();
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
