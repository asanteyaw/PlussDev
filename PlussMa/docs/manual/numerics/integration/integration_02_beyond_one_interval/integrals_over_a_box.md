<!-- generated from integrals_over_a_box.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Integrals over a box

[Back to Beyond One Interval](../integration_02_beyond_one_interval.md)


`quadra(fn, bounds)` takes an integrand of a vector of coordinate Series, one per axis, and a vector of `[lo, hi]` pairs. It builds the tensor-product rule: the cost is the per-axis resolution to the power of the dimension (`QuadraNdOptions`: `rule`, `resolution`, `gauss_points`, and a cap `max_total_nodes`). It suits two or three dimensions. `aleatoria` with the same signature estimates it by Monte Carlo, whose error does not depend on the dimension.

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
  std::cout << std::fixed << std::setprecision(8);
  integ::IntegrandNdFn xy = [](const std::vector<Series>& c) { return c[0] * c[1]; };
  const auto r2 = integ::quadra(xy, {{0.0, 1.0}, {0.0, 1.0}});
  std::cout << "2-D quadrature : " << r2.value.item<double>() << "  (exact 0.25000000), " << r2.evaluations << " evaluations\n";
  integ::IntegrandNdFn xyz = [](const std::vector<Series>& c) { return c[0] * c[1] * c[2]; };
  const auto r3 = integ::quadra(xyz, {{0.0, 1.0}, {0.0, 1.0}, {0.0, 1.0}});
  std::cout << "3-D quadrature : " << r3.value.item<double>() << "  (exact 0.12500000), " << r3.evaluations << " evaluations\n";
  integ::AleatoriaOptions mc;
  mc.samples = 200000;
  mc.seed = 7;
  const auto m = integ::aleatoria(xyz, std::vector<std::pair<double, double>>{{0.0, 1.0}, {0.0, 1.0}, {0.0, 1.0}}, mc);
  std::cout << "3-D Monte Carlo: " << m.value.item<double>() << " +- " << m.standard_error << ", " << m.samples << " samples\n";
}
```

Output

```text
2-D quadrature : 0.25000000  (exact 0.25000000), 625 evaluations
3-D quadrature : 0.12500000  (exact 0.12500000), 15625 evaluations
3-D Monte Carlo: 0.12500787 +- 0.00032820, 200000 samples
```
The 2-D rule is exact for this polynomial with 625 evaluations and the 3-D rule with 15625; the Monte Carlo estimate with 200000 samples is accurate to about 1e-5 with a standard error of 3e-4, which it reports. For a smooth integrand in few dimensions quadrature wins by orders of magnitude; the balance reverses as the dimension grows.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="batches_and_derivatives.html">
    <span class="label">Previous</span>
    <span class="title">Batches and derivatives</span>
  </a>
  <a class="next-link" href="monte_carlo.html">
    <span class="label">Next</span>
    <span class="title">Monte Carlo</span>
  </a>
</div>
<!-- apiref:nav:end -->
