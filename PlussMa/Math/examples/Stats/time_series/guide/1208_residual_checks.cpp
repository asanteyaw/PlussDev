// Checking a fitted model. A good model leaves residuals that look like white noise. `fit.infer(y)` returns the one-step prediction errors (the
// innovations; NaN where a differencing step has used up the observation). Test them with `ts::ljung_box`, setting `model_df` to the number of ARMA
// coefficients so the degrees of freedom are reduced accordingly, and look at their autocorrelations. Here: the right model (AR(1)) and a too-small one
// (white noise, order 0) on the AR(1) series.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
// A reproducible pseudo-random sequence in (-0.5, 0.5): e_t = frac(sin(12.9898 t + 78.233) * 43758.5453) - 0.5, t = 1, 2, ...
std::vector<double> noise(int n) {
  std::vector<double> e(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[static_cast<std::size_t>(i)] = u - 0.5;
  }
  return e;
}
// AR(1): x_t = phi x_{t-1} + e_t
std::vector<double> ar1(int n, double phi) {
  auto e = noise(n);
  std::vector<double> x(static_cast<std::size_t>(n));
  double carry = 0.0;
  for (int i = 0; i < n; ++i) { carry = phi * carry + e[static_cast<std::size_t>(i)]; x[static_cast<std::size_t>(i)] = carry; }
  return x;
}
std::vector<double> running_sum(const std::vector<double>& e) {
  std::vector<double> w(e.size());
  double s = 0.0;
  for (std::size_t i = 0; i < e.size(); ++i) { s += e[i]; w[i] = s; }
  return w;
}
void print_row(const char* label, const Series& s, int precision = 4) {
  std::cout << std::left << std::setw(16) << label << std::fixed << std::setprecision(precision);
  for (double v : s.host<double>()) std::cout << " " << std::setw(8) << v;
  std::cout << "\n";
}
int main() {
  const Series y = vec(ar1(200, 0.7));
  std::cout << std::left << std::setw(26) << "model" << std::setw(14) << "Q(10)" << std::setw(8) << "df" << std::setw(14) << "p-value" << "residual acf lag 1\n";
  for (int p = 0; p <= 1; ++p) {
    ts::ArimaOptions o;
    o.ar_order = p;
    const auto fit = ts::Arima::fit_mle(y, o);
    const Series resid = fit.infer(y);
    ts::LjungBoxOptions lb;
    lb.lags = {10};
    lb.model_df = p;
    const auto q = ts::ljung_box(resid, lb);
    ts::AcfOptions a;
    a.max_lag = 1;
    std::cout << std::left << std::setw(26) << (p == 0 ? "mean only (no AR term)" : "AR(1)") << std::fixed << std::setprecision(3) << std::setw(14) << q.stat.host<double>()[0] << std::setw(8) << std::setprecision(0)
              << q.dof.host<double>()[0] << std::scientific << std::setprecision(2) << std::setw(14) << q.pvalue.host<double>()[0] << std::fixed << std::setprecision(3) << ts::acf(resid, a).host<double>()[1] << "\n";
  }
}
