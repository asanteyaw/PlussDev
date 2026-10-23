// Does one series help predict another? The Granger test compares the fitted VAR with and without the lags of the candidate cause in the equation of the effect.
// `fit.granger_test(y, GrangerOptions)` tests, for each pair, the hypothesis that all lags of `cause` have zero coefficients in the equation of `effect`; with the
// defaults it tests every ordered pair (`LeaveOneOut`). The result lists `cause`, `effect`, `stat`, `pvalue`, the degrees of freedom `dof1` (number of restrictions)
// and `dof2` (residual degrees of freedom of the equation) and a `reject` flag at `alpha`. `GrangerStat::ChiSquare` (default) gives the Wald statistic; `F` gives the
// F form, which has the better small-sample behaviour.
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
