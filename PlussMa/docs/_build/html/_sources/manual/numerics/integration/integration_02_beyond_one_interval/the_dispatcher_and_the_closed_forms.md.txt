<!-- generated from the_dispatcher_and_the_closed_forms.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The dispatcher and the closed forms

[Back to Beyond One Interval](../integration_02_beyond_one_interval.md)


`integra(f, a, b, options)` tries quadrature first. If quadrature misses its tolerance, it also runs Monte Carlo and returns whichever answer has the smaller error estimate; `strategy_used` says which, and `converged` is true only if the chosen method met its target. `analytica` holds exact integrals of common families: `gaussian`, `gaussian_probability`, `gaussian_moment`, `polynomial`, `exponential`, `power`, `sine`, `cosine`, `lorentzian`, `hyperbolic_sine`, `hyperbolic_cosine`, `hyperbolic_tangent`, `sigmoid`, `arcsine_kernel`. It is called by name when you know the form, never chosen automatically.

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
  std::cout << std::setprecision(10);
  {
    integ::IntegrandFn f = [](const Series& x) { return x.exp(); };
    const auto r = integ::integra(f, 0.0, 1.0);
    std::cout << "smooth : " << r.value.item<double>() << "  strategy " << (r.strategy_used == integ::IntegraStrategy::Quadrature ? "Quadrature" : "MonteCarlo")
              << ", converged " << r.converged << "\n";
  }
  {
    integ::IntegrandFn step = [](const Series& x) { return x.gt(0.4).to(f64); };
    integ::IntegraOptions o;
    o.quadrature.max_refinements = 6;
    o.monte_carlo.samples = 400000;
    o.monte_carlo.seed = 5;
    const auto r = integ::integra(step, 0.0, 1.0, o);
    std::cout << "jump   : " << r.value.item<double>() << "  (exact 0.6)  strategy " << (r.strategy_used == integ::IntegraStrategy::Quadrature ? "Quadrature" : "MonteCarlo")
              << ", error estimate " << std::scientific << std::setprecision(1) << r.error_estimate << std::fixed << std::setprecision(10) << "\n";
  }
  integ::IntegrandFn g = [](const Series& x) { return (x * x * -0.5).exp(); };
  const double q = integ::quadra(g, -1.0, 2.0).value.item<double>() / std::sqrt(2.0 * pi);
  std::cout << "gaussian P(-1 < Z < 2): analytica " << integ::analytica::gaussian_probability(-1.0, 2.0) << ", quadrature " << q << "\n";
  std::cout << "polynomial 1 + 2x + 3x^2 over [0, 2]: " << integ::analytica::polynomial({1.0, 2.0, 3.0}, 0.0, 2.0) << "  (exact 14)\n";
  std::cout << "sine(k = 2) over [0, pi/2]: " << integ::analytica::sine(2.0, 0.0, pi / 2.0) << "  (exact 1)\n";
}
```

Output

```text
smooth : 1.718281828  strategy Quadrature, converged 1
jump   : 0.5999716278  (exact 0.6)  strategy Quadrature, error estimate 2.0e-05
gaussian P(-1 < Z < 2): analytica 0.8185946141, quadrature 0.8185946141
polynomial 1 + 2x + 3x^2 over [0, 2]: 14.0000000000  (exact 14)
sine(k = 2) over [0, pi/2]: 1.0000000000  (exact 1)
```
The jump cannot reach 1e-10 by quadrature, and the dispatcher compares the two error estimates and keeps the better answer (quadrature's 2e-5 beats Monte Carlo's). The closed forms agree with quadrature to ten digits and serve as independent checks.
