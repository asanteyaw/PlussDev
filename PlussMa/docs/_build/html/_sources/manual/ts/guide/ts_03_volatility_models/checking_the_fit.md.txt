<!-- generated from checking_the_fit.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Checking the fit

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
std::vector<double> shocks(int n) {
  std::vector<double> z(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    z[static_cast<std::size_t>(i)] = (u - 0.5) * std::sqrt(12.0);
  }
  return z;
}
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
void report(const char* label, const Series& x) {
  ts::ArchLmOptions o;
  o.lags = 5;
  const auto lm = ts::arch_lm(x, o);
  ts::LjungBoxOptions lb;
  lb.lags = {10};
  const auto q = ts::ljung_box(x * x, lb);
  std::cout << std::left << std::setw(24) << label << std::fixed << std::setprecision(2) << "ARCH-LM " << std::setw(8) << lm.stat << std::scientific << std::setprecision(2)
            << "p " << std::setw(10) << lm.pvalue << "Ljung-Box on squares p " << q.pvalue.to_vec<double>()[0] << "\n";
}
int main() {
  const auto r = garch_returns(1000);
  const Series y = vec(r);
  const auto fit = ts::Garch11::fit_mle(y);
  const auto h = fit.infer(y).to_vec<double>();
  std::vector<double> z(r.size());
  for (std::size_t t = 0; t < r.size(); ++t) z[t] = (r[t] - fit.mean) / std::sqrt(h[t]);
  report("raw returns", y);
  report("standardised residuals", vec(z));
}
```

Output

```text
raw returns             ARCH-LM 74.16   p 1.39e-14  Ljung-Box on squares p 6.53e-22
standardised residuals  ARCH-LM 2.23    p 8.17e-01  Ljung-Box on squares p 9.21e-01
```
The raw returns show strong ARCH effects (p ≈ 10⁻¹⁴ for the ARCH-LM test and 10⁻²² for the Ljung–Box test on squares). After dividing by the fitted conditional standard deviation both tests are silent (p = 0.82 and 0.92): the model has absorbed the clustering. If the standardised residuals still show effects, add lags (`arch_order`, `garch_order`), or try an asymmetric model.
