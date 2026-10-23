<!-- generated from the_methods_compared.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The methods compared

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
  const roots::Fn df = [](const Series& x) { return x.exp(); };
  std::cout << std::left << std::setw(12) << "iterations" << std::setw(12) << "bisect" << std::setw(12) << "illinois" << std::setw(18)
            << "newton (exact)" << std::setw(18) << "newton (diff.)" << "secant\n";
  for (std::int64_t k : {2, 4, 6, 8, 12}) {
    roots::Options o;
    o.max_iterations = k;
    const auto b = roots::bisect(f, vec({0.0}), vec({3.0}), o);
    const auto il = roots::illinois(f, vec({0.0}), vec({3.0}), o);
    const auto nx = roots::newton(f, vec({1.0}), df, o);
    const auto nd = roots::newton(f, vec({1.0}), {}, o);
    const auto sc = roots::secant(f, vec({1.0}), vec({2.0}), o);
    std::cout << std::left << std::setw(12) << k << std::scientific << std::setprecision(1) << std::setw(12) << std::abs(b.residual.item<double>())
              << std::setw(12) << std::abs(il.residual.item<double>()) << std::setw(18) << std::abs(nx.residual.item<double>()) << std::setw(18)
              << std::abs(nd.residual.item<double>()) << std::abs(sc.residual.item<double>()) << "\n";
  }
}
```

Output

```text
iterations  bisect      illinois    newton (exact)    newton (diff.)    secant
2           1.5e+00     2.2e+00     1.2e-01           1.2e-01           1.1e-01
4           7.8e-02     4.3e-01     2.2e-07           2.2e-07           7.8e-05
6           3.9e-02     1.1e-03     0.0e+00           0.0e+00           4.3e-13
8           9.5e-03     1.2e-07     0.0e+00           0.0e+00           0.0e+00
12          3.0e-04     0.0e+00     0.0e+00           0.0e+00           0.0e+00
```
The table reads as rates. Bisection gains about one binary digit per step and its residual does not fall evenly, because the midpoint can land on either side of the root. Illinois starts slower here (its early steps are long) and then accelerates sharply. Newton, with the exact derivative or with the built-in central difference, is quadratic: the number of correct digits roughly doubles each step, and it is at rounding level by step 6 (the `0.0e+00` entries are residuals below 1e-16). The secant method, which needs no derivative, is nearly as fast. Use a bracketed method when you have a bracket and want certainty, Newton when you can supply or cheaply difference the derivative, and the secant method when you have neither.
