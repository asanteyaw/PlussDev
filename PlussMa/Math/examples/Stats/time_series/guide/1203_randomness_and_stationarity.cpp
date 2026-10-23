// Tests on a series. The Ljung-Box test asks whether the first m autocorrelations are jointly zero (white noise); its statistic is
// n (n + 2) sum_{k<=m} r_k^2 / (n - k), chi-squared with m - model_df degrees of freedom (model_df is the number of ARMA parameters when testing
// residuals). The augmented Dickey-Fuller test has a unit root as its null hypothesis (rejecting means stationary); `lags` is the number of lagged
// differences and `model` the deterministic terms (AR none, ARD constant, TS constant and trend); its p-values and critical values are MacKinnon's
// finite-sample approximations. The KPSS test has stationarity as its null (rejecting means a unit root or trend); `trend` makes the null
// trend-stationarity and `lags` is the bandwidth of the long-run variance. Two series of 200 values: an AR(1) with coefficient 0.7 (stationary)
// and a random walk (unit root).
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
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
  for (double v : s.to_vec<double>()) std::cout << " " << std::setw(8) << v;
  std::cout << "\n";
}
int main() {
  const auto e = noise(200);
  const Series stationary = vec(ar1(200, 0.7)), walk = vec(running_sum(e));
  std::cout << std::fixed << std::setprecision(4);

  ts::LjungBoxOptions lb;
  lb.lags = {5, 10};
  const auto q = ts::ljung_box(stationary, lb);
  const auto stat = q.stat.to_vec<double>(), p = q.pvalue.to_vec<double>();
  std::cout << "Ljung-Box on the AR(1): Q(5) = " << stat[0] << " (p = " << std::scientific << std::setprecision(2) << p[0] << std::fixed << std::setprecision(4) << "), Q(10) = " << stat[1] << " (p = "
            << std::scientific << std::setprecision(2) << p[1] << ")\n";
  lb.lags = {10};
  lb.model_df = 1;
  const auto q1 = ts::ljung_box(stationary, lb);
  std::cout << std::fixed << std::setprecision(4) << "  with one fitted parameter: " << q1.dof.to_vec<double>()[0] << " degrees of freedom\n\n";

  std::cout << std::left << std::setw(26) << "ADF (2 lags)" << std::setw(12) << "statistic" << std::setw(12) << "p-value" << std::setw(12) << "5% critical" << "decision\n";
  for (auto model : {ts::AdfModel::AR, ts::AdfModel::ARD, ts::AdfModel::TS}) {
    const char* name = model == ts::AdfModel::AR ? "no constant" : (model == ts::AdfModel::ARD ? "constant" : "constant and trend");
    for (int which = 0; which < 2; ++which) {
      ts::AdfOptions o;
      o.lags = 2;
      o.model = model;
      const auto r = ts::adf_test(which == 0 ? stationary : walk, o);
      std::cout << std::left << std::setw(26) << (std::string(name) + (which == 0 ? ", AR(1)" : ", walk")) << std::setw(12) << r.stat << std::setw(12) << r.pvalue << std::setw(12) << r.critical
                << (r.reject ? "reject unit root" : "keep unit root") << "\n";
    }
  }
  std::cout << "\n" << std::left << std::setw(26) << "KPSS (4 lags)" << std::setw(12) << "statistic" << std::setw(12) << "p-value" << std::setw(12) << "5% critical" << "decision\n";
  for (bool trend : {false, true}) {
    for (int which = 0; which < 2; ++which) {
      ts::KpssOptions o;
      o.lags = 4;
      o.trend = trend;
      const auto r = ts::kpss_test(which == 0 ? stationary : walk, o);
      std::cout << std::left << std::setw(26) << std::string(trend ? "trend, " : "level, ") + (which == 0 ? "AR(1)" : "walk") << std::setw(12) << r.stat << std::setw(12) << r.pvalue << std::setw(12) << r.critical
                << (r.reject ? "reject stationarity" : "keep stationarity") << "\n";
    }
  }
}
