// Impulse responses and variance decompositions. `fit.irf(IrfOptions)` returns an h x k x k array: entry [s][j][i] is the response of series i, s steps after a
// one-standard-deviation shock to series j (step 0 is the impact). The orthogonalised responses (the default) use the Cholesky factor of the covariance, so the
// ordering of the columns matters: the first series is treated as not responding to the others within the period. `IrfMethod::Generalized` gives shocks that do not
// depend on the ordering. `fit.fevd(FevdOptions)` returns the share of each series' forecast-error variance due to each shock: entry [s][j][i] is the share of
// series i's variance, s+1 steps ahead, caused by shock j; the shares of each series add up to one (for the generalised method they are normalised to).
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
void show(const char* title, const Series& r) {
  std::cout << title << "\n";
  const auto v = r.host<double>();
  for (std::int64_t s = 0; s < r.sizes()[0]; ++s)
    std::cout << "  step " << s << "   shock 0: " << std::setw(8) << v[static_cast<std::size_t>(4 * s)] << std::setw(8) << v[static_cast<std::size_t>(4 * s + 1)] << "   shock 1: " << std::setw(8)
              << v[static_cast<std::size_t>(4 * s + 2)] << std::setw(8) << v[static_cast<std::size_t>(4 * s + 3)] << "\n";
}
int main() {
  const Series y = linked_pair(300);
  ts::VarOptions o;
  o.lag_order = 1;
  const auto fit = ts::Var::fit_mle(y, o);
  std::cout << std::fixed << std::setprecision(4);
  ts::IrfOptions io;
  io.horizon = 5;
  show("orthogonalised responses (each shock: response of series 0, series 1)", fit.irf(io));
  io.method = ts::IrfMethod::Generalized;
  show("generalised responses", fit.irf(io));
  ts::FevdOptions fo;
  fo.horizon = 5;
  show("variance shares (each shock: share of series 0, series 1)", fit.fevd(fo));
  fo.method = ts::IrfMethod::Generalized;
  show("generalised variance shares (normalised to add up to one)", fit.fevd(fo));
}
