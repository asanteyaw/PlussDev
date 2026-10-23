<!-- generated from a_fourth_order_solver_and_the_derivative_of_the.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A fourth-order solver and the derivative of the root

[Back to Root Finding: Solving Equations](../roots_01_solving_equations.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  Series p = pluss::series::make(2.0);
  p.tracks_slope_(true);
  const auto r = roots::householder(
      [p](const Series& x) {
        return roots::Derivatives{x.square() - p, x * 2.0, pluss::series::full_like(x, 2.0), pluss::series::zeros_like(x)};
      },
      pluss::series::make(1.0), {12, 1e-13, 1e-13, 1});
  std::cout << std::setprecision(15) << "root        : " << r.root.item<double>() << "  (sqrt 2 = " << std::sqrt(2.0) << ")\n";
  std::cout << "iterations  : " << r.iterations << "\n";
  r.root.reverse();
  std::cout << "d root / dp : " << p.sensitivity().item<double>() << "  (exact " << 0.5 / std::sqrt(2.0) << ")\n";
}
```

Output

```text
root        : 1.4142135623731  (sqrt 2 = 1.4142135623731)
iterations  : 4
d root / dp : 0.353553390593274  (exact 0.353553390593274)
```
`householder` converges in four steps here. It also keeps the dependence of the root on any Series the function captured: marking the parameter with `tracks_slope_(true)` and calling `reverse()` on the root gives d(root)/dp from the implicit function theorem, whatever the starting guess. The root is rebuilt from the final evaluation, so the history of the iterations is not stored.
