<!-- generated from batches_and_derivatives.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Batches and derivatives

[Back to Beyond One Interval](../integration_02_beyond_one_interval.md)


If the integrand returns shape `[B, n]` for a node vector of length `n`, the integral runs along the last axis and returns `B` values; any number of leading batch axes is allowed. A parameter captured by the integrand and marked `tracks_slope_(true)` receives its derivative when you call `reverse()` on a scalar built from the result.

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
  Series k = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4, 1}).to(f64);
  k.tracks_slope_(true);
  integ::IntegrandFn f = [&](const Series& x) { return (x * x).reshape({1, x.numel()}) * k; };
  integ::QuadraOptions o;
  o.rule = integ::QuadraRule::GaussLegendre;
  const auto r = integ::quadra(f, 0.0, 1.0, o);
  const auto v = r.value.host<double>();
  std::cout << std::fixed << std::setprecision(6) << "I(k) = k/3   : " << v[0] << " " << v[1] << " " << v[2] << " " << v[3] << "   (shape " << r.value.numel() << ")\n";
  r.value.sum().reverse();
  const auto g = k.sensitivity().host<double>();
  std::cout << "dI/dk each   : " << g[0] << " " << g[1] << " " << g[2] << " " << g[3] << "   (exact 0.333333)\n";
}
```

Output

```text
I(k) = k/3   : 0.333333 0.666667 1.000000 1.333333   (shape 4)
dI/dk each   : 0.333333 0.333333 0.333333 0.333333   (exact 0.333333)
```
This is how an integral enters a model that is being fitted: a distribution's parameters inside a likelihood integral, for example, are differentiated through the quadrature with no extra code.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="infinite_limits.html">
    <span class="label">Previous</span>
    <span class="title">Infinite limits</span>
  </a>
  <a class="next-link" href="integrals_over_a_box.html">
    <span class="label">Next</span>
    <span class="title">Integrals over a box</span>
  </a>
</div>
<!-- apiref:nav:end -->
