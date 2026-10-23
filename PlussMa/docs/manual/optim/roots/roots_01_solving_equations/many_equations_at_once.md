<!-- generated from many_equations_at_once.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Many equations at once

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
  const Series k = vec({0.5, 2.0, 6.0, 24.0, 60.0, 120.0});
  const roots::Fn f = [&](const Series& x) { return x.pow(3.0) - x - k; };
  const auto r = roots::illinois(f, k * 0.0 + 1.0, k * 0.0 + 10.0, {200, 0.0, 1e-14, 8});
  const auto x = r.root.to_vec<double>();
  const auto kv = k.to_vec<double>();
  const auto res = r.residual.to_vec<double>();
  std::cout << std::left << std::setw(8) << "k" << std::setw(20) << "root" << "residual\n";
  for (std::size_t i = 0; i < x.size(); ++i)
    std::cout << std::left << std::fixed << std::setprecision(1) << std::setw(8) << kv[i] << std::setprecision(12) << std::setw(20) << x[i] << std::scientific << std::setprecision(1) << res[i] << "\n";
  std::cout << "iterations: " << r.iterations << " (all lanes together)\n";
}
```

Output

```text
k       root                residual
0.5     1.191487883953      4.4e-16
2.0     1.521379706805      0.0e+00
6.0     2.000000000000      0.0e+00
24.0    3.000000000000      0.0e+00
60.0    4.000000000000      0.0e+00
120.0   5.000000000000      0.0e+00
iterations: 16 (all lanes together)
```
One call, six different cubics, one set of array operations per iteration. The loop stops when the *slowest* lane is done, so every lane gets the same iteration count. This is the form to use for a sweep or a calibration of many instruments; there is no per-lane host loop to pay for.
