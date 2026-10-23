<!-- generated from a_first_root.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A first root

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
  const roots::Fn f = [](const Series& x) { return x.pow(3.0) - x * 2.0 - 5.0; };
  const auto r = roots::bisect(f, vec({2.0}), vec({3.0}));
  std::cout << std::setprecision(15) << "root       : " << r.root.item<double>() << "\n";
  std::cout << std::scientific << std::setprecision(2) << "residual   : " << r.residual.item<double>() << "\n";
  std::cout << "iterations : " << r.iterations << "\n";
  std::cout << "bracketed  : " << (r.bracketed ? "yes" : "no") << "\n";
}
```

Output

```text
root       : 2.09455148154233
residual   : 3.55e-15
iterations : 100
bracketed  : yes
```
No tolerance was set, so all 100 bisection steps ran; the residual of 3.6e-15 is the rounding floor of the function near the root. With a tolerance the same answer takes far fewer steps (Program 4).
