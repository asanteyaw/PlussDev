<!-- generated from heat_flow.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Heat flow

[Back to General Systems and Exponentials](../krylov_02_general_systems_and_exponentials.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace krylov = pluss::subsp::math::krylov;
namespace lo = pluss::subsp::math::linear_operator;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
Series ones(std::vector<std::int64_t> shape) { return Series::zeros(shape, f64) + 1.0; }
// the 1-D second-difference operator (2 on the diagonal, -1 beside it) applied to the last axis, without forming a matrix
Series shift_in(const Series& x, bool from_left) {
  const std::int64_t n = x.size(-1);
  const Series zero = pluss::series::zeros_like(x.slice(-1, 0, 1, 1));
  return from_left ? zero.unite({x.slice(-1, 0, n - 1, 1)}, -1) : x.slice(-1, 1, n, 1).unite({zero}, -1);
}
Series laplacian(const Series& x) { return x * 2.0 - shift_in(x, true) - shift_in(x, false); }
int main() {
  const std::int64_t n = 200;
  const double pi = 3.14159265358979323846;
  const double t = 20.0;
  const std::int64_t center = 100;
  std::vector<double> spike(n, 0.0), exact(n, 0.0);
  spike[center - 1] = 1.0;
  for (std::int64_t j = 1; j <= n; ++j)
    for (std::int64_t k = 1; k <= n; ++k)
      exact[j - 1] += std::exp(-t * (2.0 - 2.0 * std::cos(pi * k / (n + 1)))) * std::sin(pi * k * j / (n + 1)) * std::sin(pi * k * center / (n + 1)) * 2.0 / (n + 1);
  std::cout << std::left << std::setw(12) << "basis size" << std::setw(12) << "converged" << std::setw(18) << "residual est." << "error vs exact\n";
  for (std::int64_t size : {4, 8, 16, 32, 64}) {
    krylov::ExpOptions o;
    o.max_iterations = size;
    const auto r = krylov::exp_action(laplacian, vec(spike), {-1}, -t, o);
    const auto y = r.value.host<double>();
    double worst = 0.0;
    for (std::int64_t j = 0; j < n; ++j) worst = std::max(worst, std::abs(y[j] - exact[j]));
    std::cout << std::left << std::setw(12) << size << std::setw(12) << r.converged.squeeze().item<bool>() << std::scientific << std::setprecision(1) << std::setw(18) << r.residual_estimate.squeeze().item<double>() << worst << "\n";
  }
}
```

Output

```text
basis size  converged   residual est.     error vs exact
4           0           6.4e-03           5.2e-02
8           0           1.6e-02           2.8e-02
16          0           2.9e-03           2.6e-03
32          0           6.2e-07           2.7e-07
64          1           1.0e-20           5.2e-16
```
The exact answer is known from the eigen-expansion, so the table shows the accuracy honestly. The error falls steeply with the size of the Krylov space, from 5e-2 at 4 vectors to 5e-16 at 64, and `converged` flips only when the estimate meets the tolerance (at 64). The estimate tracks the true error, which makes it usable when no exact answer exists: grow `max_iterations` until it passes. Evolution to a longer time or a rougher start needs a larger space; for a long evolution, advance it in several shorter steps.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="the_restart_length.html">
    <span class="label">Previous</span>
    <span class="title">The restart length</span>
  </a>
</div>
<!-- apiref:nav:end -->
