// How many long-run relations tie the series together? Johansen's test examines the eigenvalues of the canonical correlations between the changes and the lagged
// levels. For each r = 0, 1, ... it tests "at most r cointegrating relations" against more, with the trace statistic (against any larger rank) or the maximum-eigenvalue
// statistic (against exactly r + 1). `JohansenOptions`: `lag_order` (the order p of the VAR in levels; p - 1 lagged differences are held fixed), `include_intercept` (an
// unrestricted constant), `include_trend`, `statistic`, `alpha`. The p-values and critical values come from the non-standard limiting distributions, tabulated for each
// deterministic specification and each number of series. `selected_rank` is the first r that is not rejected.
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
// Three series (n x 3): x is a random walk, y = x + u with u a stationary AR(1) (so y is tied to x), and w is an unrelated random walk.
Series three_series(int n) {
  const auto e1 = noise(n, 0), e2 = noise(n, 5000), e3 = noise(n, 9000);
  std::vector<double> y(static_cast<std::size_t>(3 * n));
  double x = 0.0, u = 0.0, w = 0.0;
  for (int t = 0; t < n; ++t) {
    x += e1[static_cast<std::size_t>(t)];
    u = 0.6 * u + e2[static_cast<std::size_t>(t)];
    w += e3[static_cast<std::size_t>(t)];
    y[static_cast<std::size_t>(3 * t)] = x;
    y[static_cast<std::size_t>(3 * t + 1)] = x + u;
    y[static_cast<std::size_t>(3 * t + 2)] = w;
  }
  return pluss::make_series(y, {n, 3}).to(f64);
}
int main() {
  const Series y = three_series(400);
  for (auto stat : {ts::JohansenStat::Trace, ts::JohansenStat::MaxEigen}) {
    ts::JohansenOptions o;
    o.lag_order = 1;
    o.statistic = stat;
    const auto r = ts::johansen_test(y, o);
    const auto lambda = r.eigenvalues.host<double>(), s = r.stat.host<double>(), c = r.critical.host<double>(), p = r.pvalue.host<double>();
    const auto rej = r.reject.host<std::int64_t>();
    std::cout << (stat == ts::JohansenStat::Trace ? "trace statistic\n" : "maximum-eigenvalue statistic\n");
    std::cout << std::setw(10) << "H0: rank<=" << std::setw(12) << "eigenvalue" << std::setw(12) << "statistic" << std::setw(12) << "5% critical" << std::setw(10) << "p-value" << "\n";
    for (std::size_t i = 0; i < s.size(); ++i)
      std::cout << std::setw(10) << i << std::fixed << std::setprecision(4) << std::setw(12) << lambda[i] << std::setprecision(2) << std::setw(12) << s[i] << std::setw(12) << c[i] << std::setprecision(4)
                << std::setw(10) << p[i] << (rej[i] ? "   reject" : "   keep") << "\n";
    std::cout << "selected rank " << r.selected_rank << "\n\n";
  }
}
