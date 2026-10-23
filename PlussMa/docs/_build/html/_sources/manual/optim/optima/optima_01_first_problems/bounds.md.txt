<!-- generated from bounds.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Bounds

[Back to Your First Constrained Problems](../optima_01_first_problems.md)


Bounds restrict each coordinate to an interval. This program puts the unconstrained minimum at `(3, -2)` and the box at `[0, 2] x [0, 2]`, so the solver has to settle on the corner of the box nearest the target. Bounds go into a `constraints()` value, passed as the fourth argument.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  const auto target = vec({3.0, -2.0});
  auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - optim::trace(target, false))); };

  const auto box = optim::constraints().lbs(vec({0.0, 0.0})).ubs(vec({2.0, 2.0}));
  const auto result = optim::optima(objective, vec({1.0, 1.0}), {}, box);

  const auto x = result.params.at("x").to_vec<double>();
  std::cout << "x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "objective = " << result.evaluation.value.item<double>() << "\n";
  std::cout << "max violation = " << result.evaluation.max_constraint_violation << "\n";
  return 0;
}
```

Output

```text
x = [2, 7.18148e-12]
objective = 5
max violation = 0
```
The answer is the corner `(2, 0)`. The second coordinate prints as `7.18148e-12` instead of exactly `0`: a coordinate that should sit on a bound can come back a hair away from it, within the stopping tolerance. The reported violation of `0` confirms the point is feasible.
