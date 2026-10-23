<!-- generated from fitting_a_garch_1_1.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Fitting a GARCH(1, 1)

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
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
// Reproducible shocks with unit variance: z_t = (frac(sin(12.9898 t + 78.233) * 43758.5453) - 0.5) * sqrt(12), t = 1, 2, ...
std::vector<double> shocks(int n) {
  std::vector<double> z(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    z[static_cast<std::size_t>(i)] = (u - 0.5) * std::sqrt(12.0);
  }
  return z;
}
// GARCH(1,1) returns: h_t = 0.05 + 0.15 r_{t-1}^2 + 0.75 h_{t-1}, r_t = sqrt(h_t) z_t.
std::vector<double> garch_returns(int n) {
  const auto z = shocks(n);
  std::vector<double> r(static_cast<std::size_t>(n));
  double h = 0.5, prev = 0.0;
  for (int t = 0; t < n; ++t) {
    h = 0.05 + 0.15 * prev * prev + 0.75 * h;
    r[static_cast<std::size_t>(t)] = std::sqrt(h) * z[static_cast<std::size_t>(t)];
    prev = r[static_cast<std::size_t>(t)];
  }
  return r;
}
int main() {
  const Series y = vec(garch_returns(1000));
  const auto fit = ts::Garch11::fit_mle(y);
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "mean   " << fit.mean << "\nomega  " << fit.omega << "   (true 0.0500)\nalpha  " << fit.alpha << "   (true 0.1500)\nbeta   " << fit.beta
            << "   (true 0.7500)\n";
  std::cout << "alpha + beta " << fit.alpha + fit.beta << "   unconditional variance " << fit.unconditional_var() << "\nlog-likelihood " << std::setprecision(2)
            << fit.loglik(y) << "\n\n";
  std::cout << std::setprecision(4) << "forecast of the variance, by horizon\n";
  const auto f = fit.forecast(100, y).to_vec<double>();
  for (int h : {1, 2, 5, 10, 20, 50, 100}) std::cout << "  h = " << std::setw(3) << h << "   " << f[static_cast<std::size_t>(h - 1)] << "\n";
  const auto v = fit.infer(y).to_vec<double>();
  std::cout << "last fitted conditional variance " << v.back() << "\n";
}
```

Output

```text
mean   0.0215
omega  0.0893   (true 0.0500)
alpha  0.1434   (true 0.1500)
beta   0.6733   (true 0.7500)
alpha + beta 0.8166   unconditional variance 0.4870
log-likelihood -1047.15

forecast of the variance, by horizon
  h =   1   0.3767
  h =   2   0.3969
  h =   5   0.4379
  h =  10   0.4692
  h =  20   0.4847
  h =  50   0.4870
  h = 100   0.4870
last fitted conditional variance 0.3725
```
The estimates of `α` (0.143) and `β` (0.673) are close to their true values 0.15 and 0.75. The constant `ω` (0.089 against 0.05) is estimated with the least precision: a larger `ω` with a smaller `β` produces nearly the same variance process, which is the usual weakly identified direction of a GARCH likelihood. The persistence `α + β` is 0.82, so the forecasts climb from the current low variance (0.377) to the unconditional level 0.487 within about 50 steps and stay there.
