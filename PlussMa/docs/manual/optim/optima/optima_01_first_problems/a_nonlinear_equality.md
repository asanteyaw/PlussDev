<!-- generated from a_nonlinear_equality.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A nonlinear equality

[Back to Your First Constrained Problems](../optima_01_first_problems.md)


An equality `h(x) = 0` forces the answer onto a curve. This program asks for the point on the unit circle nearest `(3, 3)`, which is `(1/sqrt 2, 1/sqrt 2)`.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <iomanip>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  const auto target = vec({3.0, 3.0});
  auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - optim::trace(target, false))); };

  const auto circle = optim::constraints().nl_eq(
      [](const optim::ParameterPack<optim::Trace>& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });
  const auto result = optim::optima(objective, vec({1.0, 0.0}), {}, circle);

  const auto x = result.params.at("x").to_vec<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "x^2 + y^2 = " << x[0] * x[0] + x[1] * x[1] << "\n";
  std::cout << "expected 1/sqrt(2) = " << 0.70710678 << "\n";
  return 0;
}
```

Output

```text
x = [0.7071, 0.7071]
x^2 + y^2 = 1.0000
expected 1/sqrt(2) = 0.7071
```
The answer is `(0.7071, 0.7071)`, matching the exact `(0.70711, 0.70711)` to four decimals, and `x^2 + y^2` is `1` to four decimals. Unlike an inequality, an equality is always active.
