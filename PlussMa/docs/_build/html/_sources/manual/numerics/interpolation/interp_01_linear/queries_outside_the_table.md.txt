<!-- generated from queries_outside_the_table.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Queries outside the table

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
  const Series x = vec({1.0, 2.0, 3.0});
  const Series y = vec({2.0, 4.0, 6.0});
  const Series q = vec({0.0, 2.0, 5.0});
  std::cout << std::fixed << std::setprecision(1);
  for (auto mode : {interp::EdgeMode::Clamp, interp::EdgeMode::Linear}) {
    const auto v = interp::linear(x, y, q, mode).host<double>();
    std::cout << std::left << std::setw(8) << (mode == interp::EdgeMode::Clamp ? "Clamp" : "Linear") << ": " << v[0] << ", " << v[1] << ", " << v[2] << "\n";
  }
  try {
    (void)interp::linear(x, y, q, interp::EdgeMode::Error);
  } catch (const std::out_of_range& e) {
    std::cout << "Error   : out_of_range thrown\n";
  }
  const auto inside = interp::linear(x, y, vec({1.5, 2.5}), interp::EdgeMode::Error).host<double>();
  std::cout << "Error mode inside the table: " << inside[0] << ", " << inside[1] << "\n";
}
```

Output

```text
Clamp   : 2.0, 4.0, 6.0
Linear  : 0.0, 4.0, 10.0
Error   : out_of_range thrown
Error mode inside the table: 3.0, 5.0
```
Choose by what an out-of-range query means. `Clamp` is right when the quantity really levels off at the ends and quietly hides a mistake otherwise. `Linear` extrapolates the trend and is only as good as the data's linearity. `Error` finds queries that should not happen; use it while developing and in code where the table must cover every query.
