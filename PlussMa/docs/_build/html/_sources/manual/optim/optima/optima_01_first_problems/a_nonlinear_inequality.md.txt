<!-- generated from a_nonlinear_inequality.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A nonlinear inequality

[Back to Your First Constrained Problems](../optima_01_first_problems.md)


A nonlinear inequality returns `g(x)`, required to be at most zero. Here `g(x) = x0^2 + x1^2 - 1`, so the answer must lie inside the unit disc. The target `(1, 2)` is outside the disc, so the best feasible point is the point of the disc closest to the target, on the circle.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  const auto target = vec({1.0, 2.0});
  auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - optim::trace(target, false))); };

  const auto disc = optim::constraints().nl_ineq(
      [](const optim::ParameterPack<optim::Trace>& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });
  const auto result = optim::optima(objective, vec({0.0, 0.0}), {}, disc);

  const auto x = result.params.at("x").to_vec<double>();
  const double norm = std::sqrt(x[0] * x[0] + x[1] * x[1]);
  std::cout << "x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "distance from origin = " << norm << "\n";
  std::cout << "closest disc point to (1, 2) is (1, 2)/sqrt(5) = [" << 1.0 / std::sqrt(5.0) << ", "
            << 2.0 / std::sqrt(5.0) << "]\n";
  return 0;
}
```

Output

```text
x = [0.447214, 0.894427]
distance from origin = 1
closest disc point to (1, 2) is (1, 2)/sqrt(5) = [0.447214, 0.894427]
```
The solver lands on the circle at the direction of the target, matching `(1, 2) / sqrt(5)` to five digits. When the unconstrained minimum is outside the feasible region, the answer sits on the boundary, and the constraint is said to be active there.
