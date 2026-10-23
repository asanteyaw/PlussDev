<!-- generated from ito_s_lemma_for_three_functions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Ito's lemma for three functions

[Back to Differentials and Ito's Lemma](../stochastic_02_ito_lemma.md)


The program evaluates `g = x²`, `exp(x)` and `sin(x)` at the states `x = 0.5, 1, −0.3` with drifts `0.1, 0.2, −0.1` and volatilities `0.3, 0.5, 0.2`, and compares each result with the formula above.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  const std::vector<double> xv = {0.5, 1.0, -0.3}, mv = {0.1, 0.2, -0.1}, sv = {0.3, 0.5, 0.2};
  const pluss::Series x = vec(xv), mu = vec(mv), sigma = vec(sv);

  struct Row {
    const char* name;
    std::function<pluss::Series(const pluss::Series&)> g;
    std::function<double(double, double, double)> drift;      // closed form
    std::function<double(double, double, double)> diffusion;  // closed form
  };
  const std::vector<Row> rows = {
      {"x^2", [](const pluss::Series& s) { return s.mul(s); },
       [](double x, double m, double s) { return 2 * x * m + s * s; }, [](double x, double, double s) { return 2 * x * s; }},
      {"exp(x)", [](const pluss::Series& s) { return s.exp(); },
       [](double x, double m, double s) { return std::exp(x) * (m + 0.5 * s * s); },
       [](double x, double, double s) { return std::exp(x) * s; }},
      {"sin(x)", [](const pluss::Series& s) { return s.sin(); },
       [](double x, double m, double s) { return std::cos(x) * m - 0.5 * std::sin(x) * s * s; },
       [](double x, double, double s) { return std::cos(x) * s; }}};

  std::cout << std::fixed << std::setprecision(5);
  for (const auto& row : rows) {
    const auto d = x.calc.diff.ito(row.g, mu, sigma);
    const auto drift = d.drift.to_vec<double>();
    const auto diffusion = d.diffusion.to_vec<double>();
    double worst = 0.0;
    std::cout << "g(x) = " << row.name << "\n";
    for (std::size_t i = 0; i < xv.size(); ++i) {
      const double want_drift = row.drift(xv[i], mv[i], sv[i]);
      const double want_diffusion = row.diffusion(xv[i], mv[i], sv[i]);
      worst = std::max({worst, std::abs(drift[i] - want_drift), std::abs(diffusion[i] - want_diffusion)});
      std::cout << "  x = " << std::setw(5) << xv[i] << "   drift " << std::setw(8) << drift[i] << "   diffusion "
                << std::setw(8) << diffusion[i] << "\n";
    }
    std::cout << "  largest difference from the closed form: " << std::scientific << std::setprecision(1) << worst
              << std::fixed << std::setprecision(5) << "\n";
  }
  return 0;
}
```

Output

```text
g(x) = x^2
  x = 0.50000   drift  0.19000   diffusion  0.30000
  x = 1.00000   drift  0.65000   diffusion  1.00000
  x = -0.30000   drift  0.10000   diffusion -0.12000
  largest difference from the closed form: 0.0e+00
g(x) = exp(x)
  x = 0.50000   drift  0.23906   diffusion  0.49462
  x = 1.00000   drift  0.88344   diffusion  1.35914
  x = -0.30000   drift -0.05927   diffusion  0.14816
  largest difference from the closed form: 1.1e-16
g(x) = sin(x)
  x = 0.50000   drift  0.06618   diffusion  0.26327
  x = 1.00000   drift  0.00288   diffusion  0.27015
  x = -0.30000   drift -0.08962   diffusion  0.19107
  largest difference from the closed form: 3.0e-18
```
The difference from the closed form is at most `1.1e-16`, which is rounding. For `x²` the lemma has the familiar form: drift `2xμ + σ²`, diffusion `2xσ`. At `x = 1` the drift is 0.65000 and the diffusion 1.00000.
