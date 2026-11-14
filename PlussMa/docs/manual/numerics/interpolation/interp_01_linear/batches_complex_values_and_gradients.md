<!-- generated from batches_complex_values_and_gradients.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Batches, complex values and gradients

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
  const Series x = vec({0.0, 1.0, 2.0, 3.0});
  const Series curves = pluss::make_series(std::vector<double>{0, 1, 4, 9, 0, 2, 4, 6, 1, 1, 1, 1}, {3, 4}).to(f64);
  const auto q = vec({0.5, 2.25});
  const auto r = interp::linear(x, curves, q);
  const auto v = r.host<double>();
  std::cout << "shape [" << r.size(0) << ", " << r.size(1) << "]\n";
  for (int i = 0; i < 3; ++i) std::cout << "curve " << i << ": " << v[2 * i] << ", " << v[2 * i + 1] << "\n";

  const Series z = pluss::make_series(std::vector<std::complex<double>>{{0, 0}, {1, 2}, {2, 4}, {3, 6}}, {4}).to(pluss::ScalarType::Complex128);
  const auto c = interp::linear(x, z, pluss::make_series(1.5).to(f64)).item<std::complex<double>>();
  std::cout << "complex values at 1.5: " << c.real() << " + " << c.imag() << "i\n";

  Series tracked = pluss::make_series(std::vector<double>{0, 1, 4, 9}, {4}).to(f64);
  tracked.tracks_slope_(true);
  interp::linear(x, tracked, pluss::make_series(2.25).to(f64)).reverse();
  const auto g = tracked.sensitivity().host<double>();
  std::cout << "d value(2.25) / d samples: " << g[0] << " " << g[1] << " " << g[2] << " " << g[3] << "   (weights 0.75 and 0.25 on the samples at 2 and 3)\n";
}
```

Output

```text
shape [3, 2]
curve 0: 0.5, 5.25
curve 1: 1, 4.5
curve 2: 1, 1
complex values at 1.5: 1.5 + 3i
d value(2.25) / d samples: 0 0 0.75 0.25   (weights 0.75 and 0.25 on the samples at 2 and 3)
```
Each curve in the batch is read at the same queries in one call, and the result keeps the batch axes. Complex values are interpolated componentwise. The result is built from differentiable operations, so a Series marked `tracks_slope_(true)` receives its gradient: the interpolation weights of the two samples that bracket the query, and zero for all others. This makes an interpolated table usable inside a fitted model.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="queries_outside_the_table.html">
    <span class="label">Previous</span>
    <span class="title">Queries outside the table</span>
  </a>
  <a class="next-link" href="accuracy.html">
    <span class="label">Next</span>
    <span class="title">Accuracy</span>
  </a>
</div>
<!-- apiref:nav:end -->
