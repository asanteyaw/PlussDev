<!-- generated from several_series_with_a_constant_correlation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Several series with a constant correlation

[Back to Volatility Models](../ts_03_volatility_models.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
std::vector<double> shocks(int n, int offset) {
  std::vector<double> z(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    z[static_cast<std::size_t>(i)] = (u - 0.5) * std::sqrt(12.0);
  }
  return z;
}
int main() {
  const int n = 1500;
  const auto z1 = shocks(n, 0), w = shocks(n, 40000);
  std::vector<double> y(static_cast<std::size_t>(2 * n));
  double h1 = 0.4, h2 = 0.4, p1 = 0.0, p2 = 0.0;
  for (int t = 0; t < n; ++t) {
    h1 = 0.05 + 0.15 * p1 * p1 + 0.75 * h1;
    h2 = 0.10 + 0.10 * p2 * p2 + 0.80 * h2;
    const double e1 = z1[static_cast<std::size_t>(t)], e2 = 0.6 * e1 + 0.8 * w[static_cast<std::size_t>(t)];
    p1 = std::sqrt(h1) * e1;
    p2 = std::sqrt(h2) * e2;
    y[static_cast<std::size_t>(2 * t)] = p1;
    y[static_cast<std::size_t>(2 * t + 1)] = p2;
  }
  const Series Y = pluss::make_series(y, {n, 2}).to(f64);

  const auto fit = ts::MvGarch::fit_mle(Y);
  std::cout << std::fixed << std::setprecision(3);
  for (std::size_t j = 0; j < fit.marginals.size(); ++j)
    std::cout << "series " << j << ": omega " << fit.marginals[j].omega << "  alpha " << fit.marginals[j].arch[0] << "  beta " << fit.marginals[j].garch[0] << "\n";
  const auto corr = fit.corr.to_vec<double>();
  std::cout << "estimated correlation " << corr[1] << "   (true 0.600)\n";
  std::cout << "joint log-likelihood " << std::setprecision(2) << fit.loglik(Y) << "\n";

  // Without the correlation (an identity matrix) the same marginals fit worse.
  auto independent = fit;
  independent.corr = pluss::series::eye(2, 2).to(f64);
  std::cout << "log-likelihood with the correlation set to zero " << independent.loglik(Y) << "\n\n";

  std::cout << std::setprecision(3) << "forecast of the variances and of the covariance implied by the correlation\n";
  const auto f = fit.forecast(5, Y).to_vec<double>();
  for (int h = 0; h < 5; ++h) {
    const double v1 = f[static_cast<std::size_t>(2 * h)], v2 = f[static_cast<std::size_t>(2 * h + 1)];
    std::cout << "  step " << h + 1 << "   var0 " << v1 << "   var1 " << v2 << "   cov " << corr[1] * std::sqrt(v1 * v2) << "\n";
  }
}
```

Output

```text
series 0: omega 0.072  alpha 0.138  beta 0.712
series 1: omega 0.107  alpha 0.093  beta 0.804
estimated correlation 0.612   (true 0.600)
joint log-likelihood -3338.32
log-likelihood with the correlation set to zero -3690.42

forecast of the variances and of the covariance implied by the correlation
  step 1   var0 0.530   var1 0.676   cov 0.366
  step 2   var0 0.522   var1 0.714   cov 0.374
  step 3   var0 0.516   var1 0.747   cov 0.380
  step 4   var0 0.510   var1 0.777   cov 0.385
  step 5   var0 0.505   var1 0.804   cov 0.390
```
Each series has its own GARCH(1,1) (the estimates are close to the generating 0.05/0.15/0.75 and 0.10/0.10/0.80) and the estimated correlation, 0.612, is close to the true 0.6. The joint model is far better than the same marginals with zero correlation (log-likelihood −3338 against −3690). The forecasts are of the two variances; the covariance implied by the constant correlation is `corr × sqrt(var0 × var1)`, which rises with the variances. The correlation is a constant, estimated from the standardised residuals after the marginals are fitted (`MvGarchOptions::corr_shrinkage` shrinks it towards the identity, by default 10⁻⁶, and `corr_jitter` keeps it positive definite); a series whose correlation drifts needs a dynamic correlation model, which is not provided.
