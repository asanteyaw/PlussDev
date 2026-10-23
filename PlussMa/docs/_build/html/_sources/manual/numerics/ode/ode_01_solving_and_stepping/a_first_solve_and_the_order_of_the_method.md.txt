<!-- generated from a_first_solve_and_the_order_of_the_method.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A first solve, and the order of the method

[Back to Solving and Stepping](../ode_01_solving_and_stepping.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const ode::OdeFn decay = [](double, const Series& y, const Series&) { return y * -0.5; };
  const double exact = std::exp(-2.0);
  std::cout << std::left << std::setw(8) << "steps" << std::setw(22) << "y(4)" << std::setw(14) << "error" << "error ratio\n";
  double previous = 0.0;
  for (std::int64_t steps : {5, 10, 20, 40, 80}) {
    ode::OdeOptions o;
    o.steps = steps;
    const auto r = ode::fluxa(decay, vec({1.0}), 0.0, 4.0, pluss::series::undefined(), o);
    const double y = r.y_final.item<double>();
    const double err = std::abs(y - exact);
    std::cout << std::left << std::setw(8) << steps << std::setprecision(15) << std::fixed << std::setw(22) << y << std::scientific << std::setprecision(2) << std::setw(14) << err;
    if (previous > 0.0) std::cout << std::fixed << std::setprecision(1) << previous / err;
    std::cout << "\n";
    previous = err;
  }
}
```

Output

```text
steps   y(4)                  error         error ratio
5       0.135416014628182     8.07e-05      
10      0.135339548430510     4.27e-06      18.9
20      0.135335528421791     2.45e-07      17.4
40      0.135335297934204     1.47e-08      16.7
80      0.135335284136256     9.00e-10      16.3
```
Doubling the steps divides the error by about 16, the signature of a fourth-order method: error ∝ h⁴. The ratio drifts from 19 toward 16 as the steps get small enough for the asymptotic behaviour to set in. Use this halving test on any solver and any problem to confirm you are in the regime where the error estimate means what it says.
