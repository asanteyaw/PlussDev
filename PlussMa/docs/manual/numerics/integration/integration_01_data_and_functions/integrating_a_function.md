<!-- generated from integrating_a_function.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Integrating a function

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
  integ::IntegrandFn f = [](const Series& x) { return x.exp(); };
  const auto r = integ::quadra(f, 0.0, 1.0);
  std::cout << std::setprecision(15) << "value       : " << r.value.item<double>() << "  (e - 1 = " << std::exp(1.0) - 1.0 << ")\n";
  std::cout << std::scientific << std::setprecision(2) << "error est.  : " << r.error_estimate << "\n";
  std::cout << "evaluations : " << r.evaluations << "\n";
  std::cout << "refinements : " << r.refinements << "\n";
  std::cout << "converged   : " << (r.converged ? "yes" : "no") << "\n";
  std::cout << "rule used   : " << rule_name(r.rule_used) << "\n";
}
```

Output

```text
value       : 1.71828182845918  (e - 1 = 1.71828182845905)
error est.  : 2.02e-12
evaluations : 774
refinements : 1
converged   : yes
rule used   : Simpson
```
`quadra` returns a `QuadraResult`:

| Field | Meaning |
|---|---|
| `value` | the integral, as a Series |
| `error_estimate` | estimated error, from comparing two resolutions |
| `evaluations` | integrand evaluations used, over all refinements |
| `refinements` | how many times the node count was doubled |
| `converged` | whether the error estimate reached `tolerance` |
| `rule_used` | the rule that produced `value` (Simpson by default) |

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="integrating_data.html">
    <span class="label">Previous</span>
    <span class="title">Integrating data</span>
  </a>
  <a class="next-link" href="the_rules.html">
    <span class="label">Next</span>
    <span class="title">The rules</span>
  </a>
</div>
<!-- apiref:nav:end -->
