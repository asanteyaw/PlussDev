<!-- generated from integrating_data.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Integrating data

[Back to Data and Functions](../integration_01_data_and_functions.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
const double pi = 3.14159265358979323846;
const char* rule_name(integ::QuadraRule r) {
  switch (r) { case integ::QuadraRule::Trapezoidal: return "Trapezoidal"; case integ::QuadraRule::Simpson: return "Simpson"; case integ::QuadraRule::GaussLegendre: return "GaussLegendre"; default: return "Auto"; }
}
int main() {
  const Series x = vec({0, 1, 2, 3, 4});
  const Series y = x * x;
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "trapezoid(y, x)       : " << integ::trapezoid(y, x).item<double>() << "   (exact 21.3333)\n";
  std::cout << "trapezoid(y, dx = 1)  : " << integ::trapezoid(y, 1.0).item<double>() << "\n";
  const auto running = integ::cumulative_trapezoid(y, x).to_vec<double>();
  std::cout << "cumulative            :";
  for (double v : running) std::cout << " " << v;
  std::cout << "   (exact x^3/3: 0.3333 2.6667 9.0000 21.3333)\n";

  const Series fine_x = Series::arange(0.0, 4.0 + 1e-12, 0.05, f64);
  std::cout << "81 points             : " << integ::trapezoid(fine_x * fine_x, fine_x).item<double>() << "\n";

  const Series rows = pluss::make_series(std::vector<double>{0, 1, 4, 9, 1, 2, 5, 10}, {2, 4}).to(f64);
  const auto per_row = integ::trapezoid(rows, vec({0, 1, 2, 3})).to_vec<double>();
  std::cout << "two rows at once      : " << per_row[0] << ", " << per_row[1] << "\n";
}
```

Output

```text
trapezoid(y, x)       : 22.0000   (exact 21.3333)
trapezoid(y, dx = 1)  : 22.0000
cumulative            : 0.5000 3.0000 9.5000 22.0000   (exact x^3/3: 0.3333 2.6667 9.0000 21.3333)
81 points             : 21.3350
two rows at once      : 9.5000, 12.5000
```
The trapezoid rule joins samples by straight lines, so it overestimates a convex function such as `x²` (22 against 21.33); a finer grid cuts the error. `x` need not be equally spaced, and a 2-D `y` is integrated along its last axis, one row at a time. `cumulative_trapezoid` returns one value per interval (n-1 values for n samples).
