<!-- generated from tolerance_and_honest_failure.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Tolerance and honest failure

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
  integ::IntegrandFn f = [](const Series& x) { return x.sqrt(); };
  std::cout << std::left << std::setw(34) << "setting" << std::setw(11) << "converged" << std::setw(13) << "evaluations" << std::setw(16) << "error estimate" << "actual error\n";
  auto run = [&](const char* label, integ::QuadraOptions o) {
    const auto r = integ::quadra(f, 0.0, 1.0, o);
    std::cout << std::left << std::setw(34) << label << std::setw(11) << (r.converged ? "yes" : "no") << std::setw(13) << r.evaluations << std::scientific
              << std::setprecision(1) << std::setw(16) << r.error_estimate << std::abs(r.value.item<double>() - 2.0 / 3.0) << std::defaultfloat << "\n";
  };
  integ::QuadraOptions simpson;
  simpson.rule = integ::QuadraRule::Simpson;
  run("Simpson, tolerance 1e-10", simpson);
  auto loose = simpson; loose.tolerance = 1e-6;
  run("Simpson, tolerance 1e-6", loose);
  auto more = simpson; more.max_refinements = 16;
  run("Simpson, 16 refinements allowed", more);
  integ::QuadraOptions gl;
  gl.rule = integ::QuadraRule::GaussLegendre;
  run("GaussLegendre, tolerance 1e-10", gl);
  integ::QuadraOptions automatic;
  automatic.rule = integ::QuadraRule::Auto;
  run("Auto (tries Gauss-Legendre first)", automatic);
  auto fixed = simpson; fixed.adaptive = false; fixed.nodes = 1025;
  run("Simpson, fixed 1025 nodes", fixed);
}
```

Output

```text
setting                           converged  evaluations  error estimate  actual error
Simpson, tolerance 1e-10          no         526091       1.1e-09         6.0e-10
Simpson, tolerance 1e-6           yes        7973         5.6e-07         3.1e-07
Simpson, 16 refinements allowed   yes        4210446      4.9e-11         2.7e-11
GaussLegendre, tolerance 1e-10    no         327520       1.9e-10         1.1e-10
Auto (tries Gauss-Legendre first) no         1379701      2.8e-09         1.5e-09
Simpson, fixed 1025 nodes         yes        1027         inf             2.5e-06
```
The integral of `sqrt(x)` has an infinite slope at 0, which slows every rule. Under the default settings Simpson, Gauss–Legendre and `Auto` end with `converged` false, yet their values are accurate to 1e-9: the estimate sits just above the 1e-10 tolerance. Read `converged` together with `error_estimate`: a loose tolerance (1e-6) converges cheaply, a larger refinement allowance converges at great cost, and a fixed node count (`adaptive` off) gives no estimate at all (`inf`) but, here, still 2.5e-6 accuracy. `Auto` tries Gauss–Legendre, then Simpson, then the trapezoid rule, so on a failing integrand it spends the effort of all three.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="the_rules.html">
    <span class="label">Previous</span>
    <span class="title">The rules</span>
  </a>
</div>
<!-- apiref:nav:end -->
