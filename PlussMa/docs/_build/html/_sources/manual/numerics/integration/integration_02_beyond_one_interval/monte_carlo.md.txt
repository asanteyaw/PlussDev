<!-- generated from monte_carlo.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Monte Carlo

[Back to Beyond One Interval](../integration_02_beyond_one_interval.md)


`aleatoria(f, a, b, options)` averages the integrand over random points. `AleatoriaOptions` has `samples` (default 1,000,000), `seed`, `stratified` and `infinite_domain_epsilon`. The result carries `value`, `standard_error` and `samples`.

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
  integ::IntegrandFn f = [](const Series& x) { return x.sin(); };
  std::cout << std::left << std::setw(10) << "samples" << std::setw(24) << "plain: error / s.e." << std::setw(24) << "stratified: error / s.e." << "\n";
  for (std::int64_t n : {1000, 10000, 100000, 1000000}) {
    std::cout << std::left << std::setw(10) << n;
    for (bool strat : {false, true}) {
      integ::AleatoriaOptions o;
      o.samples = n;
      o.seed = 11;
      o.stratified = strat;
      const auto r = integ::aleatoria(f, 0.0, pi, o);
      std::ostringstream cell;
      cell << std::scientific << std::setprecision(1) << std::abs(r.value.item<double>() - 2.0) << " / " << r.standard_error;
      std::cout << std::left << std::setw(24) << cell.str();
    }
    std::cout << "\n";
  }
  integ::AleatoriaOptions same;
  same.samples = 5000; same.seed = 3;
  const double a = integ::aleatoria(f, 0.0, pi, same).value.item<double>();
  const double b = integ::aleatoria(f, 0.0, pi, same).value.item<double>();
  std::cout << "same seed twice: " << std::setprecision(12) << std::fixed << a << ", " << b << "\n";
}
```

Output

```text
samples   plain: error / s.e.     stratified: error / s.e.
1000      4.4e-02 / 3.0e-02       1.7e-04 / 3.1e-02       
10000     2.0e-03 / 9.7e-03       1.1e-06 / 9.7e-03       
100000    6.9e-04 / 3.1e-03       1.4e-07 / 3.1e-03       
1000000   9.3e-04 / 9.7e-04       8.7e-10 / 9.7e-04       
same seed twice: 2.012396034089, 2.012396034089
```
The plain estimator's error shrinks like one over the square root of the samples (a hundred times the samples for ten times the accuracy), and the reported standard error tracks the actual error. Stratified sampling places one point per stratum and is far more accurate on a smooth integrand at the same cost, but the standard error it reports is the plain formula and so overstates its real error; treat it as an upper bound. The same seed gives the same answer.
