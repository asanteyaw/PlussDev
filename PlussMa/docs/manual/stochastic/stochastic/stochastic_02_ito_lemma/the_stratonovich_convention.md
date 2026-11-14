<!-- generated from the_stratonovich_convention.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The Stratonovich convention

[Back to Differentials and Ito's Lemma](../stochastic_02_ito_lemma.md)


The same geometric Brownian motion is `dX = μX dt + σX dW` in Ito form and `dX = (μ − σ²/2) X dt + σX ∘ dW` in Stratonovich form. `strat_drift` converts the drift one way and `ito_drift` the other; the pair is an exact round trip. The program then computes `d log X` by both routes.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const double mu_c = 0.12, sg = 0.4;
  const pluss::Series x = pluss::make_series(std::vector<double>{0.5, 1.0, 2.0}, {3}).to(f64);
  const pluss::Series mu = x.mul(mu_c);
  const auto sigma_of = [sg](const pluss::Series& s) { return s.mul(sg); };  // sigma(x) = 0.4 x

  const pluss::Series mu_strat = x.calc.diff.strat_drift(mu, sigma_of);
  const pluss::Series back = x.calc.diff.ito_drift(mu_strat, sigma_of);
  std::cout << std::fixed << std::setprecision(5);
  std::cout << "Ito drift            " << mu.host<double>()[0] << " " << mu.host<double>()[1] << " " << mu.host<double>()[2] << "\n";
  std::cout << "Stratonovich drift   " << mu_strat.host<double>()[0] << " " << mu_strat.host<double>()[1] << " " << mu_strat.host<double>()[2]
            << "   (mu x - sigma^2 x / 2: " << (mu_c - 0.5 * sg * sg) * 0.5 << ", " << (mu_c - 0.5 * sg * sg) * 1.0 << ", "
            << (mu_c - 0.5 * sg * sg) * 2.0 << ")\n";
  std::cout << "converted back       " << back.host<double>()[0] << " " << back.host<double>()[1] << " " << back.host<double>()[2] << "\n\n";

  const auto g = [](const pluss::Series& s) { return s.log(); };
  const auto ito_route = x.calc.diff.ito(g, mu, x.mul(sg));
  const auto strat_route = x.calc.diff.strat(g, mu_strat, x.mul(sg));
  std::cout << "d log X, Ito's lemma:           drift " << ito_route.drift.host<double>()[0] << ", diffusion "
            << ito_route.diffusion.host<double>()[0] << "\n";
  std::cout << "d log X, ordinary chain rule:   drift " << strat_route.drift.host<double>()[0] << ", diffusion "
            << strat_route.diffusion.host<double>()[0] << "\n";
  std::cout << "both equal mu - sigma^2/2 = " << mu_c - 0.5 * sg * sg << " and sigma = " << sg << "\n";
  return 0;
}
```

Output

```text
Ito drift            0.06000 0.12000 0.24000
Stratonovich drift   0.02000 0.04000 0.08000   (mu x - sigma^2 x / 2: 0.02000, 0.04000, 0.08000)
converted back       0.06000 0.12000 0.24000

d log X, Ito's lemma:           drift 0.04000, diffusion 0.40000
d log X, ordinary chain rule:   drift 0.04000, diffusion 0.40000
both equal mu - sigma^2/2 = 0.04000 and sigma = 0.40000
```
The converted drifts are 0.02, 0.04, 0.08 for the three states, equal to `(μ − σ²/2) x`, and converting back recovers 0.06, 0.12, 0.24. Both routes for `log X` give drift 0.04 and diffusion 0.4: Ito's lemma subtracts `σ²/2` explicitly, while the ordinary chain rule applied to the Stratonovich drift needs no extra term.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="explicit_time_dependence.html">
    <span class="label">Previous</span>
    <span class="title">Explicit time dependence</span>
  </a>
  <a class="next-link" href="checking_a_prediction_by_simulation.html">
    <span class="label">Next</span>
    <span class="title">Checking a prediction by simulation</span>
  </a>
</div>
<!-- apiref:nav:end -->
