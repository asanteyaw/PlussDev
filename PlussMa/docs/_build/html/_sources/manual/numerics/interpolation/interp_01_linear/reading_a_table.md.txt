<!-- generated from reading_a_table.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Reading a table

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
  const Series minutes = vec({0.0, 2.0, 5.0, 10.0, 20.0});
  const Series temperature = vec({90.0, 78.0, 66.0, 52.0, 38.0});
  std::cout << std::fixed << std::setprecision(2);
  std::cout << "at 3.5 min : " << interp::linear(minutes, temperature, pluss::make_series(3.5).to(f64)).item<double>() << "\n";
  const auto several = interp::linear(minutes, temperature, vec({0.0, 1.0, 5.0, 7.5, 20.0})).to_vec<double>();
  std::cout << "at 0, 1, 5, 7.5, 20 min :";
  for (double v : several) std::cout << " " << v;
  std::cout << "\n";
}
```

Output

```text
at 3.5 min : 72.00
at 0, 1, 5, 7.5, 20 min : 90.00 84.00 66.00 59.00 38.00
```
A query exactly on a table point returns the table value, and the spacing is free to vary: the intervals here are 2, 3, 5 and 10 minutes wide.
