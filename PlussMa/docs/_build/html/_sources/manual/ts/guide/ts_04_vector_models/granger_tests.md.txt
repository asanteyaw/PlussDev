<!-- generated from granger_tests.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Granger tests

[Back to Vector Autoregressions and Cointegration](../ts_04_vector_models.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
// A reproducible pseudo-random sequence in (-0.5, 0.5): e_t = frac(sin(12.9898 (t + offset) + 78.233) * 43758.5453) - 0.5, t = 1, 2, ...
std::vector<double> noise(int n, int offset = 0) {
  std::vector<double> e(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[static_cast<std::size_t>(i)] = u - 0.5;
  }
  return e;
}
// Two linked series: a_t = 0.5 a_{t-1} + 0.2 b_{t-1} + e1_t,  b_t = 0.1 a_{t-1} + 0.6 b_{t-1} + 0.5 e1_t + e2_t  (n x 2, row t = (a_t, b_t)).
Series linked_pair(int n) {
  const auto e1 = noise(n, 0), e2 = noise(n, 5000);
  std::vector<double> y(static_cast<std::size_t>(2 * n));
  double a = 0.0, b = 0.0;
  for (int t = 0; t < n; ++t) {
    const double na = 0.5 * a + 0.2 * b + e1[static_cast<std::size_t>(t)];
    const double nb = 0.1 * a + 0.6 * b + 0.5 * e1[static_cast<std::size_t>(t)] + e2[static_cast<std::size_t>(t)];
    a = na;
    b = nb;
    y[static_cast<std::size_t>(2 * t)] = a;
    y[static_cast<std::size_t>(2 * t + 1)] = b;
  }
  return pluss::make_series(y, {n, 2}).to(f64);
}
int main() {
  const Series y = linked_pair(300);
  ts::VarOptions o;
  o.lag_order = 2;
  const auto fit = ts::Var::fit_mle(y, o);
  for (auto kind : {ts::GrangerStat::ChiSquare, ts::GrangerStat::F}) {
    ts::GrangerOptions g;
    g.stat = kind;
    const auto r = fit.granger_test(y, g);
    const auto cause = r.cause.to_vec<std::int64_t>(), effect = r.effect.to_vec<std::int64_t>();
    const auto stat = r.stat.to_vec<double>(), p = r.pvalue.to_vec<double>();
    const auto d1 = r.dof1.to_vec<std::int64_t>(), d2 = r.dof2.to_vec<std::int64_t>();
    const auto rej = r.reject.to_vec<std::int64_t>();
    std::cout << (kind == ts::GrangerStat::F ? "F statistic\n" : "chi-squared (Wald) statistic\n");
    for (std::size_t i = 0; i < cause.size(); ++i)
      std::cout << "  series " << cause[i] << " -> series " << effect[i] << "   stat " << std::fixed << std::setprecision(3) << stat[i] << "   p " << std::setprecision(4) << p[i] << "   dof ("
                << d1[i] << ", " << d2[i] << ")   " << (rej[i] ? "reject" : "keep") << "\n";
  }
}
```

Output

```text
chi-squared (Wald) statistic
  series 1 -> series 0   stat 9.698   p 0.0078   dof (2, 293)   reject
  series 0 -> series 1   stat 6.501   p 0.0387   dof (2, 293)   reject
F statistic
  series 1 -> series 0   stat 4.849   p 0.0085   dof (2, 293)   reject
  series 0 -> series 1   stat 3.251   p 0.0402   dof (2, 293)   reject
```
Both directions are significant at the 5% level: each series carries information about the future of the other, as the model's cross coefficients say. The F form has slightly larger p-values than the Wald form, as it should in a finite sample.
