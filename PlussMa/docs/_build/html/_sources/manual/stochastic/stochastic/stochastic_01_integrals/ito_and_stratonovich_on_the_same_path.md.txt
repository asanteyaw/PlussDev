<!-- generated from ito_and_stratonovich_on_the_same_path.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Ito and Stratonovich on the same path

[Back to Stochastic Integrals](../stochastic_01_integrals.md)


Both integrals of `W` against `dW` are taken along the same path and compared with the exact discrete identities. Ito's sum is `(W² − QV)/2`, where `QV` is the quadratic variation of the sampled path; Stratonovich's is `W²/2`; their difference is `QV/2`.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const pluss::Series like = pluss::make_series(0.0).to(f64).reshape({1});
  auto w_of_t = [](double, const pluss::Series& w) { return w; };  // the integrand f(t, W_t) = W_t

  std::cout << std::fixed << std::setprecision(5);
  std::cout << "steps      Ito     Stratonovich   (W^2 - QV)/2   W^2/2     QV/2    Strat - Ito\n";
  for (const std::int64_t steps : {10, 100, 1000, 10000}) {
    const auto path = sde::simulate_wiener_path(like, 0.0, 1.0, steps, /*seed=*/7);
    const auto ito = sde::ito_integral(w_of_t, path);
    const auto strat = sde::strat_integral(w_of_t, path);
    const double wt = path.w.select(0, steps).item<double>();
    const double qv = ito.realized_quadratic_variation.item<double>();
    std::cout << std::setw(6) << steps << "  " << std::setw(8) << ito.value.item<double>() << "   "
              << std::setw(8) << strat.value.item<double>() << "     " << std::setw(8) << 0.5 * (wt * wt - qv)
              << "   " << std::setw(8) << 0.5 * wt * wt << "  " << std::setw(8) << 0.5 * qv << "  "
              << std::setw(8) << strat.value.item<double>() - ito.value.item<double>() << "\n";
  }

  const auto path = sde::simulate_wiener_path(like, 0.0, 1.0, 10000, /*seed=*/7);
  const double wt = path.w.select(0, 10000).item<double>();
  const double ito = sde::ito_integral(w_of_t, path).value.item<double>();
  std::cout << "\nat 10000 steps: Ito = " << ito << ", (W_T^2 - T)/2 = " << 0.5 * (wt * wt - 1.0)
            << "  (the continuous-time Ito value)\n";
  return 0;
}
```

Output

```text
steps      Ito     Stratonovich   (W^2 - QV)/2   W^2/2     QV/2    Strat - Ito
    10  -0.26408    0.19663     -0.26408    0.19663   0.46070   0.46070
   100   0.94868    1.38484      0.94868    1.38484   0.43616   0.43616
  1000  -0.25604    0.25568     -0.25604    0.25568   0.51171   0.51171
 10000   1.37809    1.88336      1.37809    1.88336   0.50527   0.50527

at 10000 steps: Ito = 1.37809, (W_T^2 - T)/2 = 1.38336  (the continuous-time Ito value)
```
The first two columns equal the third and fourth to all printed digits at every step count, and the last two columns agree: the identities are exact for the sampled path, not approximate. What changes with the step count is only `QV`, which approaches 1, so `QV/2` approaches 0.5. At 10000 steps the Ito sum is 1.37809 and the continuous-time value `(W_T² − T)/2` is 1.38336; they differ because the sampled `QV` is 1.0105, not exactly 1. The Stratonovich value `W²/2` has no such gap, which is why it is the one that obeys the ordinary chain rule.
