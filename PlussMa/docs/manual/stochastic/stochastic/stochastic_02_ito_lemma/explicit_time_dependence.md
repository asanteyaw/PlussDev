<!-- generated from explicit_time_dependence.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Explicit time dependence

[Back to Differentials and Ito's Lemma](../stochastic_02_ito_lemma.md)


When `g` depends on time as well, use `ito_at(time, g, mu, sigma)`: `g` receives the time as its first argument. The program discounts a process, `g(t, x) = e^(−rt) x`. Its drift is `e^(−rt)(μ − r x)`, which is zero exactly when `μ = r x`: the risk-neutral asset has no drift once discounted.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const double r = 0.03, t = 0.8;
  const pluss::Series x = pluss::make_series(std::vector<double>{100.0, 80.0}, {2}).to(f64);
  // A traded asset with growth rate mu = r x (the risk-neutral drift) and one with a higher growth rate.
  const pluss::Series mu = pluss::make_series(std::vector<double>{r * 100.0, 0.07 * 80.0}, {2}).to(f64);
  const pluss::Series sigma = x.mul(0.2);

  const auto d = x.calc.diff.ito_at(pluss::make_series(t).to(f64),
                                    [r](const pluss::Series& time, const pluss::Series& s) { return s.mul(time.mul(-r).exp()); },
                                    mu, sigma);
  const auto drift = d.drift.to_vec<double>();
  const auto diffusion = d.diffusion.to_vec<double>();
  std::cout << std::fixed << std::setprecision(5);
  std::cout << "discounted drift:  " << drift[0] << "   " << drift[1] << "\n";
  std::cout << "exp(-r t)(mu - r x): " << std::exp(-r * t) * (r * 100.0 - r * 100.0) << "   "
            << std::exp(-r * t) * (0.07 * 80.0 - r * 80.0) << "\n";
  std::cout << "discounted diffusion: " << diffusion[0] << "   " << diffusion[1] << "   (exp(-r t) sigma = "
            << std::exp(-r * t) * 20.0 << ", " << std::exp(-r * t) * 16.0 << ")\n";
  return 0;
}
```

Output

```text
discounted drift:  0.00000   3.12411
exp(-r t)(mu - r x): 0.00000   3.12411
discounted diffusion: 19.52571   15.62057   (exp(-r t) sigma = 19.52571, 15.62057)
```
The first state is risk-neutral and has discounted drift 0.00000; the second has 3.12411, equal to `e^(−rt)(μ − r x)`. The discounted diffusion equals `e^(−rt) σ`.
