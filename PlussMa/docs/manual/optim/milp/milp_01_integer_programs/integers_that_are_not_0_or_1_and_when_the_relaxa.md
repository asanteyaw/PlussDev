<!-- generated from integers_that_are_not_0_or_1_and_when_the_relaxa.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Integers that are not 0 or 1, and when the relaxation is enough

[Back to Choosing Whole Things](../milp_01_integer_programs.md)


Integer variables can be any whole number in their bounds. The first program maximizes `5x + 4y + 3z` subject to three resource rows, with `x`, `y`, `z` integers up to 10. The second maximizes `x + y` subject to `x + y <= 3.5` with both integer.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;
namespace milp = pluss::subsp::math::milp;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto run = [](const char* label, milp::Problem p) {
    const auto relaxed = lp::solve(p.lp);
    const auto r = milp::solve(p);
    const auto x = r.x.to_vec<double>();
    std::cout << std::left << std::setw(32) << label << "relaxation " << std::setw(9) << -relaxed.objective.item<double>() << "integer optimum " << std::setw(9)
              << -r.objective.item<double>() << "nodes " << std::setw(4) << r.nodes << "x =";
    for (const double v : x) std::cout << " " << v;
    std::cout << "\n";
  };

  {
    milp::Problem p;
    p.lp.c = vec({-5, -4, -3});
    p.lp.a_ub = mat({2, 3, 1, 4, 1, 2, 3, 4, 2}, 3, 3);
    p.lp.b_ub = vec({5, 11, 8});
    p.lp.lower = vec({0, 0, 0});
    p.lp.upper = vec({10, 10, 10});
    p.integer = vec({1, 1, 1});
    run("a small production plan", p);
  }
  {
    milp::Problem p;
    p.lp.c = vec({-1, -1});
    p.lp.a_ub = mat({1, 1}, 1, 2);
    p.lp.b_ub = vec({3.5});
    p.lp.lower = vec({0, 0});
    p.lp.upper = vec({10, 10});
    p.integer = vec({1, 1});
    run("x + y <= 3.5, both integer", p);
  }
  return 0;
}
```

Output

```text
a small production plan         relaxation 13.0000  integer optimum 13.0000  nodes 1   x = 2.0000 0.0000 1.0000
x + y <= 3.5, both integer      relaxation 3.5000   integer optimum 3.0000   nodes 17  x = 2.0000 1.0000
```
For the production plan the relaxation's answer is already whole, `(2, 0, 1)` with value 13, so the search finishes at the root: 1 node. This happens often enough to be worth knowing: solve the relaxation first, and if it is integral you are done. For `x + y <= 3.5` the relaxation reaches 3.5 and no integer point does; the search needs 17 nodes and finds 3, at `(2, 1)`.
