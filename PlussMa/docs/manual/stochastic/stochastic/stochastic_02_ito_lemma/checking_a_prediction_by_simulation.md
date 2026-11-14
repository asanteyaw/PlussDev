<!-- generated from checking_a_prediction_by_simulation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Checking a prediction by simulation

[Back to Differentials and Ito's Lemma](../stochastic_02_ito_lemma.md)


Ito's lemma also predicts what simulation should find. For `g = x²` the drift divided by `g` is the growth rate of `E[X²]`, so `E[X_T²] = x₀² e^((2μ+σ²)T)`. The program reads the rate off the computed drift and compares the prediction with the mean of 60000 simulated squares.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const double mu_c = 0.15, sg = 0.35, x0 = 1.2, T = 1.0;

  // The drift of g(X) = X^2 from Ito's lemma, divided by g: the exponential growth rate of E[g(X)].
  const pluss::Series point = pluss::make_series(std::vector<double>{x0}, {1}).to(f64);
  const auto d = point.calc.diff.ito([](const pluss::Series& s) { return s.mul(s); }, point.mul(mu_c), point.mul(sg));
  const double rate = d.drift.squeeze().item<double>() / (x0 * x0);
  std::cout << std::fixed << std::setprecision(5);
  std::cout << "growth rate of E[X^2] from Ito's lemma: " << rate << "  (2 mu + sigma^2 = " << 2 * mu_c + sg * sg << ")\n";

  const std::int64_t paths = 60000;
  const pluss::Series y0 = pluss::Series::full({paths}, x0, f64);
  sde::SdeOptions options;
  options.method = sde::SdeMethod::Milstein;
  options.steps = 100;
  options.seed = 99;
  const auto run = sde::fluctua([&](double, const pluss::Series& y, const pluss::Series&) { return y.mul(mu_c); },
                                [&](double, const pluss::Series& y, const pluss::Series&) { return y.mul(sg); }, y0, 0.0, T,
                                pluss::series::undefined(), options);
  const pluss::Series squares = run.y_final.mul(run.y_final);
  const double mean = squares.mean().item<double>();
  const double stderr_ = std::sqrt(squares.sub(mean).pow(2.0).mean().item<double>() / paths);
  std::cout << "predicted E[X_T^2] = " << x0 * x0 * std::exp(rate * T) << "\n";
  std::cout << "simulated mean     = " << mean << "  (standard error " << stderr_ << ")\n";
  return 0;
}
```

Output

```text
growth rate of E[X^2] from Ito's lemma: 0.42250  (2 mu + sigma^2 = 0.42250)
predicted E[X_T^2] = 2.19711
simulated mean     = 2.19433  (standard error 0.00705)
```
The rate from the lemma is 0.42250, equal to `2μ + σ²`. The prediction 2.19711 and the simulated 2.19433 differ by 0.00278, less than the standard error 0.00705.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="the_stratonovich_convention.html">
    <span class="label">Previous</span>
    <span class="title">The Stratonovich convention</span>
  </a>
</div>
<!-- apiref:nav:end -->
