<!-- generated from infinite_limits.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Infinite limits

[Back to Beyond One Interval](../integration_02_beyond_one_interval.md)


Pass `±infinity` as a limit. `quadra` rewrites the integral on a finite interval by a change of variables and integrates that. The far tails are cut at `infinite_domain_epsilon` (default 1e-10), which sets the accuracy limit.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <complex>

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
const double pi = 3.14159265358979323846;
const char* rule_name(integ::QuadraRule r) {
  switch (r) { case integ::QuadraRule::Trapezoidal: return "Trapezoidal"; case integ::QuadraRule::Simpson: return "Simpson"; case integ::QuadraRule::GaussLegendre: return "GaussLegendre"; default: return "Auto"; }
}
int main() {
  const double inf = std::numeric_limits<double>::infinity();
  std::cout << std::setprecision(12);
  {
    integ::IntegrandFn g = [](const Series& x) { return (x * x * -1.0).exp(); };
    const auto r = integ::quadra(g, -inf, inf);
    std::cout << "gaussian, (-inf, inf)  : " << r.value.item<double>() << "  (exact " << std::sqrt(pi) << ")  converged " << r.converged << "\n";
  }
  {
    integ::IntegrandFn e = [](const Series& x) { return (x * -1.0).exp(); };
    const auto r = integ::quadra(e, 0.0, inf);
    std::cout << "exp(-x), [0, inf)      : " << r.value.item<double>() << "  (exact 1)  converged " << r.converged << "\n";
  }
  {
    integ::IntegrandFn c = [](const Series& x) {
      const Series damp = (x * -1.0).exp();
      return pluss::series::complex(damp * x.cos(), damp * x.sin());
    };
    const auto r = integ::quadra(c, 0.0, inf);
    const auto v = r.value.item<std::complex<double>>();
    std::cout << "exp(-x) e^{ix}, [0,inf): " << v.real() << " + " << v.imag() << "i  (exact 0.5 + 0.5i)\n";
  }
}
```

Output

```text
gaussian, (-inf, inf)  : 1.77245385091  (exact 1.77245385091)  converged 1
exp(-x), [0, inf)      : 1  (exact 1)  converged 1
exp(-x) e^{ix}, [0,inf): 0.499999999932 + 0.500000000087i  (exact 0.5 + 0.5i)
```
Complex integrands work the same way: the real and imaginary parts are integrated together.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="batches_and_derivatives.html">
    <span class="label">Next</span>
    <span class="title">Batches and derivatives</span>
  </a>
</div>
<!-- apiref:nav:end -->
