<!-- generated from linear_constraints.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Linear constraints

[Back to Your First Constrained Problems](../optima_01_first_problems.md)


Linear constraints are matrices. `ln_eq(Aeq, beq)` means `Aeq x = beq` and `ln_ineq(Aineq, bineq)` means `Aineq x <= bineq`, row by row. Each matrix has one column per unknown. This program keeps three numbers summing to 3 while preferring them near 2, and also requires `x1 - x2 <= -0.5`.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto mat = [&](std::vector<double> v, std::int64_t rows, std::int64_t cols) {
    return pluss::make_series(v, {rows, cols}).to(f64);
  };

  const auto target = vec({2.0, 2.0, 2.0});
  auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - optim::trace(target, false))); };

  const auto constraints = optim::constraints()
                               .ln_eq(mat({1, 1, 1}, 1, 3), vec({3.0}))
                               .ln_ineq(mat({1, -1, 0}, 1, 3), vec({-0.5}));
  const auto result = optim::optima(objective, vec({0.0, 0.0, 0.0}), {}, constraints);

  const auto x = result.params.at("x").to_vec<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "x = [" << x[0] << ", " << x[1] << ", " << x[2] << "]\n";
  std::cout << "sum = " << x[0] + x[1] + x[2] << " (equality target 3)\n";
  std::cout << "x1 - x2 = " << x[0] - x[1] << " (inequality limit -0.5)\n";
  return 0;
}
```

Output

```text
x = [0.7500, 1.2500, 1.0000]
sum = 3.0000 (equality target 3)
x1 - x2 = -0.5000 (inequality limit -0.5)
```
The sum is exactly the target, and `x1 - x2` sits at its limit of `-0.5`: the inequality is active, which is why the answer is not the symmetric point `(1, 1, 1)`.
