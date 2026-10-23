<!-- generated from tolerances_check_cadence_and_brackets.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Tolerances, check cadence and brackets

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
  const roots::Fn f = [](const Series& x) { return x.exp() - 5.0; };
  std::cout << std::left << std::setw(42) << "setting" << std::setw(12) << "iterations" << "|residual|\n";
  auto show = [&](const char* label, const roots::Result& r) {
    std::cout << std::left << std::setw(42) << label << std::setw(12) << r.iterations << std::scientific << std::setprecision(1) << std::abs(r.residual.item<double>()) << "\n";
  };
  show("illinois, no tolerance, 30 iterations", roots::illinois(f, vec({0.0}), vec({3.0}), {30, 0.0, 0.0, 8}));
  show("illinois, f_tolerance 1e-8, every 1", roots::illinois(f, vec({0.0}), vec({3.0}), {30, 0.0, 1e-8, 1}));
  show("illinois, f_tolerance 1e-8, every 8", roots::illinois(f, vec({0.0}), vec({3.0}), {30, 0.0, 1e-8, 8}));
  show("bisect, x_tolerance 1e-6, every 1", roots::bisect(f, vec({0.0}), vec({3.0}), {60, 1e-6, 0.0, 1}));

  const roots::Fn g = [](const Series& x) { return x.square() + 1.0; };  // no real root
  const auto bad = roots::bisect(g, vec({-1.0}), vec({1.0}));
  std::cout << "\nx^2 + 1 on [-1, 1]: bracketed = " << (bad.bracketed ? "yes" : "no") << ", residual " << std::fixed << std::setprecision(3) << bad.residual.item<double>() << "\n";
}
```

Output

```text
setting                                   iterations  |residual|
illinois, no tolerance, 30 iterations     30          0.0e+00
illinois, f_tolerance 1e-8, every 1       9           1.2e-11
illinois, f_tolerance 1e-8, every 8       16          0.0e+00
bisect, x_tolerance 1e-6, every 1         22          7.5e-07

x^2 + 1 on [-1, 1]: bracketed = no, residual 2.000
```
The third row shows the cadence: the same tolerance, checked every 8 iterations instead of every iteration, stops at 16 instead of 9. When `bracketed` is false the answer for that lane is not a root: always read it for bracketed solvers, and always read `residual`.
