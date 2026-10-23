<!-- generated from accuracy.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Accuracy

[Back to Linear Interpolation](../interp_01_linear.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <complex>
#include <iomanip>
#include <iostream>

namespace interp = pluss::subsp::math::interp;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const double pi = 3.14159265358979323846;
  const Series queries = Series::arange(0.0, pi, pi / 1000.0, f64);
  std::cout << std::left << std::setw(10) << "points" << std::setw(16) << "worst error" << "ratio\n";
  double previous = 0.0;
  for (int n : {5, 9, 17, 33, 65}) {
    const Series x = Series::arange(0.0, pi + 1e-12, pi / (n - 1), f64);
    const auto r = interp::linear(x, x.sin(), queries);
    const double worst = (r - queries.sin()).abs().amax().item<double>();
    std::cout << std::left << std::setw(10) << n << std::scientific << std::setprecision(2) << std::setw(16) << worst;
    if (previous > 0.0) std::cout << std::fixed << std::setprecision(2) << previous / worst;
    std::cout << "\n";
    previous = worst;
  }
}
```

Output

```text
points    worst error     ratio
5         7.04e-02        
9         1.88e-02        3.73
17        4.79e-03        3.93
33        1.20e-03        3.99
65        3.01e-04        4.00
```
The error of linear interpolation of a smooth function falls by a factor of 4 each time the spacing halves: second order, proportional to h² and to the function's curvature. Choose the table spacing from the accuracy you need and the curvature of the quantity; where the function bends more, use closer points there.
