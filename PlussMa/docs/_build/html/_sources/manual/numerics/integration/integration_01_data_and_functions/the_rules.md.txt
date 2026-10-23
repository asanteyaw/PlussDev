<!-- generated from the_rules.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The rules

[Back to Data and Functions](../integration_01_data_and_functions.md)


`QuadraOptions` chooses the rule and the effort: `rule` (`Trapezoidal`, `Simpson`, `GaussLegendre`, `Auto`), `nodes` (starting count for the first two), `panels` and `gauss_points` (5 or 10; for Gauss–Legendre), `adaptive` (double the resolution until the error estimate meets `tolerance`), `tolerance` (default 1e-10) and `max_refinements` (default 10).

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
  struct Case { const char* name; integ::IntegrandFn f; double exact; };
  const Case cases[] = {
      {"e^x", [](const Series& x) { return x.exp(); }, std::exp(1.0) - 1.0},
      {"sin(20 x)", [](const Series& x) { return (x * 20.0).sin(); }, (1.0 - std::cos(20.0)) / 20.0},
      {"|x - 0.3|", [](const Series& x) { return (x - 0.3).abs(); }, 0.5 * 0.3 * 0.3 + 0.5 * 0.7 * 0.7},
  };
  std::cout << std::left << std::setw(14) << "integrand" << std::setw(16) << "rule" << std::setw(11) << "converged" << std::setw(13) << "evaluations" << "error\n";
  for (const auto& c : cases) {
    for (auto rule : {integ::QuadraRule::Trapezoidal, integ::QuadraRule::Simpson, integ::QuadraRule::GaussLegendre}) {
      integ::QuadraOptions o;
      o.rule = rule;
      const auto r = integ::quadra(c.f, 0.0, 1.0, o);
      std::cout << std::left << std::setw(14) << c.name << std::setw(16) << rule_name(rule) << std::setw(11) << (r.converged ? "yes" : "no")
                << std::setw(13) << r.evaluations << std::scientific << std::setprecision(1) << std::abs(r.value.item<double>() - c.exact) << std::defaultfloat << "\n";
    }
  }
}
```

Output

```text
integrand     rule            converged  evaluations  error
e^x           Trapezoidal     yes        131336       3.3e-11
e^x           Simpson         yes        774          1.4e-13
e^x           GaussLegendre   yes        480          0.0e+00
sin(20 x)     Trapezoidal     yes        526090       1.4e-11
sin(20 x)     Simpson         yes        3860         1.5e-12
sin(20 x)     GaussLegendre   yes        480          5.2e-17
|x - 0.3|     Trapezoidal     yes        131336       5.5e-11
|x - 0.3|     Simpson         yes        262922       6.2e-12
|x - 0.3|     GaussLegendre   yes        163680       1.4e-11
```
- On the **smooth** integrands, Gauss–Legendre needs 480 evaluations for full accuracy, Simpson hundreds to thousands, the trapezoid rule hundreds of thousands: the higher the order of the rule, the fewer nodes a given accuracy needs.
- On the **oscillatory** integrand the gap widens, since the trapezoid rule needs half a million evaluations.
- On the **kink** no rule is fast: all lose their order of accuracy at a point where the function is not smooth, and they converge only by brute-force refinement. If you know where the kink is, split the interval there and integrate each smooth piece.
