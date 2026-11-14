<!-- generated from convergence_on_geometric_brownian_motion.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Convergence on geometric Brownian motion

[Back to Simulating and Differentiating SDEs](../stochastic_04_simulating_sdes.md)


For `dX = μX dt + σX dW` the solution `x₀ exp((μ − σ²/2)T + σ W_T)` is known, and `fluctua` returns the realized `W_T`, so the exact value is available for every path. The program measures the mean absolute error over 20000 paths at several step counts.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const double mu = 0.2, sg = 0.5, x0 = 1.0, T = 1.0;
  const std::int64_t paths = 20000;
  const pluss::Series y0 = pluss::Series::full({paths}, x0, f64);

  std::cout << std::fixed << std::setprecision(5);
  std::cout << "steps   Euler-Maruyama   Milstein   Heun (Stratonovich form)\n";
  for (std::int64_t steps : {8, 16, 32, 64, 128}) {
    double err[3];
    for (int m = 0; m < 3; ++m) {
      sde::SdeOptions o;
      o.method = m == 0 ? sde::SdeMethod::EulerMaruyama : m == 1 ? sde::SdeMethod::Milstein : sde::SdeMethod::StratonovichHeun;
      o.steps = steps;
      o.seed = 5;  // the same seed gives every scheme the same noise at the same step count
      const double drift = m == 2 ? mu - 0.5 * sg * sg : mu;
      const auto run = sde::fluctua([=](double, const pluss::Series& y, const pluss::Series&) { return y.mul(drift); },
                                    [=](double, const pluss::Series& y, const pluss::Series&) { return y.mul(sg); }, y0, 0.0, T,
                                    pluss::series::undefined(), o);
      const pluss::Series exact = run.wiener_final.mul(sg).add((mu - 0.5 * sg * sg) * T).exp().mul(x0);
      err[m] = run.y_final.sub(exact).abs().mean().item<double>();
    }
    std::cout << std::setw(5) << steps << "   " << std::setw(14) << err[0] << "   " << std::setw(8) << err[1] << "   " << err[2] << "\n";
  }
  return 0;
}
```

Output

```text
steps   Euler-Maruyama   Milstein   Heun (Stratonovich form)
    8          0.05904    0.01280   0.00862
   16          0.04244    0.00671   0.00465
   32          0.03049    0.00340   0.00240
   64          0.02127    0.00174   0.00124
  128          0.01521    0.00087   0.00062
```
Euler-Maruyama's error falls from 0.05904 at 8 steps to 0.01521 at 128, a factor of 3.9 for 16 times more steps, close to the predicted `√16 = 4`. Milstein falls from 0.01280 to 0.00087, a factor of 14.7, close to 16; Heun falls from 0.00862 to 0.00062, a factor of 13.9. At 128 steps Milstein and Heun are 17 and 24 times more accurate than Euler-Maruyama. When the volatility depends on the state, Milstein is worth its extra derivative.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="pathwise_delta.html">
    <span class="label">Next</span>
    <span class="title">Pathwise delta</span>
  </a>
</div>
<!-- apiref:nav:end -->
